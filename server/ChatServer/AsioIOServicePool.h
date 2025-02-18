#pragma once
#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"
class AsioIOServicePool:public Singleton<AsioIOServicePool>
{
	friend Singleton<AsioIOServicePool>;
public:
	using IOService = boost::asio::io_context;
	using Work = boost::asio::io_context::work;
	using WorkPtr = std::unique_ptr<Work>;
	~AsioIOServicePool();
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
	// 使用 round-robin 的方式返回一个 io_service
	boost::asio::io_context& GetIOService();
	void Stop();
private:
	AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());//cpu有几个核，就分配几个线程
	std::vector<IOService> _ioServices;	//，每一个io_context都跑在每一个线程里
	std::vector<WorkPtr> _works;
	std::vector<std::thread> _threads;	//_ioServices、_works、_threads这三个vector大小是一样的
	std::size_t                        _nextIOService;
};

