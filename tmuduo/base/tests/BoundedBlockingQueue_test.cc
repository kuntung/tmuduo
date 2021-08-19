#include <tmuduo/base/BoundedBlockingQueue.h>
#include <tmuduo/base/Thread.h>
#include <tmuduo/base/CountDownLatch.h>
#include <tmuduo/base/Timestamp.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <stdio.h>

using namespace tmuduo;

class Test
{
public:
	Test(int numThreads)
	  : queue_(20),
	    latch_(numThreads),
		threads_(numThreads)
	{
		for (int i = 0; i < numThreads; ++i)
		{
			char name[32];
			snprintf(name, sizeof name, "work thread %d", i);

			threads_.push_back(new Thread(
				boost::bind(&Test::threadFunc, this), string(name)));
		}

		for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::start, _1));
	}

	void run(int times)
	{
		printf("waiting for count down latch\n");
		latch_.wait();
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
			queue_.put("stop");
		}
		for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1));
	}

private:
	void threadFunc()
	{
		printf("tid=%d, %s started\n", CurrentThread::tid(), CurrentThread::name());

		latch_.countDown();

		bool running = true;
		while (running)
		{
			std::string d(queue_.take());
			printf("tid=%d, get data = %s, size = %zd\n", CurrentThread::tid(), d.c_str(), queue_.size());
			running = (d != "stop");
		}
		printf("tid=%d, %s started\n", CurrentThread::tid(), CurrentThread::name());

	}

	BoundedBlockingQueue<std::string> queue_;
	CountDownLatch latch_;
	boost::ptr_vector<Thread> threads_;
}; // end of test

int main(int argc, char* argv[])
{
	int threads = (argc < 2 ? 1 : atoi(argv[1]));
	
	const int kNum = 1000 * 1000;
	printf("pid=%d, tid=%d\n", ::getpid(), CurrentThread::tid());
	Test t(threads);
	Timestamp start = Timestamp::now(); 
	t.run(100);
	t.joinAll();

	printf("total time using with <%d> threads :[%f]us\n", Thread::numCreated(), kNum * timeDifference(Timestamp::now(), start));
	return 0;
}
