#include "Time.h"

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#elif defined POSIX
	#include <sys/time.h>
#endif

using namespace Utilities;

DateTime::DateTime(uint64 milliseconds) {
	this->milliseconds = milliseconds;
}

DateTime DateTime::Now() {
#ifdef WINDOWS
	FILETIME time;
	uint64 result;

	GetSystemTimeAsFileTime(&time);

	result = time.dwHighDateTime;
	result <<= 32;
	result += time.dwLowDateTime;
	result /= 10000; // to shift from 100ns intervals to 1ms intervals
	result -= 11644473600000; // to shift from epoch of 1/1/1601 to 1/1/1970

	return DateTime(result);
#elif defined POSIX
	int64 result;
	struct timeval tv;

	gettimeofday(&tv, nullptr);

	result = tv.tv_sec;
	result *= 1000000; // convert seconds to microseconds
	result += tv.tv_usec; // add microseconds
	result /= 1000; // convert to milliseconds
	return DateTime(result);
#endif
}
