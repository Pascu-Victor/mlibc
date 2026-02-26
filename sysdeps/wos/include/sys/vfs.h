#pragma once
#include <stdint.h>

#ifdef __cplusplus
#include <abi-bits/mode_t.h>
#include <bits/off_t.h>
#include <bits/size_t.h>
#include <bits/ssize_t.h>
#include <sys/callnums.h>
#include <sys/syscall.h>

namespace ker::abi::vfs {

// Operation codes for the kernel VFS syscall dispatcher
enum class ops : uint64_t {
	open,
	read,
	write,
	close,
	lseek,
	isatty,
	read_dir_entries,
	mount,
	mkdir,
	readlink,
	symlink,
	sendfile,
	stat,
	fstat,
	umount,
	dup,
	dup2,
	getcwd,
	chdir,
	access,
	unlink,
	rmdir,
	rename,
	chmod,
	truncate,
	pipe,
	pread,
	pwrite,
	fcntl,
	fchmod,
	chown,
	fchown,
	faccessat,
	unlinkat,
	renameat,
	epoll_create,
	epoll_ctl,
	epoll_pwait,
	ioctl,
	fsync,
	link,
};

// Thin syscall veneers (similar to sys/logging.h) so userspace can
// call VFS operations directly until higher-level libc paths are used.
static inline int open(const char *path, int flags, mode_t mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::open),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(flags),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline int close(int fd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::close), static_cast<uint64_t>(fd)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t read(int fd, void *buf, size_t len) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::read),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(len)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t write(int fd, const void *buf, size_t len) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::write),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(len)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline off_t lseek(int fd, off_t offset, int whence) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::lseek),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(offset),
	    static_cast<uint64_t>(whence)
	);
	return static_cast<off_t>((int64_t)r);
}

static inline bool isatty(int fd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::isatty), static_cast<uint64_t>(fd)
	);
	return r != 0;
}

static inline ssize_t read_dir_entries(int fd, void *buffer, size_t max_size) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::read_dir_entries),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buffer),
	    static_cast<uint64_t>(max_size)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline int mount(const char *source, const char *target, const char *fstype) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::mount),
	    reinterpret_cast<uint64_t>(source),
	    reinterpret_cast<uint64_t>(target),
	    reinterpret_cast<uint64_t>(fstype)
	);
	return static_cast<int>((int64_t)r);
}

static inline int mkdir(const char *path, int mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::mkdir),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t readlink(const char *path, char *buf, size_t bufsize) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::readlink),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(bufsize)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline int symlink(const char *target, const char *linkpath) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::symlink),
	    reinterpret_cast<uint64_t>(target),
	    reinterpret_cast<uint64_t>(linkpath)
	);
	return static_cast<int>((int64_t)r);
}

static inline int stat_path(const char *path, void *statbuf) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::stat),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(statbuf)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fstat_fd(int fd, void *statbuf) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::fstat),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(statbuf)
	);
	return static_cast<int>((int64_t)r);
}

static inline int umount(const char *target) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::umount),
	    reinterpret_cast<uint64_t>(target)
	);
	return static_cast<int>((int64_t)r);
}

static inline int dup(int oldfd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::dup), static_cast<uint64_t>(oldfd)
	);
	return static_cast<int>((int64_t)r);
}

static inline int dup2(int oldfd, int newfd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::dup2),
	    static_cast<uint64_t>(oldfd),
	    static_cast<uint64_t>(newfd)
	);
	return static_cast<int>((int64_t)r);
}

static inline int getcwd(char *buf, size_t size) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::getcwd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(size)
	);
	return static_cast<int>((int64_t)r);
}

static inline int chdir(const char *path) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::chdir), reinterpret_cast<uint64_t>(path)
	);
	return static_cast<int>((int64_t)r);
}

static inline int access(const char *path, int mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::access),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline int unlink(const char *path) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::unlink),
	    reinterpret_cast<uint64_t>(path)
	);
	return static_cast<int>((int64_t)r);
}

static inline int rmdir(const char *path) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::rmdir), reinterpret_cast<uint64_t>(path)
	);
	return static_cast<int>((int64_t)r);
}

static inline int rename(const char *oldpath, const char *newpath) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::rename),
	    reinterpret_cast<uint64_t>(oldpath),
	    reinterpret_cast<uint64_t>(newpath)
	);
	return static_cast<int>((int64_t)r);
}

static inline int chmod(const char *path, mode_t mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::chmod),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline int truncate(int fd, off_t length) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::truncate),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(length)
	);
	return static_cast<int>((int64_t)r);
}

static inline int pipe(int pipefd[2]) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::pipe),
	    reinterpret_cast<uint64_t>(pipefd)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::pread),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(count),
	    static_cast<uint64_t>(offset)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::pwrite),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(count),
	    static_cast<uint64_t>(offset)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline int fcntl(int fd, int cmd, uint64_t arg) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::fcntl),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(cmd),
	    arg
	);
	return static_cast<int>((int64_t)r);
}

static inline int fchmod(int fd, mode_t mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::fchmod),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline int chown(const char *path, uid_t owner, gid_t group) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::chown),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(owner),
	    static_cast<uint64_t>(group)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fchown(int fd, uid_t owner, gid_t group) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::fchown),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(owner),
	    static_cast<uint64_t>(group)
	);
	return static_cast<int>((int64_t)r);
}

static inline int faccessat(int dirfd, const char *path, int mode, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::faccessat),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(mode),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int unlinkat(int dirfd, const char *path, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::unlinkat),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::renameat),
	    static_cast<uint64_t>(olddirfd),
	    reinterpret_cast<uint64_t>(oldpath),
	    static_cast<uint64_t>(newdirfd),
	    reinterpret_cast<uint64_t>(newpath)
	);
	return static_cast<int>((int64_t)r);
}

static inline int epoll_create_vfs(int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::epoll_create),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int epoll_ctl_vfs(int epfd, int op, int fd, void *event) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::epoll_ctl),
	    static_cast<uint64_t>(epfd),
	    static_cast<uint64_t>(op),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(event)
	);
	return static_cast<int>((int64_t)r);
}

static inline int epoll_pwait_vfs(int epfd, void *events, int maxevents, int timeout) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::epoll_pwait),
	    static_cast<uint64_t>(epfd),
	    reinterpret_cast<uint64_t>(events),
	    static_cast<uint64_t>(maxevents),
	    static_cast<uint64_t>(timeout)
	);
	return static_cast<int>((int64_t)r);
}

static inline int ioctl_vfs(int fd, unsigned long cmd, unsigned long arg) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::ioctl),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(cmd),
	    static_cast<uint64_t>(arg)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fsync_vfs(int fd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::fsync), static_cast<uint64_t>(fd)
	);
	return static_cast<int>((int64_t)r);
}

static inline int link_vfs(const char *oldpath, const char *newpath) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::link),
	    reinterpret_cast<uint64_t>(oldpath),
	    reinterpret_cast<uint64_t>(newpath)
	);
	return static_cast<int>((int64_t)r);
}

} // namespace ker::abi::vfs
#endif /* __cplusplus */
