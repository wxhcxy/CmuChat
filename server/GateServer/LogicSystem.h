#pragma once

#include "const.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;	//回调函数

class LogicSystem : public Singleton<LogicSystem> {

	friend class Singleton<LogicSystem>;

public:
	~LogicSystem(){}
	bool HandleGet(std::string, std::shared_ptr<HttpConnection>);	//处理get请求
	void RegGet(std::string, HttpHandler handler); //注册get请求

	void RegPost(std::string url, HttpHandler handler);	//post
	bool HandlePost(std::string, std::shared_ptr<HttpConnection>);	//处理post请求

private:
	LogicSystem();
	std::map<std::string, HttpHandler> _post_handlers;	//处理post请求的集合
	std::map<std::string, HttpHandler> _get_handlers;	//处理get请求的集合
};