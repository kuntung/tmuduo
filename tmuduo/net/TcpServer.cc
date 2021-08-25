#include <tmuduo/net/TcpServer.h>

#include <tmuduo/base/Logging.h>
#include <tmuduo/net/Acceptor.h>
#include <tmuduo/net/EventLoop.h>
#include <tmuduo/net/EventLoopThreadPool.h>
#include <tmuduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h> // snprintf

using namespace tmuduo;
using namespace tmuduo::net;

TcpServer::TcpServer(EventLoop* loop,
				const InetAddress& listenAddr,
				const string& nameArg)
  : loop_(CHECK_NOTNULL(loop)),
    hostport_(listenAddr.toIpPort()),
	name_(nameArg),
	acceptor_(new Acceptor(loop, listenAddr)),
	threadPool_(new EventLoopThreadPool(loop)),
	connectionCallback_(defaultConnectionCallback),
	 messageCallback_(defaultMessageCallback),
	started_(false),
	nextConnId_(1)
{
	// Acceptor::handleRead函数会调用TcpServer::newConnection
	// _1对应的是socket文件描述符，_2对应的是对等方的地址(InetAddress)
	acceptor_->setNewConnectionCallback(
		boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
	loop_->assertInLoopThread();
	LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

	for (ConnectionMap::iterator it(connections_.begin());
		it != connections_.end(); ++it)
	{
		TcpConnectionPtr conn = it->second; 
		it->second.reset(); // 释放当前控制的对象，引用计数减一，至于谁负责析构，大概率是Acceptor的逻辑处理
		conn->getLoop()->runInLoop(
			boost::bind(&TcpConnection::connectDestroyed, conn)); 
			// 让TCPconnection所属的eventloop通过connectDestroyed处理
		conn.reset(); // 释放当前所控制的对象，引用计数减一
	}
}

void TcpServer::setThreadNum(int numThreads)
{
	assert(0 <= numThreads);
	threadPool_->setThreadNum(numThreads);
}

// 可以多次调用start，是ok的
// 可以跨线程调用
void TcpServer::start()
{
	if (!started_)
	{
		started_ = true;
		threadPool_->start(threadInitCallback_);
	}

	if (!acceptor_->listenning())
	{
		loop_->runInLoop(
			boost::bind(&Acceptor::listen, get_pointer(acceptor_))); // 如果acceptor没有处于监听状态，
			// 通过loop_->runInLoop执行监听函数。开启socket的listen，以及enableReading
	}
}
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
	loop_->assertInLoopThread();
	// 按照轮叫的方式选择一个EventLoop
	EventLoop* ioLoop = threadPool_->getNextLoop(); 
	char buf[32];
	snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
	++nextConnId_;
	string connName = name_ + buf;

	LOG_INFO << "TcpServer::newConnection [" << name_
			 << "] - new connection [" << connName
			 << "] from " << peerAddr.toIpPort();
	
	InetAddress localAddr(sockets::getLocalAddr(sockfd));

	/* TcpConnectionPtr conn(new TcpConnection(loop_,
											connName,
											sockfd,
											localAddr,
											peerAddr)); */

	TcpConnectionPtr conn(new TcpConnection(ioLoop,
											connName,
											sockfd,
											localAddr,
											peerAddr)); // 将新来的连接挂载到所选的IO线程上

	LOG_TRACE << "[1] usecount=" << conn.use_count();
	connections_[connName] = conn;
	LOG_TRACE << "[2] usecount=" << conn.use_count();
	conn->setConnectionCallback(connectionCallback_); // ?
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);

	conn->setCloseCallback(
		boost::bind(&TcpServer::removeConnection, this, _1)); // 这里可能也涉及到IO线程跨线程调用,提供一个
		// removeConnectionInLoop
	
	// conn->connectEstablished(); // 这里就不能再直接调用, 因为是main reactor在轮叫，可能跨线程调用
	ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
	LOG_TRACE << "[5] usecount=" << conn.use_count();
	
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	/* loop_->assertInLoopThread();

	LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
			 << "] - connection " << conn->name();

	LOG_TRACE << "[8] usecount=" << conn.use_count();

	size_t n = connections_.erase(conn->name());
	LOG_TRACE << "[9] usecount=" << conn.use_count();

	(void)n;
	assert(n==1);

	loop_->queueInLoop(
		boost::bind(&TcpConnection::connectDestroyed, conn));

	LOG_TRACE << "[10] usecount=" << conn.use_count(); */

	loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
			 << "] - connection " << conn->name();

	LOG_TRACE << "[8] usecount=" << conn.use_count();
	size_t n = connections_.erase(conn->name()); // 移除连接
	LOG_TRACE << "[9] usecount=" << conn.use_count();

	(void)n;
	assert(n == 1);

	EventLoop* ioLoop = conn->getLoop(); // 调用connect的loop，然后注册connectDestroyed函数
	ioLoop->queueInLoop(
			boost::bind(&TcpConnection::connectDestroyed, conn)); 
	LOG_TRACE << "[10] usecount=" << conn.use_count();
}
