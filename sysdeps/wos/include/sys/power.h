#pragma once

#include <callnums/power.h>
#include <stdint.h>
#include <sys/callnums.h>
#include <sys/syscall.h>

namespace ker::abi::power {

static inline int64_t reboot(uint64_t cmd) {
	return static_cast<int64_t>(
	    syscall(ker::abi::callnums::power, static_cast<uint64_t>(ops::REBOOT), cmd)
	);
}

static inline int64_t get_state() {
	return static_cast<int64_t>(
	    syscall(ker::abi::callnums::power, static_cast<uint64_t>(ops::GET_STATE))
	);
}

static inline int64_t prepare_shutdown() {
	return static_cast<int64_t>(
	    syscall(ker::abi::callnums::power, static_cast<uint64_t>(ops::PREPARE))
	);
}

} // namespace ker::abi::power
