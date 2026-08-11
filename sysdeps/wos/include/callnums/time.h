#pragma once
#include <stdint.h>

namespace ker::abi {
enum class sys_time_ops : uint64_t {
	GETTIMEOFDAY = 0,
	CLOCK_GETTIME = 1,
	NANOSLEEP = 2,
	TIMES = 3,
	SETITIMER = 4,
	GETITIMER = 5,
	gettimeofday = GETTIMEOFDAY,
	clock_gettime = CLOCK_GETTIME,
	nanosleep = NANOSLEEP,
	times = TIMES,
	setitimer = SETITIMER,
	getitimer = GETITIMER,
};
} // namespace ker::abi
