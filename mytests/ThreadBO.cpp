#include <iostream>
#include "ThreadBO.h"

using namespace std;

Thread::Thread(const ThreadFunc& func) : autoDelete_(false), func_(func)
{
    cout << "Thread()..." << endl;
}



void Thread::Start()
{
    pthread_create(&threadId_, NULL, ThreadRoutine, this); 
}

void *Thread::ThreadRoutine(void *arg)
{
    Thread* thread = static_cast<Thread*>(arg); // 通过线程入口函数传入this指针，使得可以访问Run()

    thread->Run(); 

    return NULL;
}

void Thread::Join()
{
    pthread_join(threadId_, NULL);
}

void Thread::SetAutoDelete(bool tag)
{
    autoDelete_ = tag;
}

void Thread::Run()
{
    func_(); // Run只是调用func
}
