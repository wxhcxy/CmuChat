#include "LogicSystem.h"
#include "HttpConnection.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "StatusGrpcClient.h"
#include "VarifyGrpcClient.h"

void LogicSystem::RegGet(std::string url, HttpHandler handler) {	//注册get请求, handler是回调函数
	_get_handlers.insert(make_pair(url, handler));	//make_pair(url, handler)组成一个键值对，插入map里
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {	//注册get请求, handler是回调函数
	_post_handlers.insert(make_pair(url, handler));	//make_pair(url, handler)组成一个键值对，插入map里
}


LogicSystem::LogicSystem() {
	//注册get请求,测试用
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection){
		beast::ostream(connection->_response.body()) << "receive get_test req";
		int i = 0;
		for (auto& elem : connection->_get_params) {
			i++;
			beast::ostream(connection->_response.body()) << " param " << i << " key is " << elem.first;//往body里写数据
			beast::ostream(connection->_response.body()) << " , " << i << " value is " << elem.second << std::endl;
		}
		});//lamda表达式是回调函数

	//注册post请求
	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data()); //将收到的body转成string
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");//json类型 //设置回给对方的数据内容的返回类型
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;//来源
		bool parse_success = reader.parse(body_str, src_root);//解析body_str，解析完放进src_root里
		if (!parse_success) {//解析没成功
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();//将json转成标准字符串
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}


		//解析成功
		//email不存在
		if (!src_root.isMember("email")) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();//将json转成标准字符串
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//email存在
		auto email = src_root["email"].asString();//它会发一个email过来,我们读取它
		GetVarifyRsp rsp = VarifyGrpcClient::GetInstance()->GetVarifyCode(email); //向grpc发送，获取一个验证码
		std::cout << "email is " << email << std::endl;
		root["error"] = rsp.error();
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();//将json转成字符串，因为tcp是面向字节流的,不能直接发送json
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});



	//server端接受注册请求，客户端会发过来注册请求的url
	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}



		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();

		if (pwd != confirm) {
			std::cout << "password err " << std::endl;
			root["error"] = ErrorCodes::PasswdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//先查找redis中email对应的验证码是否合理
		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}


		//查找数据库判断用户是否存在
		int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd);	//先注册这个用户，根据返回的uid，判断这个用户是否已经存在
		if (uid == 0 || uid == -1) {	//注册失败，0、-1表示用户名已存在 或邮箱已存在 或出现一些异常
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//注册成功，执行下面代码
		root["error"] = 0;
		root["uid"] = uid;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["confirm"] = confirm;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});


	//重置密码回调逻辑
	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();

		//先查找redis中email对应的验证码是否合理
		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//查询数据库判断用户名和邮箱是否匹配
		bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
		if (!email_valid) {
			std::cout << " user email not match" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//更新密码为最新密码
		bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
		if (!b_up) {
			std::cout << " update pwd failed" << std::endl;
			root["error"] = ErrorCodes::PasswdUpFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to update password" << pwd << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});


	//用户登录逻辑
	RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
        std::cout << "email: " << email << std::endl;
        std::cout << "pwd: " << pwd << std::endl;
        UserInfo userInfo;
        //查询数据库判断用户名和密码是否匹配
		bool pwd_valid = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
        std::cout << "pwd_valid: " << pwd_valid << std::endl;
        if (!pwd_valid) {
			std::cout << " user pwd not match" << std::endl;
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//查询StatusServer找到合适的连接
		auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
		if (reply.error()) {
			std::cout << " grpc get chat server failed, error is " << reply.error() << std::endl;
			root["error"] = ErrorCodes::RPCFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["uid"] = userInfo.uid;
		root["token"] = reply.token();
		root["host"] = reply.host();
		root["port"] = reply.port();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

}


bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con) {
	if (_get_handlers.find(path) == _get_handlers.end()) {//没找到,	path是在RegGet()函数里注册的
		return false;//让网络层去抛404错误
	}

	_get_handlers[path](con);//找到就处理
	//_get_handlers[path]返回回调函数，回调函数就调用，(con)是回调函数的参数
	return true;
}


bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;
	}

	_post_handlers[path](con);
	return true;
}
