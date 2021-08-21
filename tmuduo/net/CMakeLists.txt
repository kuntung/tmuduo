set(net_SRCS 
Channel.cc
Poller.cc
EventLoop.cc
poller/DefaultPoller.cc
poller/EPollPoller.cc
poller/PollPoller.cc
Timer.cc
TimerQueue.cc
)

add_library(tmuduo_net ${net_SRCS})
target_link_libraries(tmuduo_net tmuduo_base)

install(TARGETS tmuduo_net DESTINATION lib)
file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/tmuduo/net)
