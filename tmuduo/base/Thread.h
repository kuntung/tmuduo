#ifndef TMUDUO_BASE_THREAD_H
#define TMUDUO_BASE_THREAD_H

#include <tmuduo/base/Atomic.h>
#include <tmuduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace tmuduo
{
class Thread : boost::noncopyable
{
public:
	typedef boost::function<void ()> ThreadFunc;

	explicit Thread(const ThreadFunc&, const string& name = string());

	~Thread();

	void start();
	int join(); // return pthread_join()

	bool started() const { return started_; }

	pid_t tid() const { return tid_; }

	const string& name() const { return name_; }

	static int numCreated() { return numCreated_.get(); }

private:
	static void* startThread(void *thread);
	void runInThread();

	bool started_;
	pthread_t pthreadId_;
	pid_t tid_;
	ThreadFunc func_;
	string name_;
	static AtomicInt32 numCreated_;
}; // end of thread

}; // end of tmuduo
#endif // TMUDUO_BASE_THREAD_H
