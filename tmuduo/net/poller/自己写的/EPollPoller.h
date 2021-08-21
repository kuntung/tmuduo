#ifndef TMUDUO_NET_EPOLLPOLLER_H
#define TMUDUO_NET_EPOLLPOLLER_H

#include <tmuduo/net/Poller.h>
#include <map>
#include <vector>

struct epoll_event;

namespace tmuduo
{
namespace net
{
// IO Multiplexing with epoll4

class EPollPoller : public Poller
{
public:
	EPollPoller(EventLoop* loop);

	virtual ~EPollPoller();

	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
	virtual void updateChannel(Channel* channel);
	virtual void removeChannel(Channel* channel);

private:
	static const int kInitEventListSize = 16;

	void fillActiveChannels(int numEvents,
						ChannelList* activeChannels) const;
	void update(int operation, Channel* channel); // 额外的一个函数，用于epoll_ctl

	typedef std::vector<struct epoll_event> EventList;
	typedef std::map<int, Channel*> ChannelMap;

	int epollfd_;
	EventList events_; // 实际上返回的通道列表：activeChannels
	ChannelMap channels_; // 关注的通道列表
}; // end of EPollPoller
} // end of net

} // end of tmuduo

#endif // TMUDUO_NET_EPOLLPOLLER_H
