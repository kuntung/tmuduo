#include <tmuduo/base/AsyncLogging.h>
#include <tmuduo/base/Logging.h>
#include <tmuduo/base/Timestamp.h>

#include <stdio.h>
#include <sys/resource.h>

int kRollSize = 500*1000*1000; // 日志滚动大小实现

tmuduo::AsyncLogging* g_asynclog = NULL;

void asyncOutput(const char* msg, int len)
{
	g_asynclog->append(msg, len);
}

void bench(bool longLog)
{
	tmuduo::Logger::setOutput(asyncOutput); // 前端线程通过asyncOutput向日志线程写入
	int cnt = 0;
	const int kBatch = 1000;

	tmuduo::string empty = " ";
	tmuduo::string longStr(3000, 'X');
	longStr += " ";

	for (int t = 0; t < 30; ++t)
	{
		tmuduo::Timestamp start = tmuduo::Timestamp::now();

		for (int i = 0; i < kBatch; ++i)
		{
			LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz "
					 << (longLog ? longStr : empty)
					 << cnt;
			++cnt;
		}

	tmuduo::Timestamp end = tmuduo::Timestamp::now();
	printf("%f\n", timeDifference(end, start)*1000000/kBatch);

	// 减少消息堆积
	struct timespec ts = {0, kRollSize };
	nanosleep(&ts, NULL);
	}
}

int main(int argc, char* argv[])
{
	size_t kOneGB = 1000*1024*1024;
	rlimit rl = {2*kOneGB, 2*kOneGB };
	setrlimit(RLIMIT_AS, &rl);
	
	printf("pid = %d\n", getpid());

	char name[256];
	strncpy(name, argv[0], 256);
	tmuduo::AsyncLogging log(::basename(name), kRollSize);
	log.start();
	g_asynclog = &log;

	bool longLog = argc > 1;
	bench(longLog);

	return 0;
}
