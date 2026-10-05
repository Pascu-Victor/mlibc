#ifndef MLIBC_FRIGG_ALLOC
#define MLIBC_FRIGG_ALLOC

#include <bits/ensure.h>
#include <frg/slab.hpp>
#include <internal-config.h>
#include <mlibc/lock.hpp>

#if !MLIBC_DEBUG_ALLOCATOR

struct VirtualAllocator {
public:
	// Link virgin free objects only as they are allocated, so constructing a slab
	// does not fault in every page during rtld and libc startup. Keep the slab
	// buckets through 16 KiB and the existing 64 KiB capacity;
	// larger allocations use frg's direct large-object path. For that largest
	// bucket, 64 KiB leaves one frame-sized object, the returned object, and two
	// spare objects, so a newly constructed slab can always join the partial tree.
	static constexpr size_t num_buckets = 12;
	static constexpr size_t slabsize = 64 * 1024;
	static constexpr size_t sb_size = slabsize;
	static constexpr bool lazy_freelist = true;

	uintptr_t map(size_t length);

	void unmap(uintptr_t address, size_t length);
};

typedef frg::slab_pool<VirtualAllocator, FutexLock> MemoryPool;

typedef frg::slab_allocator<VirtualAllocator, FutexLock> MemoryAllocator;

MemoryAllocator &getAllocator();

#else

struct MemoryAllocator {
	void *allocate(size_t size);
	void free(void *ptr);
	void deallocate(void *ptr, size_t size);
	void *reallocate(void *ptr, size_t size);
	size_t get_size(void *ptr);
};

MemoryAllocator &getAllocator();

#endif // !MLIBC_DEBUG_ALLOCATOR

#endif // MLIBC_FRIGG_ALLOC
