#pragma once
#include <stdint.h>

namespace ker::abi::shm {
enum class ops : uint64_t {
	get,
	attach,
	detach,
	ctl,
};
} // namespace ker::abi::shm
