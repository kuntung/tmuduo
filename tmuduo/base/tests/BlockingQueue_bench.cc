#include <tmuduo/base/BlockingQueue.h>
#include <tmuduo/base/CountDownLatch.h>
#include <tmuduo/base/Thread.h>
#include <tmuduo/base/Timestamp.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <map>
#include <string>
#include <stdio.h>

class Bench
{
 public:
  Bench(int numThreads)
    : latch_(numThreads),
      threads_(numThreads)
  {
    for (int i = 0; i < numThreads; ++i)
    {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);
      threads_.push_back(new tmuduo::Thread(
            boost::bind(&Bench::threadFunc, this), tmuduo::string(name)));
    }
    for_each(threads_.begin(), threads_.end(), boost::bind(&tmuduo::Thread::start, _1));
  }

  void run(int times)
  {
    printf("waiting for count down latch\n");
    latch_.wait();
    printf("all threads started\n");
    for (int i = 0; i < times; ++i)
    {
      tmuduo::Timestamp now(tmuduo::Timestamp::now());
      queue_.put(now);
      usleep(1000);
    }
  }

  void joinAll()
  {
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      queue_.put(tmuduo::Timestamp::invalid());
    }

    for_each(threads_.begin(), threads_.end(), boost::bind(&tmuduo::Thread::join, _1));
  }

 private:

  void threadFunc()
  {
    printf("tid=%d, %s started\n",
           tmuduo::CurrentThread::tid(),
           tmuduo::CurrentThread::name());

    std::map<int, int> delays;
    latch_.countDown();
    bool running = true;
    while (running)
    {
      tmuduo::Timestamp t(queue_.take());
      tmuduo::Timestamp now(tmuduo::Timestamp::now());
      if (t.valid())
      {
        int delay = static_cast<int>(timeDifference(now, t) * 1000000);
        // printf("tid=%d, latency = %d us\n",
        //        tmuduo::CurrentThread::tid(), delay);
        ++delays[delay];
      }
      running = t.valid();
    }

    printf("tid=%d, %s stopped\n",
           tmuduo::CurrentThread::tid(),
           tmuduo::CurrentThread::name());
    for (std::map<int, int>::iterator it = delays.begin();
        it != delays.end(); ++it)
    {
      printf("tid = %d, delay = %d, count = %d\n",
             tmuduo::CurrentThread::tid(),
             it->first, it->second);
    }
  }

  tmuduo::BlockingQueue<tmuduo::Timestamp> queue_;
  tmuduo::CountDownLatch latch_;
  boost::ptr_vector<tmuduo::Thread> threads_;
};

int main(int argc, char* argv[])
{
  int threads = argc > 1 ? atoi(argv[1]) : 1;

  Bench t(threads);
  t.run(10000);
  t.joinAll();
}
