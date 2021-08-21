#include <tmuduo/net/Poller.h>

using namespace tmuduo;
using namespace tmuduo::net;

Poller::Poller(EventLoop* loop)
  :  ownerLoop_(loop)
{
}

Poller::~Poller()
{
}
