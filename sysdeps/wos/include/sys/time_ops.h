// User-space definition of time syscall operation codes (must match kernel)
#pragma once

namespace ker::abi {
enum class sys_time_ops : uint64_t {
	gettimeofday = 0,
	clock_gettime = 1,
	nanosleep = 2,
};
} // namespace ker::abi
