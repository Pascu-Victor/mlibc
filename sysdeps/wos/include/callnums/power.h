#pragma once
#include <stdint.h>

namespace ker::abi::power {
enum class ops : uint64_t {
	REBOOT = 0,
	GET_STATE = 1,
	PREPARE = 2,
};
} // namespace ker::abi::power
