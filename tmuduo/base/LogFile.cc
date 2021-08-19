#include <tmuduo/base/LogFile.h>
#include <tmuduo/base/Logging.h>
#include <tmuduo/base/ProcessInfo.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace tmuduo;

class LogFile::File : boost::noncopyable
{
public:
	explicit File(const string& filename)
	  : fp_(::fopen(filename.data(), "ae")),
	    writtenBytes_(0)
	{
		assert(fp_);
		::setbuffer(fp_, buffer_, sizeof buffer_); // 
	}

	~File()
	{
		::fclose(fp_);
	}

	void append(const char* logline, const size_t len)
	{
		size_t n = write(logline, len);
		size_t remain = len - n; 
		// remain > 0，表示没写完，需要继续写
		while (remain > 0)
		{
			size_t x = write(logline + n, remain);
			if (x == 0)
			{
				int err = ferror(fp_);
				if (err)
				{
					fprintf(stderr, "LogFile::File::append() failed %s\n", strerror_tl(err)); //Logging.h
				}

				break;
			}
			n += x;
			remain = len - n;
		}
		
		writtenBytes_ += len;
	}

	void flush()
	{
		::fflush(fp_);
	}
	
	size_t writtenBytes() const { return writtenBytes_; }

private:
	size_t write(const char* logline, size_t len)
	{
#undef fwrite_unlocked
	return ::fwrite_unlocked(logline, 1, len, fp_);

	}


	FILE* fp_;
	char buffer_[64*1024];
	size_t writtenBytes_;
};

LogFile::LogFile(const string& basename,
				size_t rollSize,
				bool threadSafe,
				int flushInterval)
  : basename_(basename),
    rollSize_(rollSize),
	flushInterval_(flushInterval),
	count_(0),
	mutex_(threadSafe ? new MutexLock : NULL),
	startOfPeriod_(0),
	lastRoll_(0),
	lastFlush_(0)
{
	assert(basename.find('/') == string::npos);
	rollFile(); // 初始化滚动文件
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* logline, int len)
{
	if (mutex_)
	{
		MutexLockGuard lock(*mutex_); // mutex_是一个scopt_ptr
		append_unlocked(logline, len);
	}
	else
	{
		append_unlocked(logline, len);
	}
}

void LogFile::flush()
{
	if (mutex_)
	{
		MutexLockGuard lock(*mutex_);
		file_->flush();
	}
	else
	{
		file_->flush();
	}
}

void LogFile::append_unlocked(const char* logline, int len)
{
	file_->append(logline, len);

	if (file_->writtenBytes() > rollSize_) // 达到滚动条件
	{
		rollFile();
	}
	else
	{
		if (count_ > kCheckTimeRoll_)
		{
			count_ = 0;
			time_t now = ::time(NULL);
			time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;

			if (thisPeriod_ != startOfPeriod_)
			{
				// 新的一天，滚动日志
				rollFile();
			}
			else if (now - lastFlush_ > flushInterval_)
			{
				lastFlush_ = now;
				file_->flush(); // 输出
			}
		}
		else
		{
			++count_; // 添加的次数
		}
	}
} // end of append_unlocked

void LogFile::rollFile()
{
	time_t now = 0;
	string filename = getLogFileName(basename_, &now); // 根据当前时间生成日志文件名

	time_t start = now / kRollPerSeconds_ * kRollPerSeconds_; // 对齐当当天零点

	if (now > lastRoll_)
	{
		lastRoll_ = now;
		lastFlush_ = now;
		startOfPeriod_ = start; // 新的记录
		file_.reset(new File(filename)); // 创建一个新的日志文件，通过FILE构造函数
	}
}

string LogFile::getLogFileName(const string& basename, time_t* now)
{
	string filename;
	filename.reserve(basename.size() + 64);
	filename = basename;

	char timebuf[32];
	char pidbuf[32];
	struct tm tm;
	*now = time(NULL);
	gmtime_r(now, &tm);
	strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
	filename += timebuf;
	filename += ProcessInfo::hostname();
	snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
	filename += pidbuf;
	filename += ".log"; // 后缀


	return filename;
}
