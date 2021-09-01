#ifndef TMUDUO_BASE_ASYNCLOGGING_H
#define TMUDUO_BASE_ASYNCLOGGING_H

#include <tmuduo/base/BlockingQueue.h>
#include <tmuduo/base/BoundedBlockingQueue.h>
#include <tmuduo/base/CountDownLatch.h>
#include <tmuduo/base/Mutex.h>
#include <tmuduo/base/Thread.h>

#include <tmuduo/base/LogStream.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace tmuduo
{

class AsyncLogging : boost::noncopyable
{
public:
	AsyncLogging(const string& basename,
			size_t rollSize,
			int flushInterval = 3);
	
	~AsyncLogging()
	{
		if (running_)
		{
			stop();
		}
	}

	// 供前端生产者线程调用（日志数据写到缓冲区）
	void append(const char* logline, int len);

	void start()
	{
		running_ = true;
		thread_.start(); // 日志线程启动
		latch_.wait(); // 等待线程函数threadFunc执行完毕
	}

	void stop()
	{
		running_ = false;
		cond_.notify(); // 唤醒后端阻塞线程
		thread_.join(); // 等到后端线程终止
	}

private:
	AsyncLogging(const AsyncLogging&);
	void operator=(const AsyncLogging&);

	// 供后端消费者线程调用（将数据写到日志文件）
	void threadFunc();

	typedef tmuduo::detail::FixedBuffer<tmuduo::detail::kLargeBuffer> Buffer;
	typedef boost::ptr_vector<Buffer> BufferVector;
	typedef BufferVector::auto_type BufferPtr; // 可以理解为Buffer的智能指针，能管理Buffer的生存期
											// 类似于C++11中的unique_ptr，具有移动语义
											// 两个unique_ptr不能指向同一个对象，不能进行复制操作
	const int flushInterval_; // 超时时间，在flushInterval_秒内，缓冲区没写满也会将缓冲区中的日志记录写入文件
	bool running_;
	string basename_;
	size_t rollSize_; // 日志滚动条件
	tmuduo::Thread thread_; // 后端消费者线程
	tmuduo::CountDownLatch latch_;
	tmuduo::MutexLock mutex_;
	tmuduo::Condition cond_;
	BufferPtr currentBuffer_; // 当前缓冲区
	BufferPtr nextBuffer_; // 预备缓冲区
	BufferVector buffers_; // 待写入文件的已填满的缓冲区
}; // end of AsyncLogging

} // end of tmuduo
#endif // TMUDUO_BASE_ASYNCLOGGING_H
