#pragma once
#include <stdint.h>

#ifdef __cplusplus
namespace ker::abi {
enum class callnums : uint64_t {
	sys_log,
	futex,
	threading,
	process,
	time,
	vfs,
	net,
	vmem,
	vmem_map,
};
} // namespace ker::abi
#endif /* __cplusplus */
