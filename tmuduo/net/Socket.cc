#include <tmuduo/net/Socket.h>

#include <tmuduo/net/InetAddress.h>
#include <tmuduo/net/SocketsOps.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h> // bzero

using namespace tmuduo;
using namespace tmuduo::net;

Socket::~Socket()
{
	sockets::close(sockfd_); // 关闭套接字
}

void Socket::bindAddress(const InetAddress& addr)
{
	sockets::bindOrDie(sockfd_, addr.getSockAddrInet());
}

void Socket::listen()
{
	sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress* peeraddr) /*同时保存对等方地址*/
{
	struct sockaddr_in addr;

	bzero(&addr, sizeof addr);
	int connfd = sockets::accept(sockfd_, &addr);

	if (connfd >= 0)
	{
		peeraddr->setSockAddrInet(addr);
	}

	return connfd; // 返回建立连接的套接字
}

void Socket::shutdownWrite()
{
  sockets::shutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
               &optval, sizeof optval);
  // FIXME CHECK
}

void Socket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof optval);
  // FIXME CHECK
}

void Socket::setKeepAlive(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
               &optval, sizeof optval);
  // FIXME CHECK
}
