#ifndef TMUDUO_NET_EVENTLOOP_H
#define TMUDUO_NET_EVENTLOOP_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <tmuduo/base/CurrentThread.h>
#include <tmuduo/base/Thread.h>
#include <tmuduo/base/Timestamp.h>
#include <tmuduo/net/Callbacks.h>
#include <tmuduo/net/TimerId.h>
#include <tmuduo/base/Mutex.h>

namespace tmuduo
{
namespace net
{

class Channel;
class Poller;
class TimerQueue;

// reactormoshi，一个线程最多一个eventloop对象

class EventLoop : boost::noncopyable
{
public:
	typedef boost::function<void()> Functor; // 用户或者其他线程添加的任务
	EventLoop();
	~EventLoop();

	void loop();
	void quit();

	Timestamp pollReturnTime() const { return pollReturnTime_; }

	// Runs callback immediately in the loop thread
	// It wakes up the loop, and run the cb
	// If in the same loop, cb is run within the function
	// Safe to cal from other threads
	void runInLoop(const Functor& cb);

	// Queues callback in the loop thread
	// Runs after finish polling. 会添加一个eventfd用于其他线程通知当前线程wakeup
	// safe to ...
	void queueInLoop(const Functor& cb);
	// timers的一些操作
	// 在指定的时间time后触发	
	TimerId runAt(const Timestamp& time, const TimerCallback& cb);
	/// Runs callback after @c delay seconds.
	/// Safe to call from other threads.
	///
	TimerId runAfter(double delay, const TimerCallback& cb);
	///
	/// Runs callback every @c interval seconds.
	/// Safe to call from other threads.
	///
	TimerId runEvery(double interval, const TimerCallback& cb);
	///
	/// Cancels the timer.
	/// Safe to call from other threads.
	///
	void cancel(TimerId timerId);

	// internal usage
	void wakeup(); // 用于其他线程唤醒当前线程
	void updateChannel(Channel* channel); // 在poller中添加或者更新通道
	void removeChannel(Channel* channel); // 在poller中移除通道

	void assertInLoopThread()
	{
		if (!isInLoopThread())
		{
			abortNotInLoopThread();
		}
	}

	bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

	static EventLoop* getEventLoopOfCurrentThread(); // 返回当前thread包含的eventloop

private:
	void abortNotInLoopThread();
	void handleRead(); // wake up
	void doPendingFunctors();
	void printActiveChannels() const; // for debug

	typedef std::vector<Channel*> ChannelList;// 用来管理当前的channel列表

	bool looping_; // atomic,因为这是bool类型
	bool quit_;
	bool eventHandling_; 
	bool callingPendingFunctors_;
	const pid_t threadId_; // 当前eventloop所属ID
	Timestamp pollReturnTime_;
	boost::scoped_ptr<Poller> poller_; // 通过poller_来实现事件监听
	boost::scoped_ptr<TimerQueue> timerQueue_;
	int wakeupFd_; // 用于eventfd
	boost::scoped_ptr<Channel> wakeupChannel_; // 该通道会纳入poller_管理，eventloop只负责wakeupChannel_生命周期
	ChannelList activeChannels_; // Poller返回的活动通道
	Channel* currentActiveChannel_; // 当前正在处理的活动通道

	MutexLock mutex_; // 用于执行Functors的baohu 
	std::vector<Functor> pendingFunctors_;
}; // end of EventLoop

} // end of net

} // end of tmuduo
#endif // TMUDUO_NET_EVENTLOOP_H
