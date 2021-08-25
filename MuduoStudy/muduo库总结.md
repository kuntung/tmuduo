# base库

**工作描述：**

1. 实现时间戳类、原子性操作类、异常类等的封装；实现线程安全的单例模式类
2. 线程类、底层IPC类（互斥锁、条件变量、倒计时门闩）、线程本地存储ThreadLocal类的封装实现，用于支持多线程并发
3. 日志系统实现：支持不同的日志记录级别、支持日志滚动、支持**多线程写入日志文件**
   - 通过宏定义实现调用不同的构造函数Logger
   - LogFile实现后两者
4. 线程池实现：能够动态销毁线程
   - 需要添加的功能，需要知道原理。（pthread_cond_timedwait)
5. 定时器Timer实现：支持注册关注定时事件

**设计思想：**

1. RAII资源管理技术（MutexLockGuard）
2. 基于对象的编程思想（函数回调的形式）

# 

# net库

## TcpServer的完整业务逻辑

### TcpServer的设置

**成员变量：**

```c++
typedef std::map<string, TcpConnectionPtr> ConnectionMap;

EventLoop* loop_;  // the acceptor loop
const string hostport_;		// 服务端口
const string name_;			// 服务名
boost::scoped_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
boost::scoped_ptr<EventLoopThreadPool> threadPool_;
ConnectionCallback connectionCallback_;
MessageCallback messageCallback_;
WriteCompleteCallback writeCompleteCallback_;		// 数据发送完毕，会回调此函数
ThreadInitCallback threadInitCallback_;	// IO线程池中的线程在进入事件循环前，会回调用此函数
bool started_;
// always in loop thread
int nextConnId_;				// 下一个连接ID
ConnectionMap connections_;	// 连接列表
```

**成员函数**

```c++
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
  // Acceptor::handleRead函数中会回调用TcpServer::newConnection
  // _1对应的是socket文件描述符，_2对应的是对等方的地址(InetAddress)
  acceptor_->setNewConnectionCallback(
      boost::bind(&TcpServer::newConnection, this, _1, _2));
}

// 通过构造函数，初始化server的信息:
// 1. 由acceptor那个eventloop负责担任main reactor
// 2. 服务器的地址(用于打印显示）：InetAddress的toIpPort()会调用socketsOps封装好的
void sockets::toIpPort(char* buf, size_t size,
                       const struct sockaddr_in& addr)
{
  char host[INET_ADDRSTRLEN] = "INVALID";
  toIp(host, sizeof host, addr);
  uint16_t port = sockets::networkToHost16(addr.sin_port);
  snprintf(buf, size, "%s:%u", host, port);
}

// 3. 服务器端的名字
// 4. 初始化一个acceptor用来监听连接请求，绑定到当前loop，地址为listenAddr
// 会调用，在TcpServer构造时，构造的成员变量accptor_，进而构造的acceptSocket_的listen()
Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie()),
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.bindAddress(listenAddr); // 绑定地址
  acceptChannel_.setReadCallback(
      boost::bind(&Acceptor::handleRead, this));
} 
// 在这里已经设置了当前acceptor套接字acceptSocket_的一些属性setReuseAddr
// 设置了有连接到来的事件处理函数
void Acceptor::handleRead()
{
  loop_->assertInLoopThread();
  InetAddress peerAddr(0);
  //FIXME loop until no more
  int connfd = acceptSocket_.accept(&peerAddr);
  if (connfd >= 0) // 成功返回
  {
    // string hostport = peerAddr.toIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (newConnectionCallback_)
    {
      newConnectionCallback_(connfd, peerAddr); 
        // 如果注册了相应的事件处理函数，就回调
		// ----->通过如下函数注册 
        // void setNewConnectionCallback(const NewConnectionCallback& cb)
        // 在TcpServer构造函数{}中注册，注册为TcpServer::newConnection
        // 目的在于，当连接到来的时候，应该回调TcpServer提供的newConnection处理业务
    }
    else
    {
      sockets::close(connfd);
    }
  }
  else 
  {
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of livev.
    if (errno == EMFILE)
    {
      ::close(idleFd_);
      idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
      ::close(idleFd_);
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

// Acceptor在handleRead的时候，处理的实际业务逻辑
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
  loop_->assertInLoopThread();
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
  ++nextConnId_;
  string connName = name_ + buf; // 生成新链接的名字
    
  InetAddress localAddr(sockets::getLocalAddr(sockfd)); // 生成本地地址
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));  
    // 为当前连接创建一个TcpConnectionPtr来管理
  connections_[connName] = conn;
    // TcpConnectionPtr的回调函数注册
  conn->setConnectionCallback(connectionCallback_); // 连接响应函数，由用户提供
  conn->setMessageCallback(messageCallback_); // 消息到来响应函数，由用户提供
  conn->setCloseCallback(
      boost::bind(&TcpServer::removeConnection, this, _1));
  conn->connectEstablished(); 
    // 调用connectEstablished，更改相应的连接状态，并且channel加入到poller监听
}
-------------------------------/*connectEstablished()*/---------------------
void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->tie(shared_from_this()); // shared_from_this临时shared_ptr对象
  channel_->enableReading();	// TcpConnection所对应的通道加入到Poller关注

  connectionCallback_(shared_from_this());
}

// 紧接着，初始化一个IO线程池，默认大小为0，可以通过server_.setThreadNum设定
// 初始化连接到来、消息到来，消息发送完毕的回调函数
```

