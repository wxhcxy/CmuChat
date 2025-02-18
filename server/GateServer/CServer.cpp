#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
	:_ioc(ioc),
	_acceptor(ioc, tcp::endpoint(tcp::v4(), port)){

}


void CServer::Start() {

	//防止一些回调，没有回调过来的时候，怕这个类被析构，所以把这个类对象生成一个智能指针掉
	auto self = shared_from_this();	

	//线程池管理连接
	//使用我们定义的AsioIOServicePool连接池
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);	//创建连接 //创建新连接，并且创建HttpConnection类管理这个连接

	_acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {//这里捕获智能指针self，引用计数加1，防止改类对象被析构
		try
		{
			//出错放弃这连接，继续监听其他连接
			if (ec) {
				self->Start();	//继续监听其他连接
				return;
			}
			
			//启动
			new_con->Start();

            std::cout << "CServer::Start() _acceptor.async_accept" << std::endl;

            //继续监听
			self->Start();
		}
		catch (std::exception& exp)
		{

		}
		});
}
