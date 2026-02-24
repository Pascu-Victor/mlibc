#pragma once
#include <callnums/process.h>
#include <stdint.h>
#include <sys/callnums.h>
#include <sys/syscall.h>

namespace ker::process {

inline void exit(uint64_t status) {
	syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::exit, status);
}

// NOT UNIX-STYLE: This is a WOS-specific function that combines fork+exec in one call, and does not
// return to the caller on success. It is intended for the common case of spawning a new process
// from an existing one, without
// needing the full flexibility of separate fork and exec calls. For more complex use cases, fork
// and rpexec can be used separately.
inline uint64_t exec(const char *path, const char *const argv[], const char *const envp[]) {
	return syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::exec,
	    (uint64_t)(uintptr_t)path,
	    (uint64_t)(uintptr_t)argv,
	    (uint64_t)(uintptr_t)envp
	);
}

inline uint64_t waitpid(int64_t pid, int32_t *status, int32_t options) {
	return syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::waitpid,
	    (uint64_t)pid,
	    (uint64_t)(uintptr_t)status,
	    (uint64_t)options
	);
}

inline uint64_t getpid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::getpid);
}

inline uint64_t getppid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::getppid);
}

inline int64_t fork() {
	return (int64_t)syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::fork);
}

inline int64_t sigaction(int signum, const void *act, void *oldact) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::sigaction,
	    (uint64_t)signum,
	    (uint64_t)(uintptr_t)act,
	    (uint64_t)(uintptr_t)oldact
	);
}

inline int64_t sigprocmask(int how, const void *set, void *oldset) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::sigprocmask,
	    (uint64_t)how,
	    (uint64_t)(uintptr_t)set,
	    (uint64_t)(uintptr_t)oldset
	);
}

inline int64_t kill(int64_t pid, int sig) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::kill,
	    (uint64_t)pid,
	    (uint64_t)sig
	);
}

inline int64_t sigreturn() {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::sigreturn
	);
}

inline uint64_t getuid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::getuid);
}

inline uint64_t geteuid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::geteuid);
}

inline uint64_t getgid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::getgid);
}

inline uint64_t getegid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::getegid);
}

inline int64_t setuid(uint64_t uid) {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::setuid, uid
	);
}

inline int64_t setgid(uint64_t gid) {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::setgid, gid
	);
}

inline int64_t seteuid(uint64_t euid) {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::seteuid, euid
	);
}

inline int64_t setegid(uint64_t egid) {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::setegid, egid
	);
}

inline uint64_t getumask() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::getumask);
}

inline uint64_t setumask(uint64_t mask) {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::setumask, mask);
}

} // namespace ker::process
