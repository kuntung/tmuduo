#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../build/debug/mytests/") add_executable(testThreadOO Thread_OO.cpp Thread.cpp)
#target_link_libraries(testThreadOO pthread)


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
