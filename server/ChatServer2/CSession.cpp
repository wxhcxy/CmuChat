#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include "RedisMgr.h"
#include <iostream>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <sstream>

CSession::CSession(boost::asio::io_context& io_context, CServer* server) :
	_socket(io_context), _server(server), _b_close(false), _b_head_parse(false), _user_uid(0) {
	boost::uuids::uuid  a_uuid = boost::uuids::random_generator()();
	_session_id = boost::uuids::to_string(a_uuid);
	_recv_head_node = make_shared<MsgNode>(HEAD_TOTAL_LEN);
}
CSession::~CSession() {
	std::cout << "~CSession destruct" << endl;
}

tcp::socket& CSession::GetSocket() {
	return _socket;
}

std::string& CSession::GetSessionId() {
	return _session_id;
}

void CSession::SetUserId(int uid)
{
	_user_uid = uid;
}

int CSession::GetUserId()
{
	return _user_uid;
}


void CSession::Start(){
	AsyncReadHead(HEAD_TOTAL_LEN);	//接收头部4字节
}

void CSession::Send(std::string msg, short msgid) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << endl;
		return;
	}

	_send_que.push(make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
	if (send_que_size > 0) {
		return;
	}
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Send(char* msg, short max_length, short msgid) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << endl;
		return;
	}

	_send_que.push(make_shared<SendNode>(msg, max_length, msgid));
	if (send_que_size>0) {
		return;
	}
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len), 
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Close() {
	_socket.close();
	_b_close = true;
}

std::shared_ptr<CSession>CSession::SharedSelf() {
	return shared_from_this();
}

void CSession::AsyncReadBody(int total_len)
{
	auto self = shared_from_this();
	asyncReadFull(total_len, [self, this, total_len](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if (ec) {
				std::cout << "handle read failed, error is " << ec.what() << endl;
				Close();

                //对redis操作，加分布式锁
                //加锁清除session
                auto uid_str = std::to_string(_user_uid);
                auto lock_key = LOCK_PREFIX + uid_str;
                auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key,
                                                                       LOCK_TIME_OUT,
                                                                       ACQUIRE_TIME_OUT);
                Defer defer([identifier, lock_key, self, this]() {
                    _server->ClearSession(_session_id);
                    RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
                });

                if (identifier.empty()) {
                    return;
                }
                std::string redis_session_id = "";
                auto bsuccess = RedisMgr::GetInstance()->Get(USER_SESSION_PREFIX + uid_str,
                                                             redis_session_id);
                if (!bsuccess) {
                    return;
                }

                if (redis_session_id != _session_id) {
                    //说明有客户在其他服务器异地登录了
                    return;
                }

                RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
                //清除用户登录信息
                RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);

                return;
            }

            if (bytes_transfered < total_len) {
                std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
                          << total_len << "]" << endl;
                Close();
				_server->ClearSession(_session_id);
				return;
            }

            memcpy(_recv_msg_node->_data, _data, bytes_transfered);
            _recv_msg_node->_cur_len += bytes_transfered;
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
			cout << "receive data is " << _recv_msg_node->_data << endl;
			//此处将消息投递到逻辑队列中
			LogicSystem::GetInstance()->PostMsgToQue(make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
			//继续监听头部接受事件
			AsyncReadHead(HEAD_TOTAL_LEN);
		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << endl;
		}
		});
}

