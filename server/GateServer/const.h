﻿#pragma once

#include "Singleton.h"
#include <atomic>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <cassert>
#include <condition_variable> //条件变量
#include <functional>
#include <hiredis/hiredis.h>
#include <iostream>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,
	RPCFailed = 1002,
	VarifyExpired = 1003, //验证码过期
	VarifyCodeErr = 1004, //验证码错误
	UserExist = 1005,     //用户已经存在   
	PasswdErr = 1006,    //密码错误
	EmailNotMatch = 1007,  //邮箱不匹配
	PasswdUpFailed = 1008, //更新密码失败
	PasswdInvalid = 1009,  //密码更新失败
};



// Defer类
class Defer {
public:
	//接受一个lambda表达式或者函数指针
	Defer(std::function<void()> func) : func_(func) {}

	//析构函数中执行传入的函数
	~Defer() {
		func_();
	}

private:
	std::function<void()> func_;
};



#define CODEPREFIX "code_"	//前缀
