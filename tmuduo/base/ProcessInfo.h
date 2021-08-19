#ifndef TMUDUO_BASE_PROCESSINFO_H
#define TMUDUO_BASE_PROCESSINFO_H

#include <tmuduo/base/Types.h>
#include <tmuduo/base/Timestamp.h>
#include <vector>

namespace tmuduo
{

namespace ProcessInfo
{

	pid_t pid();
	string pidString();
	uid_t uid();
	string username();
	uid_t euid();
	Timestamp startTime();

	string hostname();

	// read /proc/self/status
	string procStatus();

	int openedFiles();
	int maxOpenFiles();

	int numThreads();

	std::vector<pid_t> threads();
} // end of ProcessInfo

} // end of tmuduo

#endif // TMUDUO_BASE_PROCESSINFO_H
