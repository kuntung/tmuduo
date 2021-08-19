#ifndef TMUDUO_BASE_EXCEPTION_H
#define TMUDUO_BASE_EXCEPTION_H

#include <tmuduo/base/Types.h>
#include <exception>

namespace tmuduo
{
class Exception : public std::exception
{
public:
    explicit Exception(const char *what);
    explicit Exception(const string& what);
    virtual ~Exception() throw(); // what is throw()?
    virtual const char* what() const throw();
    const char* stackTrace() const throw();

private:
    void fillStackTrace(); // mark trace information
    string demangle(const char* symbol); // demangle char *, which is mangling by stacktrace

    string message_;
    string stack_;

}; // end of Exception

}; // end of tmuduo

#endif // TMUDUO_BASE_EXCEPTION_H
