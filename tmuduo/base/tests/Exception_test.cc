#include <tmuduo/base/Exception.h>
#include <stdio.h>

using namespace tmuduo;

class Bar
{
public:
    void test()
    {
       throw Exception("oops");        
    }
}; // end of Bar

void foo()
{
    Bar b;
    b.test();
}

int main(void)
{
    printf("entering main...\n");
    try
    {
        foo();
    }
    catch (const Exception& ex)
    {
        printf("reason: %s\n", ex.what());
        printf("stack trace: %s\n", ex.stackTrace());
    }
    return 0;
}
