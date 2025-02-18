#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"

//提升GateServer并发
//添加ASIO IOContext Pool 结构，让多个iocontext跑在不同的线程中

class AsioIOServicePool :public Singleton<AsioIOServicePool>
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
    AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()*/);
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    std::size_t                        _nextIOService;
};