#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>

class Thread
{
public:
    Thread();
    virtual ~Thread();
    
    void Start();
    void Join();
private:
    virtual void Run() = 0; // 不能够直接调用
    static void *ThreadRoutine(void* arg); // 声明为static，一方面为了封装，二者是为了不要this指针
    pthread_t threadId_;
};

#endif // _THREAD_H_
