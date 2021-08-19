#ifndef TMUDUO_BASE_THREADPOOL_H
#define TMUDUO_BASE_THREADPOOL_H

#include <tmuduo/base/Thread.h>
#include <tmuduo/base/Condition.h>
#include <tmuduo/base/Mutex.h>
#include <tmuduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

namespace tmuduo
{

class ThreadPool : boost::noncopyable
{
public:
	typedef boost::function<void ()> Task;

	explicit ThreadPool(const string& name = string());
	~ThreadPool();

	void start(int numThreads);
	void stop();

	void run(const Task& f);

private:
	void runInThread(); // 线程池中线程执行
	Task take();

	MutexLock mutex_;
	Condition cond_;
	string name_;
	boost::ptr_vector<tmuduo::Thread> threads_;
	std::deque<Task> queue_;
	bool running_;
}; // end of Thread

} // end of tmuduo
#endif //TMUDUO_BASE_THREADPOOL_H
