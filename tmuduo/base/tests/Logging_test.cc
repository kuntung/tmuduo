#include <tmuduo/base/Logging.h>
#include <tmuduo/base/LogFile.h>
#include <tmuduo/base/ThreadPool.h>

#include <stdio.h> // setbuffer

int g_total;
FILE* g_file;
boost::scoped_ptr<tmuduo::LogFile> g_logFile;

void dummyOutput(const char* msg, int len)
{
	g_total += len;
	if (g_file)
	{
		fwrite(msg, 1, len, g_file);
	}
	else if (g_logFile)
	{
		g_logFile->append(msg, len);
	}
}

void bench(const char* type)
{
	tmuduo::Logger::setOutput(dummyOutput);
	tmuduo::Timestamp start(tmuduo::Timestamp::now());

	g_total = 0;

	int n = 1000 * 1000;
	const bool kLongLog = false;
	tmuduo::string empty = " ";
	tmuduo::string logStr(3000, 'X');
	logStr += " ";

	for (int i = 0; i < n; ++i)
	{
		LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
				 << (kLongLog ? logStr : empty)
				 << i;
	}

	tmuduo::Timestamp end(tmuduo::Timestamp::now());
	double seconds = timeDifference(end, start);
	printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
		   type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

void logInThread()
{
	LOG_INFO << "logInThread";
	usleep(1000);
}

int main(void)
{
	getppid();

	tmuduo::ThreadPool pool("pool"); // 开启一个线程池
	pool.start(5);
	pool.run(logInThread);
	pool.run(logInThread);
	pool.run(logInThread);
	pool.run(logInThread);
	pool.run(logInThread);


	LOG_TRACE << "trace";
	LOG_DEBUG << "debug";
	LOG_INFO << "Hello";
	LOG_WARN << "World";
	LOG_ERROR << "Error";
	LOG_INFO << sizeof(tmuduo::Logger);
	LOG_INFO << sizeof(tmuduo::LogStream);
	LOG_INFO << sizeof(tmuduo::Fmt);
	LOG_INFO << sizeof(tmuduo::LogStream::Buffer);

	sleep(1);
	bench("nop");

	char buffer[64*1024];

	g_file = fopen("/dev/null", "w");
	setbuffer(g_file, buffer, sizeof buffer);
	bench("/dev/null");
	fclose(g_file);

	g_file = fopen("/tmp/log", "w");
	setbuffer(g_file, buffer, sizeof buffer);
	bench("/tmp/log");
	fclose(g_file);

	g_file = NULL;
	g_logFile.reset(new tmuduo::LogFile("test_log_st", 500*1000*1000, false));
	bench("test_log_st");

	g_logFile.reset(new tmuduo::LogFile("test_log_mt", 500*1000*1000, true));
	bench("test_log_mt");
	g_logFile.reset();
	return 0;
}
