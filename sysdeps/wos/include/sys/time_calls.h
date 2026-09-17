#pragma once
#include <sys/callnums.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/time_ops.h>

namespace ker::time {
[[gnu::weak]]
uint64_t gettimeofday(struct timeval *tv) {
	return syscall(
	    (long)abi::callnums::time, (uint64_t)abi::sys_time_ops::GETTIMEOFDAY, (uint64_t)tv
	);
}

[[gnu::weak]]
uint64_t clock_gettime(int clock_id, struct timespec *ts) {
	return syscall(
	    (long)abi::callnums::time,
	    (uint64_t)abi::sys_time_ops::CLOCK_GETTIME,
	    (uint64_t)ts,
	    (uint64_t)clock_id
	);
}

[[gnu::weak]]
uint64_t times(void *tms, void *out) {
	return syscall(
	    (long)abi::callnums::time, (uint64_t)abi::sys_time_ops::TIMES, (uint64_t)tms, (uint64_t)out
	);
}

// Preserve the original low-level overload for existing callers.
[[gnu::weak]]
uint64_t setitimer(int which, const void *new_value) {
	return syscall((long)abi::callnums::time, (uint64_t)abi::sys_time_ops::SETITIMER,
	    (uint64_t)(uintptr_t)which, (uint64_t)new_value);
}

[[gnu::weak]]
uint64_t setitimer(int which, const void *new_value, void *old_value) {
	abi::SetItimerRequest const request{
	    .which = which, .reserved = 0,
	    .new_value_address = reinterpret_cast<uint64_t>(new_value),
	    .old_value_address = reinterpret_cast<uint64_t>(old_value)
	};
	return syscall((long)abi::callnums::time, (uint64_t)abi::sys_time_ops::SETITIMER_V1,
	    reinterpret_cast<uint64_t>(&request), abi::TIME_ITIMER_VERSION);
}

[[gnu::weak]]
uint64_t getitimer(int which, void *curr_value) {
	return syscall(
	    (long)abi::callnums::time,
	    (uint64_t)abi::sys_time_ops::GETITIMER,
	    (uint64_t)(uintptr_t)which,
	    (uint64_t)curr_value
	);
}

} // namespace ker::time
