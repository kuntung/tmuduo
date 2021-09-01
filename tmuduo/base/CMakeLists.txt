set(base_SRCS
 AsyncLogging.cc
 Timestamp.cc
 Exception.cc
 Thread.cc
 Condition.cc
 CountDownLatch.cc
 ThreadPool.cc
 LogStream.cc
 Logging.cc
 LogFile.cc
 ProcessInfo.cc
 FileUtil.cc
)

add_library(tmuduo_base ${base_SRCS})
target_link_libraries(tmuduo_base pthread rt) # 静态库连接

install(TARGETS tmuduo_base DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/tmuduo/base)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
    add_subdirectory(tests)
endif()
