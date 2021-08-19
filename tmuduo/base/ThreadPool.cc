#include <tmuduo/base/ThreadPool.h>
#include <tmuduo/base/Exception.h>

#include <boost/bind.hpp>
#include <assert.h>
#include <stdio.h>

using namespace tmuduo;

ThreadPool::ThreadPool(const string& name)
  : mutex_(),
    cond_(mutex_),
	name_(name),
	running_(false)
{
}

ThreadPool::~ThreadPool()
{
	if (running_)
	{
		stop();
	}
}

void ThreadPool::start(int numThreads)
{
	assert(threads_.empty());
	running_ = true;
	threads_.reserve(numThreads);
	for (int i = 0; i < numThreads; ++i)
	{
		char id[32];
		snprintf(id, sizeof id, "%d", i);
		threads_.push_back(new Thread(boost::bind(&ThreadPool::runInThread, this), name_ + id));

		threads_[i].start(); // 调用pthread_create创建线程
	}
}

void ThreadPool::stop()
{
	{
		MutexLockGuard lock(mutex_);
		running_ = false;
		cond_.notifyAll(); // 通知所有等待线程，停止
	}

	for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1));
}

void ThreadPool::run(const Task& task)
{
	if (threads_.empty())
	{
		// 当前没有工作线程, 直接运行
		task(); 
	}
	else
	{
		MutexLockGuard lock(mutex_);
		queue_.push_back(task); 
		cond_.notify();
	}
}

ThreadPool::Task ThreadPool::take()
{
	MutexLockGuard lock(mutex_);
	// always use a while loop, due to spurious wakeup
	while (queue_.empty() && running_)
	{
		cond_.wait(); // 当前队列为空，等待
	}

	Task task;
	if (!queue_.empty())
	{
		task = queue_.front();
		queue_.pop_front();
	}
	return task;
}

void ThreadPool::runInThread()
{
	try
	{
		while (running_)
		{
			Task task(take()); // 取出一个任务
			if (task)
			{
				task(); // 任务执行
			}
		}
	}
	  catch (const Exception& ex)
  	{
    	fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
   	 fprintf(stderr, "reason: %s\n", ex.what());
    	fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
  	  abort();
	  }
	  catch (const std::exception& ex)
 	 {
	    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
 	   fprintf(stderr, "reason: %s\n", ex.what());
 	   abort();
 	 }
 	 catch (...)
	  {
	    fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
  	  throw; // rethrow
  	}
}
