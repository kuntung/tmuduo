#include <tmuduo/net/Connector.h>
#include <tmuduo/base/Logging.h>
#include <tmuduo/net/Channel.h>
#include <tmuduo/net/EventLoop.h>
#include <tmuduo/net/SocketsOps.h>

#include <boost/bind.hpp>
#include <errno.h>

using namespace tmuduo;
using namespace tmuduo::net;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
  : loop_(loop),
    serverAddr_(serverAddr),
	connect_(false),
	state_(kDisconnected),
	retryDelayMs_(kInitRetryDelayMs)
{
	LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
	LOG_DEBUG << "dcto[" << this << "]";
	assert(!channel_);
}

// 可以跨线程使用的start()
void Connector::start()
{
	connect_ = true;
	loop_->runInLoop(boost::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
	loop_->assertInLoopThread();
	assert(state_ == kDisconnected);
	if (connect_)
	{
		connect();
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}

void Connector::stop()
{
	connect_ = false;
	loop_->runInLoop(
			boost::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop()
{
	loop_->assertInLoopThread();
	if (state_ == kConnecting)
	{
		setState(kDisconnected);
		int sockfd = removeAndResetChannel(); // 将通道从poller中移除关注，并将channel置空
		retry(sockfd); // 这里并非要重连，只是调用sockets::close(sockfd);
	}

}

void Connector::connect()
{
	int sockfd = sockets::createNonblockingOrDie(); // 创建非阻塞套接字
	int ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());
	int savedErrno = (ret == 0) ? 0 : errno;
	switch (savedErrno)
	{
		case 0:
		case EINPROGRESS: /* 非阻塞套接字，未连接成功返回码是EINPROGRESS表示正在连接 */
		case EINTR:
		case EISCONN:
			connecting(sockfd);
			break;

		case EAGAIN:
		case EADDRINUSE:
		case EADDRNOTAVAIL:
		case ECONNREFUSED:
		case ENETUNREACH:
			retry(sockfd); // 重连
			break;

		case EACCES:
		case EPERM:
		case EAFNOSUPPORT:
		case EALREADY:
		case EBADF:
		case EFAULT:
		case ENOTSOCK:
			LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
			sockets::close(sockfd); // 不能重连，关闭sockfd
			break;
		
		default:
			LOG_SYSERR << " Unexpected error in Connector::startInLoop " << savedErrno;
			sockets::close(sockfd);
			break;
	}
}

void Connector::restart()
{
	loop_->assertInLoopThread();
	setState(kDisconnected);
	retryDelayMs_ = kInitRetryDelayMs;
	connect_ = true;
	startInLoop();
}

void Connector::connecting(int sockfd)
{
	setState(kConnecting);
	assert(!channel_);
	// channel_与sockfd关联
	channel_.reset(new Channel(loop_, sockfd));
	// 设置可写回调函数，这时候如果socket没有错误，sockfd就处于可写状态
	channel_->setWriteCallback(
			boost::bind(&Connector::handleWrite, this));
	
	channel_->setErrorCallback(
			boost::bind(&Connector::handleError, this));

	channel_->enableWriting(); // 让poller关注可写事件
}

int Connector::removeAndResetChannel()
{
	channel_->disableAll();
	channel_->remove(); // 从poller中移除

	int sockfd = channel_->fd();

	loop_->queueInLoop(boost::bind(&Connector::resetChannel, this));

	return sockfd;
}

void Connector::resetChannel()
{
	channel_.reset();
}

void Connector::handleWrite()
{
	LOG_TRACE << "Connector::handleWrite" << state_;

	if (state_ == kConnecting)
	{
		int sockfd = removeAndResetChannel(); // 从poller中移除关注，并将channel置空
		// socket可写并不意味着连接一定成功
		// 还需要getsockopt(sockfd, SOL_SOCKET, SO_ERROR, ...) 再次确认

		int err = sockets::getSocketError(sockfd);

		if (err)
		{
			LOG_WARN << "Connector::handleWrite - SO_ERROR = "
					 << err << " " << strerror_tl(err);

			retry(sockfd);
		}
		else if (sockets::isSelfConnect(sockfd)) // 自连接
		{
			LOG_WARN << "Connector::handleWrite - Self connect";
			retry(sockfd);
		}
		else
		{
			setState(kConnected);
			if (connect_ && newConnectionCallback_)
			{
				newConnectionCallback_(sockfd);
			}
			else
			{
				sockets::close(sockfd);
			}
		}
	} // end if state_ == kConnecting
	else
	{
		assert(state_ == kDisconnected);
	}
}

void Connector::handleError()
{
	LOG_ERROR << "Connector::handleError";
	assert(state_ == kConnecting);

	int sockfd = removeAndResetChannel();
	int err = sockets::getSocketError(sockfd);

	LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);

}

// 采用back-off策略重连，即重连时间逐渐延长0.5s, 1s, 2s, ... 直至kMaxRetryDelayMs
void Connector::retry(int sockfd)
{
	sockets::close(sockfd);
	setState(kDisconnected);

	if (connect_)
	{
		LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
				 << " in " << retryDelayMs_ << " milliseconds. ";
		// 注册一个定时操作，重连
		loop_->runAfter(retryDelayMs_ / 1000.0,
				boost::bind(&Connector::startInLoop, shared_from_this()));
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}
