#include <tmuduo/base/BlockingQueue.h>
#include <tmuduo/base/CountDownLatch.h>
#include <tmuduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <stdio.h>

using namespace tmuduo;

class Test
{
public:
	Test(int numThreads)
	  : latch_(numThreads),
	    threads_(numThreads)
	{
		for (int i = 0; i < numThreads; ++i)
		{
			char name[32];
			snprintf(name, sizeof name, "work thread %d", i);
			threads_.push_back(new Thread(boost::bind(&Test::threadFunc, this), tmuduo::string(name)));
		}

		for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::start, _1));
	}

	void run(int times)
	{
		printf("waiting for count down latch\n");
		latch_.wait(); // 等待所有线程就绪
		printf("all threads started\n");

		for (int i = 0; i < times; ++i)
		{
			char buf[32];
			snprintf(buf, sizeof buf, "hello %d", i);
			queue_.put(buf);
			printf("tid=%d, put data = %s, size = %zd\n", CurrentThread::tid(), buf, queue_.size());
		}
	}

	void joinAll()
	{
		for (size_t i = 0; i < threads_.size(); ++i)
		{
			queue_.put("stop"); // 当消费者线程读取到stop->running = false
			
		}
		for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1));
		// TODO:manual of for_each
	}

private:

	void threadFunc()
	{
		printf("tid=%d, %s started\n",
			CurrentThread::tid(),
			CurrentThread::name());

		latch_.countDown(); // 当前线程就绪
		bool running = true;
		while (running)
		{
			std::string product(queue_.take());
			printf("tid=%d, get data = %s, size = %zd\n", CurrentThread::tid(), product.c_str(), queue_.size());
			running = (product != "stop");
		}
		printf("tid=%d, %s stopped\n",
			CurrentThread::tid(),
			CurrentThread::name());
	
	}

	BlockingQueue<std::string> queue_; // 字符串缓冲队列
	CountDownLatch latch_;
	boost::ptr_vector<Thread> threads_;
}; // end of test

int main(void)
{
	printf("pid=%d, tid=%d\n", ::getpid(), CurrentThread::tid());
	Test t(5);

	t.run(100);
	t.joinAll();

	printf("number of %d threads have been created...\n", Thread::numCreated());
	return 0;
}