**在初始化成功tcpserver之后，会调用start函数**

```c++
// 该函数多次调用是无害的
// 该函数可以跨线程调用
void TcpServer::start()
{
  if (!started_)
  {
    started_ = true;
    threadPool_->start(threadInitCallback_); // 线程池启动，
  }

  if (!acceptor_->listenning())
  {
	// get_pointer返回原生指针
    loop_->runInLoop(
        boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

// 线程池启动工作
void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
  assert(!started_);
  baseLoop_->assertInLoopThread();

  started_ = true;
 // 初始化numThreads_数目的IO线程，并且会执行EventLoopThread::startLoop()
  for (int i = 0; i < numThreads_; ++i)
  {
    EventLoopThread* t = new EventLoopThread(cb);
    threads_.push_back(t);
    loops_.push_back(t->startLoop());	// 启动EventLoopThread线程，在进入事件循环之前，会调用cb
  }
  if (numThreads_ == 0 && cb)
  {
    // 只有一个EventLoop，在这个EventLoop进入事件循环之前，调用cb
    cb(baseLoop_);
  }
}

// EventLoop::startLoop
EventLoop* EventLoopThread::startLoop()
{
  assert(!thread_.started());
  thread_.start();

  {
    MutexLockGuard lock(mutex_);
    while (loop_ == NULL)
    {
      cond_.wait();  
        // 会在这里等待县城入口函数执行完毕（即等待线程入口函数创建一个loop对象
    }
  }

  return loop_;
}

// Acceptor::listen所做工作：
// 将服务器启动状态started_标记为true
// 将拥有的acceptor对象启动监听。通过向loop添加事件runInLoop()注册
--------------------------------------------------------------------------
void Acceptor::listen()
{
  loop_->assertInLoopThread();
  listenning_ = true; 
  acceptSocket_.listen(); 
    // 套接字启动监听：调用了sockets::listenOrDie(sockfd_);
  acceptChannel_.enableReading(); // 将监听通道纳入poller管理
    // 这里的纳入管理，是通过enableReading调用update实现
}
```

**loop如何开始业务逻辑处理？**

> 在前面，我们将`Acceptor::listen`添加到loop的functors等待执行。可是何时运行呢？

