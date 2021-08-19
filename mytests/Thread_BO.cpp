#include "ThreadBO.h"
#include <unistd.h>
#include <iostream>
#include <boost/function.hpp>
#include <string>

using namespace std;

void ThreadFunc()
{
    cout << "ThreadFunc..." << endl;
}

void ThreadFunc2(int count)
{
    for (int i = 0; i < count; ++i)
    {
        cout << "i = " << i << endl;
        sleep(1);
    }
}

class Foo
{
public:
    void ThreadFuncMem(string &str)
    {
        for (char ch : str)
        {
            cout << ch << " ";
        }

        cout << endl;
    }
}; // 类成员函数适配

int main(void)
{
    // Thread t(ThreadFunc);
    Thread t(std::bind(ThreadFunc2, 3)); // 将ThreadFunc2转换为没有参数的函数
    t.Start();
    t.Join(); // 等待线程结束
    
    Foo fobj;
    Thread t2(std::bind(&Foo::ThreadFuncMem, &fobj, string("hello world"))); // 适配成员函数
    t2.Start();
    t2.Join(); // 等待线程结束
    return 0;
}
