#pragma once
#include <boost/asio.hpp>
#include "CSession.h"
#include <memory.h>
#include <map>
#include <mutex>
using namespace std;
using boost::asio::ip::tcp;
class CServer
{
public:
	CServer(boost::asio::io_context& io_context, short port);
	~CServer();
	void ClearSession(std::string);
    shared_ptr<CSession> GetSession(std::string);

private:
	void HandleAccept(shared_ptr<CSession>, const boost::system::error_code & error);//接收对端的一个连接
	void StartAccept();//开启连接
	boost::asio::io_context &_io_context;
	short _port;
	tcp::acceptor _acceptor;
	std::map<std::string, shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
};