void CSession::AsyncReadHead(int total_len)
{
	auto self = shared_from_this();
	//asyncReadFull里只有读完HEAD_TOTAL_LEN4字节之后，或者4字节没读全出现错误，才会回调后面的lambda函数
	asyncReadFull(HEAD_TOTAL_LEN, [self, this](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if (ec) {
				std::cout << "handle read failed, error is " << ec.what() << endl;
                Close();
                //_server->ClearSession(_session_id);

                //加锁清除session
                auto uid_str = std::to_string(_user_uid);
                auto lock_key = LOCK_PREFIX + uid_str;
                auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key,
                                                                       LOCK_TIME_OUT,
                                                                       ACQUIRE_TIME_OUT);
                Defer defer([identifier, lock_key, self, this]() {
                    _server->ClearSession(_session_id);
                    RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
                });
                if (identifier.empty()) {
                    //如果加锁失败就交给其他线程处理
                    return;
                }

                //判断如果uid对应的session和自己的session相同则删除
                std::string redis_session_id = "";
                auto success = RedisMgr::GetInstance()->Get(USER_SESSION_PREFIX + uid_str,
                                                            redis_session_id);
                if (!success) {
                    //如果session_id不存在就返回
                    return;
                }

                //redis_session_id和当前session不相同就直接返回
                if (redis_session_id != _session_id) {
                    return;
                }

                //和自己的session相同，说明没有其他人登录替换，那就安心删除session_id
                RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
                //清除用户登录信息
                RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);

                return;
            }

            if (bytes_transfered < HEAD_TOTAL_LEN) {
				std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
					<< HEAD_TOTAL_LEN << "]" << endl;
				Close();
				_server->ClearSession(_session_id);
				return;
			}

			_recv_head_node->Clear();
			memcpy(_recv_head_node->_data, _data, bytes_transfered);

			//获取头部MSGID数据
			short msg_id = 0;
			memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
			//网络字节序转化为本地字节序
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			std::cout << "msg_id is " << msg_id << endl;
			//id非法
			if (msg_id > MAX_LENGTH) {
				std::cout << "invalid msg_id is " << msg_id << endl;
				_server->ClearSession(_session_id);
				return;
			}
			short msg_len = 0;
			memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			//网络字节序转化为本地字节序
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			std::cout << "msg_len is " << msg_len << endl;

			//id非法
			if (msg_len > MAX_LENGTH) {
				std::cout << "invalid data length is " << msg_len << endl;
				_server->ClearSession(_session_id);
				return;
			}

			_recv_msg_node = make_shared<RecvNode>(msg_len, msg_id);
			AsyncReadBody(msg_len);//读取body数据，接收body
		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << endl;
		}
		});
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self) {
	//增加异常处理
	try {
        auto self = shared_from_this();
        if (!error) {
			std::lock_guard<std::mutex> lock(_send_lock);
			//cout << "send data " << _send_que.front()->_data+HEAD_LENGTH << endl;
			_send_que.pop();
			if (!_send_que.empty()) {
				auto& msgnode = _send_que.front();
				boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
					std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_self));
			}
		}
		else {
			std::cout << "handle write failed, error is " << error.what() << endl;
			Close();

            //_server->ClearSession(_session_id);
            //加分布式锁
            auto uid_str = std::to_string(_user_uid);
            auto lock_key = LOCK_PREFIX + uid_str;
            auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key,
                                                                   LOCK_TIME_OUT,
                                                                   ACQUIRE_TIME_OUT);
            Defer defer([identifier, lock_key, self, this]() {
                //加锁清除session
                _server->ClearSession(_session_id);
                RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
            });

            if (identifier.empty()) {
                return;
            }
            std::string redis_session_id = "";
            bool success = RedisMgr::GetInstance()->Get(USER_SESSION_PREFIX + uid_str,
                                                        redis_session_id);
            if (!success) {
                return;
            }

            if (redis_session_id != _session_id) {
                return;
            }

            RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
            RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
        }
    } catch (std::exception& e) {
        std::cerr << "Exception code : " << e.what() << endl;
    }
}

//读取完整长度
void CSession::asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)> handler )
{
	::memset(_data, 0, MAX_LENGTH);
	asyncReadLen(0, maxLength, handler);
}

//读取指定字节数
void CSession::asyncReadLen(std::size_t read_len, std::size_t total_len, 
	std::function<void(const boost::system::error_code&, std::size_t)> handler)
{
	auto self = shared_from_this();
	_socket.async_read_some(boost::asio::buffer(_data + read_len, total_len-read_len),
		[read_len, total_len, handler, self](const boost::system::error_code& ec, std::size_t  bytesTransfered) {
			if (ec) {
				// 出现错误，调用回调函数
				handler(ec, read_len + bytesTransfered);
				return;
			}

			if (read_len + bytesTransfered >= total_len) {
				//长度够了就调用回调函数
				handler(ec, read_len + bytesTransfered);
				return;
			}

			// 没有错误，且长度不足则继续读取
			self->asyncReadLen(read_len + bytesTransfered, total_len, handler);
	});
}

LogicNode::LogicNode(shared_ptr<CSession>  session, 
	shared_ptr<RecvNode> recvnode):_session(session),_recvnode(recvnode) {
	
}

void CSession::NotifyOffline(int uid)
{
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;

    std::string return_str = rtvalue.toStyledString();

    Send(return_str, ID_NOTIFY_OFF_LINE_REQ);
    return;
}
