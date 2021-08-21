#include <tmuduo/net/Poller.h>
#include <tmuduo/net/poller/EPollPoller.h>
#include <tmuduo/net/poller/PollPoller.h>

#include <stdlib.h>

using namespace tmuduo::net;

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
	if (::getenv("TMUDUO_USE_POLL"))
	{
		return new PollPoller(loop);
	}
	else
	{
		return new EPollPoller(loop);
	}
}
