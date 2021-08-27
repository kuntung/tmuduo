#include <tmuduo/net/TcpClient.h>

#include <tmuduo/base/Logging.h>
#include <tmuduo/net/Connector.h>
#include <tmuduo/net/EventLoop.h>
#include <tmuduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace tmuduo;
using namespace tmuduo::net;

namespace tmuduo
{
namespace net
{
namespace detail
{
void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
	loop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector)
{

}

} // end of detail

} // end of net

} // end of tmuduo


TcpClient::TcpClient(EventLoop* loop,
				const InetAddress& serverAddr,
				const string& name)
  : loop_(CHECK_NOTNULL(loop)),
    connector_(new Connector(loop, serverAddr)),
	name_(name),
	connectionCallback_(defaultConnectionCallback),
	messageCallback_(defaultMessageCallback),
	retry_(false),
	connect_(true),
	nextConnId_(1)
{
	// 设置回调
	connector_->setNewConnectionCallback(
				boost::bind(&TcpClient::newConnection, this, _1));
	
	LOG_INFO << "TcpClient::TcpClient[" << name_
			 << "] - connector " << get_pointer(connector_);
}

TcpClient::~TcpClient()
{
	LOG_INFO << "TcpClient::~TcpClient[" << name_
			 << "] - connetor " << get_pointer(connector_);
	
	TcpConnectionPtr conn;
	{
		MutexLockGuard lock(mutex_);
		conn = connection_;
	}
	if (conn)
	{
		CloseCallback cb = boost::bind(&detail::removeConnection, loop_, _1);
		loop_->runInLoop(
			boost::bind(&TcpConnection::setCloseCallback, conn, cb)); // 设置tcpconnection的关闭回调函数
			// 用于关闭这个TCP链接
	}
	else
	{
		// 这种情况说明，connector处于未连接的状态，将connector_终止
		connector_->stop();

		loop_->runAfter(1, boost::bind(&detail::removeConnector, connector_));
		// 不是绑定的TcpClient的removeConnection，因为不需要retry
	}
}

void TcpClient::connect()
{
	LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
			 << connector_->serverAddress().toIpPort();
	connect_ = true;
	connector_->start(); // 发起连接
}

// 用于连接已经建立的情况下，关闭连接。其实是shutWrite
void TcpClient::disconnect()
{
	connect_ = false;

	{
		MutexLockGuard lock(mutex_);
		if (connection_)
		{
			connection_->shutdown(); // 这里会
		}
	}
}

// 停止connector_
void TcpClient::stop()
{
	connect_ = false;
	connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
	loop_->assertInLoopThread();
	InetAddress peerAddr(sockets::getPeerAddr(sockfd));
	char buf[32];
	snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
	++nextConnId_;
	string connName = name_ + buf;

	InetAddress localAddr(sockets::getLocalAddr(sockfd));

	TcpConnectionPtr conn(new TcpConnection(loop_,
											connName,
											sockfd,
											localAddr,
											peerAddr));
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(
		boost::bind(&TcpClient::removeConnection, this, _1)); 
	{
		MutexLockGuard lock(mutex_);
		connection_ = conn; // 保存TcpConnection
	}
	conn->connectEstablished(); // 这里回调connectionCallback_,并且关注TCP套接字缓冲区消息可读事件
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());

	{
		MutexLockGuard lock(mutex_);
		assert(connection_ == conn);
		connection_.reset();
	}

	loop_->queueInLoop(
		boost::bind(&TcpConnection::connectDestroyed, conn));

	if (retry_ && connect_)
	{
		// 如果设置了重连，并且还需要connect_
		LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
			   	 << connector_->serverAddress().toIpPort();

		// 这里的重连是指：连接建立成功之后断开的重连
		connector_->restart();
	}
}
