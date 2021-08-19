#ifndef TMUDUO_BASE_SINGLETON_H
#define TMUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <stdlib.h> // atexit

namespace tmuduo
{

template<typename T>
class Singleton : boost::noncopyable
{
public:
	static T& instance() // 只对外提供一个全局入口instance
	{
		pthread_once(&ponce_, &Singleton::init); // 

		return *value_;
	}

private:
	Singleton() = default;
	~Singleton() = default;

	static void init()
	{
		value_ = new T();
		::atexit(destroy);
	}

	static void destroy()
	{
		typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
		delete value_;
	}

private:
	static pthread_once_t ponce_;
	static T* value_;
}; // end of Singleton
template<typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::value_ = NULL;
} // end of tmuduo

#endif // TMUDUO_BASE_SINGLETON_H
