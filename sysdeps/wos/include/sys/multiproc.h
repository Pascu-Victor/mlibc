#pragma once
#include <callnums.hpp>
#include <callnums/multiproc.h>
#include <syscall.hpp>

namespace ker::multiproc {
uint64_t currentThreadId() {
	return ker::abi::syscall(
	    callnums::threadInfo, (uint64_t)abi::multiproc::threadInfoOps::currentThreadId
	);
}
uint64_t nativeThreadCount() {
	return ker::abi::syscall(
	    callnums::threadInfo, (uint64_t)abi::multiproc::threadInfoOps::nativeThreadCount
	);
}
} // namespace ker::multiproc
