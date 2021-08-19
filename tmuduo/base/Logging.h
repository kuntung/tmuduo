#ifndef TMUDUO_BASE_LOGGING_H
#define TMUDUO_BASE_LOGGING_H

#include <tmuduo/base/LogStream.h>
#include <tmuduo/base/Timestamp.h>

namespace tmuduo
{

class Logger
{
public:
	enum LogLevel
	{
		TRACE,
		DEBUG,
		INFO,
		WARN,
		ERROR,
		FATAL,
		NUM_LOG_LEVELS,
	};

	// compile time calculation of basename of source file
	class SourceFile
	{
	public:
		template<int N>
		inline SourceFile(const char (&arr)[N])
		  : data_(arr),
		    size_(N-1)
		{
			const char* slash = strrchr(data_, '/'); // builtin function:返回第一次出现'/'的指针位置
			if (slash)
			{
				data_ = slash + 1;
				size_ -= static_cast<int>(data_ - arr); 
			}

		}

		explicit SourceFile(const char* filename)
		  : data_(filename)
		{
			const char* slash = strrchr(filename, '/');
			if (slash)
			{
				data_ = slash + 1;
			}
			size_ = static_cast<int>(strlen(data_));
		}

		const char* data_;
		int size_;
	}; // end of SourceFile

	Logger(SourceFile file, int line);
	Logger(SourceFile file, int line, LogLevel level);
	Logger(SourceFile file, int line, LogLevel level, const char* func);
	Logger(SourceFile file, int line, bool toAbort);
	~Logger();

	LogStream& stream() { return impl_.stream_; } // 返回LogStream

	static LogLevel logLevel();
	static void setLogLevel(LogLevel level);

	typedef void (*OutputFunc)(const char* msg, int len); // 输出函数指针
	typedef void (*FlushFunc)();

	static void setOutput(OutputFunc); // 设置输出形式
	static void setFlush(FlushFunc);

private:
	
class Impl
{
public:
	typedef Logger::LogLevel LogLevel;
	Impl(LogLevel level, int old_errno, const SourceFile& file, int line);

	void formatTime(); // 日志记录时间函数
	void finish();

	Timestamp time_;
	LogStream stream_;
	LogLevel level_;
	int line_;
	SourceFile basename_;
}; // end of Impl,前套类

	Impl impl_;
}; // end of Logger
extern Logger::LogLevel g_logLevel; // 什么意思, 全局变量属性？由initLogLevel(); 初始化

inline Logger::LogLevel Logger::logLevel()
{
	return g_logLevel;
}

// 一些宏定义，用于日志记录
#define LOG_TRACE if (tmuduo::Logger::logLevel() <= tmuduo::Logger::TRACE)\
	tmuduo::Logger(__FILE__, __LINE__, tmuduo::Logger::TRACE, __func__).stream() // 临时对象，然后析构的时候保存log流
#define LOG_DEBUG if (tmuduo::Logger::logLevel() <= tmuduo::Logger::DEBUG) \
	tmuduo::Logger(__FILE__, __LINE__, tmuduo::Logger::DEBUG, __func__).stream()	
#define LOG_INFO if (tmuduo::Logger::logLevel() <= tmuduo::Logger::INFO) \
	tmuduo::Logger(__FILE__, __LINE__).stream() // INFO级别不需要保存函数名
#define LOG_WARN tmuduo::Logger(__FILE__, __LINE__, tmuduo::Logger::WARN).stream()
#define LOG_ERROR tmuduo::Logger(__FILE__, __LINE__, tmuduo::Logger::ERROR).stream()
#define LOG_FATAL tmuduo::Logger(__FILE__, __LINE__, tmuduo::Logger::FATAL).stream()
#define LOG_SYSERR tmuduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL tmuduo::Logger(__FILE__, __LINE__, true).stream()

const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::tmuduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr) {
  if (ptr == NULL) {
   Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}
} // end of tmuduo

#endif // TMUDUO_BASE_LOGGING_H
