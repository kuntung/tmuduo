#ifndef TMUDUO_NET_CALLBACKS_H
#define TMUDUO_NET_CALLBACKS_H

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <tmuduo/base/Timestamp.h>

namespace tmuduo
{
namespace net
{
class Buffer;
class TcpConnection;

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef boost::function<void()> TimerCallback;

typedef boost::function<void(const TcpConnectionPtr&)> ConnectionCallback; // 连接到来回调函数

typedef boost::function<void(const TcpConnectionPtr&)> CloseCallback;

typedef boost::function<void (const TcpConnectionPtr&,
							  Buffer*,
							  Timestamp)> MessageCallback;

typedef boost::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef boost::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn,
					Buffer* buffer,
					Timestamp receiveTime);
} // end of net

} // end of tmuduo
#endif // TMUDUO_NET_CALLBACKS_H
