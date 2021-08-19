#include <tmuduo/base/CountDownLatch.h>

using namespace tmuduo;

CountDownLatch::CountDownLatch(int count)
  : mutex_(),
    condition_(mutex_),
	count_(count)
{

}

void CountDownLatch::wait()
{
	MutexLockGuard lock(mutex_);
	while (count_ > 0) // 等待的条件是count_ == 0
	{
		condition_.wait(); // 内部有一个条件变量pcond_
		// pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
	}
}

void CountDownLatch::countDown()
{
	MutexLockGuard lock(mutex_);
	--count_;
	if (count_ == 0)
	{
		condition_.notifyAll();
	}
}

int CountDownLatch::getCount() const
{
	MutexLockGuard lock(mutex_);

	return count_;
}