```c++
// 在I/O线程中执行某个回调函数，该函数可以跨线程调用
void EventLoop::runInLoop(const Functor& cb)
{
  if (isInLoopThread())
  {
    // 如果是当前IO线程调用runInLoop，则同步调用cb
    cb();
  }
  else
  {
    // 如果是其它线程调用runInLoop，则异步地将cb添加到队列
    queueInLoop(cb);
  }
}

// 如果是跨线程调用，则当前任务不会立即执行
// 而是添加到Loop的pendingFunctors_等待执行。
void EventLoop::queueInLoop(const Functor& cb)
{
  {
  MutexLockGuard lock(mutex_);
  pendingFunctors_.push_back(cb);
  }

  // 调用queueInLoop的线程不是IO线程需要唤醒
  // 或者调用queueInLoop的线程是IO线程，并且此时正在调用pending functor，需要唤醒
  // 只有IO线程的事件回调中调用queueInLoop才不需要唤醒
  if (!isInLoopThread() || callingPendingFunctors_)
  {
      // 如果不在当前IO线程，或者正在执行添加的一系列Functor、需要唤醒
    wakeup();
  }
}
// 如果不进行唤醒，就可能会产生饥饿
void EventLoop::wakeup()
{
  uint64_t one = 1;
  //ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
  ssize_t n = ::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

// 唤醒之后，eventLoop的loop就会有返回，即poller_->poll会返回。
// 然后顺序执行监听的activechannels相应事件
// 然后doPendingFunctors();
void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
  MutexLockGuard lock(mutex_);
  functors.swap(pendingFunctors_);
  }

  for (size_t i = 0; i < functors.size(); ++i)
  {
    functors[i]();
  }
  callingPendingFunctors_ = false;
}
```

> 可以看到，我们如果是跨线程调用了`TcpServer的start`，并且当前`TcpServer`的事件循环没有开启。我们没有办法wakeup的。因为无法启动监听。
>
> 这也是为什么，我们测试样例会有一个`loop.loop()`工作

### TcpServer的事件响应流程

1. 当有新的连接到来的时候

   - 首先，底层的poller返回的activeChannels里面，会有`Acceptor`的通道`  Channel acceptChannel_;`
   - 进而会调用我们初始化`Acceptor`时，绑定的`Acceptor::handleRead()`

