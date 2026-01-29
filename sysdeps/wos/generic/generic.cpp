#include "mlibc/ansi-sysdeps.hpp"
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
#include <sys/vfs.h>
#include <sys/vmem.h>

// SafeStack support: This variable is accessed by the compiler-generated code
// It needs to be in TLS storage and properly initialized
// The actual initialization will be done by the kernel when setting up TLS

namespace mlibc {

void sys_libc_log(const char *message) {
	ker::logging::log(message, strlen(message), ker::abi::sys_log::sys_log_device::serial);
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

int sys_open_dir(const char *path, int *fd) { return sys_open(path, O_DIRECTORY, 0, fd); }

int sys_read_entries(int handle, void *buffer, size_t max_size, size_t *bytes_read) {
	ssize_t result = ::ker::abi::vfs::read_dir_entries(handle, buffer, max_size);
	if (result < 0) {
		// Return positive errno on error
		return -result;
	}
	*bytes_read = result;
	return 0;
}

// Set FS_BASE to pointer
// TODO: will need checks for userspace and process bounds
int sys_tcb_set(void *tcb) {
	if (!tcb)
		return -1;
	return ker::multiproc::setTCB(tcb);
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
int sys_anon_free(void *addr, unsigned long size) {
	// Use the vmem syscall to free virtual memory
	int64_t result = ker::vmem::free(addr, size);

	if (result < 0) {
		// Convert kernel error code to positive errno
		return (int)(-result);
	}

	return 0; // Success
}
int sys_vm_unmap(void *addr, size_t size) {
	int result = (int)ker::vmem::free(addr, size);
	if (result < 0) {
		// Convert kernel error code to positive errno
		return (-result);
	}
	return 0; // Success
}

int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd, long offset, void **addr) {
	int64_t result = ker::vmem::map(addr, size, prot, flags, fd, offset, hint);
	if (result < 0) {
		// Convert kernel error code to positive errno
		return (int)(-result);
	}
	return 0; // Success
}
int sys_isatty(int fd) {
	bool is_tty = ker::abi::vfs::isatty(fd);
	return is_tty ? 0 : ENOTTY;
}

int sys_waitpid(pid_t pid, int *status, int options, pid_t *ret_pid) {
	int64_t result = ker::process::waitpid(pid, status, options);
	if (result < 0) {
		// Convert kernel error code to positive errno
		return (int)(-result);
	}
	if (ret_pid)
		*ret_pid = (pid_t)result;
	return 0; // Success
}

int sys_getpid(pid_t *pid) {
	uint64_t result = ker::process::getpid();
	if (result == 0) {
		return EINVAL; // Invalid PID
	}
	*pid = (pid_t)result;
	return 0; // Success
}

#ifndef MLIBC_BUILDING_RTLD

[[noreturn]] void sys_thread_exit() {
	for (;;)
		;
	__builtin_unreachable();
}

#endif

int sys_anon_allocate(size_t size, void **pointer) {
	// Use the vmem syscall for proper virtual memory allocation
	int64_t result = ker::vmem::allocate(
	    pointer,
	    size,
	    PROT_READ | PROT_WRITE,      // Read-write by default
	    MAP_PRIVATE | MAP_ANONYMOUS, // Private anonymous mapping
	    nullptr                      // No hint, let kernel choose address
	);

	if (result < 0) {
		// Convert kernel error code to positive errno
		return (int)(-result);
	}

	return 0; // Success
}

#ifndef MLIBC_BUILDING_RTLD

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

#endif

} // namespace mlibc

__attribute__((visibility("default"))) extern "C" void frg_panic(const char *mstr) {
	//	mlibc::sys_libc_log("mlibc: Call to frg_panic");
	mlibc::sys_libc_log(mstr);
	mlibc::sys_libc_panic();
}
