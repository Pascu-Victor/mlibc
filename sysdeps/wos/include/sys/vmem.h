#pragma once
#include <stdint.h>

#ifdef __cplusplus
#include <sys/syscall.h>

namespace ker {

namespace abi::vmem {

// Operations
enum class ops : uint64_t {
	anon_allocate = 0,
	anon_free = 1,
	protect = 2,
	mremap = 3,
	msync = 4,
};

} // namespace abi::vmem

// Userspace wrapper functions
namespace vmem {

// Allocate anonymous memory
// Returns virtual address on success, or negative error code on failure
static inline int64_t
allocate(void **addr, uint64_t size, uint64_t prot, uint64_t flags, void *hint = nullptr) {
	uint64_t result = syscall(
	    abi::callnums::vmem,
	    static_cast<uint64_t>(abi::vmem::ops::anon_allocate),
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
	    static_cast<uint64_t>(abi::vmem::ops::anon_free),
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
	    static_cast<uint64_t>(abi::vmem::ops::protect),
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
	    static_cast<uint64_t>(abi::vmem::ops::mremap),
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
	    static_cast<uint64_t>(abi::vmem::ops::msync),
	    reinterpret_cast<uint64_t>(addr),
	    size,
	    flags,
	    0 // unused
	);

	return static_cast<int64_t>(result);
}

} // namespace vmem
} // namespace ker

#endif // __cplusplus
