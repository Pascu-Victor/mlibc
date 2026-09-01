#pragma once
#include <stdint.h>

#ifdef __cplusplus
#include <abi-bits/gid_t.h>
#include <abi-bits/mode_t.h>
#include <abi-bits/stat.h>
#include <abi-bits/uid_t.h>
#include <bits/off_t.h>
#include <bits/size_t.h>
#include <bits/ssize_t.h>
#include <callnums/vfs.h>
#include <stddef.h>
#include <sys/callnums.h>
#include <sys/syscall.h>

namespace ker::abi::vfs {

constexpr uint32_t WKI_VFS_ROUTE_LOCAL = 0;
constexpr uint32_t WKI_VFS_ROUTE_HOST = 1;

struct metadata_batch_entry {
	const char *path;
	const char *second_path;
};

struct metadata_batch_result {
	int32_t status;
	uint32_t reserved;
	struct stat statbuf;
};

static_assert(sizeof(metadata_batch_entry) == 16);
static_assert(offsetof(metadata_batch_result, statbuf) == 8);
static_assert(sizeof(metadata_batch_result) == 152);

// Thin syscall veneers (similar to sys/logging.h) so userspace can
// call VFS operations directly until higher-level libc paths are used.
static inline int open(const char *path, int flags, mode_t mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::OPEN),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(flags),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline int openat(int dirfd, const char *path, int flags, mode_t mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::OPENAT),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(flags),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline int close(int fd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::CLOSE), static_cast<uint64_t>(fd)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t read(int fd, void *buf, size_t len) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::READ),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(len)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t write(int fd, const void *buf, size_t len) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::WRITE),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(len)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline off_t lseek(int fd, off_t offset, int whence) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::LSEEK),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(offset),
	    static_cast<uint64_t>(whence)
	);
	return static_cast<off_t>((int64_t)r);
}

static inline bool isatty(int fd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::ISATTY), static_cast<uint64_t>(fd)
	);
	return r != 0;
}

static inline ssize_t read_dir_entries(int fd, void *buffer, size_t max_size) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::READ_DIR_ENTRIES),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buffer),
	    static_cast<uint64_t>(max_size)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline int mount(
    const char *source,
    const char *target,
    const char *fstype,
    unsigned long flags = 0,
    const void *data = nullptr
) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::MOUNT),
	    reinterpret_cast<uint64_t>(source),
	    reinterpret_cast<uint64_t>(target),
	    reinterpret_cast<uint64_t>(fstype),
	    static_cast<uint64_t>(flags),
	    reinterpret_cast<uint64_t>(data)
	);
	return static_cast<int>((int64_t)r);
}

static inline int mkdir(const char *path, int mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::MKDIR),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline int mkdirat(int dirfd, const char *path, int mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::MKDIRAT),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t readlink(const char *path, char *buf, size_t bufsize) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::READLINK),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(bufsize)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t readlinkat(int dirfd, const char *path, char *buf, size_t bufsize) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::READLINKAT),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(bufsize)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline int symlink(const char *target, const char *linkpath) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::SYMLINK),
	    reinterpret_cast<uint64_t>(target),
	    reinterpret_cast<uint64_t>(linkpath)
	);
	return static_cast<int>((int64_t)r);
}

static inline int symlinkat_vfs(const char *target, int dirfd, const char *linkpath) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::SYMLINKAT),
	    reinterpret_cast<uint64_t>(target),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(linkpath)
	);
	return static_cast<int>((int64_t)r);
}

static inline int stat_path(const char *path, void *statbuf) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::STAT),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(statbuf)
	);
	return static_cast<int>((int64_t)r);
}

static inline int lstat_path(const char *path, void *statbuf) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::LSTAT),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(statbuf)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fstat_fd(int fd, void *statbuf) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FSTAT),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(statbuf)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fstat_close_fd(int fd, void *statbuf, int *stat_result) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FSTAT_CLOSE),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(statbuf),
	    reinterpret_cast<uint64_t>(stat_result)
	);
	return static_cast<int>((int64_t)r);
}

// EOPNOTSUPP guarantees that no batch request was attempted and permits a
// scalar fallback. Any other negative return may leave completed results mixed
// with EINPROGRESS entries because effects can precede a response or final
// userspace result copy; callers must not replay mutating entries.
static inline int metadata_batch(
    const metadata_batch_header *header,
    const metadata_batch_entry *entries,
    metadata_batch_result *results
) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::METADATA_BATCH),
	    reinterpret_cast<uint64_t>(header),
	    reinterpret_cast<uint64_t>(entries),
	    reinterpret_cast<uint64_t>(results)
	);
	return static_cast<int>((int64_t)r);
}

