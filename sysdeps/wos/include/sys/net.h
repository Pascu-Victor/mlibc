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
	SOCKET,
	BIND,
	LISTEN,
	ACCEPT,
	CONNECT,
	SEND,
	RECV,
	CLOSE,
	SENDTO,
	RECVFROM,
	SETSOCKOPT,
	GETSOCKOPT,
	SHUTDOWN,
	GETPEERNAME,
	GETSOCKNAME,
	SELECT,
	POLL,
	IOCTL_NET,
	SET_DEV_CPU_AFFINITY,
	NETCTL_IF_LIST,
	NETCTL_ADDR_LIST,
	NETCTL_ADDR_SET,
	NETCTL_ADDR_DEL,
	NETCTL_LINK_SET,
};

// Syscall register layout:
//   RAX = callnums::net
//   RDI = op
//   RSI = a1, RDX = a2, R8 = a3, R9 = a4, R10 = a5

static inline int socket(int domain, int type, int protocol) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::SOCKET),
	    static_cast<uint64_t>(domain),
	    static_cast<uint64_t>(type),
	    static_cast<uint64_t>(protocol)
	);
	return static_cast<int>((int64_t)r);
}

static inline int bind(int fd, const void *addr, size_t addr_len) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::BIND),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(addr),
	    static_cast<uint64_t>(addr_len)
	);
	return static_cast<int>((int64_t)r);
}

static inline int listen(int fd, int backlog) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::LISTEN),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(backlog)
	);
	return static_cast<int>((int64_t)r);
}

static inline int accept(int fd, void *addr, size_t *addr_len) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::ACCEPT),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(addr),
	    reinterpret_cast<uint64_t>(addr_len)
	);
	return static_cast<int>((int64_t)r);
}

static inline int connect(int fd, const void *addr, size_t addr_len) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::CONNECT),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(addr),
	    static_cast<uint64_t>(addr_len)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t send(int fd, const void *buf, size_t len, int flags) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::SEND),
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
	    static_cast<uint64_t>(ops::RECV),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(buf),
	    static_cast<uint64_t>(len),
	    static_cast<uint64_t>(flags)
	);
	return static_cast<ssize_t>((int64_t)r);
}

static inline int close(int fd) {
	uint64_t r = syscall(
	    ker::abi::callnums::net, static_cast<uint64_t>(ops::CLOSE), static_cast<uint64_t>(fd)
	);
	return static_cast<int>((int64_t)r);
}

static inline ssize_t sendto(int fd, const void *buf, size_t len, int flags, const void *addr) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::SENDTO),
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
	    static_cast<uint64_t>(ops::RECVFROM),
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
	    static_cast<uint64_t>(ops::SETSOCKOPT),
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
	    static_cast<uint64_t>(ops::GETSOCKOPT),
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
	    static_cast<uint64_t>(ops::SHUTDOWN),
	    static_cast<uint64_t>(fd),
	    static_cast<uint64_t>(how)
	);
	return static_cast<int>((int64_t)r);
}

static inline int getpeername(int fd, void *addr, size_t *addr_len) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::GETPEERNAME),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(addr),
	    reinterpret_cast<uint64_t>(addr_len)
	);
	return static_cast<int>((int64_t)r);
}

static inline int getsockname(int fd, void *addr, size_t *addr_len) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::GETSOCKNAME),
	    static_cast<uint64_t>(fd),
	    reinterpret_cast<uint64_t>(addr),
	    reinterpret_cast<uint64_t>(addr_len)
	);
	return static_cast<int>((int64_t)r);
}

static inline int poll(void *fds, size_t nfds, int timeout) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::POLL),
	    reinterpret_cast<uint64_t>(fds),
	    static_cast<uint64_t>(nfds),
	    static_cast<uint64_t>(timeout)
	);
	return static_cast<int>((int64_t)r);
}

static inline int ioctl_net(unsigned long request, void *arg) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::IOCTL_NET),
	    static_cast<uint64_t>(request),
	    reinterpret_cast<uint64_t>(arg)
	);
	return static_cast<int>((int64_t)r);
}

static inline int netctl_if_list(void *out, size_t *count) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::NETCTL_IF_LIST),
	    reinterpret_cast<uint64_t>(out),
	    reinterpret_cast<uint64_t>(count)
	);
	return static_cast<int>((int64_t)r);
}

static inline int netctl_addr_list(void *out, size_t *count) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::NETCTL_ADDR_LIST),
	    reinterpret_cast<uint64_t>(out),
	    reinterpret_cast<uint64_t>(count)
	);
	return static_cast<int>((int64_t)r);
}

static inline int netctl_addr_set(const void *req) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::NETCTL_ADDR_SET),
	    reinterpret_cast<uint64_t>(req)
	);
	return static_cast<int>((int64_t)r);
}

static inline int netctl_addr_del(const void *req) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::NETCTL_ADDR_DEL),
	    reinterpret_cast<uint64_t>(req)
	);
	return static_cast<int>((int64_t)r);
}

static inline int netctl_link_set(const void *req) {
	uint64_t r = syscall(
	    ker::abi::callnums::net,
	    static_cast<uint64_t>(ops::NETCTL_LINK_SET),
	    reinterpret_cast<uint64_t>(req)
	);
	return static_cast<int>((int64_t)r);
}

} // namespace ker::abi::net
