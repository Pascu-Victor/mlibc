#include "mlibc/ansi-sysdeps.hpp"
#include <abi-bits/pid_t.h>
#include <algorithm>
#include <errno.h>
#include <limits.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/fsfd_target.hpp>
#include <mlibc/tcb.hpp>
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

int sys_futex_wake(int *pointer) {
	int64_t result = ker::futex::wake(pointer);
	if (result < 0) {
		return static_cast<int>(-result); // Return positive errno
	}
	return 0;
}

int sys_futex_wait(int *pointer, int expected, timespec const *timeout) {
	int64_t result = ker::futex::wait(pointer, expected, timeout);
	if (result < 0) {
		return static_cast<int>(-result); // Return positive errno
	}
	return 0;
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

void sys_yield() { ker::multiproc::yield(); }

int sys_clock_get(int clock_id, long *seconds, long *nanoseconds) {
	(void)clock_id;
	timespec ts;
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

int sys_gettimeofday(timeval *tv, void *tz) {
	(void)tz;
	if (!tv)
		return EINVAL;
	timeval tmp;
	uint64_t res = ker::time::gettimeofday(&tmp);
	if ((int64_t)res < 0) {
		return (int)(-(int64_t)res);
	}
	tv->tv_sec = tmp.tv_sec;
	tv->tv_usec = tmp.tv_usec;
	return 0;
}

int sys_times(struct tms *tms, clock_t *out) {
	clock_t ret;
	uint64_t res = ker::time::times((void *)tms, (void *)&ret);
	if ((int64_t)res < 0) {
		return (int)(-(int64_t)res);
	}
	*out = ret;
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

int sys_waitpid(pid_t pid, int *status, int flags, rusage *ru, pid_t *ret_pid) {
	(void)ru; // WOS doesn't fill rusage yet
	int64_t result = ker::process::waitpid(pid, status, flags);
	if (result < 0) {
		// Convert kernel error code to positive errno
		return (int)(-result);
	}
	if (ret_pid)
		*ret_pid = (pid_t)result;
	return 0; // Success
}

pid_t sys_getpid() { return ker::process::getpid(); }

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

// -- Socket sysdeps --------------------------------------------------

int sys_socket(int family, int type, int protocol, int *fd) {
	int64_t r = ker::abi::net::socket(family, type & 0xFF, protocol);
	if (r < 0)
		return (int)(-r);
	*fd = (int)r;
	return 0;
}

int sys_bind(int fd, const sockaddr *addr_ptr, socklen_t addr_length) {
	int64_t r = ker::abi::net::bind(fd, addr_ptr, addr_length);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int sys_listen(int fd, int backlog) {
	int64_t r = ker::abi::net::listen(fd, backlog);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int sys_accept(int fd, int *newfd, sockaddr *addr_ptr, socklen_t *addr_length, int flags) {
	(void)flags;
	size_t alen = addr_length ? *addr_length : 0;
	for (;;) {
		int64_t r = ker::abi::net::accept(fd, addr_ptr, &alen);
		if (r == -EAGAIN)
			continue; // deferred, retry after wake
		if (r < 0)
			return (int)(-r);
		if (addr_length)
			*addr_length = (socklen_t)alen;
		*newfd = (int)r;
		return 0;
	}
}

int sys_connect(int fd, const sockaddr *addr_ptr, socklen_t addr_length) {
	for (;;) {
		int64_t r = ker::abi::net::connect(fd, addr_ptr, addr_length);
		if (r == -EAGAIN || r == -EINPROGRESS)
			continue; // deferred, retry after wake
		if (r < 0)
			return (int)(-r);
		return 0;
	}
}

int sys_msg_send(int fd, const msghdr *hdr, int flags, ssize_t *length) {
	if (!hdr || hdr->msg_iovlen == 0 || !hdr->msg_iov)
		return EINVAL;
	// Send the first iovec; simple single-buffer path
	const iovec *iov = &hdr->msg_iov[0];
	if (hdr->msg_name) {
		// sendto path (has destination address)
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
	// send path (connected socket)
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

int sys_msg_recv(int fd, msghdr *hdr, int flags, ssize_t *length) {
	if (!hdr || hdr->msg_iovlen == 0 || !hdr->msg_iov)
		return EINVAL;
	const iovec *iov = &hdr->msg_iov[0];
	if (hdr->msg_name) {
		// recvfrom path
		ssize_t r =
		    ker::abi::net::recvfrom(fd, iov->iov_base, iov->iov_len, flags, hdr->msg_name);
		if (r < 0)
			return (int)(-r);
		*length = r;
		return 0;
	}
	// recv path
	ssize_t r = ker::abi::net::recv(fd, iov->iov_base, iov->iov_len, flags);
	if (r < 0)
		return (int)(-r);
	*length = r;
	return 0;
}

ssize_t sys_sendto(
    int fd,
    const void *buffer,
    size_t size,
    int flags,
    const sockaddr *sock_addr,
    socklen_t addr_length,
    ssize_t *length
) {
	(void)addr_length; // kernel infers from socket domain
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

ssize_t sys_recvfrom(
    int fd,
    void *buffer,
    size_t size,
    int flags,
    sockaddr *sock_addr,
    socklen_t *addr_length,
    ssize_t *length
) {
	(void)addr_length; // kernel fills based on socket domain
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

int sys_setsockopt(int fd, int layer, int number, const void *buffer, socklen_t size) {
	int64_t r = ker::abi::net::setsockopt(fd, layer, number, buffer, size);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int
sys_getsockopt(int fd, int layer, int number, void *__restrict buffer, socklen_t *__restrict size) {
	size_t ksize = size ? *size : 0;
	int64_t r = ker::abi::net::getsockopt(fd, layer, number, buffer, &ksize);
	if (r < 0)
		return (int)(-r);
	if (size)
		*size = (socklen_t)ksize;
	return 0;
}

int sys_shutdown(int sockfd, int how) {
	int64_t r = ker::abi::net::shutdown(sockfd, how);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int sys_sockname(int fd, sockaddr *addr_ptr, socklen_t max_addr_length, socklen_t *actual_length) {
	size_t alen = max_addr_length;
	int64_t r = ker::abi::net::getsockname(fd, addr_ptr, &alen);
	if (r < 0)
		return (int)(-r);
	if (actual_length)
		*actual_length = (socklen_t)alen;
	return 0;
}

int sys_peername(int fd, sockaddr *addr_ptr, socklen_t max_addr_length, socklen_t *actual_length) {
	size_t alen = max_addr_length;
	int64_t r = ker::abi::net::getpeername(fd, addr_ptr, &alen);
	if (r < 0)
		return (int)(-r);
	if (actual_length)
		*actual_length = (socklen_t)alen;
	return 0;
}

int sys_poll(pollfd *fds, nfds_t count, int timeout, int *num_events) {
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

int sys_ioctl(int fd, unsigned long request, void *arg, int *result) {
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

int sys_stat(fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf) {
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
		return -r; // Convert negative error to positive errno
	}
	return 0;
}

// --- New POSIX sysdeps for busybox applet support ---

pid_t sys_getppid() { return (pid_t)ker::process::getppid(); }

int sys_getcwd(char *buffer, size_t size) {
	int r = ker::abi::vfs::getcwd(buffer, size);
	if (r < 0)
		return -r;
	return 0;
}

int sys_chdir(const char *path) {
	int r = ker::abi::vfs::chdir(path);
	if (r < 0)
		return -r;
	return 0;
}

int sys_access(const char *path, int mode) {
	int r = ker::abi::vfs::access(path, mode);
	if (r < 0)
		return -r;
	return 0;
}

int sys_faccessat(int dirfd, const char *pathname, int mode, int flags) {
	(void)flags;
	int r = ker::abi::vfs::faccessat(dirfd, pathname, mode, flags);
	if (r < 0)
		return -r;
	return 0;
}

int sys_dup(int fd, int flags, int *newfd) {
	(void)flags;
	int r = ker::abi::vfs::dup(fd);
	if (r < 0)
		return -r;
	*newfd = r;
	return 0;
}

int sys_dup2(int fd, int flags, int newfd) {
	(void)flags;
	int r = ker::abi::vfs::dup2(fd, newfd);
	if (r < 0)
		return -r;
	return 0;
}

int sys_pipe(int *fds, int flags) {
	(void)flags; // O_CLOEXEC etc not yet supported
	int r = ker::abi::vfs::pipe(fds);
	if (r < 0)
		return -r;
	return 0;
}

int sys_fcntl(int fd, int request, va_list args, int *result) {
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

int sys_unlinkat(int fd, const char *path, int flags) {
	int r = ker::abi::vfs::unlinkat(fd, path, flags);
	if (r < 0)
		return -r;
	return 0;
}

int sys_rmdir(const char *path) {
	int r = ker::abi::vfs::rmdir(path);
	if (r < 0)
		return -r;
	return 0;
}

int sys_rename(const char *old_path, const char *new_path) {
	int r = ker::abi::vfs::rename(old_path, new_path);
	if (r < 0)
		return -r;
	return 0;
}

int sys_renameat(int olddirfd, const char *old_path, int newdirfd, const char *new_path) {
	int r = ker::abi::vfs::renameat(olddirfd, old_path, newdirfd, new_path);
	if (r < 0)
		return -r;
	return 0;
}

int sys_ftruncate(int fd, size_t size) {
	int r = ker::abi::vfs::truncate(fd, (off_t)size);
	if (r < 0)
		return -r;
	return 0;
}

int sys_fchmod(int fd, mode_t mode) {
	int r = ker::abi::vfs::fchmod(fd, mode);
	if (r < 0)
		return -r;
	return 0;
}

int sys_chmod(const char *pathname, mode_t mode) {
	int r = ker::abi::vfs::chmod(pathname, mode);
	if (r < 0)
		return -r;
	return 0;
}

int sys_fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) {
	(void)flags;
	if (dirfd == AT_FDCWD || dirfd == -100) {
		// Absolute or cwd-relative path
		int r = ker::abi::vfs::chown(pathname, owner, group);
		if (r < 0)
			return -r;
		return 0;
	}
	// AT_EMPTY_PATH means operate on fd itself
	if (flags & 0x1000 /* AT_EMPTY_PATH */) {
		int r = ker::abi::vfs::fchown(dirfd, owner, group);
		if (r < 0)
			return -r;
		return 0;
	}
	// dirfd-relative path — use chown for now, kernel will resolve dirfd
	int r = ker::abi::vfs::chown(pathname, owner, group);
	if (r < 0)
		return -r;
	return 0;
}

int sys_pread(int fd, void *buf, size_t n, off_t off, ssize_t *bytes_read) {
	ssize_t r = ker::abi::vfs::pread(fd, buf, n, off);
	if (r < 0)
		return (int)(-r);
	if (bytes_read)
		*bytes_read = r;
	return 0;
}

int sys_sleep(time_t *secs, long *nanos) {
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

int sys_fork(pid_t *child) {
	int64_t r = ker::process::fork();
	if (r < 0)
		return (int)(-r);
	*child = (pid_t)r;
	return 0;
}

extern "C" void __mlibc_signal_restore();
extern "C" void __mlibc_signal_restore_rt();

int sys_sigaction(
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

int sys_sigprocmask(int how, const sigset_t *__restrict set, sigset_t *__restrict retrieve) {
	int64_t r = ker::process::sigprocmask(how, (const void *)set, (void *)retrieve);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int sys_kill(int pid, int sig) {
	int64_t r = ker::process::kill((int64_t)pid, sig);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int sys_umount2(const char *target, int flags) {
	(void)flags;
	int r = ker::abi::vfs::umount(target);
	if (r < 0)
		return -r;
	return 0;
}

int sys_mount(
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

uid_t sys_getuid() { return static_cast<uid_t>(ker::process::getuid()); }
uid_t sys_geteuid() { return static_cast<uid_t>(ker::process::geteuid()); }
gid_t sys_getgid() { return static_cast<gid_t>(ker::process::getgid()); }
gid_t sys_getegid() { return static_cast<gid_t>(ker::process::getegid()); }

int sys_setuid(uid_t uid) {
	int64_t r = ker::process::setuid(uid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int sys_seteuid(uid_t euid) {
	int64_t r = ker::process::seteuid(euid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int sys_setgid(gid_t gid) {
	int64_t r = ker::process::setgid(gid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int sys_setegid(gid_t egid) {
	int64_t r = ker::process::setegid(egid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int sys_uname(utsname *buf) {
	if (!buf)
		return EINVAL;
	memset(buf, 0, sizeof(*buf));
	memcpy(buf->sysname, "WOS", 3);
	memcpy(buf->nodename, "wos", 3);
	memcpy(buf->release, "0.1.0", 5);
	memcpy(buf->version, "0.1.0", 5);
	memcpy(buf->machine, "x86_64", 6);
	return 0;
}

int sys_epoll_create(int flags, int *fd) {
	int r = ker::abi::vfs::epoll_create_vfs(flags);
	if (r < 0)
		return -r;
	if (fd)
		*fd = r;
	return 0;
}

int sys_epoll_ctl(int epfd, int mode, int fd, epoll_event *ev) {
	int r = ker::abi::vfs::epoll_ctl_vfs(epfd, mode, fd, ev);
	if (r < 0)
		return -r;
	return 0;
}

int sys_epoll_pwait(
    int epfd, epoll_event *ev, int n, int timeout, const sigset_t *sigmask, int *raised
) {
	(void)sigmask; // signal mask not yet supported
	static constexpr int WOS_ERESTARTSYS = 512;
	for (;;) {
		int r = ker::abi::vfs::epoll_pwait_vfs(epfd, ev, n, timeout);
		if (r == -WOS_ERESTARTSYS)
			continue;
		if (r == -EINTR) {
			if (raised) *raised = 0;
			return EINTR;
		}
		if (r < 0)
			return -r;
		if (raised)
			*raised = r;
		return 0;
	}
}

// --- Dropbear SSH required sysdeps ---

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
	int64_t r = ker::process::execve(
	    path, const_cast<const char *const *>(argv), const_cast<const char *const *>(envp)
	);
	if (r < 0)
		return static_cast<int>(-r);
	// On success execve does not return, but if kernel returned 0 it means
	// the context switch will happen on return from syscall
	return 0;
}

int sys_setsid(pid_t *sid) {
	int64_t r = ker::process::setsid();
	if (r < 0)
		return static_cast<int>(-r);
	if (sid)
		*sid = static_cast<pid_t>(r);
	return 0;
}

int sys_setpgid(pid_t pid, pid_t pgid) {
	int64_t r = ker::process::setpgid(pid, pgid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int sys_getpgid(pid_t pid, pid_t *pgid) {
	int64_t r = ker::process::getpgid(pid);
	if (r < 0)
		return static_cast<int>(-r);
	if (pgid)
		*pgid = static_cast<pid_t>(r);
	return 0;
}

int sys_getsid(pid_t pid, pid_t *sid) {
	int64_t r = ker::process::getsid(pid);
	if (r < 0)
		return static_cast<int>(-r);
	if (sid)
		*sid = static_cast<pid_t>(r);
	return 0;
}

int sys_openpt(int oflags, int *fd) {
	(void)oflags;
	// Open /dev/ptmx to allocate a new PTY pair
	int r = ker::abi::vfs::open("/dev/ptmx", 2 /* O_RDWR */, 0);
	if (r < 0)
		return -r;
	if (fd)
		*fd = r;
	return 0;
}

int sys_ptsname(int masterfd, char *buffer, size_t length) {
	// Get the PTY number via TIOCGPTN ioctl
	int pty_num = -1;
	int r = ker::abi::vfs::ioctl_vfs(
	    masterfd, 0x80045430 /* TIOCGPTN */, reinterpret_cast<unsigned long>(&pty_num)
	);
	if (r < 0)
		return -r;

	// Build the slave name: "/dev/pts/<N>"
	char name[32];
	int pos = 0;
	const char *prefix = "/dev/pts/";
	while (*prefix)
		name[pos++] = *prefix++;
	// Convert number to string
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

int sys_ttyname(int fd, char *buf, size_t size) {
	// Check if it's a TTY first
	if (!ker::abi::vfs::isatty(fd))
		return ENOTTY;

	// Try TIOCGPTN ioctl to see if it's a PTY (master or slave)
	int pty_num = -1;
	int r = ker::abi::vfs::ioctl_vfs(
	    fd, 0x80045430 /* TIOCGPTN */, reinterpret_cast<unsigned long>(&pty_num)
	);
	if (r >= 0 && pty_num >= 0) {
		// It's a PTY — format "/dev/pts/<N>"
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

	// Not a PTY — try known console/serial devices
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
		// Try to open and compare — simple heuristic
		int tfd = ker::abi::vfs::open(path, 0 /* O_RDONLY */, 0);
		if (tfd >= 0) {
			ker::abi::vfs::close(tfd);
			memcpy(buf, path, len + 1);
			return 0;
		}
	}

	return ENOTTY;
}

int sys_unlockpt(int fd) {
	// Unlock the slave side via TIOCSPTLCK ioctl (value 0 = unlock)
	int unlock = 0;
	int r = ker::abi::vfs::ioctl_vfs(
	    fd, 0x40045431 /* TIOCSPTLCK */, reinterpret_cast<unsigned long>(&unlock)
	);
	if (r < 0)
		return -r;
	return 0;
}

int sys_getentropy(void *buffer, size_t length) {
	// Read from /dev/urandom
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

int sys_tcgetattr(int fd, termios *attr) {
	if (!attr)
		return EINVAL;
	// Use TCGETS ioctl (0x5401) to get termios from kernel PTY
	int r = ker::abi::vfs::ioctl_vfs(fd, 0x5401, reinterpret_cast<unsigned long>(attr));
	if (r < 0)
		return -r;
	return 0;
}

int sys_tcsetattr(int fd, int optional_actions, const termios *attr) {
	if (!attr)
		return EINVAL;
	unsigned long cmd;
	switch (optional_actions) {
		case TCSANOW:
			cmd = 0x5402;
			break; // TCSETS
		case TCSADRAIN:
			cmd = 0x5403;
			break; // TCSETSW
		case TCSAFLUSH:
			cmd = 0x5404;
			break; // TCSETSF
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

int sys_tcflush(int fd, int queue) {
	int r = ker::abi::vfs::ioctl_vfs(fd, 0x540B /* TCFLSH */, static_cast<unsigned long>(queue));
	if (r < 0)
		return -r;
	return 0;
}

int sys_tcdrain(int fd) {
	// No-op for our PTY — data is immediately available, no hardware buffer to drain
	(void)fd;
	return 0;
}

int sys_fsync(int fd) {
	int r = ker::abi::vfs::fsync_vfs(fd);
	if (r < 0)
		return -r;
	return 0;
}

int sys_link(const char *old_path, const char *new_path) {
	int r = ker::abi::vfs::link_vfs(old_path, new_path);
	if (r < 0)
		return -r;
	return 0;
}

int sys_gethostname(char *buffer, size_t bufsize) {
	const char *name = "wos";
	size_t len = 3; // strlen("wos")
	if (len + 1 > bufsize)
		return ENAMETOOLONG;
	memcpy(buffer, name, len + 1);
	return 0;
}

int sys_getrlimit(int resource, rlimit *limit) {
	// Return permissive defaults — WOS has no resource limits
	(void)resource;
	if (limit) {
		limit->rlim_cur = RLIM_INFINITY;
		limit->rlim_max = RLIM_INFINITY;
	}
	return 0;
}

int sys_setrlimit(int resource, const rlimit *limit) {
	// No-op stub — WOS doesn't enforce resource limits
	(void)resource;
	(void)limit;
	return 0;
}

int sys_umask(mode_t mode, mode_t *old) {
	uint64_t prev = ker::process::setumask(static_cast<uint64_t>(mode & 0777));
	if (old)
		*old = static_cast<mode_t>(prev);
	return 0;
}

int sys_pselect(
    int num_fds,
    fd_set *read_set,
    fd_set *write_set,
    fd_set *except_set,
    const timespec *timeout,
    const sigset_t *sigmask,
    int *num_events
) {
	(void)sigmask;

	// Inline fd_set bit helpers (fds_bits is uint8_t[128] in mlibc)
	auto fd_is_set = [](int fd, fd_set *s) -> bool {
		return (s->fds_bits[fd / 8] >> (fd % 8)) & 1;
	};
	auto fd_set_bit = [](int fd, fd_set *s) {
		s->fds_bits[fd / 8] |= static_cast<unsigned char>(1 << (fd % 8));
	};
	auto fd_zero = [](fd_set *s) { memset(s->fds_bits, 0, sizeof(fd_set)); };

	// Convert timeout to epoll milliseconds
	int timeout_ms = -1;
	if (timeout) {
		timeout_ms = static_cast<int>((timeout->tv_sec * 1000) + (timeout->tv_nsec / 1000000));
		timeout_ms = std::max(timeout_ms, 0);
	}

	// Create an epoll instance
	int epfd = ker::abi::vfs::epoll_create_vfs(0);
	if (epfd < 0)
		return ENOMEM;

	// Register all fds from the fd_sets into epoll
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

	// WOS_ERESTARTSYS (512) is the kernel-internal "yield and retry" code.
	// -EINTR means a signal is pending and will be delivered on return.
	static constexpr int WOS_ERESTARTSYS = 512;

	// Wait for events (retry on WOS_ERESTARTSYS; break on events, timeout, or signal)
	epoll_event out_events[64];
	int max = num_fds < 64 ? num_fds : 64;
	int ready;
	for (;;) {
		ready = ker::abi::vfs::epoll_pwait_vfs(epfd, out_events, max, timeout_ms);
		if (ready != -WOS_ERESTARTSYS)
			break;
	}

	// Handle signal interruption: a signal handler has already run (during
	// the sysret path).  Return EINTR so the caller knows to re-check.
	if (ready == -EINTR) {
		ker::abi::vfs::close(epfd);
		*num_events = 0;
		return EINTR;
	}

	// Clear the input sets — we'll only set bits that are ready
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

// --- Additional POSIX sysdeps for ash/busybox ---

int sys_openat(int dirfd, const char *path, int flags, mode_t mode, int *fd) {
	// For AT_FDCWD (-100) or absolute paths, delegate to regular open
	if (dirfd == -100 || (path && path[0] == '/')) {
		return sys_open(path, flags, mode, fd);
	}
	// Non-AT_FDCWD relative opens are not yet supported
	return ENOSYS;
}

int sys_mkdirat(int dirfd, const char *path, mode_t mode) {
	if (dirfd == -100 || (path && path[0] == '/')) {
		int r = ker::abi::vfs::mkdir(path, static_cast<int>(mode));
		if (r < 0)
			return -r;
		return 0;
	}
	return ENOSYS;
}

int sys_readlink(const char *path, void *buffer, size_t max_size, ssize_t *length) {
	ssize_t r = ker::abi::vfs::readlink(path, static_cast<char *>(buffer), max_size);
	if (r < 0)
		return static_cast<int>(-r);
	if (length)
		*length = r;
	return 0;
}

int sys_readlinkat(int dirfd, const char *path, void *buffer, size_t max_size, ssize_t *length) {
	if (dirfd == -100 || (path && path[0] == '/')) {
		return sys_readlink(path, buffer, max_size, length);
	}
	return ENOSYS;
}

int sys_symlinkat(const char *target, int dirfd, const char *linkpath) {
	if (dirfd == -100 || (linkpath && linkpath[0] == '/')) {
		int r = ker::abi::vfs::symlink(target, linkpath);
		if (r < 0)
			return -r;
		return 0;
	}
	return ENOSYS;
}

int sys_fchdir(int fd) {
	// Attempt to get the path for this fd via /proc/self/fd/<N> readlink
	// For now, if fd refers to a directory that was opened, we need
	// kernel support. Return success as a stub since most shells can cope.
	(void)fd;
	return 0;
}

int sys_socketpair(int domain, int type_and_flags, int proto, int *fds) {
	(void)domain;
	(void)type_and_flags;
	(void)proto;
	// Use pipe as a unidirectional fallback — sufficient for many ash uses
	// (here-documents, command substitution internal fds).
	// A proper Unix domain socketpair would require kernel support.
	int r = ker::abi::vfs::pipe(fds);
	if (r < 0)
		return -r;
	return 0;
}

int sys_sigsuspend(const sigset_t *set) {
	// Atomically replace signal mask and wait for a signal.
	// Simplified: set the mask and spin-yield until a signal is delivered.
	sigset_t old;
	int r = sys_sigprocmask(SIG_SETMASK, set, &old);
	if (r)
		return r;
	// Yield repeatedly — the kernel will deliver pending signals on syscall return
	for (int i = 0; i < 10000; i++) {
		ker::multiproc::yield();
	}
	// Restore old mask
	sys_sigprocmask(SIG_SETMASK, &old, nullptr);
	return EINTR; // sigsuspend always returns EINTR
}

int sys_tcflow(int fd, int action) {
	// No-op — WOS PTY doesn't implement XON/XOFF flow control
	(void)fd;
	(void)action;
	return 0;
}

int sys_getgroups(size_t size, const gid_t *list, int *retval) {
	(void)list;
	// WOS doesn't support supplementary groups yet
	if (retval)
		*retval = 0;
	(void)size;
	return 0;
}

int sys_setgroups(size_t size, const gid_t *list) {
	(void)size;
	(void)list;
	// No-op stub
	return 0;
}

} // namespace mlibc

__attribute__((visibility("default"))) extern "C" void frg_panic(const char *mstr) {
	//	mlibc::sys_libc_log("mlibc: Call to frg_panic");
	mlibc::sys_libc_log(mstr);
	mlibc::sys_libc_panic();
}
