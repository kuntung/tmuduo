#include <tmuduo/net/TcpConnection.h>

#include <tmuduo/base/Logging.h>
#include <tmuduo/net/Channel.h>
#include <tmuduo/net/EventLoop.h>
#include <tmuduo/net/Socket.h>
#include <tmuduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace tmuduo;
using namespace tmuduo::net;

TcpConnection::TcpConnection(EventLoop* loop,
						const string& nameArg,
						int sockfd,
						const InetAddress& localAddr,
						const InetAddress& peerAddr)
  : loop_(CHECK_NOTNULL(loop)),
    name_(nameArg),
	state_(kConnecting),
	socket_(new Socket(sockfd)),
	channel_(new Channel(loop, sockfd)),
	localAddr_(localAddr),
	peerAddr_(peerAddr) /*
	highWaterMark_(64*1024*1024)*/
{
	// 通道可读事件到来的时候，回调TcpConnection::handleRead, _1是事件发生时间
	channel_->setReadCallback(
		boost::bind(&TcpConnection::handleRead, this, _1));
	
	// 连接关闭，回调TcpConnection::handleClose
	channel_->setCloseCallback(
		boost::bind(&TcpConnection::handleClose, this));
	
	// 错误处理函数:TcpConnection::handleError
	channel_->setErrorCallback(
		boost::bind(&TcpConnection::handleError, this));
	
	LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
			  << " fd = " << sockfd;
	
	socket_->setKeepAlive(true); // 心跳检测
}

TcpConnection::~TcpConnection()
{
	LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
			  << " fd=" << channel_->fd();
}

void TcpConnection::connectEstablished()
{
	loop_->assertInLoopThread();
	assert(state_ == kConnecting);
	setState(kConnected);

	LOG_TRACE << "[3] usecount=" << shared_from_this().use_count();
	channel_->tie(shared_from_this()); // 绑定一个临时的shared_ptr对象

	channel_->enableReading(); // TcpConnection所对应的通道加入到Poller关注

	connectionCallback_(shared_from_this()); // 建立执行connectionCallback_

	LOG_TRACE << "[4] usecount=" << shared_from_this().use_count();
}

void TcpConnection::connectDestroyed() /* 链接关闭函数 */
{
	loop_->assertInLoopThread();

	if (state_ == kConnected)
	{
		setState(kDisconnected);
		channel_->disableAll();

		connectionCallback_(shared_from_this()); // 连接关闭的时候，也要调用用户注册的onConnection函数。
		// 只是此时是根据state_来执行不同的逻辑
	}

	channel_->remove(); // 会调用loop->removeChannel(this); 从poller中取消关注
}


void TcpConnection::handleRead(Timestamp receiveTime)
{
	/*
	loop_->assertInLoopThread();
	int savedErrno = 0;
	ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
	if (n > 0)
	{
	messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
	}
	else if (n == 0)
	{
	handleClose();
	}
	else
	{
	errno = savedErrno;
	LOG_SYSERR << "TcpConnection::handleRead";
	handleError();
	}
	*/
	loop_->assertInLoopThread();
	int savedErrno = 0;
	char buf[65536];
	ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
	if (n > 0)
	{
		messageCallback_(shared_from_this(), buf, n);
	}
	else if (n == 0) /* 如果read返回为0，说明连接关闭 */
	{
		handleClose();
	}
	else
	{
		errno = savedErrno;
		LOG_SYSERR << "TcpConnection::handleRead";
		handleError();
	}
	
}

void TcpConnection::handleClose()
{
	loop_->assertInLoopThread();
	LOG_TRACE << "fd = " << channel_->fd() << " state = " << state_;
	assert(state_ == kConnected || state_ == kDisconnecting); // 
	setState(kDisconnected);

	channel_->disableAll(); // 取消监听

	TcpConnectionPtr guardThis(shared_from_this()); // 为了保证当前TcpConnectionPtr不被销毁
	connectionCallback_(guardThis); 
	LOG_TRACE << "[7] usecount=" << guardThis.use_count();

	closeCallback_(guardThis);
	LOG_TRACE << "[8] usecount=" << guardThis.use_count();
	
}


void TcpConnection::handleError()
{
	int err = sockets::getSocketError(channel_->fd());

	LOG_ERROR << "TcpConnection::handleError [" << name_
			  << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
