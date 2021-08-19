// 所有线程共享一个单例，但是单例的value是每个线程独有的
#include <tmuduo/base/Thread.h>
#include <tmuduo/base/ThreadLocal.h>
#include <tmuduo/base/CurrentThread.h>
#include <tmuduo/base/Singleton.h>

#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <stdio.h>

using namespace tmuduo;

class Test : boost::noncopyable
{
public:
	Test()
	{
		printf("tid=%d, constructing %p\n", CurrentThread::tid(), this);
	}

	~Test()
	{
		printf("tid=%d, destructing %p %s\n", CurrentThread::tid(), this, name_.c_str());	
	}

	const std::string& name() const { return name_; }

	void setName(const std::string& n) { name_ = n; }

private:
	std::string name_;
};

#define STL Singleton<ThreadLocal<Test> >::instance().value()

void print()
{
	printf("tid=%d, %p name=%s\n",
		CurrentThread::tid(),
		&STL,
		STL.name().c_str());
}

void threadFunc(const char* changeTo)
{
	print();
	STL.setName(changeTo);
	sleep(1);
	print();
}


int main(void)
{
	STL.setName(std::string("main one"));
	Thread t1(boost::bind(threadFunc, "thread1"));
	Thread t2(boost::bind(threadFunc, "thread2"));
	t1.start();
	t2.start();
	t1.join();
	print();
	t2.join();

	return 0;
}
