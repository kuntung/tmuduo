#ifndef TMUDUO_BASE_THREADLOCALSINGLETON_H
#define TMUDUO_BASE_THREADLOCALSINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace tmuduo
{

template<typename T>
class ThreadLocalSingleton : boost::noncopyable
{

public:
	
	static T& instance()
	{
		if (t_value_ == nullptr)
		{
			t_value_ = new T();
			deleter_.set(t_value_); // 设置成线程特有数据
		}

		return *t_value_;
	}

	static T* pointer()
	{
		return t_value_;
	}

// 以上所做的工作，只是表明t_value_是线程局部存储的
private:
	static void destructor(void* obj)
	{
		assert(obj = t_value_);
		delete t_value_; // 释放内存

		t_value_ = 0;
	}

	class Deleter
	{
	public:
		Deleter()
		{
			pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor); // 创建一个线程key
		}
	
		~Deleter()
		{
			pthread_key_delete(pkey_);
		}

		void set(T* newObj)
		{
			assert(pthread_getspecific(pkey_) == NULL);
			pthread_setspecific(pkey_, newObj); // 将该线程的__thread同时设定成TSD, 目的在于为了自动销毁
		}

	private:
		pthread_key_t pkey_;
	}; 
private:
	static __thread T* t_value_;
	static Deleter deleter_;
}; // end of ThreadLocalSingleton

// 类静态成员变量初始化
template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;

template<typename T>
// typename是为了编译器正常编译
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_; // 默认初始化
} // end of tmuduo

#endif // TMUDUO_BASE_THREADLOCALSINGLETON_H
