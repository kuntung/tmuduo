#ifndef TMUDUO_NET_ACCEPTOR_H
#define TMUDUO_NET_ACCEPTOR_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
// Acceptor有一个socket文件描述符，并且生成一个channel用于监听
// 只能属于一个eventloop
#include <tmuduo/net/Channel.h>
#include <tmuduo/net/Socket.h>

namespace tmuduo
{
namespace net
{
class EventLoop;
class InetAddress;

// acceptor of incoming TCP connections;
class Acceptor : boost::noncopyable
{
public:
	typedef boost::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

	Acceptor(EventLoop* loop, const InetAddress& listenAddr);
	~Acceptor();

	void setNewConnectionCallback(const NewConnectionCallback& cb) 
	{ newConnectionCallback_ = cb; }

	bool listenning() const { return listenning_; }

	void listen();

private:
	void handleRead();

	EventLoop* loop_; // 套接字所属的eventloop
	Socket acceptSocket_; // 监听套接字
	Channel acceptChannel_; // 通道，观察套接字的连接请求
	NewConnectionCallback newConnectionCallback_;
	bool listenning_;
	int idleFd_; // 处理EMPFILE
}; // end of Acceptor

} // end of net

} // end of tmuduo

#endif // TMUDUO_NET_ACCEPTOR_H
