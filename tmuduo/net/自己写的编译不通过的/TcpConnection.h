#ifndef TMUDUO_NET_TCPCONNECTION_H
#define TMUDUO_NET_TCPCONNECTION_H

#include <tmuduo/base/Mutex.h>
#include <tmuduo/base/StringPiece.h>
#include <tmuduo/base/Types.h>
#include <tmuduo/net/Callbacks.h>

#include <tmuduo/net/InetAddress.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace tmuduo
{
namespace net
{
class Channel;
class EventLoop;
class Socket;

// TcpConnection, for both client and server usage
// this is and iterface class, so do no expose too much details
//
class TcpConnection : boost::noncopyable,
				public boost::enable_shared_from_this<TcpConnection>
{
public:
	
	// Constructs a TcpConnection with a connected sockfd
	//
	// user should not create this object
	//
	TcpConnection(EventLoop* loop,
				const string& name,
				int sockfd,
				const InetAddress& localAddr,
				const InetAddress& peerAddr);
	
	~TcpConnection();

	EventLoop* getLoop() const { return loop_; }
	const string& name() const { return name_; }
	const InetAddress& localAddres() { return localAddr_; }
	const InetAddress& peerAddress() { return peerAddr_; }

	bool connected() const { return state_ == kConnected; }

	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }

	/// Internal use only.
	void setCloseCallback(const CloseCallback& cb)
	{ closeCallback_ = cb; }

	// called when TcpServer accepts a new connection
	void connectEstablished();   // should be called only once
	// called when TcpServer has removed me from its map
	void connectDestroyed();  // should be called only once

private:
	enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

	void handleRead(Timestamp receiveTime); 
	void handleClose();
	void handleError();
	void setState(StateE s) { state_ = s; }

	EventLoop* loop_; // 所属的EventLoop
	string name_; // 连接名
	StateE state_; 

	boost::scoped_ptr<Socket> socket_:
	boost::scoped_ptr<Channel> channel_;
	InetAddress localAddr_;
	InetAddress peerAddr_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	CloseCallback closeCallback_;
}; // end of TcpConnection
typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;
} // end of net

} // end of tmuduo

#endif // TMUDUO_NET_TCPCONNECTION_H
