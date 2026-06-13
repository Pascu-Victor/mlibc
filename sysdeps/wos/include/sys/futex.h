#pragma once
#include <callnums/futex.h>
#include <limits.h>
#include <sys/callnums.h>
#include <sys/syscall.h>
#include <time.h>

#ifdef __cplusplus
namespace ker::futex {

[[gnu::weak]] inline int64_t wait(int *addr, int expected, const timespec *timeout) {
	return static_cast<int64_t>(syscall(
	    abi::callnums::futex,
	    static_cast<uint64_t>(abi::futex::futex_ops::FUTEX_WAIT),
	    reinterpret_cast<uint64_t>(addr),
	    static_cast<uint64_t>(expected),
	    reinterpret_cast<uint64_t>(timeout)
	));
}

[[gnu::weak]] inline int64_t wake(int *addr, int count = 1) {
	return static_cast<int64_t>(syscall(
	    abi::callnums::futex,
	    static_cast<uint64_t>(abi::futex::futex_ops::FUTEX_WAKE),
	    reinterpret_cast<uint64_t>(addr),
	    static_cast<uint64_t>(count)
	));
}

[[gnu::weak]] inline int64_t wake_all(int *addr) { return wake(addr, INT_MAX); }

} // namespace ker::futex
#endif
