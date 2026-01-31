#pragma once
#include <stdint.h>

#include <bits/size_t.h>
#include <bits/ssize_t.h>
#include <sys/callnums.h>
#include <sys/syscall.h>

namespace ker::abi::net {

// Operation codes for the kernel net syscall dispatcher.
// Must match modules/kern/src/abi/callnums/net.h
enum class ops : uint64_t {
	socket,
	bind,
	listen,
	accept,
	connect,
	send,
	recv,
	close,
	sendto,
	recvfrom,
	setsockopt,
	getsockopt,
	shutdown,
	getpeername,
	getsockname,
	select,
	poll,
	ioctl_net,
};

// Syscall register layout:
//   RAX = callnums::net
//   RDI = op
//   RSI = a1, RDX = a2, R8 = a3, R9 = a4, R10 = a5

static inline int socket(int domain, int type, int protocol) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::socket),
	    static_cast<uint64_t>(domain),
	    static_cast<uint64_t>(type),
	    static_cast<uint64_t>(protocol)
	);
	return static_cast<int>((int64_t)r);
}

static inline int bind(int fd, const void *addr, size_t addr_len) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::bind),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(addr),
	    static_cast<uint64_t>(addr_len)
	);
	return static_cast<int>((int64_t)r);
}

static inline int listen(int fd, int backlog) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::listen),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(backlog)
	);
	return static_cast<int>((int64_t)r);
}

static inline int accept(int fd, void *addr, size_t *addr_len) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::accept),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(addr),
	    reinterpret_cast<uint64_t>(addr_len)
	);
	return static_cast<int>((int64_t)r);
}

static inline int connect(int fd, const void *addr, size_t addr_len) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::connect),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(addr),
	    static_cast<uint64_t>(addr_len)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t send(int fd, const void *buf, size_t len, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::send),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(len),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t recv(int fd, void *buf, size_t len, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::recv),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(len),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline int close(int fd) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::close),
	    static_cast<uint64_t>(fd)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t sendto(int fd, const void *buf, size_t len, int flags, const void *addr) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::sendto),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(len),
	    static_cast<uint64_t>(flags),
	    reinterpret_cast<uint64_t>(addr)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline ssize_t recvfrom(int fd, void *buf, size_t len, int flags, void *addr) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::recvfrom),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(len),
	    static_cast<uint64_t>(flags),
	    reinterpret_cast<uint64_t>(addr)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline int setsockopt(int fd, int level, int optname, const void *optval, size_t optlen) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::setsockopt),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(level),
	    static_cast<uint64_t>(optname),
	    reinterpret_cast<uint64_t>(optval),
	    static_cast<uint64_t>(optlen)
	);
	return static_cast<int>((int64_t)r);
}

static inline int getsockopt(int fd, int level, int optname, void *optval, size_t *optlen) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::getsockopt),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(level),
	    static_cast<uint64_t>(optname),
	    reinterpret_cast<uint64_t>(optval),
	    reinterpret_cast<uint64_t>(optlen)
	);
	return static_cast<int>((int64_t)r);
}

static inline int shutdown(int fd, int how) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::shutdown),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(how)
	);
	return static_cast<int>((int64_t)r);
}

static inline int getpeername(int fd, void *addr, size_t *addr_len) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::getpeername),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(addr),
	    reinterpret_cast<uint64_t>(addr_len)
	);
	return static_cast<int>((int64_t)r);
}

static inline int getsockname(int fd, void *addr, size_t *addr_len) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::getsockname),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(addr),
	    reinterpret_cast<uint64_t>(addr_len)
	);
	return static_cast<int>((int64_t)r);
}

static inline int poll(void *fds, size_t nfds, int timeout) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::poll),
	    reinterpret_cast<uint64_t>(fds),
	    static_cast<uint64_t>(nfds),
	    static_cast<uint64_t>(timeout)
	);
	return static_cast<int>((int64_t)r);
}

static inline int ioctl_net(unsigned long request, void *arg) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::ioctl_net),
	    static_cast<uint64_t>(request),
	    reinterpret_cast<uint64_t>(arg)
	);
	return static_cast<int>((int64_t)r);
}

} // namespace ker::abi::net
