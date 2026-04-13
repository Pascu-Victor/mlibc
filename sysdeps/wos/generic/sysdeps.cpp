#include <abi-bits/pid_t.h>
#include <algorithm>
#include <errno.h>
#include <limits.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/fsfd_target.hpp>
#include <mlibc/tcb.hpp>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/callnums.h>
#include <sys/epoll.h>
#include <sys/futex.h>
#include <sys/logging.h>
#include <sys/multiproc.h>
#include <sys/net.h>
#include <sys/poll.h>
#include <sys/process.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/time_calls.h>
#include <sys/time_ops.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/vmem.h>
#include <termios.h>

// SafeStack support: This variable is accessed by the compiler-generated code
// It needs to be in TLS storage and properly initialized
// The actual initialization will be done by the kernel when setting up TLS
extern "C" __attribute__((visibility("default"))) __thread void *__safestack_unsafe_stack_ptr =
    nullptr;

namespace [[gnu::visibility("hidden")]] mlibc {

bool cpu_bit_is_set(const cpu_set_t *set, size_t cpusetsize, size_t cpu) {
	size_t byte_index = cpu / CHAR_BIT;
	if (byte_index >= cpusetsize) {
		return false;
	}
	auto bytes = reinterpret_cast<const unsigned char *>(set);
	return (bytes[byte_index] & (1U << (cpu % CHAR_BIT))) != 0;
}

void cpu_bit_set(cpu_set_t *set, size_t cpusetsize, size_t cpu) {
	size_t byte_index = cpu / CHAR_BIT;
	if (byte_index >= cpusetsize) {
		return;
	}
	auto bytes = reinterpret_cast<unsigned char *>(set);
	bytes[byte_index] |= static_cast<unsigned char>(1U << (cpu % CHAR_BIT));
}

int cpuset_to_mask(size_t cpusetsize, const cpu_set_t *set, uint64_t *mask) {
	uint64_t affinity_mask = 0;
	size_t max_bits = cpusetsize * CHAR_BIT;
	for (size_t cpu = 0; cpu < max_bits; ++cpu) {
		if (!cpu_bit_is_set(set, cpusetsize, cpu)) {
			continue;
		}
		if (cpu >= 64) {
			return ENOTSUP;
		}
		affinity_mask |= 1ULL << cpu;
	}
	*mask = affinity_mask;
	return 0;
}

int mask_to_cpuset(uint64_t mask, size_t cpusetsize, cpu_set_t *set) {
	memset(set, 0, cpusetsize);
	for (size_t cpu = 0; cpu < 64; ++cpu) {
		if ((mask & (1ULL << cpu)) == 0) {
			continue;
		}
		if (cpu >= cpusetsize * CHAR_BIT) {
			return ERANGE;
		}
		cpu_bit_set(set, cpusetsize, cpu);
	}
	return 0;
}

// ---- Base sysdeps (always required) ----

void Sysdeps<LibcLog>::operator()(const char *message) {
	ker::logging::logLine(message, strlen(message), ker::abi::sys_log::sys_log_device::serial);
}

[[noreturn]]
void Sysdeps<LibcPanic>::operator()() {
	sysdep<LibcLog>("\nMLIBC PANIC\n");
	sysdep<Exit>(1);
	__builtin_unreachable();
}

pid_t Sysdeps<FutexTid>::operator()() {
	uint64_t tid = ker::multiproc::currentThreadId();
	return tid;
}

int Sysdeps<FutexWake>::operator()(int *pointer, bool) {
	int64_t result = ker::futex::wake(pointer);
	if (result < 0) {
		return static_cast<int>(-result);
	}
	return 0;
}

int Sysdeps<FutexWait>::operator()(int *pointer, int expected, timespec const *timeout) {
	static constexpr int WOS_ERESTARTSYS = 512;
	int64_t result = ker::futex::wait(pointer, expected, timeout);
	if (result < 0) {
		int e = static_cast<int>(-result);
		if (e == WOS_ERESTARTSYS)
			return EAGAIN;
		return e;
	}
	return 0;
}

int Sysdeps<AnonAllocate>::operator()(size_t size, void **pointer) {
	return sysdep<VmMap>(
	    nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0, pointer
	);
}

int Sysdeps<AnonFree>::operator()(void *pointer, size_t size) {
	return sysdep<VmUnmap>(pointer, size);
}

int Sysdeps<VmMap>::operator()(
    void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window
) {
	int64_t result = ker::vmem::map(window, size, prot, flags, fd, offset, hint);
	if (result < 0) {
		return (int)(-result);
	}
	return 0;
}

int Sysdeps<VmUnmap>::operator()(void *pointer, size_t size) {
	int result = (int)ker::vmem::free(pointer, size);
	if (result < 0) {
		return (-result);
	}
	return 0;
}

int Sysdeps<VmProtect>::operator()(void *pointer, size_t size, int prot) {
	int64_t result = ker::vmem::protect(pointer, size, prot);
	if (result < 0) {
		return (int)(-result);
	}
	return 0;
}

int Sysdeps<Stat>::operator()(
    fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf
) {
	int r = 0;
	// AT_SYMLINK_NOFOLLOW = 0x100 — when set, do NOT follow symlinks (lstat)
	bool follow_symlinks = !(flags & 0x100);
	switch (fsfdt) {
		case fsfd_target::path:
			if (follow_symlinks) {
				// stat() — follow symlinks by resolving the path first
				char resolved[512];
				size_t plen = 0;
				while (path[plen] != '\0' && plen < 511) {
					resolved[plen] = path[plen];
					plen++;
				}
				resolved[plen] = '\0';
				// Try to resolve symlinks via readlink loop
				for (int depth = 0; depth < 8; depth++) {
					char target[512];
					ssize_t lr = ker::abi::vfs::readlink(resolved, target, 511);
					if (lr <= 0)
						break; // Not a symlink or error
					target[lr] = '\0';
					if (target[0] == '/') {
						// Absolute target
						for (size_t i = 0; i <= (size_t)lr; i++)
							resolved[i] = target[i];
					} else {
						// Relative target: replace last component
						size_t last_slash = 0;
						bool found = false;
						for (size_t i = 0; resolved[i]; i++) {
							if (resolved[i] == '/') {
								last_slash = i;
								found = true;
							}
						}
						size_t prefix = found ? last_slash + 1 : 0;
						for (size_t i = 0; i <= (size_t)lr; i++)
							resolved[prefix + i] = target[i];
					}
				}
				r = ker::abi::vfs::stat_path(resolved, statbuf);
			} else {
				// lstat() — do not follow symlinks
				r = ker::abi::vfs::stat_path(path, statbuf);
			}
			break;
		case fsfd_target::fd:
			r = ker::abi::vfs::fstat_fd(fd, statbuf);
			break;
		case fsfd_target::fd_path:
			if (follow_symlinks) {
				char resolved[512];
				size_t plen = 0;
				while (path[plen] != '\0' && plen < 511) {
					resolved[plen] = path[plen];
					plen++;
				}
				resolved[plen] = '\0';
				for (int depth = 0; depth < 8; depth++) {
					char target[512];
					ssize_t lr = ker::abi::vfs::readlink(resolved, target, 511);
					if (lr <= 0)
						break;
					target[lr] = '\0';
					if (target[0] == '/') {
						for (size_t i = 0; i <= (size_t)lr; i++)
							resolved[i] = target[i];
					} else {
						size_t last_slash = 0;
						bool found = false;
						for (size_t i = 0; resolved[i]; i++) {
							if (resolved[i] == '/') {
								last_slash = i;
								found = true;
							}
						}
						size_t prefix = found ? last_slash + 1 : 0;
						for (size_t i = 0; i <= (size_t)lr; i++)
							resolved[prefix + i] = target[i];
					}
				}
				r = ker::abi::vfs::stat_path(resolved, statbuf);
			} else {
				r = ker::abi::vfs::stat_path(path, statbuf);
			}
			break;
		default:
			return EINVAL;
	}
	if (r < 0) {
		return -r;
	}
	return 0;
}

int Sysdeps<TcbSet>::operator()(void *tcb) {
	if (!tcb)
		return -1;
	return ker::multiproc::setTCB(tcb);
}

[[noreturn]]
void Sysdeps<Exit>::operator()(int status) {
	ker::process::exit(status);
	__builtin_unreachable();
}

int Sysdeps<Open>::operator()(const char *pathname, int flags, mode_t mode, int *fd) {
	int r = ker::abi::vfs::open(pathname, flags, mode);
	if (r < 0)
		return -r;
	*fd = r;
	return 0;
}

int Sysdeps<OpenDir>::operator()(const char *path, int *fd) {
	return sysdep<Open>(path, O_DIRECTORY, 0, fd);
}

int
Sysdeps<ReadEntries>::operator()(int handle, void *buffer, size_t max_size, size_t *bytes_read) {
	ssize_t result = ::ker::abi::vfs::read_dir_entries(handle, buffer, max_size);
	if (result < 0) {
		return -result;
	}
	*bytes_read = result;
	return 0;
}

int Sysdeps<Read>::operator()(int fd, void *buf, size_t count, ssize_t *bytes_read) {
	static constexpr ssize_t WOS_ERESTARTSYS = 512;
	for (;;) {
		ssize_t r = ker::abi::vfs::read(fd, buf, count);
		if (r == -WOS_ERESTARTSYS)
			continue;
		if (r < 0)
			return (int)(-r);
		if (bytes_read)
			*bytes_read = r;
		return 0;
	}
}

int Sysdeps<Write>::operator()(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
	static constexpr ssize_t WOS_ERESTARTSYS = 512;
	for (;;) {
		ssize_t r = ker::abi::vfs::write(fd, buf, count);
		if (r == -WOS_ERESTARTSYS)
			continue;
		if (r < 0)
			return (int)(-r);
		if (bytes_written)
			*bytes_written = r;
		return 0;
	}
}

int
Sysdeps<Writev>::operator()(int fd, const struct iovec *iovs, int iovc, ssize_t *bytes_written) {
	ssize_t total = 0;
	static constexpr ssize_t WOS_ERESTARTSYS = 512;
	for (int i = 0; i < iovc; i++) {
		if (iovs[i].iov_len == 0)
			continue;
		ssize_t r;
		for (;;) {
			r = ker::abi::vfs::write(fd, iovs[i].iov_base, iovs[i].iov_len);
			if (r != -WOS_ERESTARTSYS)
				break;
		}
		if (r < 0) {
			if (total > 0)
				break;
			if (bytes_written)
				*bytes_written = 0;
			return (int)(-r);
		}
		total += r;
		if (static_cast<size_t>(r) < iovs[i].iov_len)
			break;
	}
	if (bytes_written)
		*bytes_written = total;
	return 0;
}

int Sysdeps<Readv>::operator()(int fd, const struct iovec *iovs, int iovc, ssize_t *bytes_read) {
	ssize_t total = 0;
	static constexpr ssize_t WOS_ERESTARTSYS = 512;
	for (int i = 0; i < iovc; i++) {
		if (iovs[i].iov_len == 0)
			continue;
		ssize_t r;
		for (;;) {
			r = ker::abi::vfs::read(fd, iovs[i].iov_base, iovs[i].iov_len);
			if (r != -WOS_ERESTARTSYS)
				break;
		}
		if (r < 0) {
			if (total > 0)
				break;
			if (bytes_read)
				*bytes_read = 0;
			return (int)(-r);
		}
		total += r;
		if (r == 0 || static_cast<size_t>(r) < iovs[i].iov_len)
			break;
	}
	if (bytes_read)
		*bytes_read = total;
	return 0;
}

int Sysdeps<Pread>::operator()(int fd, void *buf, size_t n, off_t off, ssize_t *bytes_read) {
	ssize_t r = ker::abi::vfs::pread(fd, buf, n, off);
	if (r < 0)
		return (int)(-r);
	if (bytes_read)
		*bytes_read = r;
	return 0;
}

int Sysdeps<Seek>::operator()(int fd, off_t offset, int whence, off_t *new_offset) {
	off_t r = ker::abi::vfs::lseek(fd, offset, whence);
	if (r < 0)
		return (int)(-r);
	*new_offset = r;
	return 0;
}

int Sysdeps<Close>::operator()(int fd) {
	int r = ker::abi::vfs::close(fd);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<ClockGet>::operator()(int clock_id, time_t *secs, long *nanos) {
	timespec ts;
	uint64_t res = ker::time::clock_gettime(clock_id, &ts);
	if ((int64_t)res < 0) {
		return (int)(-(int64_t)res);
	}
	if (secs)
		*secs = ts.tv_sec;
	if (nanos)
		*nanos = ts.tv_nsec;
	return 0;
}

int Sysdeps<Sleep>::operator()(time_t *secs, long *nanos) {
	timespec req;
	req.tv_sec = secs ? *secs : 0;
	req.tv_nsec = nanos ? *nanos : 0;
	timespec rem = {0, 0};

	uint64_t r = syscall(
	    ker::abi::callnums::time,
	    static_cast<uint64_t>(ker::abi::sys_time_ops::nanosleep),
	    reinterpret_cast<uint64_t>(&req),
	    reinterpret_cast<uint64_t>(&rem)
	);
	if ((int64_t)r < 0)
		return (int)(-(int64_t)r);
	if (secs)
		*secs = rem.tv_sec;
	if (nanos)
		*nanos = rem.tv_nsec;
	return 0;
}

int Sysdeps<Isatty>::operator()(int fd) {
	bool is_tty = ker::abi::vfs::isatty(fd);
	return is_tty ? 0 : ENOTTY;
}

int Sysdeps<Rmdir>::operator()(const char *path) {
	int r = ker::abi::vfs::rmdir(path);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Unlinkat>::operator()(int dirfd, const char *path, int flags) {
	int r = ker::abi::vfs::unlinkat(dirfd, path, flags);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Rename>::operator()(const char *old_path, const char *new_path) {
	int r = ker::abi::vfs::rename(old_path, new_path);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Sigprocmask>::operator()(
    int how, const sigset_t *__restrict set, sigset_t *__restrict retrieve
) {
	int64_t r = ker::process::sigprocmask(how, (const void *)set, (void *)retrieve);
	if (r < 0)
		return (int)(-r);
	return 0;
}

#if !MLIBC_BUILDING_RTLD
extern "C" void __mlibc_signal_restore();
extern "C" void __mlibc_signal_restore_rt();

int Sysdeps<Sigaction>::operator()(
    int signum, const struct sigaction *__restrict act, struct sigaction *__restrict oldact
) {
	struct sigaction modified_act;
	const struct sigaction *act_ptr = act;
	if (act) {
		modified_act = *act;
		modified_act.sa_flags |= SA_RESTORER;
		modified_act.sa_restorer =
		    (act->sa_flags & SA_SIGINFO) ? __mlibc_signal_restore_rt : __mlibc_signal_restore;
		act_ptr = &modified_act;
	}

	int64_t r = ker::process::sigaction(signum, (const void *)act_ptr, (void *)oldact);
	if (r < 0)
		return (int)(-r);
	return 0;
}
#endif // !MLIBC_BUILDING_RTLD

int Sysdeps<Fork>::operator()(pid_t *child) {
	int64_t r = ker::process::fork();
	if (r < 0)
		return (int)(-r);
	*child = (pid_t)r;
	return 0;
}

int
Sysdeps<Waitpid>::operator()(pid_t pid, int *status, int flags, struct rusage *ru, pid_t *ret_pid) {
	int64_t result = ker::process::waitpid(pid, status, flags, ru);
	if (result < 0) {
		return (int)(-result);
	}
	if (ret_pid)
		*ret_pid = (pid_t)result;
	return 0;
}

int Sysdeps<Execve>::operator()(const char *path, char *const argv[], char *const envp[]) {
	int64_t r = ker::process::execve(
	    path, const_cast<const char *const *>(argv), const_cast<const char *const *>(envp)
	);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

void Sysdeps<Yield>::operator()() { ker::multiproc::yield(); }

pid_t Sysdeps<GetPid>::operator()() { return ker::process::getpid(); }

int Sysdeps<Kill>::operator()(pid_t pid, int sig) {
	int64_t r = ker::process::kill((int64_t)pid, sig);
	if (r < 0)
		return (int)(-r);
	return 0;
}

// ---- POSIX sysdeps ----

#if __MLIBC_POSIX_OPTION && !MLIBC_BUILDING_RTLD

int
Sysdeps<Pwrite>::operator()(int fd, const void *buf, size_t n, off_t off, ssize_t *bytes_written) {
	ssize_t r = ker::abi::vfs::pwrite(fd, buf, n, off);
	if (r < 0)
		return (int)(-r);
	if (bytes_written)
		*bytes_written = r;
	return 0;
}

int Sysdeps<Access>::operator()(const char *path, int mode) {
	int r = ker::abi::vfs::access(path, mode);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Faccessat>::operator()(int dirfd, const char *pathname, int mode, int flags) {
	int r = ker::abi::vfs::faccessat(dirfd, pathname, mode, flags);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Dup>::operator()(int fd, int flags, int *newfd) {
	(void)flags;
	int r = ker::abi::vfs::dup(fd);
	if (r < 0)
		return -r;
	*newfd = r;
	return 0;
}

int Sysdeps<Dup2>::operator()(int fd, int flags, int newfd) {
	(void)flags;
	int r = ker::abi::vfs::dup2(fd, newfd);
	if (r < 0)
		return -r;
	return 0;
}

int
Sysdeps<Readlink>::operator()(const char *path, void *buffer, size_t max_size, ssize_t *length) {
	ssize_t r = ker::abi::vfs::readlink(path, static_cast<char *>(buffer), max_size);
	if (r < 0)
		return static_cast<int>(-r);
	if (length)
		*length = r;
	return 0;
}

int Sysdeps<Readlinkat>::operator()(
    int dirfd, const char *path, void *buffer, size_t max_size, ssize_t *length
) {
	if (dirfd == -100 || (path && path[0] == '/')) {
		return sysdep<Readlink>(path, buffer, max_size, length);
	}
	return ENOSYS;
}

int Sysdeps<Ftruncate>::operator()(int fd, size_t size) {
	int r = ker::abi::vfs::truncate(fd, (off_t)size);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Openat>::operator()(int dirfd, const char *path, int flags, mode_t mode, int *fd) {
	if (dirfd == -100 || (path && path[0] == '/')) {
		return sysdep<Open>(path, flags, mode, fd);
	}
	return ENOSYS;
}

int Sysdeps<Socket>::operator()(int family, int type, int protocol, int *fd) {
	int64_t r = ker::abi::net::socket(family, type & 0xFF, protocol);
	if (r < 0)
		return (int)(-r);
	*fd = (int)r;
	return 0;
}

int Sysdeps<MsgSend>::operator()(int fd, const struct msghdr *hdr, int flags, ssize_t *length) {
	if (!hdr || hdr->msg_iovlen == 0 || !hdr->msg_iov)
		return EINVAL;
	const iovec *iov = &hdr->msg_iov[0];
	if (hdr->msg_name) {
		for (;;) {
			ssize_t r =
			    ker::abi::net::sendto(fd, iov->iov_base, iov->iov_len, flags, hdr->msg_name);
			if (r == -EAGAIN)
				continue;
			if (r < 0) {
				return (int)(-r);
			}
			*length = r;
			return 0;
		}
	}
	for (;;) {
		ssize_t r = ker::abi::net::send(fd, iov->iov_base, iov->iov_len, flags);
		if (r == -EAGAIN)
			continue;
		if (r < 0)
			return (int)(-r);
		*length = r;
		return 0;
	}
}

int Sysdeps<Sendto>::operator()(
    int fd,
    const void *buffer,
    size_t size,
    int flags,
    const struct sockaddr *sock_addr,
    socklen_t addr_length,
    ssize_t *length
) {
	(void)addr_length;
	for (;;) {
		ssize_t r;
		if (sock_addr) {
			r = ker::abi::net::sendto(fd, buffer, size, flags, sock_addr);
		} else {
			r = ker::abi::net::send(fd, buffer, size, flags);
		}
		if (r == -EAGAIN)
			continue;
		if (r < 0)
			return (int)(-r);
		*length = r;
		return 0;
	}
}

int Sysdeps<MsgRecv>::operator()(int fd, struct msghdr *hdr, int flags, ssize_t *length) {
	if (!hdr || hdr->msg_iovlen == 0 || !hdr->msg_iov)
		return EINVAL;
	const iovec *iov = &hdr->msg_iov[0];
	if (hdr->msg_name) {
		ssize_t r = ker::abi::net::recvfrom(fd, iov->iov_base, iov->iov_len, flags, hdr->msg_name);
		if (r < 0)
			return (int)(-r);
		*length = r;
		return 0;
	}
	ssize_t r = ker::abi::net::recv(fd, iov->iov_base, iov->iov_len, flags);
	if (r < 0)
		return (int)(-r);
	*length = r;
	return 0;
}

int Sysdeps<Recvfrom>::operator()(
    int fd,
    void *buffer,
    size_t size,
    int flags,
    struct sockaddr *sock_addr,
    socklen_t *addr_length,
    ssize_t *length
) {
	(void)addr_length;
	ssize_t r;
	if (sock_addr) {
		r = ker::abi::net::recvfrom(fd, buffer, size, flags, sock_addr);
	} else {
		r = ker::abi::net::recv(fd, buffer, size, flags);
	}
	if (r < 0)
		return (int)(-r);
	*length = r;
	return 0;
}

int Sysdeps<Listen>::operator()(int fd, int backlog) {
	int64_t r = ker::abi::net::listen(fd, backlog);
	if (r < 0)
		return (int)(-r);
	return 0;
}

gid_t Sysdeps<GetGid>::operator()() { return static_cast<gid_t>(ker::process::getgid()); }
gid_t Sysdeps<GetEgid>::operator()() { return static_cast<gid_t>(ker::process::getegid()); }
uid_t Sysdeps<GetUid>::operator()() { return static_cast<uid_t>(ker::process::getuid()); }
uid_t Sysdeps<GetEuid>::operator()() { return static_cast<uid_t>(ker::process::geteuid()); }

pid_t Sysdeps<GetTid>::operator()() {
	return static_cast<pid_t>(ker::multiproc::currentThreadId());
}

pid_t Sysdeps<GetPpid>::operator()() { return (pid_t)ker::process::getppid(); }

int Sysdeps<GetPgid>::operator()(pid_t pid, pid_t *pgid) {
	int64_t r = ker::process::getpgid(pid);
	if (r < 0)
		return static_cast<int>(-r);
	if (pgid)
		*pgid = static_cast<pid_t>(r);
	return 0;
}

int Sysdeps<GetSid>::operator()(pid_t pid, pid_t *sid) {
	int64_t r = ker::process::getsid(pid);
	if (r < 0)
		return static_cast<int>(-r);
	if (sid)
		*sid = static_cast<pid_t>(r);
	return 0;
}

int Sysdeps<SetPgid>::operator()(pid_t pid, pid_t pgid) {
	int64_t r = ker::process::setpgid(pid, pgid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<SetUid>::operator()(uid_t uid) {
	int64_t r = ker::process::setuid(uid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<SetEuid>::operator()(uid_t euid) {
	int64_t r = ker::process::seteuid(euid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<SetGid>::operator()(gid_t gid) {
	int64_t r = ker::process::setgid(gid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<SetEgid>::operator()(gid_t egid) {
	int64_t r = ker::process::setegid(egid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<GetGroups>::operator()(size_t size, gid_t *list, int *retval) {
	(void)list;
	(void)size;
	if (retval)
		*retval = 0;
	return 0;
}

int Sysdeps<Pselect>::operator()(
    int num_fds,
    fd_set *read_set,
    fd_set *write_set,
    fd_set *except_set,
    const struct timespec *timeout,
    const sigset_t *sigmask,
    int *num_events
) {
	(void)sigmask;

	auto fd_is_set = [](int fd, fd_set *s) -> bool {
		return (s->fds_bits[fd / 8] >> (fd % 8)) & 1;
	};
	auto fd_set_bit = [](int fd, fd_set *s) {
		s->fds_bits[fd / 8] |= static_cast<unsigned char>(1 << (fd % 8));
	};
	auto fd_zero = [](fd_set *s) { memset(s->fds_bits, 0, sizeof(fd_set)); };

	int timeout_ms = -1;
	if (timeout) {
		timeout_ms = static_cast<int>((timeout->tv_sec * 1000) + (timeout->tv_nsec / 1000000));
		timeout_ms = std::max(timeout_ms, 0);
	}

	int epfd = ker::abi::vfs::epoll_create_vfs(0);
	if (epfd < 0)
		return ENOMEM;

	for (int fd = 0; fd < num_fds; fd++) {
		uint32_t events = 0;
		if (read_set && fd_is_set(fd, read_set))
			events |= EPOLLIN;
		if (write_set && fd_is_set(fd, write_set))
			events |= EPOLLOUT;
		if (except_set && fd_is_set(fd, except_set))
			events |= EPOLLERR | EPOLLHUP;
		if (events == 0)
			continue;

		epoll_event ev;
		ev.events = events;
		ev.data.fd = fd;
		ker::abi::vfs::epoll_ctl_vfs(epfd, EPOLL_CTL_ADD, fd, &ev);
	}

	static constexpr int WOS_ERESTARTSYS = 512;

	epoll_event out_events[64];
	int max = num_fds < 64 ? num_fds : 64;
	int ready;
	for (;;) {
		ready = ker::abi::vfs::epoll_pwait_vfs(epfd, out_events, max, timeout_ms);
		if (ready != -WOS_ERESTARTSYS)
			break;
	}

	if (ready == -EINTR) {
		ker::abi::vfs::close(epfd);
		*num_events = 0;
		return EINTR;
	}

	if (read_set)
		fd_zero(read_set);
	if (write_set)
		fd_zero(write_set);
	if (except_set)
		fd_zero(except_set);

	int count = 0;
	if (ready > 0) {
		for (int i = 0; i < ready; i++) {
			int fd = out_events[i].data.fd;
			if (read_set && (out_events[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR)))
				fd_set_bit(fd, read_set);
			if (write_set && (out_events[i].events & EPOLLOUT))
				fd_set_bit(fd, write_set);
			if (except_set && (out_events[i].events & (EPOLLERR | EPOLLHUP)))
				fd_set_bit(fd, except_set);
			count++;
		}
	}

	ker::abi::vfs::close(epfd);
	*num_events = count;
	return 0;
}

int Sysdeps<GetRlimit>::operator()(int resource, struct rlimit *limit) {
	(void)resource;
	if (limit) {
		limit->rlim_cur = RLIM_INFINITY;
		limit->rlim_max = RLIM_INFINITY;
	}
	return 0;
}

int Sysdeps<SetRlimit>::operator()(int resource, const struct rlimit *limit) {
	(void)resource;
	(void)limit;
	return 0;
}

int Sysdeps<SetPriority>::operator()(int which, id_t who, int prio) {
	(void)which;
	(void)who;
	(void)prio;
	return 0;
}

int Sysdeps<GetCwd>::operator()(char *buffer, size_t size) {
	int r = ker::abi::vfs::getcwd(buffer, size);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Chdir>::operator()(const char *path) {
	int r = ker::abi::vfs::chdir(path);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Fchdir>::operator()(int fd) {
	(void)fd;
	return 0;
}

int Sysdeps<Mkdir>::operator()(const char *path, mode_t mode) {
	int r = ker::abi::vfs::mkdir(path, static_cast<int>(mode));
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Mkdirat>::operator()(int dirfd, const char *path, mode_t mode) {
	if (dirfd == -100 || (path && path[0] == '/')) {
		return sysdep<Mkdir>(path, mode);
	}
	return ENOSYS;
}

int Sysdeps<Link>::operator()(const char *old_path, const char *new_path) {
	int r = ker::abi::vfs::link_vfs(old_path, new_path);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Linkat>::operator()(
    int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags
) {
	(void)flags;
	if ((olddirfd == AT_FDCWD || olddirfd == -100 || (oldpath && oldpath[0] == '/'))
	    && (newdirfd == AT_FDCWD || newdirfd == -100 || (newpath && newpath[0] == '/'))) {
		return sysdep<Link>(oldpath, newpath);
	}
	return ENOSYS;
}

int Sysdeps<Symlink>::operator()(const char *target, const char *linkpath) {
	int r = ker::abi::vfs::symlink(target, linkpath);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Symlinkat>::operator()(const char *target, int dirfd, const char *linkpath) {
	if (dirfd == -100 || (linkpath && linkpath[0] == '/')) {
		return sysdep<Symlink>(target, linkpath);
	}
	return ENOSYS;
}

int Sysdeps<Renameat>::operator()(
    int olddirfd, const char *old_path, int newdirfd, const char *new_path
) {
	int r = ker::abi::vfs::renameat(olddirfd, old_path, newdirfd, new_path);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Fcntl>::operator()(int fd, int request, va_list args, int *result) {
	uint64_t arg = 0;
	// F_DUPFD=0, F_GETFD=1, F_SETFD=2, F_GETFL=3, F_SETFL=4, F_DUPFD_CLOEXEC=1030
	if (request == 0 || request == 2 || request == 4 || request == 1030) {
		arg = va_arg(args, uint64_t);
	}
	int r = ker::abi::vfs::fcntl(fd, request, arg);
	if (r < 0) {
		return -r;
	}
	if (result)
		*result = r;
	return 0;
}

int Sysdeps<Ttyname>::operator()(int fd, char *buf, size_t size) {
	if (!ker::abi::vfs::isatty(fd))
		return ENOTTY;

	int pty_num = -1;
	int r = ker::abi::vfs::ioctl_vfs(
	    fd, 0x80045430 /* TIOCGPTN */, reinterpret_cast<unsigned long>(&pty_num)
	);
	if (r >= 0 && pty_num >= 0) {
		char name[32];
		int pos = 0;
		const char *prefix = "/dev/pts/";
		while (*prefix)
			name[pos++] = *prefix++;
		if (pty_num < 10) {
			name[pos++] = '0' + static_cast<char>(pty_num);
		} else {
			name[pos++] = '0' + static_cast<char>(pty_num / 10);
			name[pos++] = '0' + static_cast<char>(pty_num % 10);
		}
		name[pos] = '\0';
		if (static_cast<size_t>(pos) + 1 > size)
			return ERANGE;
		memcpy(buf, name, static_cast<size_t>(pos) + 1);
		return 0;
	}

	static const char *known_ttys[] = {"/dev/console", "/dev/tty0", "/dev/ttyS0"};
	for (const char *path : known_ttys) {
		size_t len = 0;
		const char *p = path;
		while (*p) {
			len++;
			p++;
		}
		if (len + 1 > size)
			continue;
		int tfd = ker::abi::vfs::open(path, 0 /* O_RDONLY */, 0);
		if (tfd >= 0) {
			ker::abi::vfs::close(tfd);
			memcpy(buf, path, len + 1);
			return 0;
		}
	}

	return ENOTTY;
}

int Sysdeps<Fsync>::operator()(int fd) {
	int r = ker::abi::vfs::fsync_vfs(fd);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Chmod>::operator()(const char *pathname, mode_t mode) {
	int r = ker::abi::vfs::chmod(pathname, mode);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Fchmod>::operator()(int fd, mode_t mode) {
	int r = ker::abi::vfs::fchmod(fd, mode);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Fchmodat>::operator()(int dirfd, const char *pathname, mode_t mode, int flags) {
	(void)flags;
	if (dirfd == AT_FDCWD || dirfd == -100 || (pathname && pathname[0] == '/')) {
		return sysdep<Chmod>(pathname, mode);
	}
	return ENOSYS;
}

int Sysdeps<SetSid>::operator()(pid_t *sid) {
	int64_t r = ker::process::setsid();
	if (r < 0)
		return static_cast<int>(-r);
	if (sid)
		*sid = static_cast<pid_t>(r);
	return 0;
}

int Sysdeps<Tcgetattr>::operator()(int fd, struct termios *attr) {
	if (!attr)
		return EINVAL;
	int r = ker::abi::vfs::ioctl_vfs(fd, 0x5401, reinterpret_cast<unsigned long>(attr));
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Tcsetattr>::operator()(int fd, int optional_actions, const struct termios *attr) {
	if (!attr)
		return EINVAL;
	unsigned long cmd;
	switch (optional_actions) {
		case TCSANOW:
			cmd = 0x5402;
			break;
		case TCSADRAIN:
			cmd = 0x5403;
			break;
		case TCSAFLUSH:
			cmd = 0x5404;
			break;
		default:
			return EINVAL;
	}
	int r = ker::abi::vfs::ioctl_vfs(
	    fd, cmd, reinterpret_cast<unsigned long>(const_cast<termios *>(attr))
	);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Tcflow>::operator()(int fd, int action) {
	(void)fd;
	(void)action;
	return 0;
}

int Sysdeps<Tcflush>::operator()(int fd, int queue) {
	int r = ker::abi::vfs::ioctl_vfs(fd, 0x540B /* TCFLSH */, static_cast<unsigned long>(queue));
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Tcdrain>::operator()(int fd) {
	(void)fd;
	return 0;
}

int Sysdeps<Pipe>::operator()(int *fds, int flags) {
	(void)flags;
	int r = ker::abi::vfs::pipe(fds);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Socketpair>::operator()(int domain, int type_and_flags, int proto, int *fds) {
	(void)domain;
	(void)type_and_flags;
	(void)proto;
	int r = ker::abi::vfs::pipe(fds);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Poll>::operator()(struct pollfd *fds, nfds_t count, int timeout, int *num_events) {
	static constexpr int WOS_ERESTARTSYS = 512;
	for (;;) {
		int r = ker::abi::net::poll(fds, count, timeout);
		if (r == -WOS_ERESTARTSYS)
			continue;
		if (r == -EINTR) {
			*num_events = 0;
			return EINTR;
		}
		if (r < 0)
			return -r;
		*num_events = r;
		return 0;
	}
}

int Sysdeps<Sendfile>::operator()(int outfd, int infd, off_t *offset, size_t count, ssize_t *out) {
	static constexpr ssize_t WOS_ERESTARTSYS = 512;
	ssize_t r;
	for (;;) {
		r = ker::abi::vfs::sendfile(outfd, infd, offset, count);
		if (r == -WOS_ERESTARTSYS)
			continue;
		break;
	}
	if (r < 0)
		return static_cast<int>(-r);
	if (out)
		*out = r;
	return 0;
}

int Sysdeps<Ioctl>::operator()(int fd, unsigned long request, void *arg, int *result) {
	// Route network ioctls (SIOC* range 0x8900-0x89FF) through net syscall
	if (request >= 0x8900 && request <= 0x89FF) {
		int r = ker::abi::net::ioctl_net(request, arg);
		if (r < 0)
			return -r;
		if (result)
			*result = 0;
		return 0;
	}
	// Route all other ioctls through VFS (device ioctl)
	int r = ker::abi::vfs::ioctl_vfs(fd, request, reinterpret_cast<unsigned long>(arg));
	if (r < 0)
		return -r;
	if (result)
		*result = r;
	return 0;
}

int Sysdeps<GetSockopt>::operator()(
    int fd, int layer, int number, void *__restrict buffer, socklen_t *__restrict size
) {
	size_t ksize = size ? *size : 0;
	int64_t r = ker::abi::net::getsockopt(fd, layer, number, buffer, &ksize);
	if (r < 0)
		return (int)(-r);
	if (size)
		*size = (socklen_t)ksize;
	return 0;
}

int
Sysdeps<SetSockopt>::operator()(int fd, int layer, int number, const void *buffer, socklen_t size) {
	int64_t r = ker::abi::net::setsockopt(fd, layer, number, buffer, size);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int Sysdeps<Shutdown>::operator()(int sockfd, int how) {
	int64_t r = ker::abi::net::shutdown(sockfd, how);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int Sysdeps<Accept>::operator()(
    int fd, int *newfd, struct sockaddr *addr_ptr, socklen_t *addr_length, int flags
) {
	(void)flags;
	size_t alen = addr_length ? *addr_length : 0;
	for (;;) {
		int64_t r = ker::abi::net::accept(fd, addr_ptr, &alen);
		if (r == -EAGAIN)
			continue;
		if (r < 0)
			return (int)(-r);
		if (addr_length)
			*addr_length = (socklen_t)alen;
		*newfd = (int)r;
		return 0;
	}
}

int Sysdeps<Bind>::operator()(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
	int64_t r = ker::abi::net::bind(fd, addr_ptr, addr_length);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int Sysdeps<Connect>::operator()(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
	for (;;) {
		int64_t r = ker::abi::net::connect(fd, addr_ptr, addr_length);
		if (r == -EAGAIN || r == -EINPROGRESS)
			continue;
		if (r < 0)
			return (int)(-r);
		return 0;
	}
}

int Sysdeps<Sockname>::operator()(
    int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length, socklen_t *actual_length
) {
	size_t alen = max_addr_length;
	int64_t r = ker::abi::net::getsockname(fd, addr_ptr, &alen);
	if (r < 0)
		return (int)(-r);
	if (actual_length)
		*actual_length = (socklen_t)alen;
	return 0;
}

int Sysdeps<Peername>::operator()(
    int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length, socklen_t *actual_length
) {
	size_t alen = max_addr_length;
	int64_t r = ker::abi::net::getpeername(fd, addr_ptr, &alen);
	if (r < 0)
		return (int)(-r);
	if (actual_length)
		*actual_length = (socklen_t)alen;
	return 0;
}

int Sysdeps<GetHostname>::operator()(char *buffer, size_t bufsize) {
	int64_t r = ker::process::gethostname(buffer, bufsize);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<GetEntropy>::operator()(void *buffer, size_t length) {
	int fd = ker::abi::vfs::open("/dev/urandom", 0 /* O_RDONLY */, 0);
	if (fd < 0)
		return EIO;
	size_t total = 0;
	while (total < length) {
		ssize_t r = ker::abi::vfs::read(fd, static_cast<char *>(buffer) + total, length - total);
		if (r <= 0) {
			ker::abi::vfs::close(fd);
			return EIO;
		}
		total += static_cast<size_t>(r);
	}
	ker::abi::vfs::close(fd);
	return 0;
}

int Sysdeps<Umask>::operator()(mode_t mode, mode_t *old) {
	uint64_t prev = ker::process::setumask(static_cast<uint64_t>(mode & 0777));
	if (old)
		*old = static_cast<mode_t>(prev);
	return 0;
}

int Sysdeps<Fchownat>::operator()(
    int dirfd, const char *pathname, uid_t owner, gid_t group, int flags
) {
	if (dirfd == AT_FDCWD || dirfd == -100) {
		int r = ker::abi::vfs::chown(pathname, owner, group);
		if (r < 0)
			return -r;
		return 0;
	}
	if (flags & 0x1000 /* AT_EMPTY_PATH */) {
		int r = ker::abi::vfs::fchown(dirfd, owner, group);
		if (r < 0)
			return -r;
		return 0;
	}
	int r = ker::abi::vfs::chown(pathname, owner, group);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Sigsuspend>::operator()(const sigset_t *set) {
	sigset_t old;
	int r = sysdep<Sigprocmask>(SIG_SETMASK, set, &old);
	if (r)
		return r;
	for (int i = 0; i < 10000; i++) {
		ker::multiproc::yield();
	}
	sysdep<Sigprocmask>(SIG_SETMASK, &old, nullptr);
	return EINTR;
}

int Sysdeps<SetGroups>::operator()(size_t size, const gid_t *list) {
	(void)size;
	(void)list;
	return 0;
}

int Sysdeps<GetItimer>::operator()(int which, struct itimerval *curr_value) {
	uint64_t res = ker::time::getitimer(which, (void *)curr_value);
	if ((int64_t)res < 0)
		return (int)(-(int64_t)res);
	return 0;
}

int Sysdeps<SetItimer>::operator()(
    int which, const struct itimerval *new_value, struct itimerval *old_value
) {
	if (old_value) {
		uint64_t res = ker::time::getitimer(which, (void *)old_value);
		if ((int64_t)res < 0)
			return (int)(-(int64_t)res);
	}
	uint64_t res = ker::time::setitimer(which, (const void *)new_value);
	if ((int64_t)res < 0)
		return (int)(-(int64_t)res);
	return 0;
}

int Sysdeps<Times>::operator()(struct tms *tms, clock_t *out) {
	clock_t ret;
	uint64_t res = ker::time::times((void *)tms, (void *)&ret);
	if ((int64_t)res < 0) {
		return (int)(-(int64_t)res);
	}
	*out = ret;
	return 0;
}

int Sysdeps<Uname>::operator()(struct utsname *buf) {
	if (!buf)
		return EINVAL;
	memset(buf, 0, sizeof(*buf));
	memcpy(buf->sysname, "WOS", 4);
	ker::process::gethostname(buf->nodename, sizeof(buf->nodename));
	memcpy(buf->release, "0.1.0", 6);
	memcpy(buf->version, "0.1.0", 6);
	memcpy(buf->machine, "x86_64", 7);
	return 0;
}

int Sysdeps<Openpt>::operator()(int oflags, int *fd) {
	(void)oflags;
	int r = ker::abi::vfs::open("/dev/ptmx", 2 /* O_RDWR */, 0);
	if (r < 0)
		return -r;
	if (fd)
		*fd = r;
	return 0;
}

int Sysdeps<Ptsname>::operator()(int masterfd, char *buffer, size_t length) {
	int pty_num = -1;
	int r = ker::abi::vfs::ioctl_vfs(
	    masterfd, 0x80045430 /* TIOCGPTN */, reinterpret_cast<unsigned long>(&pty_num)
	);
	if (r < 0)
		return -r;

	char name[32];
	int pos = 0;
	const char *prefix = "/dev/pts/";
	while (*prefix)
		name[pos++] = *prefix++;
	if (pty_num < 10) {
		name[pos++] = '0' + static_cast<char>(pty_num);
	} else {
		name[pos++] = '0' + static_cast<char>(pty_num / 10);
		name[pos++] = '0' + static_cast<char>(pty_num % 10);
	}
	name[pos] = '\0';

	if (static_cast<size_t>(pos) + 1 > length)
		return ERANGE;
	memcpy(buffer, name, static_cast<size_t>(pos) + 1);
	return 0;
}

int Sysdeps<Unlockpt>::operator()(int fd) {
	int unlock = 0;
	int r = ker::abi::vfs::ioctl_vfs(
	    fd, 0x40045431 /* TIOCSPTLCK */, reinterpret_cast<unsigned long>(&unlock)
	);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<GetAffinity>::operator()(pid_t, size_t, cpu_set_t *) { return ENOSYS; }

int Sysdeps<GetThreadaffinity>::operator()(pid_t tid, size_t cpusetsize, cpu_set_t *mask) {
	if (!mask || cpusetsize == 0) {
		return EINVAL;
	}

	int64_t result = ker::multiproc::getThreadAffinityMask((uint64_t)tid);
	if (result < 0) {
		return (int)(-result);
	}

	return mask_to_cpuset((uint64_t)result, cpusetsize, mask);
}

int Sysdeps<SetAffinity>::operator()(pid_t, size_t, const cpu_set_t *) { return ENOSYS; }

int Sysdeps<SetThreadaffinity>::operator()(pid_t tid, size_t cpusetsize, const cpu_set_t *mask) {
	if (!mask || cpusetsize == 0) {
		return EINVAL;
	}

	uint64_t affinity_mask = 0;
	if (int e = cpuset_to_mask(cpusetsize, mask, &affinity_mask); e) {
		return e;
	}

	int64_t result = ker::multiproc::setThreadAffinityMask((uint64_t)tid, affinity_mask);
	if (result < 0) {
		return (int)(-result);
	}
	return 0;
}

#endif // __MLIBC_POSIX_OPTION && !MLIBC_BUILDING_RTLD

int Sysdeps<Mount>::operator()(
    const char *source,
    const char *target,
    const char *fstype,
    unsigned long flags,
    const void *data
) {
	(void)flags;
	(void)data;
	int r = ker::abi::vfs::mount(source, target, fstype);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Umount2>::operator()(const char *target, int flags) {
	(void)flags;
	int r = ker::abi::vfs::umount(target);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<EpollCreate>::operator()(int flags, int *fd) {
	int r = ker::abi::vfs::epoll_create_vfs(flags);
	if (r < 0)
		return -r;
	if (fd)
		*fd = r;
	return 0;
}

int Sysdeps<EpollCtl>::operator()(int epfd, int mode, int fd, epoll_event *ev) {
	int r = ker::abi::vfs::epoll_ctl_vfs(epfd, mode, fd, ev);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<EpollPwait>::operator()(
    int epfd, epoll_event *ev, int n, int timeout, const sigset_t *sigmask, int *raised
) {
	(void)sigmask;
	static constexpr int WOS_ERESTARTSYS = 512;
	for (;;) {
		int r = ker::abi::vfs::epoll_pwait_vfs(epfd, ev, n, timeout);
		if (r == -WOS_ERESTARTSYS)
			continue;
		if (r == -EINTR) {
			if (raised)
				*raised = 0;
			return EINTR;
		}
		if (r < 0)
			return -r;
		if (raised)
			*raised = r;
		return 0;
	}
}

int Sysdeps<Getcpu>::operator()(int *cpu) {
	*cpu = (int)ker::multiproc::getCurrentCpu();
	return 0;
}

} // namespace mlibc

__attribute__((visibility("default"))) extern "C" void frg_panic(const char *mstr) {
	mlibc::sysdep<LibcLog>(mstr);
	mlibc::sysdep<LibcPanic>();
}
