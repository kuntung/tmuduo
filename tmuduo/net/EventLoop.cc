// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <tmuduo/net/EventLoop.h>

#include <tmuduo/base/Logging.h>
#include <tmuduo/net/Channel.h>
#include <tmuduo/net/Poller.h>
#include <tmuduo/net/TimerQueue.h>

//#include <poll.h>

#include <boost/bind.hpp> // for boost::bind
#include <sys/eventfd.h> // for wake up fd
using namespace tmuduo;
using namespace tmuduo::net;

namespace
{
// 当前线程EventLoop对象指针
// 线程局部存储
__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000;
}

// createEventFd()
int createEventFd()
{
	int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evtfd < 0)
	{
		LOG_SYSERR << "Failed in eventfd";
		abort();
	}

	return evtfd;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
    eventHandling_(false),
	callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
	poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)),
	wakeupFd_(createEventFd()),
	wakeupChannel_(new Channel(this, wakeupFd_)),
	currentActiveChannel_(NULL)
{
  LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
  // 如果当前线程已经创建了EventLoop对象，终止(LOG_FATAL)
  if (t_loopInThisThread)
  {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;
  }
  else
  {
    t_loopInThisThread = this;
  }

  wakeupChannel_->setReadCallback(
  		boost::bind(&EventLoop::handleRead, this)); // 当这个通道响应的时候，应该去doPendingFunctors
		
  wakeupChannel_->enableReading(); // 加入关注

}

EventLoop::~EventLoop()
{
  ::close(wakeupFd_); // 只负责这一个文件描述符的生命你该周期
  t_loopInThisThread = NULL;
}

// 事件循环，该函数不能跨线程调用
// 只能在创建该对象的线程中调用
void EventLoop::loop()
{
  assert(!looping_);
  // 断言当前处于创建该对象的线程中
  assertInLoopThread();
  looping_ = true;
  LOG_TRACE << "EventLoop " << this << " start looping";

  //::poll(NULL, 0, 5*1000);
  while (!quit_)
  {
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    //++iteration_;
    if (Logger::logLevel() <= Logger::TRACE)
    {
      printActiveChannels();
    }
    // TODO sort channel by priority
    eventHandling_ = true;
    for (ChannelList::iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
      currentActiveChannel_ = *it;
      currentActiveChannel_->handleEvent(pollReturnTime_);
    }
    currentActiveChannel_ = NULL;
    eventHandling_ = false;
    doPendingFunctors();
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

// 该函数可以跨线程调用:g_loop->quit()
void EventLoop::quit()
{
  quit_ = true;
  if (!isInLoopThread())
  {
    wakeup();
  }
}

// runInLoop：在IO线程中执行某个添加的回调函数，该函数可以跨线程调用
void EventLoop::runInLoop(const Functor& cb)
{
	if (isInLoopThread())
	{
		cb(); // 如果是当前IO线程调用runInLoop，则同步调用cb
	}
	else
	{
		queueInLoop(cb); // 否则，将该cb添加到队列，等待poll返回执行
	}

}

void EventLoop::queueInLoop(const Functor& cb)
{
	{
		MutexLockGuard lock(mutex_);
		pendingFunctors_.push_back(cb); // 互斥的访问临界资源pendingFunctors_
	}
	// 只有IO线程的事件回调中调用queueInLoop才不需要唤醒
	if (!isInLoopThread() || callingPendingFunctors_)
	{
		// callingPendingFunctors_ = true表示当前IO线程正在执行pendingFunctor
		wakeup();
	}
}

void EventLoop::wakeup()
{
	uint64_t one = 1; // 因为eventfd包含一个64位的integer

	ssize_t n = ::write(wakeupFd_, &one, sizeof one);

	if (n != sizeof one)
	{
		LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8.";
	}
}

void EventLoop::handleRead() // wakeupChannel注册的回调函数，读走写入的8个字节。为了避免busy-loop
{
	uint64_t one = 1;

	ssize_t n = ::read(wakeupFd_, &one, sizeof one);

	if (n != sizeof one)
	{
		LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8.";
	}
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true; // 表明当前处于自定义事件的处理状态

	{
		MutexLockGuard lock(mutex_); // 保护临界区
		functors.swap(pendingFunctors_);
	}

	for (size_t i = 0; i < functors.size(); ++i)
	{
		functors[i]();
	}

	callingPendingFunctors_ = false;
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
  return timerQueue_->addTimer(cb, time, 0.0); // 定时计时器
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), delay)); 
  return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(cb, time, interval);  // 间隔定时计时器器
}

void EventLoop::cancel(TimerId timerId)
{
  return timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (eventHandling_)
  {
    assert(currentActiveChannel_ == channel ||
        std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}

void EventLoop::printActiveChannels() const
{
  for (ChannelList::const_iterator it = activeChannels_.begin();
      it != activeChannels_.end(); ++it)
  {
    const Channel* ch = *it;
    LOG_TRACE << "{" << ch->reventsToString() << "} ";
  }
}
