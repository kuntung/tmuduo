#ifndef TMUDUO_BASE_TIMESTAMP_H
#define TMUDUO_BASE_TIMESTAMP_H

#include <tmuduo/base/Types.h>
#include <tmuduo/base/copyable.h>
#include <boost/operators.hpp>

namespace tmuduo
{

class Timestamp : public tmuduo::copyable, 
              public boost::less_than_comparable<Timestamp>
{
public:
    // construct an invalid timestamp
    Timestamp()
     : microSecondsSinceEpoch_(0)
    {

    }

    // construct a Timestamp at specific time
    // @param microSecondsSinceEpoch
    explicit Timestamp(int64_t microSecondsSinceEpoch);

    void swap(Timestamp &that)
    {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_); // swap
    }
    
    // format microSecondsSinceEpoch_ to calender time
    string toString() const;
    string toFormattedString() const;

    bool valid() const { return microSecondsSinceEpoch_ > 0; }
    
    // for internal usage
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; } 
    time_t secondsSinceEpoch() const 
    { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }
    
    
    // get time of now

    static Timestamp now();
    static Timestamp invalid();

    static const int kMicroSecondsPerSecond = 1000 * 1000; // 1s = 10^6 us

private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

// gets time difference of two timestamps, result in seconds.
//
// @param high, low
// @return (high - low) in seconds
//
inline double timeDifference(const Timestamp high, const Timestamp low) // passed by const reference
{
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();

    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

// add seconds to given timestamp
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);

    return (Timestamp(timestamp.microSecondsSinceEpoch() + delta));
}

};

#endif // TMUDUO_BASE_TIMESTAMP_H
