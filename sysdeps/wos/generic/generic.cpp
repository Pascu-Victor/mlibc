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
