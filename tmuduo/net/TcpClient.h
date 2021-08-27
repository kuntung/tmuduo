#ifndef TMUDUO_NET_TCPCLIENT_H
#define TMUDUO_NET_TCPCLIENT_H

#include <boost/noncopyable.hpp>
#include <tmuduo/base/Mutex.h>
#include <tmuduo/net/TcpConnection.h>

namespace tmuduo
{
namespace net
{

class Connector;
typedef boost::shared_ptr<Connector> ConnectorPtr;

class TcpClient : boost::noncopyable
{
public:
	TcpClient(EventLoop* loop,
			const InetAddress& serverAddr,
			const string& name);
	
	~TcpClient();

	void connect(); 
	void disconnect();
	void stop();

	TcpConnectionPtr connection() const
	{
		MutexLockGuard lock(mutex_);
		return connection_;
	}

	bool retry() const;
	void enableRetry() { retry_ = true; }

	// set connection callback
	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	// set message callback
	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }
	
	// set write complete callback
	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{ writeCompleteCallback_ = cb; }

private:
	void newConnection(int sockfd);
	void removeConnection(const TcpConnectionPtr& conn); 

	EventLoop* loop_;
	ConnectorPtr connector_; // 用于主动发起连接
	const string name_; // 客户端名称
	ConnectionCallback connectionCallback_; // 连接建立回调函数
	MessageCallback messageCallback_; // 消息到来回调函数
	WriteCompleteCallback writeCompleteCallback_; // 消息发送完毕回调函数
	bool retry_;
	bool connect_; 
	int nextConnId_; // name_ + nextConnId_标识一个连接
	mutable MutexLock mutex_;
	TcpConnectionPtr connection_; // Connector成功以后，得到一个TcpConnectionPtr对象
}; // end of TcpClient 

} // end of net

} // end of tmduo
#endif // TMUDUO_NET_TCPCLIENT_H
