#include <tmuduo/base/Timestamp.h>
#include <vector>
#include <iostream>

using tmuduo::Timestamp;
using std::vector;
using std::cout;
using std::endl;

void passByValue(Timestamp x)
{
    cout << x.toString() << endl;

}

void passByConstReference(const Timestamp& x)
{
    cout << x.toString() << endl;
}

void benchmark()
{
    const int kNumber = 1000 * 1000;
    vector<Timestamp> stamps;

    stamps.reserve(kNumber);

    for (int i = 0; i < kNumber; ++i)
    {
        stamps.emplace_back(Timestamp::now());
    }

    cout << "start of insert: " << stamps.front().toString() << endl;
    cout << "end of insert: " << stamps.back().toString() << endl;
    cout << "using total time: " << timeDifference(stamps.back(), stamps.front())<< endl;
    
    int increments[100] = {0};

    int64_t start = stamps.front().microSecondsSinceEpoch();

    for (int i = 1; i < kNumber; ++i)
    {
        int64_t next = stamps[i].microSecondsSinceEpoch();

        int64_t inc = next - start;
        start = next;

        if (inc < 0)
        {
            cout << "reverse!\n" << endl;
        }
        else if (inc < 100) // 统计[0, 99]的时间间隔数
        {
            ++increments[inc];
        }
        else
        {
            cout << "big gap" << static_cast<int>(inc);
        }

    }

    for (int i = 0; i < 100; ++i)
    {
        printf("%2d: %d\n", i, increments[i]);
    }
}

int main(void)
{
    Timestamp now(Timestamp::now());
    
    cout << now.toString() << endl;
    passByValue(now);
    passByConstReference(now);

    benchmark();


    return 0;
}
