#ifndef TMUDUO_NET_CALLBACKS_H
#define TMUDUO_NET_CALLBACKS_H

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <tmuduo/base/Timestamp.h>

namespace tmuduo
{
namespace net
{

typedef boost::function<void()> TimerCallback;
} // end of net

} // end of tmuduo
#endif // TMUDUO_NET_CALLBACKS_H