2. 在处理新的连接请求的时候，会调用`  int connfd = acceptSocket_.accept(&peerAddr);`来接收该连接。

   - 如果`connfd>= 0`，表明接收成功。此时，如果我们需要对这个成功的连接进行一些业务逻辑处理。就需要在初始化`Acceptor`的时候设置`newConnectionCallback_`

     > 这一步是在，TcpServer构造函数中实现的

     ```c++
     void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
     {
       loop_->assertInLoopThread();
       // 按照轮叫的方式选择一个EventLoop
       EventLoop* ioLoop = threadPool_->getNextLoop();
       char buf[32];
       snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
       ++nextConnId_;
       string connName = name_ + buf;
     
       InetAddress localAddr(sockets::getLocalAddr(sockfd));
       TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                               connName,
                                               sockfd,
                                               localAddr,
                                               peerAddr));
     
       connections_[connName] = conn;
         // 将tcp的回调函数注册为TcpServer的成员变量
       conn->setConnectionCallback(connectionCallback_);
       conn->setMessageCallback(messageCallback_);
       conn->setWriteCompleteCallback(writeCompleteCallback_);
     
       conn->setCloseCallback(
           boost::bind(&TcpServer::removeConnection, this, _1));
         // 这里的轮叫，是跨线程调用，所以应该runInLoop的形式
       ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
     
     }
     ```

   - 如果没有设置新连接处理函数，我们直接`sockets::close(connfd)`-

   - 如果`connfd<0`，说明出错

     - 如果错误为`EMFILE`，表明当前连接数达到上限，因此需要处理

       ```c
       if (errno == EMFILE)
       {
           ::close(idleFd_);
           idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
           ::close(idleFd_);
           idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
       }

     - 其他错误，就忽略？

3. 通常情况下，我们对于新的连接，需要在TcpServer中进行管理。即将这个连接，用一个`TcpConnectionPtr`智能指针对象来管理。

   - 对该新连接进行初始化
     - conneName
     - 所属的eventloop
     - 文件描述符
     - 对等端地址
     
     - tcp通道回调函数注册（用于已连接套接字的业务逻辑处理）

   ```c++
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
     // 通道可读事件到来的时候，回调TcpConnection::handleRead，_1是事件发生时间
     channel_->setReadCallback(
         boost::bind(&TcpConnection::handleRead, this, _1));
     // 通道可写事件到来的时候，回调TcpConnection::handleWrite
     channel_->setWriteCallback(
         boost::bind(&TcpConnection::handleWrite, this));
     // 连接关闭，回调TcpConnection::handleClose
     channel_->setCloseCallback(
         boost::bind(&TcpConnection::handleClose, this));
     // 发生错误，回调TcpConnection::handleError
     channel_->setErrorCallback(
         boost::bind(&TcpConnection::handleError, this));
     LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
               << " fd=" << sockfd;
     socket_->setKeepAlive(true);
   }
   ```

   - 注册的回调函数`connectionCallback_,messageCallback_ `，是由TcpServer的成员函数提供。

     > 这些回调函数，又是通过包含`TcpServer`的具体`Server`类在构造函数中提供。

   - 跨线程调用`TcpConnection::connectEstablished()`，将该套接字对应的channel注册（通过enableReading)

     ```c++
     void TcpConnection::connectEstablished()
     {
       loop_->assertInLoopThread();
       assert(state_ == kConnecting);
       setState(kConnected);
       channel_->tie(shared_from_this()); // shared_from_this临时shared_ptr对象
       // void tie(const std::shared_ptr<void>&);
       channel_->enableReading();	// TcpConnection所对应的通道加入到Poller关注
       connectionCallback_(shared_from_this());
     }
     ```

4. 对于每一个`TcpConnection`对象，它都拥有

   - 一个智能指针管理的`Socket`(主要是文件描述符)
   - 一个智能指针管理的`Channel`，用于事件响应

5. 当poller返回活跃通道之后，EventLoop会遍历执行channel的`handleEvent`函数

   ```c++
   void Channel::handleEvent(Timestamp receiveTime)
   {
     boost::shared_ptr<void> guard;
       // 根据当前是否绑定，在TcpConnetion::connectEstablished中绑定
       // 目的在于：防止在tcpServer中就将该channel析构，造成core dump
     if (tied_)
     {
       guard = tie_.lock();
       if (guard)
       {
         LOG_TRACE << "[6] usecount=" << guard.use_count();
         handleEventWithGuard(receiveTime);
   	  LOG_TRACE << "[12] usecount=" << guard.use_count();
       }
     }
     else
     {
       handleEventWithGuard(receiveTime);
     }
   }
   
   // 具体业务逻辑就是由：handleEventWithGuard执行
   void Channel::handleEventWithGuard(Timestamp receiveTime)
   {
     eventHandling_ = true;
     if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) 
     {
       if (logHup_)
       {
         LOG_WARN << "Channel::handle_event() POLLHUP";
       }
       if (closeCallback_) closeCallback_(); // TcpConnection::handleClose
     }
   
     if (revents_ & POLLNVAL)
     {
       LOG_WARN << "Channel::handle_event() POLLNVAL";
     }
   
     if (revents_ & (POLLERR | POLLNVAL))
     {
       if (errorCallback_) errorCallback_(); // TcpConnection::handleError
     }
     if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
     {
       if (readCallback_) readCallback_(receiveTime);   
         // TcpConnection::handleRead
     }
     if (revents_ & POLLOUT)
     {
       if (writeCallback_) writeCallback_(); // TcpConnection::handleWrite
     }
     eventHandling_ = false;
   }
   
   // 根据具体的POLL事件类型，执行不同的业务。由TcpConnection提供的回调函数
   // 在具体的handleClose，handleWrite等中，其实又涉及到TcpConnection接收注册的
   //  ConnectionCallback connectionCallback_;
   //  MessageCallback messageCallback_;
   // 由TcpServer的newConnection提供。这又有外部提供。
   // POLLIN和POLLHUP事件：发生在client端read为0，调用close
   ```

6. 进而，调用channel的相应的处理函数

### TcpConnection的数据发送和接收

**因为TCP是面向连接的字节流协议，因此，存在着粘包，数据消息不完整的情况。**

为了避免IO线程阻塞在数据收发的逻辑上，因此需要提供应用层缓冲区，buffer。

1. 当要发送数据的时候，直接调用`send`发送数据(可以跨线程调用)

2. 如果是跨线程调用，则`runInLoop`添加`sendInLoop`
3. 当`sendInLoop`发送完毕，如果注册了`消息发送完毕`事件，那么需要通知应用层继续发送数据`writeCompleteCallback`
4. 如果没有发送完毕，则添加关注`POLLOUT`事件。
5. 之后就是`handleWrite`继续处理应用层数据发送事件。当发送完毕，调用writeCompleteCallback，并且取消关注`POLLOUT`事件

### TcpConnection的连接断开与shutdownWrite

1. 当应用层调用shutdown的时候，会将当前连接状态置为kDisconnecting
2. 并且，在loop中添加一个任务shutdownInLoop
3. 如果此时没有消息需要发送，（此时不会关注POLLOUT）
4. 那么就直接调用sockets::shutdownWrite()
5. 如果此时，处于消息发送状态，那么，会持续关注POLLOUT
6. 并且当内核缓冲区可写，即handleWrite处理业务逻辑的时候。
7. 当`outputBuffer_.readableBytes() == 0`的时候，会通知上层应用程序，`writeCompleteCallback_`

```c++
if (state_ == kDisconnecting)                                
{                                                            
   shutdownInLoop(); // 关闭write端                         
}                                                                             
```

**也许会认为，writeCompleteCallback_回调里面也有send，会继续填充outputBuffer\_?**
**答案：不会，因为send的时候，会判断state_ == kConnected**

```c++
// 线程安全，可以跨线程调用
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
                    //std::forward<string>(message)));
    }
  }
}
```

### chargen测试代码实现

```c++
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

