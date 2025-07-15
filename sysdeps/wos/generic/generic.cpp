#include <errno.h>
#include <limits.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>
#include <stdlib.h>
#include <sys/callnums.h>
#include <sys/process.h>
#include <sys/syscall.h>
#include <sys/syslog.h>

namespace mlibc {

void sys_libc_log(const char *message) {
	ker::logging::log(message, strlen(message), ker::abi::sys_log::sys_log_device::vga);
}

void sys_libc_panic() {
	sys_libc_log("\nMLIBC PANIC\n");
	sys_exit(1);
	__builtin_unreachable();
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
int sys_tcb_set(void *) {
	// no TCB support for now just panic
	sys_libc_log("sys_tcb_set not supported");
	sys_libc_panic();
}
int sys_close(int) {
	// no close support for now just panic
	sys_libc_log("sys_close not supported");
	sys_libc_panic();
}
int sys_clock_get(int, long *, long *) {
	// no clock support for now just panic
	sys_libc_log("sys_clock_get not supported");
	sys_libc_panic();
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
}

} // namespace mlibc
