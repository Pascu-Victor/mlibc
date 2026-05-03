#pragma once
#include <callnums/sys_log.h>
#include <sys/callnums.h>
#include <sys/syscall.h>

namespace ker::logging {

uint64_t log(const char *str, uint64_t len, abi::sys_log::sys_log_device device);

uint64_t logLine(const char *str, uint64_t len, abi::sys_log::sys_log_device device);

uint64_t logEx(const char *module, abi::sys_log::sys_log_level level, const char *str, uint64_t len);

} // namespace ker::logging
