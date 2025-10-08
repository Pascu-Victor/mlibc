#pragma once
#include <sys/callnums.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/time_ops.h>

namespace ker::time {
[[gnu::weak]]
uint64_t gettimeofday(struct timeval *tv) {
	return syscall(
	    (long)abi::callnums::time, (uint64_t)abi::sys_time_ops::gettimeofday, (uint64_t)tv
	);
}

[[gnu::weak]]
uint64_t clock_gettime(struct timespec *ts) {
	return syscall(
	    (long)abi::callnums::time, (uint64_t)abi::sys_time_ops::clock_gettime, (uint64_t)ts
	);
}

} // namespace ker::time
