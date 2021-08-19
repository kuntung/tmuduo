#include <tmuduo/base/Thread.h>
#include <tmuduo/base/CurrentThread.h> // 用来缓存当前线程的真实ID和线程名
#include <tmuduo/base/Exception.h>
// #include <tmuduo/base/Logging.h> // 日志记录

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp> // 用来判断类型是否一致

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace tmuduo
{

namespace CurrentThread
{
// __thread修饰POD类型的线程局部存储
	__thread int t_cachedTid = 0; // 线程真实ID的缓存。为了较少::syscall(SYS_gettid)系统调用
	__thread char t_tidString[32]; // tid的字符串表示形式
	__thread const char* t_threadName = "unknown";
	const bool sameType = boost::is_same<int, pid_t>::value; 

	BOOST_STATIC_ASSERT(sameType); 
} // end of CurrentThread

namespace detail
{

pid_t gettid()
{
	return static_cast<pid_t>(::syscall(SYS_gettid)); // 系统调用返回线程tid
}

void afterFork()
{
	// 在fork之后将变成的id，名字进行更改。因为进程可能不是在主线程中fork
	tmuduo::CurrentThread::t_cachedTid = 0; // 
	tmuduo::CurrentThread::t_threadName = "main"; // 调用fork的线程即为该fork子进程的主线程
	CurrentThread::tid();
}

class ThreadNameInitializer
{
public:
	ThreadNameInitializer()
	{
		tmuduo::CurrentThread::t_threadName = "main";
		CurrentThread::tid();
		pthread_atfork(NULL, NULL, afterFork); // 子进程调用afterfork
	}
}; // end define class

ThreadNameInitializer init; // 先于Thread类构造

} // end of detail

} // end of tmuduo

using namespace tmuduo;

// 开始CurrentThread成员函数定义

void CurrentThread::cacheTid()
{
	if (t_cachedTid == 0) // 首次缓存
	{
		t_cachedTid = detail::gettid();
		int n = snprintf(t_tidString, sizeof t_tidString, "%5d", t_cachedTid);
		//assert(n == 6); (void) n;
	}
}

bool CurrentThread::isMainThread() // 判断是否是主线程
{
	return tid() == ::getpid();
}

// 开始Thread类成员函数定义
AtomicInt32 Thread::numCreated_; // 静态成员变量类外初始化

Thread::Thread(const ThreadFunc& func, const string& n)
  : started_(false),
    pthreadId_(0),
	tid_(0),
	func_(func),
	name_(n)
{
	numCreated_.increment(); // 创建一个线程就+1
}

Thread::~Thread()
{
	// no join
}

void Thread::start()
{
	assert(!started_);
	started_ = true;
	// errno是errno.h的一个全局变量
	errno = pthread_create(&pthreadId_, NULL, &startThread, this); // 线程入口函数为startThread, 传入参数为this
	if (errno != 0)
	{
		// LOG_SYSFATAL << "Failed in pthread_create";
	}
}

int Thread::join()
{
	assert(started_);
	return pthread_join(pthreadId_, NULL); // 等待线程结束
}

void* Thread::startThread(void* obj)
{
	Thread* thread = static_cast<Thread*>(obj);
	thread->runInThread();

	return nullptr;
}

void Thread::runInThread()
{
	tid_ = CurrentThread::tid(); // 获取线程ID
	tmuduo::CurrentThread::t_threadName = name_.c_str();
	try
	{
		func_(); // 执行注册业务处理逻辑函数
		tmuduo::CurrentThread::t_threadName = "finished"; // 通过t_threadName来表示线程执行状态
	}
	catch (const Exception& ex)
	{
		tmuduo::CurrentThread::t_threadName = "crashed";
		fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
		fprintf(stderr, "reason: %s\n",  ex.what());
		fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
		abort(); 
	}
	catch (const std::exception& ex)
	{
		tmuduo::CurrentThread::t_threadName = "crashed";
		fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
		fprintf(stderr, "reason: %s\n",  ex.what());
		abort(); 
	}
	catch (...)
	{
		tmuduo::CurrentThread::t_threadName = "crashed";
		fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
		throw; // rethrow	
	}
}
