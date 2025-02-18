// ChatServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//asio实现tcp服务器

#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ChatServiceImpl.h"
#include "ConfigMgr.h"
#include "LogicSystem.h"
#include "RedisMgr.h"
#include <csignal>
#include <mutex>
#include <thread>

//qingxiang ChatServer

using namespace std;
bool bstop = false;
std::condition_variable cond_quit; //这个条件变量和下面的互斥量，都用来管理退出
std::mutex mutex_quit;

int main()
{
    auto& cfg = ConfigMgr::Inst();
    auto server_name = cfg["SelfServer"]["Name"];
    try {
        auto pool = AsioIOServicePool::GetInstance();
        //将登录数设置为0
        RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");

        //定义一个GrpcServer

        std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
        ChatServiceImpl service;
        grpc::ServerBuilder builder;
        // 监听端口和添加服务
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        // 构建并启动gRPC服务器
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        std::cout << "RPC Server listening on " << server_address << std::endl;

        //单独启动一个线程处理grpc服务
        std::thread grpc_server_thread([&server]() { server->Wait(); });

        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, pool, &server](auto, auto) {
            io_context.stop();
            pool->Stop();
            server->Shutdown();
        });
        auto port_str = cfg["SelfServer"]["Port"];
        CServer s(io_context, atoi(port_str.c_str()));
        io_context.run();
        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
        RedisMgr::GetInstance()->Close();
        grpc_server_thread.join();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << endl;
        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
        RedisMgr::GetInstance()->Close();
    }
}
