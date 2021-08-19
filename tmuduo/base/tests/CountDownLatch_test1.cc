// 用于测试主线程通知所有线程运行
#include <tmuduo/base/CountDownLatch.h>
#include <tmuduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <stdio.h>

using namespace std;
using namespace tmuduo;

class Test
{
public:
	Test(int numThreads)
	  : latch_(1),
	    threads_(numThreads)
	{
		for (int i = 0; i < numThreads; ++i)
		{
			char name[32];
			snprintf(name, sizeof name, "work thread %d", i);
			threads_.push_back(new Thread(boost::bind(&Test::threadFunc, this), tmuduo::string(name)));
		}

		for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::start, _1));
		// 每个Thread都执行start，并且将其指针作为参数传入,供startThread使用
	}

	void run()
	{
		latch_.countDown();
	}

	void joinAll()
	{
		for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1));
	}

private:
	void threadFunc()
	{
		printf("tid=%d, %s started\n", CurrentThread::tid(), CurrentThread::name());
		latch_.wait(); // 等待主线程执行run,使得latch-1
		printf("tid=%d, %s stopped\n", CurrentThread::tid(), CurrentThread::name());
	}

	CountDownLatch latch_;
	boost::ptr_vector<Thread> threads_;
};


int main()
{
	printf("pid=%d, tid=%d\n", ::getpid(), CurrentThread::tid());
	
	Test t(3);
	sleep(3); // 3s后通知其他线程运行
	printf("pid=%d, tid=%d %s running...\n", ::getpid(), CurrentThread::tid(), CurrentThread::name());

	t.run();
	t.joinAll();

	printf("number of created threads %d\n", Thread::numCreated());

	return 0;
}
