// IO线程池实现
#ifndef TMUDUO_NET_EVENTLOOPTHREADPOOL_H
#define TMUDUO_NET_EVENTLOOPTHREADPOOL_H

#include <tmuduo/base/Condition.h>
#include <tmuduo/base/Mutex.h>
#include <vector>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace tmuduo
{
namespace net
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : boost::noncopyable
{
public:
	typedef boost::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThreadPool(EventLoop* baseLoop);
	~EventLoopThreadPool();

	void setThreadNum(int numThreads) { numThreads_ = numThreads; }
	void start(const ThreadInitCallback& cb = ThreadInitCallback()); // 传入参数用于IO线程初始化的时候执行

	EventLoop* getNextLoop(); // 获取轮叫的下一个时事件循环
private:
	
	EventLoop* baseLoop_; // 主线程的loop循环，与acceptor所属eventloop相同
	bool started_;
	int numThreads_; // IO线程的个数
	int next_; // 新连接到来，所选择的EventLoop对象下标
	boost::ptr_vector<EventLoopThread> threads_; // IO线程列表
	std::vector<EventLoop*> loops_; // EventLoop列表
}; // end of EventLoopThreadPool

} // end of net

} // end of tmuduo


#endif // TMUDUO_NET_EVENTLOOPTHREADPOOL_H
