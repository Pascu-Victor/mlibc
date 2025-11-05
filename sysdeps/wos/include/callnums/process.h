#pragma once
#include <stdint.h>

namespace ker::abi::process {
enum class procmgmt_ops : uint64_t { exit, exec };
} // namespace ker::abi::process
