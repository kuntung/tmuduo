add_executable(testThreadBO Thread_BO.cpp ThreadBO.cpp)
target_link_libraries(testThreadBO pthread)

# BOOST_STATIC_ASSERT
add_executable(BSAtest BSA.cc)

# pthread_atfork
add_executable(Pthread_atfork_test Pthread_atfork_test.cc)
target_link_libraries(Pthread_atfork_test pthread)

# 多线程多进程fork造成死锁
add_executable(Deadlock_test Deadlock_test.cc)
target_link_libraries(Deadlock_test pthread)

add_executable(Deadlock_test2 Deadlock_test2.cc)
target_link_libraries(Deadlock_test2 pthread)

# logging_test
add_executable(Logging_test1 Log_test1.cc)
target_link_libraries(Logging_test1 tmuduo_base)

# reactor测试3：计时器超时事件捕获
add_executable(reactor_test03 Reactor_test03.cc)
target_link_libraries(reactor_test03 tmuduo_net)

## reactor_test03.1间隔触发
add_executable(reactor_test03.1 Reactor_test03.1.cc)
target_link_libraries(reactor_test03.1 tmuduo_net)


# eventloop关注定时器事件
add_executable(reactor_test4 Reactor_test04.cc)
target_link_libraries(reactor_test4 tmuduo_net)

# 支持向IO线程添加事件的runInLoop测试
add_executable(reactor_test05 Reactor_test05.cc)
target_link_libraries(reactor_test05 tmuduo_net)


# 测试EventLoopThread
add_executable(reactor_test06 Reactor_test06.cc)
target_link_libraries(reactor_test06 tmuduo_net)


# 测试Acceptor，socket和地址
add_executable(reactor_test07 Reactor_test07.cc)
target_link_libraries(reactor_test07 tmuduo_net)

# 完整的TcpServer测试，封装了Buffer
## 输出100个A，200个B给对等方
add_executable(reactor_test12 Reactor_test12.cc)
target_link_libraries(reactor_test12 tmuduo_net)

### chagen测试
add_executable(reactor_test13 Reactor_test13.cc)
target_link_libraries(reactor_test13 tmuduo_net)

## 测试TcpServer的业务逻辑
### 不封装TestServer
#add_executable(reactor_test08 Reactor_test08.cc)
#target_link_libraries(reactor_test08 tmuduo_net)
#
### 封装了TestServer
#add_executable(reactor_test09 Reactor_test09.cc)
#target_link_libraries(reactor_test09 tmuduo_net)
#
#
## multiple reactors测试
#add_executable(reactor_test10 Reactor_test10.cc)
#target_link_libraries(reactor_test10 tmuduo_net)
