#include <tmuduo/net/EventLoopThreadPool.h>
#include <tmuduo/base/Logging.h>
#include <tmuduo/net/EventLoop.h>
#include <tmuduo/net/EventLoopThread.h>

#include <boost/bind.hpp>

using namespace tmuduo;
using namespace tmuduo::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop) /* base loop为acceptor的eventloop */
  : baseLoop_(baseLoop),
    started_(false),
	numThreads_(0),
	next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
	// do not need delete loops, they are stack variables
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
	assert(!started_);
	baseLoop_->assertInLoopThread(); // 也就是说明，EventLoopThreadPool::start应该在acceptor的线程中调用

	started_ = true;

	for (int i = 0; i < numThreads_; ++i)
	{
		EventLoopThread* t = new EventLoopThread(cb); // 将start接收到的参数用于thread初始化之前执行
		//char buf[200];
		//snprintf(buf, sizeof buf, "EventLoopThreadPool::start build  <%d> EventLoopThread, address = %p\n", i, t);
		//LOG_TRACE << buf;
		threads_.push_back(t); // 虽然t是一个堆上的对象，但是用ptr_vector管理，就可以自动管理生命周期
		loops_.push_back(t->startLoop()); // EventLoopThread::startLoop()返回一个指向IO线程的栈上的EventLoop*
	}

	if (numThreads_ == 0 && cb)
	{
		cb(baseLoop_); // 单线程模式
	}
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
	baseLoop_->assertInLoopThread();
	EventLoop* loop = baseLoop_;

	// 如果loops_为空，则loop指向baseLoop_;
	// 如果不为空，则进行轮叫
	if (!loops_.empty())
	{
		loop = loops_[next_];
		++next_;
		if (implicit_cast<size_t>(next_) >= loops_.size())
		{
			next_ = 0;
		}
	}

	return loop; // 返回一个IO线程，可能是主线程
}


