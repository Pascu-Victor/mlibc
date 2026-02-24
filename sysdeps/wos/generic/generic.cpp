#include "mlibc/ansi-sysdeps.hpp"
#include <abi-bits/pid_t.h>
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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/time_calls.h>
#include <sys/time_ops.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/vmem.h>

// SafeStack support: This variable is accessed by the compiler-generated code
// It needs to be in TLS storage and properly initialized
// The actual initialization will be done by the kernel when setting up TLS
extern "C" __attribute__((visibility("default"))) __thread void *__safestack_unsafe_stack_ptr =
    nullptr;

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

int sys_bind(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
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

int sys_accept(int fd, int *newfd, struct sockaddr *addr_ptr, socklen_t *addr_length, int flags) {
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

int sys_connect(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
	for (;;) {
		int64_t r = ker::abi::net::connect(fd, addr_ptr, addr_length);
		if (r == -EAGAIN || r == -EINPROGRESS)
			continue; // deferred, retry after wake
		if (r < 0)
			return (int)(-r);
		return 0;
	}
}

int sys_msg_send(int fd, const struct msghdr *hdr, int flags, ssize_t *length) {
	if (!hdr || hdr->msg_iovlen == 0 || !hdr->msg_iov)
		return EINVAL;
	// Send the first iovec; simple single-buffer path
	const struct iovec *iov = &hdr->msg_iov[0];
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

int sys_msg_recv(int fd, struct msghdr *hdr, int flags, ssize_t *length) {
	if (!hdr || hdr->msg_iovlen == 0 || !hdr->msg_iov)
		return EINVAL;
	const struct iovec *iov = &hdr->msg_iov[0];
	if (hdr->msg_name) {
		// recvfrom path
		for (;;) {
			ssize_t r =
			    ker::abi::net::recvfrom(fd, iov->iov_base, iov->iov_len, flags, hdr->msg_name);
			if (r == -EAGAIN)
				continue;
			if (r < 0)
				return (int)(-r);
			*length = r;
			return 0;
		}
	}
	// recv path
	for (;;) {
		ssize_t r = ker::abi::net::recv(fd, iov->iov_base, iov->iov_len, flags);
		if (r == -EAGAIN)
			continue;
		if (r < 0)
			return (int)(-r);
		*length = r;
		return 0;
	}
}

ssize_t sys_sendto(
    int fd,
    const void *buffer,
    size_t size,
    int flags,
    const struct sockaddr *sock_addr,
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
    struct sockaddr *sock_addr,
    socklen_t *addr_length,
    ssize_t *length
) {
	(void)addr_length; // kernel fills based on socket domain
	for (;;) {
		ssize_t r;
		if (sock_addr) {
			r = ker::abi::net::recvfrom(fd, buffer, size, flags, sock_addr);
		} else {
			r = ker::abi::net::recv(fd, buffer, size, flags);
		}
		if (r == -EAGAIN)
			continue;
		if (r < 0)
			return (int)(-r);
		*length = r;
		return 0;
	}
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

int sys_sockname(
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

int sys_peername(
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

int sys_poll(struct pollfd *fds, nfds_t count, int timeout, int *num_events) {
	for (;;) {
		int r = ker::abi::net::poll(fds, count, timeout);
		if (r == -EAGAIN)
			continue;
		if (r < 0)
			return -r;
		*num_events = r;
		return 0;
	}
}

int sys_ioctl(int fd, unsigned long request, void *arg, int *result) {
	(void)fd;
	// Route network ioctls (SIOC* range 0x8900-0x89FF) through net syscall
	if (request >= 0x8900 && request <= 0x89FF) {
		int r = ker::abi::net::ioctl_net(request, arg);
		if (r < 0)
			return -r;
		if (result)
			*result = 0;
		return 0;
	}
	return ENOSYS;
}

int sys_stat(fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf) {
	(void)flags; // AT_SYMLINK_NOFOLLOW not yet implemented
	int r = 0;
	switch (fsfdt) {
		case fsfd_target::path:
			r = ker::abi::vfs::stat_path(path, statbuf);
			break;
		case fsfd_target::fd:
			r = ker::abi::vfs::fstat_fd(fd, statbuf);
			break;
		case fsfd_target::fd_path:
			// For fd_path with AT_FDCWD, treat as path-based stat
			// TODO: Proper fstatat implementation with dirfd
			r = ker::abi::vfs::stat_path(path, statbuf);
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
	// F_DUPFD=0, F_GETFD=1, F_SETFD=2, F_GETFL=3, F_SETFL=4
	if (request == 0 || request == 2 || request == 4) {
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
	struct timespec req;
	req.tv_sec = secs ? *secs : 0;
	req.tv_nsec = nanos ? *nanos : 0;
	struct timespec rem = {0, 0};

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

int sys_uname(struct utsname *buf) {
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

int sys_epoll_ctl(int epfd, int mode, int fd, struct epoll_event *ev) {
	int r = ker::abi::vfs::epoll_ctl_vfs(epfd, mode, fd, ev);
	if (r < 0)
		return -r;
	return 0;
}

int sys_epoll_pwait(
    int epfd, struct epoll_event *ev, int n, int timeout, const sigset_t *sigmask, int *raised
) {
	(void)sigmask; // signal mask not yet supported
	for (;;) {
		int r = ker::abi::vfs::epoll_pwait_vfs(epfd, ev, n, timeout);
		if (r == -EAGAIN)
			continue;
		if (r < 0)
			return -r;
		if (raised)
			*raised = r;
		return 0;
	}
}

} // namespace mlibc

__attribute__((visibility("default"))) extern "C" void frg_panic(const char *mstr) {
	//	mlibc::sys_libc_log("mlibc: Call to frg_panic");
	mlibc::sys_libc_log(mstr);
	mlibc::sys_libc_panic();
}
