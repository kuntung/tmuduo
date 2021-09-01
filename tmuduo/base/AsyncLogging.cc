#include <tmuduo/base/AsyncLogging.h>
#include <tmuduo/base/LogFile.h>
#include <tmuduo/base/Timestamp.h>

#include <stdio.h>

using namespace tmuduo;

AsyncLogging::AsyncLogging(const string& basename,
						size_t rollSize,
						int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
	basename_(basename),
	rollSize_(rollSize),
	thread_(boost::bind(&AsyncLogging::threadFunc, this), "Logging"),
	latch_(1),
	mutex_(),
	cond_(mutex_),
	currentBuffer_(new Buffer),
	nextBuffer_(new Buffer),
	buffers_()
{
	currentBuffer_->bzero(); // 清空缓冲区内容
	nextBuffer_->bzero(); 
	buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len)
{
	MutexLockGuard lock(mutex_); // 可能有多个业务线程向当前缓冲区写入数据

	if (currentBuffer_->avail() > len)
	{
		// 当前缓冲区未满，将数据追加到末尾
		currentBuffer_->append(logline, len);
	}
	else
	{
		// 当前缓冲区已满，将当前缓冲区添加到待写入文件的已填满的缓冲区列表
		buffers_.push_back(currentBuffer_.release());

		// 将当前缓冲区设置为预备缓冲区
		if (nextBuffer_)
		{
			currentBuffer_ = boost::ptr_container::move(nextBuffer_);
		}
		else
		{
			// 这种情况，极少发生，前端写入太快，一下子把两块缓冲区都写完。
			// 只好再分配一块新的缓冲区（应该避免内存分配）
			currentBuffer_.reset(new Buffer);
		}
		currentBuffer_->append(logline, len);
		cond_.notify(); // 通知后端开始写入日志
	}
}

void AsyncLogging::threadFunc()
{
	assert(running_ == true);
	latch_.countDown(); // 让后端线程
	LogFile output(basename_, rollSize_, false); // 默认是单线程写，不需要线程安全mutexlock
	// 准备两块空闲缓冲区
	BufferPtr newBuffer1(new Buffer);
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16); // 预留16块缓冲区

	while (running_)
	{
		assert(newBuffer1 && newBuffer1->length() == 0);
		assert(newBuffer2 && newBuffer2->length() == 0);
		assert(buffersToWrite.empty());
	

	{ // 临界区开始, 因为此时前端生产者线程也会写满一个buffer，调用buffers_.push_back()
		tmuduo::MutexLockGuard lock(mutex_); 
		if (buffers_.empty()) // 非常规用法， 因为为了避免虚假唤醒，我们通常需要while等待条件
		{
			cond_.waitForSeconds(flushInterval_);
		}
		buffers_.push_back(currentBuffer_.release()); // 将未写完的currentBuffer_也一并写了
		currentBuffer_ = boost::ptr_container::move(newBuffer1); // 将空闲的newBuffer1置为currentBuffer_
		buffersToWrite.swap(buffers_); // 减少锁竞争范围
		if (!nextBuffer_)
		{
			nextBuffer_ = boost::ptr_container::move(newBuffer2); // 确保前端始终有一个预备buffer_可供调配
																  // 减少前端临界区分配内存的概率，缩短前端临界区范围
																  // 前端如果经常分配预备缓冲区nextBuffer_，会增加锁竞争时间
																  // 影响并发性
		}
	}

	assert(!buffersToWrite.empty());

    // 消息堆积的解决
    // 前端陷入死循环，拼命发送日志消息，超过后端的处理能力，这就是典型的
    // 生产速度超过消费速度问题，会造成数据在内存中堆积，严重时引发性能问题（可用内存不足）
    // 或程序崩溃（分配内存失败）
    if (buffersToWrite.size() > 25)
    {
      char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffersToWrite.size()-2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end()); // ���������־�����ڳ��ڴ棬���������黺����
    }

    for (size_t i = 0; i < buffersToWrite.size(); ++i)
    {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffersToWrite[i].data(), buffersToWrite[i].length());
    }

    if (buffersToWrite.size() > 2)
    {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2); // 写完之后，仅保留两个buffer_用于newBuffer1与newBuffer2
    }

    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush(); // 更新日志文件
  }
  output.flush();
	
}
