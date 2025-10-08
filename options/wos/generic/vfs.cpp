#include <mlibc/all-sysdeps.hpp>
#include <mlibc/tcb.hpp>

#include <sys/callnums.h>
#include <sys/syscall.h>
#include <sys/vfs.h>

namespace mlibc {

int sys_open(const char *pathname, int flags, mode_t mode, int *fd) {
	(void)mode; // not used in kernel yet
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ker::abi::vfs::ops::open),
	    reinterpret_cast<uint64_t>(pathname),
	    static_cast<uint64_t>(flags)
	);
	if (static_cast<int64_t>(r) < 0)
		return static_cast<int>(-static_cast<int64_t>(r));
	if (fd)
		*fd = static_cast<int>(r);
	return 0;
}

int sys_close(int fd) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ker::abi::vfs::ops::close),
	    static_cast<uint64_t>(fd)
	);
	if (static_cast<int64_t>(r) < 0)
		return static_cast<int>(-static_cast<int64_t>(r));
	return 0;
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ker::abi::vfs::ops::read),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(count)
	);
	if (static_cast<int64_t>(r) < 0)
		return static_cast<int>(-static_cast<int64_t>(r));
	if (bytes_read)
		*bytes_read = static_cast<ssize_t>(r);
	return 0;
}

int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ker::abi::vfs::ops::write),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(count)
	);
	if (static_cast<int64_t>(r) < 0)
		return static_cast<int>(-static_cast<int64_t>(r));
	if (bytes_written)
		*bytes_written = static_cast<ssize_t>(r);
	return 0;
}

int sys_seek(int fd, long offset, int whence, long *new_offset) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ker::abi::vfs::ops::lseek),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(offset),
	    static_cast<uint64_t>(whence)
	);
	if (static_cast<int64_t>(r) < 0)
		return static_cast<int>(-static_cast<int64_t>(r));
	if (new_offset)
		*new_offset = static_cast<long>(r);
	return 0;
}

} // namespace mlibc
