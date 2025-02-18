#pragma once

//查询状态的客户端

#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include <grpcpp/grpcpp.h> 
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <atomic>

using grpc::Channel;	//grpc做连接的通道
using grpc::Status;	//grpc访问的结果，一个状态
using grpc::ClientContext;	//用来通信的上下文

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::StatusService;


//状态的连接池
class StatusConPool {
public:
	StatusConPool(size_t poolSize, std::string host, std::string port)
		: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
		for (size_t i = 0; i < poolSize_; ++i) {

			//根据host和port创建连接
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
				grpc::InsecureChannelCredentials());

			connections_.push(StatusService::NewStub(channel));//将创建的连接放进连接池
		}
	}

	~StatusConPool() {
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}

	std::unique_ptr<StatusService::Stub> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !connections_.empty();
			});
		//
		if (b_stop_) {
			return  nullptr;
		}
		auto context = std::move(connections_.front());
		connections_.pop();
		return context;
	}

	void returnConnection(std::unique_ptr<StatusService::Stub> context) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		connections_.push(std::move(context));
		cond_.notify_one();
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

private:
	std::atomic<bool> b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<StatusService::Stub>> connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

class StatusGrpcClient :public Singleton<StatusGrpcClient>
{
	friend class Singleton<StatusGrpcClient>;
public:
	~StatusGrpcClient() {

	}
	GetChatServerRsp GetChatServer(int uid);	//获取聊天服务器的ip
	LoginRsp Login(int uid, std::string token);
private:
	StatusGrpcClient();
	std::unique_ptr<StatusConPool> pool_;

};

