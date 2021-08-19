// IPC机制：mutex实现
#ifndef TMUDUO_BASE_MUTEX_H
#define TMUDUO_BASE_MUTEX_H

#include <tmuduo/base/CurrentThread.h>
#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace tmuduo
{

class MutexLock : boost::noncopyable
{
public:
	MutexLock()
	  : holder_(0)
	{
		int ret = pthread_mutex_init(&mutex_, NULL);
		assert(ret == 0); (void) ret;
	}

	~MutexLock()
	{
		assert(holder_ == 0); // 确保当前没有线程占有锁资源
		int ret = pthread_mutex_destroy(&mutex_); 
		assert(ret == 0); (void) ret;
	}

	bool isLockedByThisThread()
	{
		return holder_ == CurrentThread::tid();
	}

	void assertLocked()
	{
		assert(isLockedByThisThread());
	}

	// internal usage
	void lock()
	{
		pthread_mutex_lock(&mutex_);
		holder_ = CurrentThread::tid();
	}

	void unlock()
	{
		holder_ = 0;
		pthread_mutex_unlock(&mutex_);
	}

	pthread_mutex_t* getPthreadMutex()
	{
		return &mutex_;
	}
private:
	
	pthread_mutex_t mutex_; // 锁资源
	pid_t holder_; // 锁持有线程
}; // end of MutexLock


class MutexLockGuard : boost::noncopyable // RAII资源管理
{
public:
	explicit MutexLockGuard(MutexLock& mutex)
	  : mutex_(mutex)
	{
		mutex_.lock(); 
	}

	~MutexLockGuard()
	{
		mutex_.unlock();
	}

private:
	MutexLock& mutex_; // 该类和MutexLock为聚合关系

}; // end of MutexLockGuard

} // end of tmuduo

// 通过宏定义避免创建临时的MutexLockGuard独享:MutexLockGuard(mutex_);

#define MutexLockGuard(x) error "Missing guard object name"
#endif // TMUDUO_BASE_MUTEX_H
