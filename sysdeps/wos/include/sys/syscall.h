#pragma once
#include <sys/callnums.h>

static uint64_t syscall(
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

// compatibility with other codebases
template <
    typename T1 = uint64_t,
    typename T2 = uint64_t,
    typename T3 = uint64_t,
    typename T4 = uint64_t,
    typename T5 = uint64_t,
    typename T6 = uint64_t>
static uint64_t
syscall(long callnum, T1 a1 = 0, T2 a2 = 0, T3 a3 = 0, T4 a4 = 0, T5 a5 = 0, T6 a6 = 0) {
	// callnum -> r10
	// a1 -> RDI
	// a2 -> RSI
	// a3 -> RDX
	// a4 -> RCX
	// a5 -> R8
	// a6 -> R9
	uint64_t v1 = (uint64_t)(a1);
	uint64_t v2 = (uint64_t)(a2);
	uint64_t v3 = (uint64_t)(a3);
	uint64_t v4 = (uint64_t)(a4);
	uint64_t v5 = (uint64_t)(a5);
	uint64_t v6 = (uint64_t)(a6);
	uint64_t retVal;
	asm volatile("syscall"
	             : "=a"(retVal)
	             : "a"(callnum), "D"(v1), "S"(v2), "d"(v3), "c"(v4), "r"(v5), "r"(v6)
	             : "memory");
	return retVal;
}
