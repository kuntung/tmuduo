#ifndef TMUDUO_NET_CONNECTOR_H
#define TMUDUO_NET_CONNECTOR_H

#include <tmuduo/net/InetAddress.h>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace tmuduo
{
namespace net
{

class Channel;
class EventLoop;

// 主动发起连接，带自动重连功能
class Connector : boost::noncopyable,
			public boost::enable_shared_from_this<Connector>
{
public:
	typedef boost::function<void (int sockfd)> NewConnectionCallback;

	Connector(EventLoop* loop, const InetAddress& serverAddr);
	~Connector();

	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{ newConnectionCallback_ = cb; }

	void start(); // can be called in any thread
	void restart(); // must be called in loop thread
	void stop(); // can be called in any thread

	const InetAddress& serverAddress() const { return serverAddr_; }

private:
	enum States { kDisconnected, kConnecting, kConnected };
	static const int kMaxRetryDelayMs = 30 * 1000; // 30s，最大重连延迟时间
	static const int kInitRetryDelayMs = 500; // 0.5s

	void setState(States s) { state_ = s; }
	void startInLoop(); 
	void stopInLoop();
	void connect();
	void connecting(int sockfd);
	void handleWrite();
	void handleError();
	void retry(int sockfd);
	int removeAndResetChannel();
	void resetChannel();

	EventLoop* loop_; // 所属EventLoop
	InetAddress serverAddr_; 
	bool connect_;
	States state_;
	boost::scoped_ptr<Channel> channel_; // Connector所对应的Channel
	NewConnectionCallback newConnectionCallback_; // 连接成功返回回调函数
	int retryDelayMs_; // 重连延迟时间：ms

}; // end of Connector

} // end of net

} // end of tmuduo

#endif // TMUDUO_NET_CONNECTOR_H
