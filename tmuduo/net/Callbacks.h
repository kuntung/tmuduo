#ifndef TMUDUO_NET_CALLBACKS_H
#define TMUDUO_NET_CALLBACKS_H

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <tmuduo/base/Timestamp.h>

namespace tmuduo
{
namespace net
{

class TcpConnection;

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef boost::function<void()> TimerCallback;

typedef boost::function<void(const TcpConnectionPtr&)> ConnectionCallback; // 连接到来回调函数

typedef boost::function<void(const TcpConnectionPtr&)> CloseCallback;

typedef boost::function<void (const TcpConnectionPtr&,
							  const char* data,
							  ssize_t len)> MessageCallback;
} // end of net

} // end of tmuduo
#endif // TMUDUO_NET_CALLBACKS_H
