#pragma once
#include <callnums/process.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/callnums.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace ker::process {

constexpr uint32_t WKI_TARGET_FLAG_STRICT = 1U << 0;
constexpr uint32_t WKI_TARGET_FLAG_LOCAL = 1U << 1;     // pin task to local node
constexpr uint32_t WKI_TARGET_FLAG_NOINHERIT = 1U << 2; // don't propagate to children
constexpr uint32_t WKI_TARGET_FLAG_REMOTE = 1U << 3;    // prefer remote placement

inline void exit(uint64_t status) {
	syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::EXIT, status);
}

// NOT UNIX-STYLE: This is a WOS-specific function that combines fork+exec in one call, and does not
// return to the caller on success. It is intended for the common case of spawning a new process
// from an existing one, without
// needing the full flexibility of separate fork and exec calls. For more complex use cases, fork
// and rpexec can be used separately.
inline uint64_t exec(const char *path, const char *const argv[], const char *const envp[]) {
	return syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::EXEC,
	    (uint64_t)(uintptr_t)path,
	    (uint64_t)(uintptr_t)argv,
	    (uint64_t)(uintptr_t)envp
	);
}

inline int64_t waitpid(int64_t pid, int32_t *status, int32_t options, rusage *ru) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::WAITPID,
	    (uint64_t)pid,
	    (uint64_t)(uintptr_t)status,
	    (uint64_t)options,
	    (uint64_t)(uintptr_t)ru
	);
}

inline uint64_t getpid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::GETPID);
}

inline uint64_t getppid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::GETPPID);
}

inline int64_t fork() {
	return (int64_t)syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::FORK);
}

inline int64_t sigaction(int signum, const void *act, void *oldact) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::SIGACTION,
	    (uint64_t)signum,
	    (uint64_t)(uintptr_t)act,
	    (uint64_t)(uintptr_t)oldact
	);
}

inline int64_t sigprocmask(int how, const void *set, void *oldset) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::SIGPROCMASK,
	    (uint64_t)how,
	    (uint64_t)(uintptr_t)set,
	    (uint64_t)(uintptr_t)oldset
	);
}

inline int64_t sigpending(void *set) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::SIGPENDING,
	    (uint64_t)(uintptr_t)set
	);
}

inline int64_t sigsuspend(const void *set) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::SIGSUSPEND,
	    (uint64_t)(uintptr_t)set
	);
}

inline int64_t kill(int64_t pid, int sig) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::KILL,
	    (uint64_t)pid,
	    (uint64_t)sig
	);
}

inline int64_t sigreturn() {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::SIGRETURN
	);
}

inline uint64_t getuid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::GETUID);
}

inline uint64_t geteuid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::GETEUID);
}

inline uint64_t getgid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::GETGID);
}

inline uint64_t getegid() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::GETEGID);
}

inline int64_t getresuid(uid_t *ruid, uid_t *euid, uid_t *suid) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::GETRESUID,
	    (uint64_t)(uintptr_t)ruid,
	    (uint64_t)(uintptr_t)euid,
	    (uint64_t)(uintptr_t)suid
	);
}

inline int64_t getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::GETRESGID,
	    (uint64_t)(uintptr_t)rgid,
	    (uint64_t)(uintptr_t)egid,
	    (uint64_t)(uintptr_t)sgid
	);
}

inline int64_t getgroups(uint64_t size, gid_t *list) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::GETGROUPS,
	    size,
	    (uint64_t)(uintptr_t)list
	);
}

inline int64_t setuid(uint64_t uid) {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::SETUID, uid
	);
}

inline int64_t setgid(uint64_t gid) {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::SETGID, gid
	);
}

inline int64_t seteuid(uint64_t euid) {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::SETEUID, euid
	);
}

inline int64_t setegid(uint64_t egid) {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::SETEGID, egid
	);
}

inline int64_t setgroups(uint64_t size, const gid_t *list) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::SETGROUPS,
	    size,
	    (uint64_t)(uintptr_t)list
	);
}

inline uint64_t getumask() {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::GETUMASK);
}

inline uint64_t setumask(uint64_t mask) {
	return syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::SETUMASK, mask);
}

inline int64_t setsid() {
	return (int64_t)syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::SETSID);
}

inline int64_t getsid(int64_t pid) {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::GETSID, (uint64_t)pid
	);
}

inline int64_t setpgid(int64_t pid, int64_t pgid) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::SETPGID,
	    (uint64_t)pid,
	    (uint64_t)pgid
	);
}

