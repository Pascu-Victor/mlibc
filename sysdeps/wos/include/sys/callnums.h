#pragma once
#include <stdint.h>

namespace ker::abi {
enum class callnums : uint64_t { sys_log, futex, threadInfo, process };
} // namespace ker::abi
