#include <mlibc/all-sysdeps.hpp>
#include <mlibc/tcb.hpp>

#include <errno.h>
#include <sys/callnums.h>
#include <sys/syscall.h>
#include <sys/uio.h>
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
	// WOS_ERESTARTSYS (512) is a kernel-internal code meaning "device not ready,
	// yield and retry" (e.g. PTY with no data).  We spin on this code so
	// blocking device reads work.  True EAGAIN (11) passes through to the
	// application so non-blocking I/O works correctly.
	static constexpr int64_t WOS_ERESTARTSYS = 512;
	for (;;) {
		uint64_t r = syscall(
		    ker::abi::callnums::vfs,
		    static_cast<uint64_t>(ker::abi::vfs::ops::read),
		    static_cast<uint64_t>(fd),
		    reinterpret_cast<uint64_t>(buf),
		    static_cast<uint64_t>(count),
		    reinterpret_cast<uint64_t>(bytes_read)
		);
		if (static_cast<int64_t>(r) == -WOS_ERESTARTSYS)
			continue; // Retry: device has no data yet (e.g. PTY with empty buffer)
		if (static_cast<int64_t>(r) < 0)
			return static_cast<int>(-static_cast<int64_t>(r));
		if (bytes_read)
			*bytes_read = static_cast<ssize_t>(r);
		return 0;
	}
}

int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
	static constexpr int64_t WOS_ERESTARTSYS = 512;
	for (;;) {
		uint64_t r = syscall(
		    ker::abi::callnums::vfs,
		    static_cast<uint64_t>(ker::abi::vfs::ops::write),
		    static_cast<uint64_t>(fd),
		    reinterpret_cast<uint64_t>(buf),
		    static_cast<uint64_t>(count),
		    reinterpret_cast<uint64_t>(bytes_written)
		);
		if (static_cast<int64_t>(r) == -WOS_ERESTARTSYS)
			continue;
		if (static_cast<int64_t>(r) < 0)
			return static_cast<int>(-static_cast<int64_t>(r));
		if (bytes_written)
			*bytes_written = static_cast<ssize_t>(r);
		return 0;
	}
}

int sys_seek(int fd, long offset, int whence, long *new_offset) {
	uint64_t r = syscall(
	    ker::abi::callnums::vfs,
	    static_cast<uint64_t>(ker::abi::vfs::ops::lseek),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(offset),
	    static_cast<uint64_t>(whence),
	    reinterpret_cast<uint64_t>(new_offset)
	);
	if (static_cast<int64_t>(r) < 0)
		return static_cast<int>(-static_cast<int64_t>(r));
	if (new_offset)
		*new_offset = static_cast<long>(r);
	return 0;
}

int sys_sendfile(int outfd, int infd, off_t *offset, size_t count, ssize_t *out) {
	static constexpr int64_t WOS_ERESTARTSYS = 512;
	uint64_t r;
	for (;;) {
		r = syscall(
		    ker::abi::callnums::vfs,
		    static_cast<uint64_t>(ker::abi::vfs::ops::sendfile),
		    static_cast<uint64_t>(outfd),
		    static_cast<uint64_t>(infd),
		    reinterpret_cast<uint64_t>(offset),
		    static_cast<uint64_t>(count)
		);
		if (static_cast<int64_t>(r) == -WOS_ERESTARTSYS)
			continue;
		break;
	}
	if (static_cast<int64_t>(r) < 0)
		return static_cast<int>(-static_cast<int64_t>(r));
	if (out)
		*out = static_cast<ssize_t>(r);
	return 0;
}

int sys_writev(int fd, const struct iovec *iovs, int iovc, ssize_t *bytes_written) {
	ssize_t total = 0;
	static constexpr int64_t WOS_ERESTARTSYS = 512;
	for (int i = 0; i < iovc; i++) {
		if (iovs[i].iov_len == 0)
			continue;
		uint64_t r;
		for (;;) {
			r = syscall(
			    ker::abi::callnums::vfs,
			    static_cast<uint64_t>(ker::abi::vfs::ops::write),
			    static_cast<uint64_t>(fd),
			    reinterpret_cast<uint64_t>(iovs[i].iov_base),
			    static_cast<uint64_t>(iovs[i].iov_len),
			    0ULL
			);
			if (static_cast<int64_t>(r) != -WOS_ERESTARTSYS)
				break;
		}
		if (static_cast<int64_t>(r) < 0) {
			if (total > 0)
				break; // partial write: return what we have
			if (bytes_written)
				*bytes_written = 0;
			return static_cast<int>(-static_cast<int64_t>(r));
		}
		total += static_cast<ssize_t>(r);
		if (static_cast<size_t>(r) < iovs[i].iov_len)
			break; // short write
	}
	if (bytes_written)
		*bytes_written = total;
	return 0;
}

int sys_readv(int fd, const struct iovec *iovs, int iovc, ssize_t *bytes_read) {
	ssize_t total = 0;
	static constexpr int64_t WOS_ERESTARTSYS = 512;
	for (int i = 0; i < iovc; i++) {
		if (iovs[i].iov_len == 0)
			continue;
		uint64_t r;
		for (;;) {
			r = syscall(
			    ker::abi::callnums::vfs,
			    static_cast<uint64_t>(ker::abi::vfs::ops::read),
			    static_cast<uint64_t>(fd),
			    reinterpret_cast<uint64_t>(iovs[i].iov_base),
			    static_cast<uint64_t>(iovs[i].iov_len),
			    0ULL
			);
			if (static_cast<int64_t>(r) != -WOS_ERESTARTSYS)
				break;
		}
		if (static_cast<int64_t>(r) < 0) {
			if (total > 0)
				break;
			if (bytes_read)
				*bytes_read = 0;
			return static_cast<int>(-static_cast<int64_t>(r));
		}
		total += static_cast<ssize_t>(r);
		if (r == 0 || static_cast<size_t>(r) < iovs[i].iov_len)
			break; // EOF or short read
	}
	if (bytes_read)
		*bytes_read = total;
	return 0;
}

} // namespace mlibc
