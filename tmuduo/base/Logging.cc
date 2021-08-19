#include <tmuduo/base/Logging.h>
#include <tmuduo/base/CurrentThread.h>
#include <tmuduo/base/StringPiece.h>
#include <tmuduo/base/Timestamp.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

namespace tmuduo
{

__thread char t_errnobuf[512];
__thread char t_time[32];
__thread time_t t_lastSecond;

const char* strerror_tl(int savedErrno)
{
	return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf); // 将错误码的信息保存
}

Logger::LogLevel initLogLevel() // 初始化日志级别
{
	return Logger::TRACE;
	/*
	 if (::getenv("TMUDUO_LOG_TRACE"))
	 	return Logger::TRACE;
	else if (::getenv("TMUDUO_LOG_DEBUG"))
		return Logger::DEBUG;
	else
		return Logger::INFO;
	*/ 
}

Logger::LogLevel g_logLevel = initLogLevel(); // 定义全局变量：日志级别

const char* LogLevelName[Logger::NUM_LOG_LEVELS] = 
{
	"TRACE",
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"FATAL",
};

// helper class for known string length at compile time
class T
{
public:
	T(const char* str, unsigned len)
	  : str_(str),
	    len_(len)
	{
		// assert(strlen(str_) == len_);
	}

	const char* str_;
	const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v) // 重载operator<<
{
	s.append(v.str_, v.len_); 

	return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
	s.append(v.data_, v.size_);
	return s;
}

void defaultOutput(const char* msg, int len)
{
	size_t n = fwrite(msg, 1, len, stdout); // 默认输出到标准输出

	(void)n;
}

void defaultFlush()
{
	fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput; // 默认输出方式
Logger::FlushFunc g_flush = defaultFlush; // 默认渲染方式
} // end of tmuduo


using namespace tmuduo;


Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
  : time_(Timestamp::now()),
    stream_(),
	level_(level),
	line_(line),
	basename_(file)
{
	formatTime();
	CurrentThread::tid(); // 当前线程TID
	stream_ << T(CurrentThread::tidString(), 6) << ' '; // 调用operator<<(LogStream& s, T v)重载形式
	stream_ << T(LogLevelName[level], 6) << ' ';
	if (savedErrno != 0)
	{
		stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") "; // 哪里saved？
	}
}

void Logger::Impl::formatTime()
{
	int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
	time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / 1000000); // 计算
	int microseconds = static_cast<int>(microSecondsSinceEpoch % 1000000);

	if (seconds != t_lastSecond)
	{
		t_lastSecond = seconds;
		struct tm tm_time;
		::gmtime_r(&seconds, &tm_time);

		int len = snprintf(t_time, sizeof t_time, "%4d%02d%02d %02d:%02d:%02d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec); // 将日志记录时间保存保存为年月日 时：分：秒的形式

		assert(len == 17);
	}

	Fmt us(".%06dZ ", microseconds); // 构造Fmt对象
	assert(us.length() == 9);
	stream_ << T(t_time, 17) << T(us.data(), 9);
} // end of format time

void Logger::Impl::finish() // 记录结束，添加文件名和行号
{
	stream_ << "-" << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line) // 日志级别INFO
  : impl_(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level) // 日志级别：WARN, ERROR, FATAL
  : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func) // 日志级别TRACE, DEBUG
  : impl_(level, 0, file, line)
{
	impl_.stream_ << func <<' ';
}

Logger::Logger(SourceFile file, int line, bool toAbort) // 日志级别SYSERR, SYSFATAL(true)
  : impl_(toAbort ? FATAL : ERROR, errno, file, line)
{
}

Logger::~Logger()
{
	impl_.finish(); // 在析构的时候，调用finish添加文件名和当前行line
	const LogStream::Buffer& buf(stream().buffer()); 
	// 将impl的stream_返回（一个LogStream对象），在调用.buffer()返回LogStream的流对象（FixedBuffer）
	g_output(buf.data(), buf.length());

	if (impl_.level_ == FATAL)
	{
		g_flush();
		abort(); // 会终止程序，终止前先flush
	}
}

void Logger::setLogLevel(Logger::LogLevel level)
{
	g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
	g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
	g_flush = flush;
}
