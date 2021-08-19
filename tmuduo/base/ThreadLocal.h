#ifndef TMUDUO_BASE_THREADLOCAL_H
#define TMUDUO_BASE_THREADLOCAL_H

#include  <boost/noncopyable.hpp>
#include <pthread.h>

namespace tmuduo
{

template<typename T>
class ThreadLocal : boost::noncopyable
{
public:
	ThreadLocal()
	{
		pthread_key_create(&pkey_, &ThreadLocal::destructor); // 注册绑定线程特定数据销毁函数destructor
	}

	~ThreadLocal()
	{
		pthread_key_delete(pkey_);
	}

	T& value()
	{
		T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));

		if (!perThreadValue)
		{
			T* newObj = new T();
			pthread_setspecific(pkey_, newObj);
			perThreadValue = newObj;
		}

		return *perThreadValue;
	}

private:
	
	static void destructor(void* x)
	{
		T* obj = static_cast<T*>(x);
		typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
		delete obj;
	}
	pthread_key_t pkey_;
}; // end of ThreadLocal

} // end of tmuduo

#endif // TMUDUO_BASE_THREADLOCAL_H
