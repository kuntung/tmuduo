#ifndef TMUDUO_NET_TIMERQUEUE_H
#define TMUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include <boost/noncopyable.hpp>
#include <tmuduo/base/Mutex.h>
#include <tmuduo/base/Timestamp.h>
#include <tmuduo/net/Callbacks.h>
#include <tmuduo/net/Channel.h>


namespace tmuduo
{
namespace net
{

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : boost::noncopyable
{
public:
	TimerQueue(EventLoop* loop); // 每个timerqueue都是属于一个eventloop

	~TimerQueue();

	// 可以跨线程调用的函数
	TimerId addTimer(const TimerCallback& cb,
					Timestamp when,
					double interval);
	void cancel(TimerId timerId);

private:
  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // unique_ptr是C++ 11标准的一个独享所有权的智能指针
  // 无法得到指向同一对象的两个unique_ptr指针
  // 但可以进行移动构造与移动赋值操作，即所有权可以移动到另一个对象（而非拷贝构造）
	
	typedef std::pair<Timestamp, Timer*> Entry;
	typedef std::set<Entry> TimerList;
	typedef std::pair<Timer*, int64_t> ActiveTimer;
	typedef std::set<ActiveTimer> ActiveTimerSet;

	// 以下成员函数只可能在其所属的IO线程中调用。因为不必加锁
	// 要尽可能地减少锁竞争
	void addTimerInLoop(Timer* timer);
	void cancelInLoop(TimerId timerId);

	void handleRead();

	// move out all expired timers
	//
	std::vector<Entry> getExpired(Timestamp now);
	void reset(const std::vector<Entry>& expired, Timestamp now);

	bool insert(Timer* timer);

	EventLoop* loop_; 
	const int timerfd_; // 当前timerqueue生成的文件描述符
	Channel timerfdChannel_;
	TimerList timers_; // timers_按到期时间排序

	// for cancel()
	// timers_和activeTimers_保存的是相同的数据
	// timers_是按到期时间排序，activeTimers_是按对象地址排序
	ActiveTimerSet activeTimers_;
	bool callingExpiredTimers_;
	ActiveTimerSet cancelingTimers_; // 保存的是被取消的定时器
}; // end of TimerQueue

} // end of net
} // end of tmuduo
#endif // TMUDUO_NET_TIMERQUEUE_H
