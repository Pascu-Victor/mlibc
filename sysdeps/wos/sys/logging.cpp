#include <sys/logging.h>
namespace ker::logging {

uint64_t log(const char *str, uint64_t len, abi::sys_log::sys_log_device device) {
	return syscall(
	    ker::abi::callnums::sys_log,
	    (uint64_t)abi::sys_log::sys_log_ops::LOG,
	    (uint64_t)str,
	    len,
	    (uint64_t)device
	);
}

uint64_t logLine(const char *str, uint64_t len, abi::sys_log::sys_log_device device) {
	return syscall(
	    ker::abi::callnums::sys_log,
	    (uint64_t)abi::sys_log::sys_log_ops::LOG_LINE,
	    (uint64_t)str,
	    len,
	    (uint64_t)device
	);
}

uint64_t
logEx(const char *module, abi::sys_log::sys_log_level level, const char *str, uint64_t len) {
	return syscall(
	    ker::abi::callnums::sys_log,
	    (uint64_t)abi::sys_log::sys_log_ops::LOG_EX,
	    (uint64_t)str,
	    len,
	    (uint64_t)level,
	    (uint64_t)module
	);
}

uint64_t beginLogBlock() {
	return syscall(
	    ker::abi::callnums::sys_log, (uint64_t)abi::sys_log::sys_log_ops::LOG_BLOCK_BEGIN
	);
}

uint64_t endLogBlock(uint64_t cookie) {
	return syscall(
	    ker::abi::callnums::sys_log,
	    (uint64_t)abi::sys_log::sys_log_ops::LOG_BLOCK_END,
	    0,
	    0,
	    0,
	    0,
	    cookie
	);
}

uint64_t
logBlock(uint64_t cookie, const char *str, uint64_t len, abi::sys_log::sys_log_device device) {
	return syscall(
	    ker::abi::callnums::sys_log,
	    (uint64_t)abi::sys_log::sys_log_ops::LOG,
	    (uint64_t)str,
	    len,
	    (uint64_t)device,
	    0,
	    cookie
	);
}

uint64_t
logLineBlock(uint64_t cookie, const char *str, uint64_t len, abi::sys_log::sys_log_device device) {
	return syscall(
	    ker::abi::callnums::sys_log,
	    (uint64_t)abi::sys_log::sys_log_ops::LOG_LINE,
	    (uint64_t)str,
	    len,
	    (uint64_t)device,
	    0,
	    cookie
	);
}

uint64_t logExBlock(
    uint64_t cookie,
    const char *module,
    abi::sys_log::sys_log_level level,
    const char *str,
    uint64_t len
) {
	return syscall(
	    ker::abi::callnums::sys_log,
	    (uint64_t)abi::sys_log::sys_log_ops::LOG_EX,
	    (uint64_t)str,
	    len,
	    (uint64_t)level,
	    (uint64_t)module,
	    cookie
	);
}

} // namespace ker::logging
