#pragma once
#include <callnums/multiproc.h>
#include <sys/callnums.h>
#include <sys/multiproc.h>
#include <sys/syscall.h>

namespace ker::multiproc {
[[gnu::weak]]
uint64_t currentThreadId() {
	return syscall(
	    abi::callnums::threading, (uint64_t)abi::multiproc::threadInfoOps::currentThreadId
	);
}
[[gnu::weak]]
uint64_t nativeThreadCount() {
	return syscall(
	    abi::callnums::threading, (uint64_t)abi::multiproc::threadInfoOps::nativeThreadCount
	);
}
[[gnu::weak]]
uint64_t setTCB(void *ptr) {
	return syscall(
	    abi::callnums::threading, (uint64_t)abi::multiproc::threadControlOps::setTCB, (uint64_t)ptr
	);
} // namespace ker::multiproc

} // namespace ker::multiproc
