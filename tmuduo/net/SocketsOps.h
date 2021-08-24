#ifndef TMUDUO_NET_SOCKETSOPS_H
#define TMUDUO_NET_SOCKETSOPS_H

#include <arpa/inet.h> 

namespace tmuduo
{
namespace net
{

namespace sockets
{
// 一个服务器套接字的流程
// 1.创建create
// 2.绑定bind
// 3.监听listen
// 4.接收处理新的连接请求accept
// 5.处理已连接事件，readv/writev
// 6. 关闭连接close/shutdown

// create a non-blocking socket file descriptor
// abort if any error
int createNonblockingOrDie();

int connect(int sockfd, const struct sockaddr_in& addr);
void bindOrDie(int sockfd, const struct sockaddr_in& addr);
void listenOrDie(int sockfd);
int accept(int sockfd, struct sockaddr_in* addr);
ssize_t read(int sockfd, void* buf, size_t count);
ssize_t readv(int sockfd, const struct iovec* iov, int iovcnt);
ssize_t write(int sockfd, const void* buf, size_t count);
void close(int sockfd);
void shutdownWrite(int sockfd);

void toIpPort(char* buf, size_t size, const struct sockaddr_in& addr);
void toIp(char* buf, size_t size, const struct sockaddr_in& addr);
void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr); // 根据IP和port创建地址

int getSocketError(int sockfd);
void getFileStatus(int sockfd);

struct sockaddr_in getLocalAddr(int sockfd); // 获取本机地址
struct sockaddr_in getPeerAddr(int sockfd); //获取对等方地址

bool isSelfConnect(int sockfd);

} // end of sockets

} // end of net

} // end of tmuduo

#endif // TMUDUO_NET_SOCKETSOPS_H
