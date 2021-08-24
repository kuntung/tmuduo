#include <tmuduo/net/Acceptor.h>
#include <tmuduo/net/EventLoop.h>
#include <tmuduo/net/InetAddress.h>
#include <tmuduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>

using namespace tmuduo;
using namespace tmuduo::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie()),
	acceptChannel_(loop, acceptSocket_.fd()), /* 进行套接字的channel绑定 */
	listenning_(false),
	idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
	assert(idleFd_ >= 0);
	acceptSocket_.setReuseAddr(true);
	acceptSocket_.bindAddress(listenAddr);
	acceptChannel_.setReadCallback(
			boost::bind(&Acceptor::handleRead, this)); // 设置channel的回调函数
	
}

Acceptor::~Acceptor()
{
	acceptChannel_.disableAll();
	acceptChannel_.remove();
	::close(idleFd_);
}

void Acceptor::listen()
{
	loop_->assertInLoopThread();
	listenning_ = true;
	acceptSocket_.listen();
	acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
	loop_->assertInLoopThread();
	InetAddress peerAddr(0);

	int connfd = acceptSocket_.accept(&peerAddr);
	
	if (connfd >= 0)
	{
		if (newConnectionCallback_)
		{
			newConnectionCallback_(connfd, peerAddr);
		}
		else
		{
			sockets::close(connfd); // 如果没有注册响应操作函数，直接关闭
		}
	}
	else /* accept失败 */
	{
	    // Read the section named "The special problem of
    	// accept()ing when you can't" in libev's doc.
    	// By Marc Lehmann, author of livev.
    	if (errno == EMFILE)
    	{
      		::close(idleFd_); // 是LT触发，避免busy-loop
      		idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
      		::close(idleFd_);
      		idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    	}	
	}
} 
