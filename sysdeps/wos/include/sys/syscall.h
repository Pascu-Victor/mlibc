#pragma once
#include <sys/callnums.h>

uint64_t syscall(
    ker::abi::callnums callnum,
    uint64_t a1 = 0,
    uint64_t a2 = 0,
    uint64_t a3 = 0,
    uint64_t a4 = 0,
    uint64_t a5 = 0,
    uint64_t a6 = 0
) {
	// callnum -> r10
	// a1 -> RDI
	// a2 -> RSI
	// a3 -> RDX
	// a4 -> RCX
	// a5 -> R8
	// a6 -> R9
	uint64_t retVal;
	asm volatile("syscall"
	             : "=a"(retVal)
	             : "a"(callnum), "D"(a1), "S"(a2), "d"(a3), "c"(a4), "r"(a5), "r"(a6)
	             : "memory");
	return retVal;
}
