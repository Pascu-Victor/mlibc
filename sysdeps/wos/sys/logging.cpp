#include <sys/logging.h>
namespace ker::logging {

uint64_t log(const char *str, uint64_t len, abi::sys_log::sys_log_device device) {
	return syscall(
	    ker::abi::callnums::sys_log,
	    (uint64_t)abi::sys_log::sys_log_ops::log,
	    (uint64_t)str,
	    len,
	    (uint64_t)device
	);
}

uint64_t logLine(const char *str, uint64_t len, abi::sys_log::sys_log_device device) {
	return syscall(
	    ker::abi::callnums::sys_log,
	    (uint64_t)abi::sys_log::sys_log_ops::logLine,
	    (uint64_t)str,
	    len,
	    (uint64_t)device
	);
}

} // namespace ker::logging
