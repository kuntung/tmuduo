#ifndef TMUDUO_BASE_ATOMIC_H
#define TMUDUO_BASE_ATOMIC_H

#include <boost/noncopyable.hpp>
#include <stdint.h>

namespace tmuduo
{

namespace detail
{
template <typename T>
class AtomicIntegerT : boost::noncopyable
{
public:
    AtomicIntegerT()
      : value_(0)
    {

    }

// uncommit if need copying and assignment
// AtomicIntegerT(const AtomicIntegerT& that)
//   : value_(that.get()) 
// {}
// 
// AtomicIntegerT& operator(const AtomicIntegerT& that)
// {
//  getAndSet(that.get());
//
//  return *this;
// }

// thread safety getValue()
T get()
{
    return __sync_val_compare_and_swap(&value_, 0, 0); 
}

T getAndAdd(T x)
{
// 返回add之前的值，并add
return __sync_fetch_and_add(&value_, x);
}

T addAndGet(T x)
{
// add first, then return added value
    return getAndAdd(x) + x; 
}

T incrementAndGet()
{
// increment and then return new value
    return addAndGet(1);
}

T decrementAndGet()
{
    return addAndGet(-1);
}

void add(T x)
{
// then value_ += x;
    getAndAdd(x);
}

void increment() // no return
{
    incrementAndGet();
}

void decrement()
{
    decrementAndGet();
}

T getAndSet(T newValue)
{
// return old value, and then value was assigned by newValue
    return __sync_lock_test_and_set(&value_, newValue);
}

private:
    volatile T value_; // value_ should be volatile

}; // end of AtomicIntegerT

}; // end of detail

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64; // 两个特化版本的原子类
}; // end of tmuduo

#endif // TMUDUO_BASE_ATOMIC_H
