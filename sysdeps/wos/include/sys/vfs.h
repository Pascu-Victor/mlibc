#pragma once
#include <stdint.h>

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

} // namespace ker::abi::vfs
