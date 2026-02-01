#pragma once
#include <callnums/futex.h>
#include <sys/callnums.h>
#include <sys/syscall.h>
#include <time.h>

namespace ker::futex {

[[gnu::weak]] inline int64_t wait(int *addr, int expected, const timespec *timeout) {
	return static_cast<int64_t>(syscall(
	    abi::callnums::futex,
	    static_cast<uint64_t>(abi::futex::futex_ops::futex_wait),
	    reinterpret_cast<uint64_t>(addr),
	    static_cast<uint64_t>(expected),
	    reinterpret_cast<uint64_t>(timeout)
	));
}

[[gnu::weak]] inline int64_t wake(int *addr) {
	return static_cast<int64_t>(syscall(
	    abi::callnums::futex,
	    static_cast<uint64_t>(abi::futex::futex_ops::futex_wake),
	    reinterpret_cast<uint64_t>(addr)
	));
}

} // namespace ker::futex