static inline int statat_path(int dirfd, const char *path, int flags, void *statbuf) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::STATAT),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(statbuf),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int utimensat_path(int dirfd, const char *path, const void *times, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::UTIMENSAT),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(times),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int umount(const char *target) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::UMOUNT),
	    reinterpret_cast<uint64_t>(target)
	);
	return static_cast<int>((int64_t)r);
}

static inline int dup(int oldfd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::DUP), static_cast<uint64_t>(oldfd)
	);
	return static_cast<int>((int64_t)r);
}

static inline int dup2(int oldfd, int newfd, int flags = 0) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::DUP2),
	    static_cast<uint64_t>(oldfd),
	    static_cast<uint64_t>(newfd),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int getcwd(char *buf, size_t size) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::GETCWD),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(size)
	);
	return static_cast<int>((int64_t)r);
}

static inline int chdir(const char *path) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::CHDIR), reinterpret_cast<uint64_t>(path)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fchdir_vfs(int fd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::FCHDIR), static_cast<uint64_t>(fd)
	);
	return static_cast<int>((int64_t)r);
}

static inline int access(const char *path, int mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::ACCESS),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline int unlink(const char *path) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::UNLINK),
	    reinterpret_cast<uint64_t>(path)
	);
	return static_cast<int>((int64_t)r);
}

static inline int rmdir(const char *path) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::RMDIR), reinterpret_cast<uint64_t>(path)
	);
	return static_cast<int>((int64_t)r);
}

static inline int rename(const char *oldpath, const char *newpath) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::RENAME),
	    reinterpret_cast<uint64_t>(oldpath),
	    reinterpret_cast<uint64_t>(newpath)
	);
	return static_cast<int>((int64_t)r);
}

static inline int chmod(const char *path, mode_t mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::CHMOD),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline int truncate(int fd, off_t length) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::TRUNCATE),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(length)
	);
	return static_cast<int>((int64_t)r);
}

static inline int pipe(int pipefd[2], int flags = 0) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::PIPE),
	    reinterpret_cast<uint64_t>(pipefd),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::PREAD),
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
	    static_cast<uint64_t>(ops::PWRITE),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(count),
	    static_cast<uint64_t>(offset)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t sendfile(int outfd, int infd, off_t *offset, size_t count) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::SENDFILE),
	    static_cast<uint64_t>(outfd),
	    static_cast<uint64_t>(infd),
	    reinterpret_cast<uint64_t>(offset),
	    static_cast<uint64_t>(count)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline int fcntl(int fd, int cmd, uint64_t arg) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FCNTL),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(cmd),
	    arg
	);
	return static_cast<int>((int64_t)r);
}

static inline int fchmod(int fd, mode_t mode) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FCHMOD),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(mode)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fchmodat_vfs(int dirfd, const char *path, mode_t mode, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FCHMODAT),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(mode),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int chown(const char *path, uid_t owner, gid_t group) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::CHOWN),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(owner),
	    static_cast<uint64_t>(group)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fchown(int fd, uid_t owner, gid_t group) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FCHOWN),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(owner),
	    static_cast<uint64_t>(group)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fchownat_vfs(int dirfd, const char *path, uid_t owner, gid_t group, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FCHOWNAT),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(owner),
	    static_cast<uint64_t>(group),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int faccessat(int dirfd, const char *path, int mode, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FACCESSAT),
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
	    static_cast<uint64_t>(ops::UNLINKAT),
	    static_cast<uint64_t>(dirfd),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::RENAMEAT),
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
	    static_cast<uint64_t>(ops::EPOLL_CREATE),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int epoll_ctl_vfs(int epfd, int op, int fd, void *event) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::EPOLL_CTL),
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
	    static_cast<uint64_t>(ops::EPOLL_PWAIT),
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
	    static_cast<uint64_t>(ops::IOCTL),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(cmd),
	    static_cast<uint64_t>(arg)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fsync_vfs(int fd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs, static_cast<uint64_t>(ops::FSYNC), static_cast<uint64_t>(fd)
	);
	return static_cast<int>((int64_t)r);
}

static inline int sync_vfs() {
	uint64_t r = syscall(ker::abi::callnums::vfs, static_cast<uint64_t>(ops::SYNC));
	return static_cast<int>((int64_t)r);
}

static inline int link_vfs(const char *oldpath, const char *newpath) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::LINK),
	    reinterpret_cast<uint64_t>(oldpath),
	    reinterpret_cast<uint64_t>(newpath)
	);
	return static_cast<int>((int64_t)r);
}

