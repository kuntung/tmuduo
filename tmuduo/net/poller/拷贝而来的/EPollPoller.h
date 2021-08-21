// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef TMUDUO_NET_POLLER_EPOLLPOLLER_H
#define TMUDUO_NET_POLLER_EPOLLPOLLER_H

#include <tmuduo/net/Poller.h>

#include <map>
#include <vector>

struct epoll_event;

namespace tmuduo
{
namespace net
{

///
/// IO Multiplexing with epoll(4).
///
class EPollPoller : public Poller
{
 public:
  EPollPoller(EventLoop* loop);
  virtual ~EPollPoller();

  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
  virtual void updateChannel(Channel* channel);
  virtual void removeChannel(Channel* channel);

 private:
  static const int kInitEventListSize = 16;

  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;
  void update(int operation, Channel* channel);

  typedef std::vector<struct epoll_event> EventList;
  typedef std::map<int, Channel*> ChannelMap;

  int epollfd_;
  EventList events_; // 实际上返回的通道列表：activeChannels
  ChannelMap channels_; // 关注的通道列表
};

}
}
#endif  // MUDUO_NET_POLLER_EPOLLPOLLER_H
