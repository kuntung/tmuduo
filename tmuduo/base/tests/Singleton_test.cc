#include <tmuduo/base/Singleton.h>
#include <tmuduo/base/Thread.h>
#include <tmuduo/base/CurrentThread.h>

#include <boost/noncopyable.hpp>
#include <stdio.h>
#include <string>

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

void threadFunc()
{
	printf("tid=%d, %p, name=%s\n",
			CurrentThread::tid(),
			&Singleton<Test>::instance(),
			Singleton<Test>::instance().name().c_str());
	Singleton<Test>::instance().setName(std::string("only one, changed"));
	
}

int main(void)
{
	Singleton<Test>::instance().setName(std::string("only one"));

	Thread t1(threadFunc, "work thread");

	t1.start();
	t1.join();

	printf("tid=%d, %p, name_=%s\n",
			CurrentThread::tid(),
			&Singleton<Test>::instance(),
			Singleton<Test>::instance().name().c_str());

	return 0;
}
