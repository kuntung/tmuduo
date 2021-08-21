#include <tmuduo/net/EventLoop.h>

#include <tmuduo/base/Logging.h>
#include <tmuduo/net/Channel.h>
#include <tmuduo/net/Poller.h>

using namespace tmuduo;
using namespace tmuduo::net;

namespace /* 匿名命名空间 */
{
// 当前线程EventLoop对象指针，线程局部存储 
__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
	eventHandling_(false),
	threadId_(CurrentThread::tid()),
	poller_(Poller::newDefaultPoller(this)),
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
}

EventLoop::~EventLoop()
{
	t_loopInThisThread = NULL;
}

// 事件循环，不能够跨线程调用，只能在创建该对象的线程中调用
void EventLoop::loop()
{
	/* 对外暴露的接口 */
	assert(!looping_);

	assertInLoopThread();
	looping_ = true;

	LOG_TRACE << "EventLoop " << this << " start looping";
	
	// 开始事件循环监听
	while (!quit_)
	{
		activeChannels_.clear();
		pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

		if (Logger::logLevel() <= Logger::TRACE)
		{
			printActiveChannels();
		}

		eventHandling_ = true;
		for (auto& channelPtr : activeChannels_)
		{
			channelPtr->handleEvent(pollReturnTime_);
		}

		currentActiveChannel_ = NULL;
		eventHandling_ = false;
		// doPendingFunctors();
	} // end of while (!quit_)
}

// 可以跨线程调用的quit
void EventLoop::quit()
{
	quit_ = true;
	if (!isInLoopThread()) // 如果不在IO线程中调用quit，就需要唤醒poll
	{
		// wakeup();
	}
}

void EventLoop::updateChannel(Channel* channel) // 由传入参数channel发起调用，并且修改该channel的关注状态
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
		assert(currentActiveChannel_ == channel /* 什么意思 */ ||
			std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
	}

	poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
	LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
			  << " was created in threadId_ = " << threadId_
			  << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::printActiveChannels() const
{
	for (auto& channelPtr : activeChannels_)
	{
		LOG_TRACE << "{" << channelPtr->reventsToString() << "}";	
	}
}
