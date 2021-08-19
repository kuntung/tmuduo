#ifndef TMUDUO_BASE_STRINGPIECE_H
#define TMUDUO_BASE_STRINGPIECE_H

#include <string.h>
#include <iosfwd>

#include <tmuduo/base/Types.h>
#ifndef TMUDUO_STD_STRING
#include <string>
#endif

namespace tmuduo
{
class StringPiece
{
private:
	const char* ptr_;
	int length_;
public:
	StringPiece()
	  : ptr_(NULL), length_(0) { }
	
	StringPiece(const char* str)
	  : ptr_(str), length_(static_cast<int>(strlen(str))) { }
	
	StringPiece(const unsigned char* str)
	  : ptr_(reinterpret_cast<const char*>(str)), length_(static_cast<int>(strlen(ptr_))) { }

	StringPiece(const string& str)
 	  : ptr_(str.data()), length_(static_cast<int>(str.size())) { }
#ifndef TMUDUO_STD_STRING
	StringPiece(const std::string& str)
 	  : ptr_(str.data()), length_(static_cast<int>(str.size())) { }
#endif
	StringPiece(const char* offset, int len)
	  : ptr_(offset), length_(len) { }
	
	const char* data() const { return ptr_; }
	int size() const { return length_; }
	bool empty() const { return length_ == 0; }

	void clear() { ptr_ = nullptr; length_ = 0; }
	void set(const char* buffer, int len) { ptr_ = buffer; length_ = len; }

	void set(const char* buffer)
	{
		ptr_ = buffer;
		length_ = static_cast<int>(strlen(buffer));
	}

	void set(const void* buffer, int len)
	{
		ptr_ = reinterpret_cast<const char*>(buffer);
		length_ = len;
	}

	char operator[](int i) const { return ptr_[i]; }

	void remove_prefix(int n)
	{
		ptr_ += n;
		length_ -= n;
	}
	
	bool operator==(const StringPiece& x) const
	{
		return ((length_ == x.size()) &&
			(memcpy(const_cast<char*>(ptr_), x.ptr_, length_) == 0));
	}

	bool operator!=(const StringPiece& x) const
	{
		return !(*this == x);
	}

// 以宏函数定义的形式，重载运算符<, <=, >=, >
#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp) \
	bool operator cmp (const StringPiece& x) const { \
		int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
		return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_))); \
}
	STRINGPIECE_BINARY_PREDICATE(<, <);
	STRINGPIECE_BINARY_PREDICATE(<=, <);
	STRINGPIECE_BINARY_PREDICATE(>=, >);
	STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE
	
	int compare(const StringPiece& x) const
	{
		int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); // 比较公共长度部分
		if (r == 0)
		{
			if (length_ < x.length_) r = -1;
			else if (length_ > x.length_) r = +1;
		}

		return r;
	}

	string as_string() const { return string(data(), size()); }

	void CopyToString(string* target) const
	{
		target->assign(ptr_, length_);
	}

#ifndef MUDUO_STD_STRING
  void CopyToStdString(std::string* target) const {
    target->assign(ptr_, length_);
  }
#endif

  // Does "this" start with "x"
  bool starts_with(const StringPiece& x) const {
    return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
  }
};

}   // namespace tmuduo

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------

/*
在STL中为了提供通用的操作而又不损失效率，我们用到了一种特殊的技巧，叫traits编程技巧。
具体的来说，traits就是通过定义一些结构体或类，并利用模板特化和偏特化的能力，
给类型赋予一些特性，这些特性根据类型的 不同而异。
在程序设计中可以使用这些traits来判断一个类型的一些特性，引发C++的函数重载机制，
实现同一种操作因类型不同而异的效果。
*/

// 这里对__type_traits进行特化，给StringPiece一些特性
#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
template<> struct __type_traits<tmuduo::StringPiece> {
  typedef __true_type    has_trivial_default_constructor;
  typedef __true_type    has_trivial_copy_constructor;
  typedef __true_type    has_trivial_assignment_operator;
  typedef __true_type    has_trivial_destructor;
  typedef __true_type    is_POD_type;
};
#endif

// allow StringPiece to be logged
// 重载流运算操作符，使得能够输出到文件或者stdout
std::ostream& operator<<(std::ostream& o, const tmuduo::StringPiece& piece);
#endif // TMUDUO_BASE_STRINGPIECE_H
