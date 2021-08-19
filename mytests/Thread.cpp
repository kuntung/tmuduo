#include <iostream>
#include "Thread.h"

using namespace std;

Thread::Thread()
{
    cout << "Thread()..." << endl;
}

Thread::~Thread()
{
    cout << "~Thread()..." << endl;
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
