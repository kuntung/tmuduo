#ifndef TMUDUO_BASE_CURRENTTHREAD_H
#define TMUDUO_BASE_CURRENTTHREAD_H

namespace tmuduo
{
namespace CurrentThread
{
	// internal
	extern __thread int t_cachedTid;
	extern __thread char t_tidString[32];
	extern __thread const char* t_threadName;

	void cacheTid();

	inline int tid() // 频繁调用->inline
	{
		if (t_cachedTid == 0)
		{
			cacheTid();
		}
		return t_cachedTid;
	}

	inline const char* tidString() // for logging
	{
		return t_tidString;
	}

	inline const char* name()
	{
		return t_threadName;
	}

	bool isMainThread();
} // end of namespace CurrentThread

} // end of namespace tmuduo

#endif // TMUDUO_BASE_CURRENTTHREAD_H
