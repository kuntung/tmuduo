#include <tmuduo/net/poller/PollPoller.h>

#include <tmuduo/base/Logging.h>
#include <tmuduo/base/Types.h>
#include <tmuduo/net/Channel.h>

#include <assert.h>
#include <poll.h> // IO Multiplexing
#include <errno.h>

using namespace tmuduo;
using namespace tmuduo::net;

PollPoller::PollPoller(EventLoop* loop)
  : Poller(loop)
{
}

PollPoller::~PollPoller()
{
}

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
	int numEvents = ::poll(&(*pollfds_.begin()), pollfds_.size(), timeoutMs);

	Timestamp now(Timestamp::now());

	if (numEvents > 0)
	{
		LOG_TRACE << numEvents << " events happended";
		fillActiveChannels(numEvents, activeChannels);
	}
	else if (numEvents == 0)
	{
		LOG_TRACE << " nothing happended"; 
	}
	else
	{
		LOG_SYSERR << " PollPoller::poll()";
	}
	return now;
}

void PollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
	for (PollFdList::const_iterator pfd = pollfds_.begin(); 
		pfd != pollfds_.end() && numEvents > 0; ++pfd)
	{
		if (pfd->revents > 0)
		{
			--numEvents;
			auto ch = channels_.find(pfd->fd); // 找到当前有响应的文件描述符和channel
			assert(ch != channels_.end());
			Channel* channel = ch->second;
			assert(channel->fd() == pfd->fd);
			channel->set_revents(pfd->revents);

			activeChannels->push_back(channel);
		}
	}
}

void PollPoller::updateChannel(Channel* channel)
{
	Poller::assertInLoopThread();

	LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();

	if (channel->index() < 0)
	{
		// index小于0说明是一个新通道
		assert(channels_.find(channel->fd()) == channels_.end());
		struct pollfd pfd; // 创建一个pollfd

		pfd.fd = channel->fd();
		pfd.events() = static_cast<short>(channel->events());
		pfd.revents = 0;
		pollfds_.push_back(pfd);

		int idx = static_cast<int>(pollfds_.size()) - 1;
		channel->set_index(idx); // 设置新的channel在pollfds的下标为当前最后位置
		channels_[pfd.fd] = channel; // 记录这个<fd, channel*>组合
	}
	else
	{
		assert(channels_.find(channel->fd()) != channels_.end()); // 当前channel存在，只是为了修改
		assert(channels_[channel->fd()] == channel);

		int idx = channel->index();
		assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
		struct pollfd& pfd = pollfds_[idx]; // 找到当前channel, fd已经生成的pollfd
		assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
		pfd.events = static_cast<short>(channel->events());
		pfd.revents = 0;

		// 将一个通道更改为暂不关注事件，但是不从poller中移除该通道
		if (channel->isNoneEvent())
		{
			pfd.fd = -channel->fd() - 1; // 为了removeChannel的优化
		}
	}
}

void PollPoller::removeChannel(Channel* channel)
{
	Poller::assertInLoopThread(); // 判断是否在当前线程调用，eventloop，poller在一个线程
	LOG_TRACE << "fd = " << channel->fd();
	  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel);
  assert(channel->isNoneEvent()); // 当前不关注任何事件
  int idx = channel->index();
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
  const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
  assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events()); 
  // 因为设置完nonEvent，会update，会导致这行为真
  size_t n = channels_.erase(channel->fd()); // 从<fd, channel*>中移除
  assert(n == 1); (void)n;
  if (implicit_cast<size_t>(idx) == pollfds_.size()-1) // 最后一个直接删除
  {
    pollfds_.pop_back();
  }
  else
  {
	// 这里移除的算法复杂度是O(1)，将待删除元素与最后一个元素交换再pop_back
    int channelAtEnd = pollfds_.back().fd;
    iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
    if (channelAtEnd < 0)
    {
      channelAtEnd = -channelAtEnd-1; // 如果是删除中间idx的channel，那么和最后一个channel交换
	  // 但是最后一个channel当前可能处于未关注事件状态，就需要将它的fd转换为正，
	  // 否则无法遍历channels_[channelAtEnd]
    }
    channels_[channelAtEnd]->set_index(idx);
    pollfds_.pop_back();
  }	
}
