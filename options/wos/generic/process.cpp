#include <mlibc/all-sysdeps.hpp>
#include <mlibc/tcb.hpp>

#include <sys/callnums.h>
#include <sys/process.h>
#include <sys/syscall.h>

namespace mlibc {

void sys_exit(int status) {
	syscall(
	    ker::abi::callnums::process,
	    static_cast<uint64_t>(ker::abi::process::procmgmt_ops::EXIT),
	    static_cast<uint64_t>(status)
	);
	__builtin_unreachable();
}

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
	uint64_t r = syscall(
	    ker::abi::callnums::process,
	    static_cast<uint64_t>(ker::abi::process::procmgmt_ops::EXECVE),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(argv),
	    reinterpret_cast<uint64_t>(envp)
	);

	// If exec returns, it failed
	if (static_cast<int64_t>(r) < 0)
		return static_cast<int>(-static_cast<int64_t>(r));

	return 0;
}

} // namespace mlibc
