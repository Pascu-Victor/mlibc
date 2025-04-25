#pragma once
#include <stdint.h>

namespace ker::abi::multiproc {
enum class threadInfoOps : uint64_t {
	currentThreadId,
	nativeThreadCount,
};

} // namespace ker::abi::multiproc
