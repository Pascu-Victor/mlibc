#pragma once
#include <stdint.h>

namespace ker::abi::vmem {
enum class ops : uint64_t {
	ANON_ALLOCATE = 0,
	ANON_FREE = 1,
	PROTECT = 2,
	MREMAP = 3,
	MSYNC = 4,
	SWAPON = 5,
	SWAPOFF = 6,
	anon_allocate = ANON_ALLOCATE,
	anon_free = ANON_FREE,
	protect = PROTECT,
	mremap = MREMAP,
	msync = MSYNC,
	swapon = SWAPON,
	swapoff = SWAPOFF,
};
} // namespace ker::abi::vmem
