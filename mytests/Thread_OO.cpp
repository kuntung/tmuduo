#include "Thread.h"
#include <unistd.h>
#include <iostream>

using namespace std;

class TestThread : public Thread
{
public:
    TestThread(int count) : count_(count)
    {
        cout << "TestThread ..." << endl;
    }

    ~TestThread()
    {
        cout << "~TestThread()..." << endl;
    }

    void Run()
    {
        while (count_--)
        {
            cout << "this is a test..." << endl;

            sleep(1);
        }
    }

    int count_;
};

int main(void)
{
    TestThread t(5);
    t.Start();
    t.Join(); // 等待线程结束
    return 0;
}
