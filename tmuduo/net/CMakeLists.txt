set(net_SRCS 
Acceptor.cc
Buffer.cc
Channel.cc
EventLoop.cc
EventLoopThread.cc
EventLoopThreadPool.cc
poller/DefaultPoller.cc
poller/EPollPoller.cc
poller/PollPoller.cc
Poller.cc
SocketsOps.cc
Socket.cc
InetAddress.cc
TcpConnection.cc
TcpServer.cc
Timer.cc
TimerQueue.cc
)

add_library(tmuduo_net ${net_SRCS})
target_link_libraries(tmuduo_net tmuduo_base)

install(TARGETS tmuduo_net DESTINATION lib)
file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/tmuduo/net)

if (NOT CMAKE_BUILD_NO_EXAMPLES)
	add_subdirectory(tests)
endif()
