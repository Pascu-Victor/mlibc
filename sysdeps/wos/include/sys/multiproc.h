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
uint64_t getCurrentCpu() {
	return syscall(abi::callnums::threading, (uint64_t)abi::multiproc::threadInfoOps::currentCpu);
}
[[gnu::weak]]
uint64_t setTCB(void *ptr) {
	return syscall(
	    abi::callnums::threading, (uint64_t)abi::multiproc::threadControlOps::setTCB, (uint64_t)ptr
	);
}
[[gnu::weak]]
uint64_t yield() {
	return syscall(abi::callnums::threading, (uint64_t)abi::multiproc::threadControlOps::yield);
}

[[gnu::weak]]
int64_t setThreadAffinityMask(uint64_t tid, uint64_t mask) {
	return (int64_t)syscall(
	    abi::callnums::threading, (uint64_t)abi::multiproc::threadControlOps::setAffinity, tid, mask
	);
}

[[gnu::weak]]
int64_t getThreadAffinityMask(uint64_t tid) {
	return (int64_t)syscall(
	    abi::callnums::threading, (uint64_t)abi::multiproc::threadControlOps::getAffinity, tid
	);
}

// Create a new userspace thread sharing the current process's address space.
// tcb:           virtual address of the mlibc Tcb* (becomes FS base)
// stack:         prepared stack pointer (entry + user_arg pushed below it)
// enter_thread:  virtual address of __mlibc_enter_thread in the process image
// Returns the new thread's TID on success, or a negative errno on failure.
[[gnu::weak]]
int64_t threadCreate(void *tcb, void *stack, void *enter_thread) {
	return (int64_t)syscall(
	    abi::callnums::threading,
	    (uint64_t)abi::multiproc::threadControlOps::threadCreate,
	    (uint64_t)tcb,
	    (uint64_t)stack,
	    (uint64_t)enter_thread
	);
}

// Exit the current thread without tearing down the process.
[[gnu::weak, gnu::noreturn]]
void threadExit() {
	syscall(abi::callnums::threading, (uint64_t)abi::multiproc::threadControlOps::threadExit);
	__builtin_unreachable();
}

} // namespace ker::multiproc
