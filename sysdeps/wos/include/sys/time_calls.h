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
uint64_t clock_gettime(int clock_id, struct timespec *ts) {
	return syscall(
	    (long)abi::callnums::time, (uint64_t)abi::sys_time_ops::clock_gettime, (uint64_t)ts, (uint64_t)clock_id
	);
}

[[gnu::weak]]
uint64_t times(void *tms, void *out) {
	return syscall(
	    (long)abi::callnums::time, (uint64_t)abi::sys_time_ops::times, (uint64_t)tms, (uint64_t)out
	);
}

[[gnu::weak]]
uint64_t setitimer(int which, const void *new_value) {
	return syscall(
	    (long)abi::callnums::time, (uint64_t)abi::sys_time_ops::setitimer,
	    (uint64_t)(uintptr_t)which, (uint64_t)new_value
	);
}

[[gnu::weak]]
uint64_t getitimer(int which, void *curr_value) {
	return syscall(
	    (long)abi::callnums::time, (uint64_t)abi::sys_time_ops::getitimer,
	    (uint64_t)(uintptr_t)which, (uint64_t)curr_value
	);
}

} // namespace ker::time
