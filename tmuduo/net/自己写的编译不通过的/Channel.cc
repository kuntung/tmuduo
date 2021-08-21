#include <tmuduo/base/Logging.h>
#include <tmuduo/net/Channel.h>
#include <tmuduo/net/EventLoop.h>

#include <sstream>

#include <poll.h>

using namespace tmuduo;
using namespace tmuduo::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd__)
  : loop_(loop),
    fd_(fd__),
	events_(0),
	revents_(-1), /* 表明此时没有被关注过 */
	logHup_(true),
	tied_(false),
	eventHandling_(false)
{
}

Channel::~Channel()
{
	assert(!eventHandling_);
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
	tie_ = obj;
	tied_ = true;
}

void Channel::update()
{
	loop_->updateChannel(this); // 调用eventloop的updateChannel
}

void Channel::remove()
{
	// 调用之前确保调用disableAll
	assert(isNoneEvent());
	loop_->removeChannel(this); // 在eventloop中移除关注该channel
}

void Channel::handleEvent(Timestamp receiveTime)
{
	std::shared_ptr<void> guard;
	if (tied_)
	{
		guard = tie_.lock();
		if (guard)
		{
			handleEventWithGuard(receiveTime);
		}
			
	}
	else
	{ handleEventWithGuard(receiveTime); }
}

// 真正的事件处理逻辑、由外部注册提供的readCallback_, writeCallback_, errorCallback_处理
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
	eventHandling_ = true;
	if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
	{
		if (logHup_)
		{
			LOG_WARN << "Channel::handleEvent() POLLHUP";
		}
		if (closeCallback_) closeCallback_();
	}

	if (revents_ & POLLNVAL)
	{
		LOG_WARN << "Channel::handleEvent() POLLNVAL";
	}

	if (revents_ & (POLLERR | POLLNVAL))
	{
		if (errorCallback_) errorCallback_();
	}

	if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
	{
		if (readCallback_) readCallback_(receiveTime);
	}

	if (revents_ & POLLOUT)
	{
		if (writeCallback_) writeCallback_();
	}

	eventHandling_ = false;
}

string Channel::reventsToString() const
{
	std::ostringstream oss;
  if (revents_ & POLLIN)
    oss << "IN ";
  if (revents_ & POLLPRI)
    oss << "PRI ";
  if (revents_ & POLLOUT)
    oss << "OUT ";
  if (revents_ & POLLHUP)
    oss << "HUP ";
  if (revents_ & POLLRDHUP)
    oss << "RDHUP ";
  if (revents_ & POLLERR)
    oss << "ERR ";
  if (revents_ & POLLNVAL)
    oss << "NVAL ";

  return oss.str().c_str();
}