class TestServer
{
 public:
  TestServer(EventLoop* loop,
             const InetAddress& listenAddr)
    : loop_(loop),
      server_(loop, listenAddr, "TestServer")
  {
     // 为TcpServer注册回调函数，用于TcpConnection
    server_.setConnectionCallback(
        boost::bind(&TestServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&TestServer::onMessage, this, _1, _2, _3));
	server_.setWriteCompleteCallback(
      boost::bind(&TestServer::onWriteComplete, this, _1));

    string line;
    for (int i = 33; i < 127; ++i)
    {
      line.push_back(char(i));
    }
    line += line;

    for (size_t i = 0; i < 127-33; ++i)
    {
      message_ += line.substr(i, 72) + '\n';
    }
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

      conn->setTcpNoDelay(true);
      conn->send(message_);
    }
    else
    {
      printf("onConnection(): connection [%s] is down\n",
             conn->name().c_str());
    }
  }

  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime)
  {
    muduo::string msg(buf->retrieveAllAsString());
	printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
           msg.size(),
           conn->name().c_str(),
           receiveTime.toFormattedString().c_str());

    conn->send(msg);
  }

  void onWriteComplete(const TcpConnectionPtr& conn)
  {
    conn->send(message_);
  }

  EventLoop* loop_;
  TcpServer server_;

  muduo::string message_;
};


int main()
{
  printf("main(): pid = %d\n", getpid());

  InetAddress listenAddr(8888);
  EventLoop loop;

  TestServer server(&loop, listenAddr);
  server.start();

  loop.loop();
}
```

## Buffer的设计

### 为什么需要应用层缓冲区buffer

### 设计的要求

**数据结构：**

- vector of char

## EventLoop的实现

### poller设计

1. 使用epoll的LT模式的原因

   ![image-20210824165131853](muduo库总结.assets/image-20210824165131853.png)

2. 因为ET模式不一定就比LT模式效率更高

   - 业务逻辑复杂
   - 比LT模式要多几次系统调用，迟延比较低。
   - 主流的网络库，libevent，boost的asio都是用LT模式

# muduo测试

## chargen测试

