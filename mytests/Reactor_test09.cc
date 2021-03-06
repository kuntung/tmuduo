#include <tmuduo/net/TcpServer.h>
#include <tmuduo/net/EventLoop.h>
#include <tmuduo/net/InetAddress.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace tmuduo;
using namespace tmuduo::net;

class TestServer
{
public:
	TestServer(EventLoop* loop,
			const InetAddress& listenAddr)
	  : loop_(loop),
	    server_(loop, listenAddr, "TestServer")
{
	server_.setConnectionCallback(
		boost::bind(&TestServer::onConnection, this, _1));
	server_.setMessageCallback(
		boost::bind(&TestServer::onMessage, this, _1, _2, _3));
}

void start()
{
	server_.start();
}

private:
	void onConnection(const TcpConnectionPtr& conn)
	{
		if (conn->connected())
		{
			printf("onConnection(): new connection [%s] from %s\n",
					conn->name().c_str(),
					conn->peerAddress().toIpPort().c_str());
		}
		else
		{
			printf("onConnection(): connection [%s] is down\n",
				conn->name().c_str());
		}
	}

	void onMessage(const TcpConnectionPtr& conn,
				const char* data,
				ssize_t len)
	{
		printf("onMessage(): received %zd bytes from connection [%s]\n",
				len, conn->name().c_str());
	}
	TcpServer server_;
	EventLoop* loop_;

}; // end of TestServer

int main(void)
{
	printf("main(): pid = %d\n", getpid());

	InetAddress listenAddr(8888); // 服务器监听任意IP地址，8888端口的连接请求
	EventLoop loop;

	TestServer server(&loop, listenAddr);
	server.start();

	loop.loop();

	return 0;
}
