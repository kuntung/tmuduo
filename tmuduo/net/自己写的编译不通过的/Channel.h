#ifndef TMUDUO_NET_CHANNEL_H
#define TMUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <memory> // std::shared_ptr, std::weak_ptr

#include <tmuduo/base/Timestamp.h>

namespace tmuduo
{

namespace net
{

class EventLoop; // 前向声明

// class Channel does not own any file descriptor
class Channel : boost::noncopyable
{
public:
	typedef boost::function<void()> EventCallback;
	typedef boost::function<void(Timestamp)> ReadEventCallback;

	Channel(EventLoop* loop, int fd);
	~Channel();

	void handleEvent(Timestamp receiveTime); // 对外暴露的接口
	// 事件回调函数注册
	void setReadCallback(const ReadEventCallback& cb)
	{	readCallback_ = cb; }
	void setWriteCallback(const EventCallback& cb)
	{	writeCallback_ = cb; }
	void setErrorCallback(const EventCallback& cb)
	{	errorCallback_ = cb; }
	void setCloseCallback(const EventCallback& cb)
	{	closeCallback_ = cb; }

	// Tie this channel_ to the owner object managed by shared_ptr
	// prevent the owner object being destroyed in handleEvent
	void tie(const std::shared_ptr<void>&);

	// 对外暴露的接口，用于设置、访问channel的属性
	int fd() const {	return fd_; }
	int events() const { return events_; }
	void set_revents(int revt) { revents_ = revt; } // used by pollers, 用于注册事件返回类型
	int revents() const { return revents_; }
	bool isNoneEvent() const { return events_ == kNoneEvent; }

	void enableReading() { events_ |= kReadEvent; update(); }
	void disableReading() { events_ &= ~kReadEvent; update(); } // 位运算的奥妙全自动
	void enableWriting() { events_ |= kWriteEvent; update(); }
	void disableWriting() { events_ &= ~kWriteEvent; update(); }
	void disableAll() { events_ = kNoneEvent; update(); }
	bool isWriting() const { return events_ & kWriteEvent; }
	bool isReading() const { return events_ & kReadEvent; }

	// for Poller
	int index() { return index_; } // 用于poller管理channelmap
	void set_index(int idx) { index_ = idx; }

	// for debug
	string reventsToString() const;
	string eventsToString() const;

	void doNotLogHup() { logHup_ = false; }

	EventLoop* ownerLoop() { return loop_; } // 返回channel的所属eventloop

	void remove();
private:
	static string eventsToString(int fd, int ev);

private:
	void update();
	void handleEventWithGuard(Timestamp receiveTime);

	static const int kNoneEvent; // 未关注任何事件
	static const int kReadEvent; // 关注可读事件
	static const int kWriteEvent; // 关注可写事件

	EventLoop* loop_; // channel所属的EventLoop
	const int fd_; // 文件描述符，但不负责关闭该文件描述符
	int events_; // 关注的事件
	int revents_; // poll/epoll返回的事件
	int index_; // used by Poller，表示在poll中事件数组中的序号
	bool logHup_; // for POLLHUP

	std::weak_ptr<void> tie_;
	bool tied_;
	bool eventHandling_; // 是否处于事件处理中
	ReadEventCallback readCallback_; // 不一样的原因是因为，有个可读事件返回时间
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
}; // end of Channel
} // end of net
} // end of tmuduo

#endif // TMUDUO_NET_CHANNEL_H
