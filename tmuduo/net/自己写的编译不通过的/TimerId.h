#ifndef TMUDUO_NET_TIMERID_H
#define TMUDUO_NET_TIMERID_H

#include <tmuduo/base/copyable.h>

namespace tmuduo
{
namespace net
{
class Timer;

// an opaque identifier, for cancleing timer

class TimerId : public tmuduo::copyable
{
public:
	TimerId()
	  : timer_(NULL),
	    sequence_(0)
{
}

	TimerId(Timer* timer, int64_t seq)
	  : timer_(timer),
	    sequence_(seq)
{
}

	friend class TimerQueue;

private:
	Timer* timer_; // 定时器地址
	int64_t sequence_; // 定时器序号
}; // end fo TimerId
} // end of net

} // end of tmuduo
#endif // TMUDUO_NET_TIMERID_H
