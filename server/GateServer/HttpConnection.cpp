#include "HttpConnection.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(boost::asio::io_context& ioc)
	: _socket(ioc)	//socket只有移动构造，没有拷贝构造
{

}

void HttpConnection::Start() {
	auto self = shared_from_this();
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transferred) {
		try
		{
			if (ec) {
				std::cout << "http read err is " << ec.what() << std::endl;
				return;
			}

			boost::ignore_unused(bytes_transferred);//http服务器不需要做粘包处理，所以忽略已经发送的字节数
			self->HandleReq();	//处理请求，先读出对方的请求，读出来之后再处理，处理完之后返回给对端
			self->CheckDeadline();	//超时检测，返回给对端时可能会超时，所以这里启动超时检测
		}
		catch (std::exception& exp)
		{
			std::cout << "exception is " << exp.what() << std::endl;
		}
		});
}





//char 转为16进制
unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//判断是否仅有数字和字母构成
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //为空字符
			strTemp += "+";
		else
		{
			//其他字符需要提前加%并且高四位和低四位分别转为16进制
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//还原+为空
		if (str[i] == '+') strTemp += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}




void HttpConnection::PreParseGetParam() {
	// 提取 URI  get_test?key1=value1&key2=value2
	auto uri = _request.target();
	// 查找查询字符串的开始位置（即 '?' 的位置）  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}

	_get_url = uri.substr(0, query_pos);
	std::string query_string = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// 处理最后一个参数对（如果没有 & 分隔符）  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}




void HttpConnection::HandleReq() {
	//设置版本
	_response.version(_request.version());
	_response.keep_alive(false); //设置短连接，http不用维持长连接，所以keep_alive设置成false

	//处理get请求
	if (_request.method() == http::verb::get) {	//处理get请求
		PreParseGetParam();//先解析url，url的参数等
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
		if (!success) {//若出错
			_response.result(http::status::not_found);//失败了，告诉它失败的原因，这里告诉404 not found
			_response.set(http::field::content_type, "text/plain");	//设置回应头, 回应的类型，文本类型还是二进制类型
			beast::ostream(_response.body()) << "url not found\r\n";//往body消息体写数据
			WriteResponse(); //写完之后，然后回包给对方
			return;
		}
		else {//没有出错
			_response.result(http::status::ok);//成功ok状态
			_response.set(http::field::server, "GateServer");//告诉一下对方，是哪个服务器告诉他的,这行代码写不写无所谓
			WriteResponse(); //写完之后，然后回包给对方
			return;
		}
	}


	//处理get请求
	if (_request.method() == http::verb::post) {	//处理get请求
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success) {//若出错
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");	//如果没成功就返回文本类型的url not found，告诉他没找到
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse(); //回包
			return;
		}
		else {//没有出错
			_response.result(http::status::ok);//成功ok状态
			_response.set(http::field::server, "GateServer");
			WriteResponse(); //回包给对方
			return;
		}
	}
}


//回包
void HttpConnection::WriteResponse() {
	auto self = shared_from_this();
	//http底层帮我们切包，我们不会处理粘包,
	//它的切包原理是通过一个包体的长度去切的，放在头里，所以我们要设置包体的长度
	_response.content_length(_response.body().size());
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t bytes_transferred) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);//关掉发送端(服务器),成功或失败,都要关掉
		self->deadline_.cancel();	//收到回调后，将定时器取消, 没有超时
		});
}


void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();
	deadline_.async_wait([self](beast::error_code ec) {
		if (!ec) {//没有出错, 将_socket关了就行, 检测到超时了
			self->_socket.close(ec);
		}
		});//定时器，异步等待
}