#pragma once

#include "const.h"

class HttpConnection: public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(boost::asio::io_context& ioc);
	void Start();
	tcp::socket& GetSocket() {
		return _socket;
	}

private:
	void CheckDeadline();	//检测超时
	void WriteResponse();	//收到数据，应答
	void HandleReq();	//处理请求,包括解析请求头，解析包体的内容
	void PreParseGetParam();//解析url
	tcp::socket _socket;	
	beast::flat_buffer _buffer{ 8192 }; //接收对方发过来的数据,8192为8k
	http::request<http::dynamic_body> _request;	//可以接收各种请求，包括字符串文本类型、图片二进制数据、表单
	http::response<http::dynamic_body> _response;	//回复
	net::steady_timer deadline_{
		_socket.get_executor(), std::chrono::seconds(60)
	//我们发送给对方的请求，如果对方60秒还没有收到，或者我们还没有发送完，就认为超时，对方连接或我们连接出现问题
	};//定时器, 花括号构造deadline_

	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
};

