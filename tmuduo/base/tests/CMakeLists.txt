# 日志测试
add_executable(Logging_test2 Logging_test.cc)
target_link_libraries(Logging_test2 tmuduo_base)
# 时间戳代码测试
add_executable(Timestamp_unittest Timestamp_unittest.cc)
target_link_libraries(Timestamp_unittest tmuduo_base)

# atomic test
add_executable(Atomic_unittest Atomic_unittest.cc)
target_link_libraries(Atomic_unittest tmuduo_base)

# exception test
add_executable(Exception_test Exception_test.cc)
target_link_libraries(Exception_test tmuduo_base)

# thread test
add_executable(Thread_test Thread_test.cc)
target_link_libraries(Thread_test tmuduo_base)

# IPC机制实现
add_executable(Mutex_test Mutex_test.cc)
target_link_libraries(Mutex_test tmuduo_base)
## CountDownLatch
add_executable(CountDownLatch_test1 CountDownLatch_test1.cc)
target_link_libraries(CountDownLatch_test1 tmuduo_base)
## 主线程等待其他线程就绪
add_executable(CountDownLatch_test2 CountDownLatch_test2.cc)
target_link_libraries(CountDownLatch_test2 tmuduo_base)

# 无界缓冲队列测试
add_executable(BlockingQueue_test BlockingQueue_test.cc)
target_link_libraries(BlockingQueue_test tmuduo_base)

add_executable(BlockingQueue_bench BlockingQueue_bench)
target_link_libraries(BlockingQueue_bench tmuduo_base)

# 有界缓冲队列实例
add_executable(BoundedBlockingQueue_test BoundedBlockingQueue_test.cc)
target_link_libraries(BoundedBlockingQueue_test tmuduo_base)

# 线程池测试
add_executable(ThreadPool_test ThreadPool_test.cc)
target_link_libraries(ThreadPool_test tmuduo_base)

# Singleton测试
add_executable(Singleton_test Singleton_test.cc)
target_link_libraries(Singleton_test tmuduo_base)

# ThreadLocal测试
add_executable(ThreadLocal_test ThreadLocal_test.cc)
target_link_libraries(ThreadLocal_test tmuduo_base)

# SingletonThreadLocal
add_executable(SingletonThreadLocal_test SingletonThreadLocal_test.cc)
target_link_libraries(SingletonThreadLocal_test tmuduo_base)

# ThreadLocalSingleton_test
add_executable(ThreadLocalSingleton_test ThreadLocalSingleton_test.cc)
target_link_libraries(ThreadLocalSingleton_test tmuduo_base)

# 日志流LogStream实现
if(BOOSTTEST_LIBRARY)
add_executable(LogStream_test LogStream_test.cc)
target_link_libraries(LogStream_test tmuduo_base boost_unit_test_framework)
endif()

## 由boost库进行单元测试
add_executable(LogStream_bench LogStream_bench.cc)
target_link_libraries(LogStream_bench tmuduo_base)
