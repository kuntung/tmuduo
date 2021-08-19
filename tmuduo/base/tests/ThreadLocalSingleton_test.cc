#include <tmuduo/base/ThreadLocalSingleton.h>
#include <tmuduo/base/Thread.h>
#include <tmuduo/base/CurrentThread.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <stdio.h>

using namespace tmuduo;


class Test : boost::noncopyable
{
public:
	Test()
	{
		printf("tid=%d, constructing...\n", CurrentThread::tid());
	}

	~Test()
	{
		printf("tid=%d, destructing...\n", CurrentThread::tid());
	}

	const std::string& name() const { return name_; }

	void setName(const std::string& n) { name_ = n; }

private:
	std::string name_;
}; // end of Test

void print(void)
{
	printf("tid=%d, %p, name=%s\n",
		CurrentThread::tid(),
		&ThreadLocalSingleton<Test>::instance(),
		ThreadLocalSingleton<Test>::instance().name().c_str());
}
void threadFunc(const std::string& changeTo)
{
	print();
	ThreadLocalSingleton<Test>::instance().setName(changeTo);
	print();
}

int main(void)
{

	ThreadLocalSingleton<Test>::instance().setName(std::string("main one"));
	Thread t1(boost::bind(threadFunc, std::string("thread 1")));
	Thread t2(boost::bind(threadFunc, std::string("thread 2")));
	t1.start();
	t2.start();
	t2.join();
	print();
	t2.join();

	return 0;
}
