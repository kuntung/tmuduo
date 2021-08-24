#include <tmuduo/net/EventLoopThread.h>

#include <tmuduo/net/EventLoop.h>

#include <boost/bind.hpp>

using namespace tmuduo;
using namespace tmuduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
  : loop_(NULL),
    exiting_(false),
	thread_(boost::bind(&EventLoopThread::threadFunc, this)),
	mutex_(),
	cond_(mutex_),
	callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
	exiting_ = true;
	loop_->quit(); // 退出IO线程，让IO线程的loop循环退出，从而退出了IO线程
	thread_.join();
}

EventLoop* EventLoopThread::startLoop()
{
	assert(!thread_.started());
	thread_.start();

	{
		MutexLockGuard lock(mutex_);
		while (loop_ == NULL)
		{
			cond_.wait(); // 等待eventloop启动
		}
	}

	return loop_;
}

void EventLoopThread::threadFunc()
{
	EventLoop loop;

	if (callback_)
	{
		callback_(&loop);
	}

	{
		MutexLockGuard lock(mutex_);
		// loop_指针指向一个栈上的对象，threadFunc退出以后，这个指针就失效了
		// threadFunc函数退出，也就意味着线程退出。EventLoopThread对象也没有存在的价值了。
		// 因此不会有太大的问题
		loop_ = &loop;
		cond_.notify(); // 通知startloop能够返回loop_指针
	}

	loop.loop();
}
