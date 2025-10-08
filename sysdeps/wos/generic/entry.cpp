#include <bits/ensure.h>
#include <mlibc/elf/startup.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/auxv.h>

extern "C" void __dlapi_enter(uintptr_t *);

extern char **environ;

size_t __hwcap;

extern "C" void
__mlibc_entry(uintptr_t *entry_stack, int (*main_fn)(int argc, char *argv[], char *env[])) {
	// TODO: support for dynamic linker
	// __dlapi_enter(entry_stack);
	// TODO: syscall for hardware capabilities for now just set to 0
	__hwcap = 0;
	// __hwcap = getauxval(AT_HWCAP);
	auto result = main_fn(mlibc::entry_stack.argc, mlibc::entry_stack.argv, environ);
	exit(result);
}
