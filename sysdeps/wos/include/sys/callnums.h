#pragma once
#include <stdint.h>

namespace ker::abi {
enum class callnums : uint64_t { sys_log, futex, threading, process, time, vfs, net };
} // namespace ker::abi
