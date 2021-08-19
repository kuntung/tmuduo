#ifndef TMUDUO_BASE_CONDITION_H
#define TMUDUO_BASE_CONDITION_H

#include <tmuduo/base/Mutex.h>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace tmuduo
{

class Condition : boost::noncopyable
{
public:
	explicit Condition(MutexLock& mutex)
	  : mutex_(mutex)
	{
		pthread_cond_init(&pcond_, NULL); 
	}

	~Condition()
	{
		pthread_cond_destroy(&pcond_);
	}

	void wait()
	{
		pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()); // 等待条件解锁
	}

	// returns true if time out, false otherwise
	bool waitForSeconds(int seconds);


	void notify() // 封装pthread_cond_signal
	{
		pthread_cond_signal(&pcond_);
	}

	void notifyAll()
	{
		pthread_cond_broadcast(&pcond_);
	}

private:
	MutexLock& mutex_;
	pthread_cond_t pcond_;

}; // end of Condition
} // end of tmuduo

#endif // TMUDUO_BASE_CONDITION_H
