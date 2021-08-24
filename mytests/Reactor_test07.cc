#include <tmuduo/net/Acceptor.h>
#include <tmuduo/net/EventLoop.h>
#include <tmuduo/net/InetAddress.h>
#include <tmuduo/net/SocketsOps.h>

#include <stdio.h>

using namespace tmuduo;
using namespace tmuduo::net;

void newConnection(int sockfd, const InetAddress& peerAddr)
{
	printf("newConnection(): accepted a new connection from %s\n",
		peerAddr.toIpPort().c_str());
	::write(sockfd, "how are you?\n", 13); // 向对等方发送消息

	sockets::close(sockfd);
}

int main(void)
{
	printf("main(): pid = %d\n", getpid());

	InetAddress listenAddr(8888);
	EventLoop loop;

	Acceptor acceptor(&loop, listenAddr);
	acceptor.setNewConnectionCallback(newConnection);
	acceptor.listen(); // 开始监听

	loop.loop(); // 循环监听


	return 0;
}
