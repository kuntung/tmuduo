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

void tmuduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
	LOG_TRACE << conn->localAddress().toIpPort() << " -> "
			  << conn->peerAddress().toIpPort() << " is "
			  << (conn->connected() ? "UP" : "DOWN");
}

void tmuduo::net::defaultMessageCallback(const TcpConnectionPtr& conn,
									Buffer* buf,
									Timestamp)
{
	buf->retrieveAll(); // 默认消息到来回调函数：取走所有数据
}

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
	peerAddr_(peerAddr),
	highWaterMark_(64*1024*1024)
{
	// 通道可读事件到来的时候，回调TcpConnection::handleRead, _1是事件发生时间
	channel_->setReadCallback(
		boost::bind(&TcpConnection::handleRead, this, _1));
	// 通道可写事件到来的时候，回调TcpConnection::handleWrite
	channel_->setWriteCallback(
		boost::bind(&TcpConnection::handleWrite, this));
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

// 可以跨线程调用的send
void TcpConnection::send(const void* data, size_t len)
{
	if (state_ == kConnected)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(data, len);
		}
		else
		{
			string message(static_cast<const char*>(data), len);
			loop_->runInLoop(
					boost::bind(&TcpConnection::sendInLoop,
							this,
							message));
		}
	}
}

void TcpConnection::send(const StringPiece& message)
{
	if (state_ == kConnected)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(message);
		}
		else
		{
			loop_->runInLoop(
					boost::bind(&TcpConnection::sendInLoop,
							this,
							message.as_string()));
		}
	}
}

// 线程安全，可以跨线程调用
void TcpConnection::send(Buffer* buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    }
    else
    {
      loop_->runInLoop(
          boost::bind(&TcpConnection::sendInLoop,
                      this,
                      buf->retrieveAllAsString()));
                    //std::forward<string>(message)));
    }
  }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
	loop_->assertInLoopThread();
	ssize_t nwrote = 0;
	size_t remaining = len;

	bool error = false;

	if (state_ == kDisconnected)
	{
		LOG_WARN << "disconnected, give up writing";
		return;
	}

	// 如果通道没有关注可写事件，并且发送缓冲恨国蛆没有数据，直接write
	if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
	{
		nwrote = sockets::write(channel_->fd(), data, len);

		if (nwrote >= 0)
		{
			remaining = len - nwrote;

			if (remaining == 0 && writeCompleteCallback_) /* 写完了并且需要关注WriteCompleteCallback */
			{
				loop_->queueInLoop(
						boost::bind(writeCompleteCallback_, shared_from_this()));
			}
		}
		else
		{
			nwrote = 0;
			if (errno != EWOULDBLOCK)
			{
				LOG_SYSERR << "TcpConnection::sendInLoop";
				if (errno == EPIPE)
				{
					error = true;
				}
			}
		}
	}

	assert(remaining <= len);
	// 没有错误，并且还有未写完的数据（说明内核发送缓冲区满，要将未写完的数据添加到发送缓冲区outputBuffer_
	if (!error && remaining > 0)
	{
		LOG_TRACE << "I am going to write more data";
		size_t oldLen = outputBuffer_.readableBytes();
		// 如果超过了高水位标highWaterMark_,回调highWaterMarkCallback
		if (oldLen + remaining >= highWaterMark_
		    && oldLen < highWaterMark_
			&& highWaterMarkCallback_)
		{
			loop_->queueInLoop(
					boost::bind(highWaterMarkCallback_, shared_from_this()));
		}

		outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);

		if (channel_->isWriting())
		{
			channel_->enableReading(); // 关注POLLOUT事件
		}
	}
}
/*
当应用层调用shutdown的时候，会将当前连接状态置为kDisconnecting
并且，在loop中添加一个任务shutdownInLoop
如果此时没有消息需要发送，（此时不会关注POLLOUT）
那么就直接调用sockets::shutdownWrite()
如果此时，处于消息发送状态，那么，会持续关注POLLOUT
并且当内核缓冲区可写，即handleWrite处理业务逻辑的时候。
当outputBuffer_.readableBytes() == 0的时候，会通知上层应用程序，writeCompleteCallback_
且
if (state_ == kDisconnecting)
{
	shutdownInLoop(); // 关闭write端
}

也许会认为，writeCompleteCallback_回调里面也有send，会继续填充outputBuffer_?
答案：不会，因为send的时候，会判断state_ == kConnected
*/
void TcpConnection::shutdown()
{
  // FIXME: use compare and swap
  if (state_ == kConnected)
  {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  if (!channel_->isWriting())
  {
    // we are not writing
    socket_->shutdownWrite();
  }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
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

  /*
  loop_->assertInLoopThread();
  int savedErrno = 0;
  char buf[65536];
  ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
  if (n > 0)
  {
    messageCallback_(shared_from_this(), buf, n);
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
}


// 当内核发送缓冲区满的时候，需要关注POLLOUT事件，并且当POLLOUT事件到来的时候，回调handleWrite

void TcpConnection::handleWrite()
{
	loop_->assertInLoopThread();
	if (channel_->isWriting())
	{
		ssize_t n = sockets::write(channel_->fd(),
								 outputBuffer_.peek(),
								 outputBuffer_.readableBytes());
	
	if (n > 0)
	{
		outputBuffer_.retrieve(n);
		if (outputBuffer_.readableBytes() == 0) // 发送缓冲区清空
		{
			channel_->disableWriting(); // 停止关注POLLOUT事件，避免出现busy-loop

			if (writeCompleteCallback_)
			{
				loop_->queueInLoop(
					boost::bind(writeCompleteCallback_, shared_from_this()));
			}
			if (state_ == kDisconnecting)
			{
				shutdownInLoop(); // 关闭连接
			}
		}
		else
		{
			LOG_TRACE << "I am going to write more data";
		}
	}
	else
	{
		LOG_SYSERR << "TcpConnection::handleWrite";

	}
	}
	else
	{
		LOG_TRACE << "connetion fd " << channel_->fd()
				<< " is down, no more writing";
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
