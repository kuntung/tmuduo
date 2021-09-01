#ifndef MUDUO_EXAMPLES_IDLECONNECTION_ECHO_H
#define MUDUO_EXAMPLES_IDLECONNECTION_ECHO_H

#include <tmuduo/net/TcpServer.h>
//#include <muduo/base/Types.h>

#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION < 104700
namespace boost
{
template <typename T>
inline size_t hash_value(const boost::shared_ptr<T>& x)
{
  return boost::hash_value(x.get());
}
}
#endif

// RFC 862
class EchoServer
{
 public:
  EchoServer(tmuduo::net::EventLoop* loop,
             const tmuduo::net::InetAddress& listenAddr,
             int idleSeconds);

  void start();

 private:
  void onConnection(const tmuduo::net::TcpConnectionPtr& conn);

  void onMessage(const tmuduo::net::TcpConnectionPtr& conn,
                 tmuduo::net::Buffer* buf,
                 tmuduo::Timestamp time);

  void onTimer();

  void dumpConnectionBuckets() const;

  typedef boost::weak_ptr<tmuduo::net::TcpConnection> WeakTcpConnectionPtr;

  struct Entry : public tmuduo::copyable
  {
    explicit Entry(const WeakTcpConnectionPtr& weakConn)
      : weakConn_(weakConn)
    {
    }

    ~Entry()
    {
      tmuduo::net::TcpConnectionPtr conn = weakConn_.lock();
      if (conn)
      {
        conn->shutdown();
      }
    }

    WeakTcpConnectionPtr weakConn_;
  };
  typedef boost::shared_ptr<Entry> EntryPtr;  // set中的元素是一个EntryPtr
  typedef boost::weak_ptr<Entry> WeakEntryPtr;
  typedef boost::unordered_set<EntryPtr> Bucket;  // 环形缓冲区每个格子存放的是一个hash_set
  typedef boost::circular_buffer<Bucket> WeakConnectionList;  // 环形缓冲区

  tmuduo::net::EventLoop* loop_;
  tmuduo::net::TcpServer server_;
  WeakConnectionList connectionBuckets_; // 连接队列环形缓冲区
};

#endif  // MUDUO_EXAMPLES_IDLECONNECTION_ECHO_H
