#pragma once
#include <callnums/process.h>
#include <stdint.h>
#include <sys/callnums.h>
#include <sys/syscall.h>

namespace ker::process {

inline void exit(uint64_t status) {
	syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::exit, status);
}

inline uint64_t exec(const char *path, const char *const argv[], const char *const envp[]) {
	return syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::exec,
	    (uint64_t)(uintptr_t)path,
	    (uint64_t)(uintptr_t)argv,
	    (uint64_t)(uintptr_t)envp
	);
}

} // namespace ker::process
