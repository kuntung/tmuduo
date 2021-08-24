#ifndef TMUDUO_NET_INETADDRESS_H
#define TMUDUO_NET_INETADDRESS_H

#include <tmuduo/base/copyable.h>
#include <tmuduo/base/StringPiece.h>

#include <netinet/in.h>

namespace tmuduo
{
namespace net
{
// wrapper of sockaddr_in
// this is an POD interface class

class InetAddress : public tmuduo::copyable
{
public:
// 仅仅指定port，不指定IP，则IP为INADDR_ANY（0.0.0.0）
// 通常用于服务器端地址绑定
explicit InetAddress(uint16_t port);

InetAddress(const StringPiece& ip, uint16_t port);

// 通常用于接收新的连接请求
InetAddress(const struct sockaddr_in& addr)
  : addr_(addr)
{ }

string toIp() const;
string toIpPort() const;

string toHostPort() const __attribute__ ((deprecated))
{ return toIpPort(); }

const struct sockaddr_in& getSockAddrInet() const { return addr_; }
void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }

uint32_t ipNetEndian() const { return addr_.sin_addr.s_addr; }
uint16_t portNetEndian() const { return addr_.sin_port; }

private:
	struct sockaddr_in addr_;
}; // end of class InetAddress

} // end of net

} // end of tmuduo

#endif // TMUDUO_NET_INETADDRESS_H
