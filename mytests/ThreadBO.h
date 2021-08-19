// 基于对象的线程类封装
#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
#include <boost/function.hpp>

class Thread
{
public:

    typedef std::function<void ()> ThreadFunc; 
    explicit Thread(const ThreadFunc& func);

    void Start();
    void Join();

    void SetAutoDelete(bool autoDelete_);
private:
    void Run();
    ThreadFunc func_;
    static void *ThreadRoutine(void* arg); // 声明为static，一方面为了封装，二者是为了不要this指针
    pthread_t threadId_;

    bool autoDelete_;
};

#endif // _THREAD_H_
