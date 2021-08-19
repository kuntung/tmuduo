#ifndef TMUDUO_BASE_COUNTDOWNLATCH_H
#define TMUDUO_BASE_COUNTDOWNLATCH_H

#include <tmuduo/base/Condition.h>
#include <tmuduo/base/Mutex.h>

#include <boost/noncopyable.hpp>

namespace tmuduo
{

class CountDownLatch : boost::noncopyable
{
public:
	explicit CountDownLatch(int count);

	void wait(); // 等待条件

	void countDown(); // 改变条件

	int getCount() const;

private:
	mutable MutexLock mutex_;
	Condition condition_;
	int count_;
}; // end of CountDownLatch

} // end of tmuduo

#endif // TMUDUO_BASE_COUNTDOWNLATCH_H
