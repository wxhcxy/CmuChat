#include "CServer.h"
#include "AsioIOServicePool.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include <iostream>

CServer::CServer(boost::asio::io_context &io_context, short port)
    : _io_context(io_context)
    , _port(port)
    , _acceptor(io_context, tcp::endpoint(tcp::v4(), port))
    , _timer(_io_context,
             std::chrono::seconds(
                 60)) //绑定定时器_timer到io_context，只要这个io_context跑起来，每隔60秒就会触发一次定时器
{
	cout << "Server start success, listen on port : " << _port << endl;
    //启动定时器，定时器每隔60秒后触发的事
    _timer.async_wait([this](boost::system::error_code ec) { on_timer(ec); });

    StartAccept();
}

CServer::~CServer() {
	cout << "Server destruct listen on port : " << _port << endl;
}

void CServer::HandleAccept(shared_ptr<CSession> new_session, const boost::system::error_code& error){
	if (!error) {
		new_session->Start();
		lock_guard<mutex> lock(_mutex);
		_sessions.insert(make_pair(new_session->GetSessionId(), new_session));
	}
	else {
		cout << "session accept failed, error is " << error.what() << endl;
	}

	StartAccept();
}

void CServer::StartAccept() {
	auto &io_context = AsioIOServicePool::GetInstance()->GetIOService();
	shared_ptr<CSession> new_session = make_shared<CSession>(io_context, this);
	_acceptor.async_accept(new_session->GetSocket(), std::bind(&CServer::HandleAccept, this, new_session, placeholders::_1));
}

//根据session的id删除session,并移除用户和session的关联
void CServer::ClearSession(std::string session_id) {
    lock_guard<mutex> lock(_mutex);
    if (_sessions.find(session_id) != _sessions.end()) {
        auto uid = _sessions[session_id]->GetUserId();
        //移除用户和session的关联
        UserMgr::GetInstance()->RmvUserSession(uid, session_id);
    }

    _sessions.erase(session_id);
}

shared_ptr<CSession> CServer::GetSession(std::string uuid)
{
    auto it = _sessions.find(uuid);
    if (it != _sessions.end()) {
        return it->second;
    }
    return nullptr;
}

void CServer::on_timer(const boost::system::error_code &ec)
{
    std::vector<std::shared_ptr<CSession>> _expired_sessions; //存储过期的连接
    int session_count = 0;
    //此处加锁遍历session
    {
        lock_guard<mutex> lock(_mutex);
        time_t now = std::time(nullptr);
        //迭代遍历所有session，检测是否有过期的session
        for (auto iter = _sessions.begin(); iter != _sessions.end(); iter++) {
            auto b_expired = iter->second->IsHeartbeatExpired(now); //判断这个session是否过期
            if (b_expired) {                                        //若过期
                //关闭socket, 其实这里也会触发async_read的错误处理
                iter->second->Close(); //关闭这个session
                //收集过期信息
                _expired_sessions.push_back(iter->second);
                continue;
            }
            session_count++;
        }
    }

    //设置session数量
    auto &cfg = ConfigMgr::Inst();
    auto self_name = cfg["SelfServer"]["Name"];
    auto count_str = std::to_string(session_count);
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, count_str);

    //处理过期session, 单独提出，防止死锁
    for (auto &session : _expired_sessions) {
        session->DealExceptionSession();
    }

    //再次设置，下一个60s检测
    _timer.expires_after(std::chrono::seconds(60));
    _timer.async_wait([this](boost::system::error_code ec) { on_timer(ec); });
}

bool CServer::CheckValid(std::string sid)
{
    lock_guard<mutex> lock(_mutex);
    auto it = _sessions.find(sid);
    if (it != _sessions.end()) {
        return true;
    }
    return false;
}