static inline int
linkat_vfs(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::LINKAT),
	    static_cast<uint64_t>(olddirfd),
	    reinterpret_cast<uint64_t>(oldpath),
	    static_cast<uint64_t>(newdirfd),
	    reinterpret_cast<uint64_t>(newpath),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int wki_rule_add_vfs(const char *prefix, uint32_t route) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::WKI_RULE_ADD),
	    reinterpret_cast<uint64_t>(prefix),
	    static_cast<uint64_t>(route)
	);
	return static_cast<int>((int64_t)r);
}

static inline int
wki_rule_get_vfs(uint32_t index, char *prefix_buf, size_t prefix_buf_size, uint32_t *route_out) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::WKI_RULE_GET),
	    static_cast<uint64_t>(index),
	    reinterpret_cast<uint64_t>(prefix_buf),
	    static_cast<uint64_t>(prefix_buf_size),
	    reinterpret_cast<uint64_t>(route_out)
	);
	return static_cast<int>((int64_t)r);
}

static inline int wki_rule_clear_vfs() {
	uint64_t r = syscall(ker::abi::callnums::vfs, static_cast<uint64_t>(ops::WKI_RULE_CLEAR));
	return static_cast<int>((int64_t)r);
}

static inline int wki_rule_get_default_vfs(
    uint32_t index, char *prefix_buf, size_t prefix_buf_size, uint32_t *route_out
) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::WKI_RULE_GET_DEFAULT),
	    static_cast<uint64_t>(index),
	    reinterpret_cast<uint64_t>(prefix_buf),
	    static_cast<uint64_t>(prefix_buf_size),
	    reinterpret_cast<uint64_t>(route_out)
	);
	return static_cast<int>((int64_t)r);
}

static inline int pivot_root_vfs(const char *new_root, const char *put_old) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::PIVOT_ROOT),
	    reinterpret_cast<uint64_t>(new_root),
	    reinterpret_cast<uint64_t>(put_old)
	);
	return static_cast<int>((int64_t)r);
}

static inline int statvfs_path(const char *path, void *buf) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::STATVFS),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(buf)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fstatvfs_fd(int fd, void *buf) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FSTATVFS),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf)
	);
	return static_cast<int>((int64_t)r);
}

static inline int realpath(const char *path, char *buf, size_t bufsize) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::REALPATH),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(bufsize)
	);
	return static_cast<int>((int64_t)r);
}

static inline int
setxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::SETXATTR),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(name),
	    reinterpret_cast<uint64_t>(value),
	    static_cast<uint64_t>(size),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int
lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::LSETXATTR),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(name),
	    reinterpret_cast<uint64_t>(value),
	    static_cast<uint64_t>(size),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fsetxattr(int fd, const char *name, const void *value, size_t size, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FSETXATTR),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(name),
	    reinterpret_cast<uint64_t>(value),
	    static_cast<uint64_t>(size),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t getxattr(const char *path, const char *name, void *value, size_t size) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::GETXATTR),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(name),
	    reinterpret_cast<uint64_t>(value),
	    static_cast<uint64_t>(size)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::LGETXATTR),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(name),
	    reinterpret_cast<uint64_t>(value),
	    static_cast<uint64_t>(size)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t fgetxattr(int fd, const char *name, void *value, size_t size) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FGETXATTR),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(name),
	    reinterpret_cast<uint64_t>(value),
	    static_cast<uint64_t>(size)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t listxattr(const char *path, char *list, size_t size) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::LISTXATTR),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(list),
	    static_cast<uint64_t>(size)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t llistxattr(const char *path, char *list, size_t size) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::LLISTXATTR),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(list),
	    static_cast<uint64_t>(size)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t flistxattr(int fd, char *list, size_t size) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FLISTXATTR),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(list),
	    static_cast<uint64_t>(size)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline int removexattr(const char *path, const char *name) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::REMOVEXATTR),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(name)
	);
	return static_cast<int>((int64_t)r);
}

static inline int lremovexattr(const char *path, const char *name) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::LREMOVEXATTR),
	    reinterpret_cast<uint64_t>(path),
	    reinterpret_cast<uint64_t>(name)
	);
	return static_cast<int>((int64_t)r);
}

static inline int fremovexattr(int fd, const char *name) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ops::FREMOVEXATTR),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(name)
	);
	return static_cast<int>((int64_t)r);
}

} // namespace ker::abi::vfs
#endif /* __cplusplus */
