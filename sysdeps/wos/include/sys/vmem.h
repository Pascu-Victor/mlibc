#pragma once
// Use the matching installed ABI even when a kernel navigation include path
// precedes this sysroot. Its similarly named header also declares mmap flags.
#include "../callnums/vmem.h"
#include <stdint.h>

#ifdef __cplusplus
#include <sys/numa.h>
#include <sys/syscall.h>

namespace ker {

// Userspace wrapper functions
namespace vmem {

// Allocate anonymous memory
// Returns virtual address on success, or negative error code on failure
static inline int64_t
allocate(void **addr, uint64_t size, uint64_t prot, uint64_t flags, void *hint = nullptr) {
	uint64_t result = syscall(
	    abi::callnums::vmem,
	    static_cast<uint64_t>(abi::vmem::ops::ANON_ALLOCATE),
	    reinterpret_cast<uint64_t>(hint),
	    size,
	    prot,
	    flags
	);

	// Check if result is an error (negative when cast to int64_t)
	int64_t signed_result = static_cast<int64_t>(result);
	if (signed_result < 0) {
		return signed_result; // Return error code
	}

	*addr = reinterpret_cast<void *>(result);
	return 0; // Success
}

// Free mapped memory
// Returns 0 on success, or negative error code on failure
static inline int64_t free(void *addr, uint64_t size) {
	uint64_t result = syscall(
	    abi::callnums::vmem,
	    static_cast<uint64_t>(abi::vmem::ops::ANON_FREE),
	    reinterpret_cast<uint64_t>(addr),
	    size,
	    0, // unused
	    0  // unused
	);

	return static_cast<int64_t>(result);
}

// Map memory
static inline int64_t
map(void **addr,
    uint64_t size,
    uint64_t prot,
    uint64_t flags,
    int fd,
    uint64_t offset,
    void *hint = nullptr) {
	uint64_t result = syscall(
	    abi::callnums::vmem_map,
	    reinterpret_cast<uint64_t>(hint),
	    size,
	    prot,
	    flags,
	    static_cast<uint64_t>(fd),
	    offset
	);

	// Check if result is an error (negative when cast to int64_t)
	auto signed_result = static_cast<int64_t>(result);
	if (signed_result < 0) {
		return signed_result; // Return error code
	}

	*addr = reinterpret_cast<void *>(result); // NOLINT

	return 0; // Success
}

// Change memory protection
// Returns 0 on success, or negative error code on failure
static inline int64_t protect(void *addr, uint64_t size, uint64_t prot) {
	uint64_t result = syscall(
	    abi::callnums::vmem,
	    static_cast<uint64_t>(abi::vmem::ops::PROTECT),
	    reinterpret_cast<uint64_t>(addr),
	    size,
	    prot,
	    0 // unused
	);

	return static_cast<int64_t>(result);
}

static inline int64_t
remap(void **addr, void *old_addr, uint64_t old_size, uint64_t new_size, uint64_t flags) {
	uint64_t result = syscall(
	    abi::callnums::vmem,
	    static_cast<uint64_t>(abi::vmem::ops::MREMAP),
	    reinterpret_cast<uint64_t>(old_addr),
	    old_size,
	    new_size,
	    flags
	);

	int64_t signed_result = static_cast<int64_t>(result);
	if (signed_result < 0) {
		return signed_result;
	}

	*addr = reinterpret_cast<void *>(result);
	return 0;
}

static inline int64_t sync(void *addr, uint64_t size, uint64_t flags) {
	uint64_t result = syscall(
	    abi::callnums::vmem,
	    static_cast<uint64_t>(abi::vmem::ops::MSYNC),
	    reinterpret_cast<uint64_t>(addr),
	    size,
	    flags,
	    0 // unused
	);

	return static_cast<int64_t>(result);
}

static inline int64_t swapon(const char *path, int flags) {
	uint64_t result = syscall(
	    abi::callnums::vmem,
	    static_cast<uint64_t>(abi::vmem::ops::SWAPON),
	    reinterpret_cast<uint64_t>(path),
	    static_cast<uint64_t>(flags),
	    0,
	    0
	);

	return static_cast<int64_t>(result);
}

static inline int64_t swapoff(const char *path) {
	uint64_t result = syscall(
	    abi::callnums::vmem,
	    static_cast<uint64_t>(abi::vmem::ops::SWAPOFF),
	    reinterpret_cast<uint64_t>(path),
	    0,
	    0,
	    0
	);

	return static_cast<int64_t>(result);
}

// Fixed-layout local NUMA observation. Returns zero or negative raw errno.
// Prefault request/output buffers and keep them disjoint from measured pages;
// control transport may fault normally, while PAGE_NODES never faults payload.
static inline int64_t query_numa(const abi::vmem::numa::QueryRequest &request) {
	return static_cast<int64_t>(syscall(
	    abi::callnums::vmem,
	    static_cast<uint64_t>(abi::vmem::ops::QUERY_NUMA),
	    reinterpret_cast<uint64_t>(&request), sizeof(request), 0, 0
	));
}

// Only LOCAL with nodes=0 is currently supported. Non-LOCAL returns -ENOTSUP.
static inline int64_t set_memory_policy(const abi::vmem::numa::PolicyRequest &request) {
	return static_cast<int64_t>(syscall(
	    abi::callnums::vmem,
	    static_cast<uint64_t>(abi::vmem::ops::SET_MEMORY_POLICY),
	    reinterpret_cast<uint64_t>(&request), sizeof(request), 0, 0
	));
}

} // namespace vmem
} // namespace ker

#endif // __cplusplus
