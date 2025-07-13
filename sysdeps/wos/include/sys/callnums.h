#pragma once
#include <bits/syscall_aliases.h>
#include <stdint.h>
#include <syscallnos.h>

namespace ker::abi {
enum class callnums : uint64_t { sys_log, futex, threadInfo, process };
} // namespace ker::abi
