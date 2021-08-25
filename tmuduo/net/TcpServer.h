#ifndef TMUDUO_NET_TCPSERVER_H
#define TMUDUO_NET_TCPSERVER_H

#include <tmuduo/base/Types.h>
#include <tmuduo/net/TcpConnection.h>

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace tmuduo
{
namespace net
{
class Acceptor;
class EventLoop;
class EventLoopThreadPool;

// TCP server, supports single-threaded and therad-poll models.
//
// this is an interface calss, so do not expose too much details
//
class TcpServer : boost::noncopyable
{
public:
	typedef boost::function<void(EventLoop*)> ThreadInitCallback; // 用于EventLoopThreadPool的IO线程调用
	//
	// TcpServer(EventLoop* loop, const InetAddress& listenAddr);
	
	TcpServer(EventLoop* loop,
			const InetAddress& listenAddr,
			const string& nameArg);
	
	~TcpServer(); 
	
	const string& hostport() const { return hostport_; }
	const string& name() const { return name_; }

	/// set the number of threads for handling input.
	//
	// Always accept new connection in loop's thread (main reactor)
	// must be called before @c start
	// @param numthreads
	// - 0 means all I/O in loop's thread, no thread will created（单线程模式）
	// - 1 means all I/O in another thread,
	// - N means a thread poll with N threads, new connections are assigned on a round-robin basis
	void setThreadNum(int numThreads);
	void setThreadInitCallback(const ThreadInitCallback& cb)
	{ threadInitCallback_ = cb; }

	// starts the server if it is not listenning
	//
	// it is harmless to call it multiple times
	void start(); // TcpServer开启服务
	
	// Set Connection Callback
	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	// set message callback
	//
	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }

	// 消息发送完毕事件回调函数
	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{	writeCompleteCallback_ = cb; }

private:
	
	// Not thread safe, but in loop
	void newConnection(int sockfd, const InetAddress& peerAddr);

	// thread safe
	void removeConnection(const TcpConnectionPtr& conn); // 传入一个scoped_ptr管理的tcpConnection对象
	void removeConnectionInLoop(const TcpConnectionPtr& conn);  

	typedef std::map<string, TcpConnectionPtr> ConnectionMap;

	EventLoop* loop_; // the acceptor loop
	const string hostport_; // 服务端口
	const string name_; // 服务名
	boost::scoped_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
	boost::scoped_ptr<EventLoopThreadPool> threadPool_;

	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	ThreadInitCallback threadInitCallback_;
	bool started_;

	int nextConnId_; // 下一个连接ID
	ConnectionMap connections_; // 连接列表：连接名和TcpConnectionPtr

}; // end of TcpServer

} // end of net

} // end of tmuduo

#endif // TMUDUO_NET_TCPSERVER_H
