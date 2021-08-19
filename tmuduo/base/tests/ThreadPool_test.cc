#include <tmuduo/base/ThreadPool.h>
#include <tmuduo/base/CountDownLatch.h>
#include <tmuduo/base/CurrentThread.h>

#include <boost/bind.hpp>
#include <string>
#include <stdlib.h>
#include <stdio.h>

using namespace tmuduo;

void print()
{
	printf("tid=%d", CurrentThread::tid());
}

void printString(const std::string& str)
{
	printf("tid=%d, str=%s\n", CurrentThread::tid(), str.c_str());
}

int main(void)
{
	ThreadPool pool("MainThreadPool");
	pool.start(5); // 启动五个线程

	pool.run(print); // 添加任务
	pool.run(print); // 添加任务
	for (int i = 0; i < 100; ++i)
	{
		char buf[32];
		snprintf(buf, sizeof buf, "task %d", i);
		pool.run(boost::bind(printString, std::string(buf)));
	}

	CountDownLatch latch(1);
	pool.run(boost::bind(&CountDownLatch::countDown, &latch));

	latch.wait();
	pool.stop(); // 即使没有stop，pool对象析构也会自动调用
}

