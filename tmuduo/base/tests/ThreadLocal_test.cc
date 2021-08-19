#include <tmuduo/base/Thread.h>
#include <tmuduo/base/ThreadLocal.h>
#include <tmuduo/base/CurrentThread.h>

#include <boost/noncopyable.hpp>
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

ThreadLocal<Test> testObj1;
ThreadLocal<Test> testObj2;

void print()
{
	printf("tid=%d, obj1 %p name=%s\n",
		CurrentThread::tid(),
		&testObj1.value(),
		testObj1.value().name().c_str());
	
	printf("tid=%d, obj2 %p name=%s\n",
		CurrentThread::tid(),
		&testObj2.value(),
		testObj2.value().name().c_str());
}

void threadFunc()
{
	print();
	testObj1.value().setName(std::string("changed 1"));
	testObj2.value().setName(std::string("changed 2"));
	print();
}


int main(void)
{
	testObj1.value().setName(std::string("main one"));
	print();
	Thread t1(threadFunc);
	t1.start();
	t1.join();

	testObj2.value().setName("main two");
	print();

	return 0;
}
