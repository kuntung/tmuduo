#ifndef TMUDUO_NET_POLLER_H
#define TMUDUO_NET_POLLER_H

#include <vector>
#include <boost/noncopyable.hpp>

#include <tmuduo/base/Timestamp.h>
#include <tmuduo/net/EventLoop.h>

namespace tmuduo
{
namespace net
{
class Channel; // 前向声明

// Base class for IO multiplexing
// this Class does not own any channel objects;

class Poller : boost::noncopyable
{
public:
	typedef std::vector<Channel*> ChannelList;

	Poller(EventLoop* loop);
	virtual ~Poller(); // 虚析构，用于多态

	// Polls the I/O events
	// must be called in the loop thread
	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0; // 纯虚函数
	
	// changes the interested I/O events
	// must be called ...
	virtual void updateChannel(Channel* channel) = 0;
	// remove the channel, when it destructs
	virtual void removeChannel(Channel* channel) = 0;

	static Poller* newDefaultPoller(EventLoop* loop); // 创建poller的行为

	void assertInLoopThread()
	{
		ownerLoop_->assertInLoopThread();
	}

private:
	EventLoop* ownerLoop_; // Poller所属的eventloop
}; // end of Poller
} // end of net
} // end of tmuduo

#endif // TMUDUO_NET_POLLER_H
