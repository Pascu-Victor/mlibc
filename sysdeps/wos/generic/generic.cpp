#include <errno.h>
#include <limits.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/tcb.hpp>
#include <stdlib.h>
#include <string.h>
#include <sys/callnums.h>
#include <sys/logging.h>
#include <sys/multiproc.h>
#include <sys/process.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/time_calls.h>
#include <sys/time_ops.h>
#include <sys/types.h>

// SafeStack support: This variable is accessed by the compiler-generated code
// It needs to be in TLS storage and properly initialized
// The actual initialization will be done by the kernel when setting up TLS
__thread void *__safestack_unsafe_stack_ptr = nullptr;

namespace mlibc {

void sys_libc_log(const char *message) {
	ker::logging::log(message, strlen(message), ker::abi::sys_log::sys_log_device::vga);
}

void sys_libc_panic() {
	sys_libc_log("\nMLIBC PANIC\n");
	sys_exit(1);
	__builtin_unreachable();
}

int sys_futex_tid() {
	uint64_t tid = ker::multiproc::currentThreadId();
	return tid;
}

void sys_exit(int status) {
	ker::process::exit(status);
	__builtin_unreachable();
}

int sys_futex_wake(int *) {
	// no futex support for now just panic
	sys_libc_log("sys_futex_wake not supported");
	sys_libc_panic();
}
int sys_futex_wait(int *, int, timespec const *) {
	// no futex support for now just panic
	sys_libc_log("sys_futex_wait not supported");
	sys_libc_panic();
}
int sys_open(char const *, int, unsigned int, int *) {
	// no open support for now just panic
	sys_libc_log("sys_open not supported");
	sys_libc_panic();
}

// Set FS_BASE to pointer
// TODO: will need checks for userspace and process bounds
int sys_tcb_set(void *tcb) {
	if (!tcb)
		return -1;
	return ker::multiproc::setTCB(tcb);
}

int sys_close(int) {
	// no close support for now just panic
	sys_libc_log("sys_close not supported");
	sys_libc_panic();
}
int sys_clock_get(int clock_id, long *seconds, long *nanoseconds) {
	(void)clock_id;
	struct timespec ts;
	uint64_t res = ker::time::clock_gettime(&ts);
	if ((int64_t)res < 0) {
		return (int)(-(int64_t)res);
	}
	if (seconds)
		*seconds = ts.tv_sec;
	if (nanoseconds)
		*nanoseconds = ts.tv_nsec;
	return 0;
}

int sys_gettimeofday(struct timeval *tv, void *tz) {
	(void)tz;
	if (!tv)
		return EINVAL;
	struct timeval tmp;
	uint64_t res = ker::time::gettimeofday(&tmp);
	if ((int64_t)res < 0) {
		return (int)(-(int64_t)res);
	}
	tv->tv_sec = tmp.tv_sec;
	tv->tv_usec = tmp.tv_usec;
	return 0;
}
int sys_anon_free(void *, unsigned long) {
	// no anon free support for now just panic
	sys_libc_log("sys_anon_free not supported");
	sys_libc_panic();
}
int sys_seek(int, long, int, long *) {
	// no seek support for now just panic
	sys_libc_log("sys_seek not supported");
	sys_libc_panic();
}
int sys_read(int, void *, unsigned long, long *) {
	// no read support for now just panic
	sys_libc_log("sys_read not supported");
	sys_libc_panic();
}
int sys_vm_map(void *, unsigned long, int, int, int, long, void **) {
	// no vm map support for now just panic
	sys_libc_log("sys_vm_map not supported");
	sys_libc_panic();
}
int sys_write(int, void const *, unsigned long, long *) {
	// no write support for now just panic
	sys_libc_log("sys_write not supported");
	sys_libc_panic();
}

#ifndef MLIBC_BUILDING_RTLD

[[noreturn]] void sys_thread_exit() {
	for (;;)
		;
	__builtin_unreachable();
}

#endif

int sys_anon_allocate(size_t size, void **pointer) {
	// create syscall for virtual memory allocation
	*pointer = malloc(size); // Temporary implementation
	return (*pointer) ? 0 : ENOMEM;
}

int sys_prepare_stack(
    void **stack,
    void *entry,
    void *arg,
    void *tcb,
    size_t *stack_size,
    size_t *guard_size,
    void **stack_base
) {
	// For now, use a simple stack preparation
	// This should be implemented properly for threading support
	*guard_size = 0x1000; // 4KB guard
	if (!*stack_size)
		*stack_size = 0x200000; // 2MB default

	*stack_base = malloc(*stack_size + *guard_size);
	if (!*stack_base)
		return ENOMEM;

	*stack = (void *)((char *)*stack_base + *stack_size);

	// Set up the stack for thread entry
	void **stack_it = (void **)*stack;
	*--stack_it = arg;
	*--stack_it = tcb;
	*--stack_it = entry;
	*stack = (void *)stack_it;

	return 0;
}

int sys_clone(void *tcb, pid_t *tid_out, void *stack) {
	// Basic thread creation - this needs to be implemented properly
	// For now just return an error since threading isn't fully implemented
	sys_libc_log("sys_clone not yet implemented");
	return ENOSYS;
}

} // namespace mlibc