inline int64_t getpgid(int64_t pid) {
	return (int64_t)syscall(
	    abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::GETPGID, (uint64_t)pid
	);
}

inline int64_t execve(const char *path, const char *const argv[], const char *const envp[]) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::EXECVE,
	    (uint64_t)(uintptr_t)path,
	    (uint64_t)(uintptr_t)argv,
	    (uint64_t)(uintptr_t)envp
	);
}

inline int64_t gethostname(char *buf, uint64_t bufsize) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::GETHOSTNAME,
	    (uint64_t)(uintptr_t)buf,
	    bufsize
	);
}

inline int64_t sethostname(const char *name, uint64_t len) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::SETHOSTNAME,
	    (uint64_t)(uintptr_t)name,
	    len
	);
}

inline int64_t setpriority(int which, int64_t who, int prio) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::SETPRIORITY,
	    (uint64_t)which,
	    (uint64_t)who,
	    (uint64_t)(int64_t)prio
	);
}

inline int64_t setwkitarget(const char *hostname, uint64_t len, uint32_t flags) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::SETWKITARGET,
	    (uint64_t)(uintptr_t)hostname,
	    len,
	    (uint64_t)flags
	);
}

inline int64_t getwkitarget(char *hostname, uint64_t hostname_size, uint32_t *flags) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::GETWKITARGET,
	    (uint64_t)(uintptr_t)hostname,
	    hostname_size,
	    (uint64_t)(uintptr_t)flags
	);
}

inline int64_t ptrace(uint64_t request, uint64_t pid, uint64_t addr, uint64_t data) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::PTRACE,
	    request,
	    pid,
	    addr,
	    data
	);
}

struct CloneVmArgs {
	uint64_t fn;
	uint64_t child_stack;
	uint64_t flags;
	uint64_t arg;
	uint64_t parent_tidptr;
	uint64_t newtls;
	uint64_t child_tidptr;
};

inline int64_t uname(struct utsname *buf) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::UNAME,
	    (uint64_t)(uintptr_t)buf
	);
}

inline int64_t clone_vm(const CloneVmArgs *args) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::CLONE_VM_PROC,
	    (uint64_t)(uintptr_t)args
	);
}

inline int64_t prctl(int option, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::PRCTL,
	    (uint64_t)option,
	    arg2,
	    arg3,
	    arg4,
	    arg5
	);
}

inline int64_t arch_prctl(int option, uint64_t arg2) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::ARCH_PRCTL,
	    (uint64_t)option,
	    arg2
	);
}

inline int64_t sigaltstack(const void *ss, void *old_ss) {
	return (int64_t)syscall(
	    abi::callnums::process,
	    (uint64_t)abi::process::procmgmt_ops::SIGALTSTACK,
	    (uint64_t)(uintptr_t)ss,
	    (uint64_t)(uintptr_t)old_ss
	);
}

inline int64_t personality(unsigned long persona) {
	return (int64_t)syscall(abi::callnums::personality, (uint64_t)persona);
}

// Read the hostname of the node that LAUNCHED this process (i.e. the submitter).
// For locally-started processes this equals the runner node.
// buf must be at least bufsize bytes; returns the number of bytes written
// (excluding NUL), or -1 on error.
inline int64_t wki_launcher_node(char *buf, uint64_t bufsize) {
	if (buf == nullptr || bufsize == 0)
		return -1;
	int fd = open("/proc/self/wki_launcher", O_RDONLY);
	if (fd < 0)
		return -1;
	ssize_t n = read(fd, buf, bufsize - 1);
	close(fd);
	if (n <= 0) {
		buf[0] = '\0';
		return n;
	}
	// strip trailing newline
	if (buf[n - 1] == '\n')
		n--;
	buf[n] = '\0';
	return n;
}

// Read the hostname of the node that is RUNNING this process.
// buf must be at least bufsize bytes; returns the number of bytes written
// (excluding NUL), or -1 on error.
inline int64_t wki_runner_node(char *buf, uint64_t bufsize) {
	if (buf == nullptr || bufsize == 0)
		return -1;
	int fd = open("/proc/self/wki_runner", O_RDONLY);
	if (fd < 0)
		return -1;
	ssize_t n = read(fd, buf, bufsize - 1);
	close(fd);
	if (n <= 0) {
		buf[0] = '\0';
		return n;
	}
	// strip trailing newline
	if (buf[n - 1] == '\n')
		n--;
	buf[n] = '\0';
	return n;
}

} // namespace ker::process
