#include <tmuduo/net/poller/EPollPoller.h>

#include <tmuduo/base/Logging.h>
#include <tmuduo/net/Channel.h>

#include <boost/static_assert.hpp>
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

using namespace tmuduo;
using namespace tmuduo::net;

BOOST_STATIC_ASSERT(EPOLLIN == POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI == POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT == POLLOUT);
BOOST_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP);
BOOST_STATIC_ASSERT(EPOLLERR == POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP == POLLHUP);

namespace
{
	const int kNew = -1;
	const int kAdded = 1;
	const int kDeleted = 2;
} // 用于表示channel的关注状态

EPollPoller::EPollPoller(EventLoop* loop)
  : Poller(loop),
    epollfd_(::epoll_create(EPOLL_CLOEXEC)),
	events_(kInitEventListSize)
{
	if (epollfd_ < 0)
	{
		LOG_SYSFATAL << "EPollPoller::EPollPoller";
	}
}

EPollPoller::~EPollPoller()
{
	::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
	int numEvents = ::epoll_wait(epollfd_,
								&*events_.begin(),
								static_cast<int>(events_.size()),
								timeoutMs);
	
	Timestamp now(Timestamp::now());

	if (numEvents > 0)
	{
		LOG_TRACE << numEvents << " events happened";
		fillActiveChannels(numEvents, activeChannels);
		if (implicit_cast<size_t>(numEvents) == events_.size())
		{
			events_.resize(events_.size() * 2);
		} // 动态扩容
	}
	else if (numEvents == 0)
	{
		LOG_TRACE << " nothing happened";
	}
	else
	{
		LOG_SYSERR << " EPollPoller::poll()";	
	}

	return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
	assert(implicit_cast<size_t>(numEvents) <= events_.size()); // events_:返回的通道列表
	for (int i = 0; i < numEvents; ++i)
	{
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
	#ifndef NDEBUG
		int fd = channel->fd();
		auto it = channels_.find(fd);
		assert(it != channels_.end());
		assert(it->second == channel);
	#endif
		channel->set_revents(events_[i].events);
		activeChannels->push_back(channel);
	}
}

// 通道的enableReading导致updatedChannel被调用
void EPollPoller::updateChannel(Channel* channel)
{
	Poller::assertInLoopThread();

	LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();

	const int index = channel->index();
	if (index == kNew || index == kDeleted) // 新增一个事件关注或重新添加关注
	{
		int fd = channel->fd();
		if (index == kNew)
		{
			assert(channels_.find(fd) == channels_.end());
			channels_[fd] = channel;
		}
		else /* index = kDeleted */
		{
			assert(channels_.find(fd) != channels_.end());
			assert(channels_[fd] == channel);
		}

		channel->set_index(kAdded);
		update(EPOLL_CTL_ADD, channel); // 注册事件
	}
	else
	{
		// update existing one with EPOLL_CTL_MOD/DEL
		int fd = channel->fd();
		(void)fd;
		assert(channels_.find(fd) != channels_.end());
		assert(channels_[fd] == channel);
		assert(index == kAdded);
		if (channel->isNoneEvent())
		{
			update(EPOLL_CTL_DEL, channel); // 取消关注事件，并没有移除该channel
			channel->set_index(kDeleted);
		}
		else
		{
			update(EPOLL_CTL_MOD, channel);
		}
	}
} // end of updatedChannel

void EPollPoller::removeChannel(Channel* channel)
{
	Poller::assertInLoopThread();

	int fd = channel->fd();
	LOG_TRACE << " fd = " << fd;
	assert(channels_.find(fd) != channels_.end());
	assert(channels_[fd] == channel);
	assert(channel->isNoneEvent());
	int index = channel->index();

	assert(index == kAdded || index == kDeleted);
	size_t n = channels_.erase(fd); // 以key移除channel
	(void)n;
	assert(n == 1);

	if (index == kAdded)
	{
		update(EPOLL_CTL_DEL, channel); // 从epoll中移除
	}

	channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel* channel)
{
	struct epoll_event event;
	bzero(&event, sizeof event);
	event.events = channel->events();
	event.data.ptr = channel;

	int fd = channel->fd();
	if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) // 最底层的epoll事件管理epoll_ctl函数
	{
		if (operation == EPOLL_CTL_DEL)
		{
			LOG_SYSERR << "epoll_ctl op=" << operation << " fd=" << fd;
		}
		else
		{
			LOG_SYSFATAL << "epoll_ctl op=" << operation << " fd=" << fd;
		}
	}

}
