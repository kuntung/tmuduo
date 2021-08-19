#ifndef TMUDUO_BASE_LOGSTREAM_H
#define TMUDUO_BASE_LOGSTREAM_H

#include <tmuduo/base/StringPiece.h>
#include <tmuduo/base/Types.h>
#include <assert.h>
#include <string.h>
#ifndef TMUDUO_STD_STRING
#include <string>
#endif

#include <boost/noncopyable.hpp>

namespace tmuduo
{

namespace detail
{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

// SIZE为非类型参数
template<int SIZE>
class FixedBuffer : boost::noncopyable // 日志缓冲区设计
{
public:
	FixedBuffer()
	  : cur_(data_)
	{
		setCookie(cookieStart);
	}

	~FixedBuffer()
	{
		setCookie(cookieEnd);
	}

	void append(const char* buf, size_t len)
	{
		if (implicit_cast<size_t>(avail()) > len)
		{
			memcpy(cur_, buf, len);
			cur_ += len;
		}
	}

	const char* data() const { return data_; }
	int length() const { return static_cast<int>(cur_ - data_); }

	// write to data_ directly
	char* current() { return cur_; }
	int avail() const { return static_cast<int>(end() - cur_); }
	void add(size_t len) { cur_ += len; }
	void reset() { cur_ = data_; }
	void bzero() { ::bzero(data_, sizeof data_); }

	// for used by GDB
	const char* debugString();
	void setCookie(void (*cookie)()) { cookie_ = cookie; }

	// for used by unit test
	string asString() const { return string(data_, length()); }

private:
	const char* end() const { return data_ + sizeof data_; }

	static void cookieStart();
	static void cookieEnd();

	void (*cookie_)(); // 函数指针
	char data_[SIZE];
	char* cur_; // 当前有效数据的位置
}; // end of FixedBuffer

} // end of detail

class LogStream : boost::noncopyable // 日志流设计，
{
typedef LogStream self;
public:
	typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer; // 底层缓冲区默认为kSmallBuffer大小的数组

	self& operator<<(bool v)
	{
		buffer_.append(v ? "1" : "0", 1);
		return *this;
	}

	self& operator<<(short);
	self& operator<<(unsigned short);
	self& operator<<(int);
	self& operator<<(unsigned int);
	self& operator<<(long );
	self& operator<<(unsigned long);
	self& operator<<(long long);
	self& operator<<(unsigned long long);

	self& operator<<(const void*);
	self& operator<<(float v)
	{
		*this<<static_cast<double>(v);

		return *this;
	}

	self& operator<<(double);
	self& operator<<(char v)
	{
		buffer_.append(&v, 1); // 调用FixedBuffer的append
		return *this;
	}

	self& operator<<(const char* v)
	{
		buffer_.append(v, strlen(v));
		return *this;
	}

	self& operator<<(const string& v)
	{
		buffer_.append(v.c_str(), v.size());
		return *this;
	}

#ifndef TMUDUO_STD_STRING
	self& operator<<(const std::string& v)
	{
		buffer_.append(v.c_str(), v.size());
		return *this;
	}
#endif

	self& operator<<(const StringPiece& v)
	{
		buffer_.append(v.data(), v.size());

		return *this;
	}

	void append(const char* data, int len) { buffer_.append(data, len); } 
	const Buffer& buffer() const { return buffer_; } // 获取消息缓冲区数据
	void resetBuffer() { buffer_.reset(); }
private:
	void staticCheck();

	template<typename T>
	void formatInteger(T);

	Buffer buffer_;
	static const int kMaxNumericSize = 32;
}; // end of LogStream

class Fmt
{
public:
	template<typename T>
	Fmt(const char* fmt, T val);

	const char* data() const { return buf_; }
	int length() const { return length_; }

private:
	char buf_[32];
	int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt) // 普通函数重载运算符<<
{
	s.append(fmt.data(), fmt.length());

	return s;
}
} // end of tmuduo

#endif // TMUDUO_BASE_LOGSTREAM_H
