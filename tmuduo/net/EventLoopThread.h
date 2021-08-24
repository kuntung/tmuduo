#ifndef TMUDUO_NET_EVENTLOOPTHREAD_H
#define TMUDUO_NET_EVENTLOOPTHREAD_H

#include <tmuduo/base/Condition.h>
#include <tmuduo/base/Mutex.h>
#include <tmuduo/base/Thread.h>

#include <boost/noncopyable.hpp>

namespace tmuduo
{
namespace net
{

class EventLoop; // 前向声明

class EventLoopThread : boost::noncopyable
{
public:
	typedef boost::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
	~EventLoopThread();

	EventLoop* startLoop(); // 启动线程，该线程就成为了IO线程
	// 在该线程内部创建一个EventLoop对象，并将EventLoop*指向该eventLoop，用于其他线程跨线程调用runInLoop
	//
private:
	void threadFunc(); // 线程入口函数

	EventLoop* loop_; // loop_指针指向一个EventLoop对象
	bool exiting_; 
	Thread thread_;
	MutexLock mutex_;
	Condition cond_;
	ThreadInitCallback callback_; // 回调函数在EventLoop::loop事件循环之前被调用
}; // end of EventLoopThread

} // end of net

} // end of tmuduo

#endif // TMUDUO_NET_EVENTLOOPTHREAD_H
