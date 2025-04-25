#pragma once
#include <callnums/process.h>
#include <sys/callnums.h>
#include <sys/syscall.h>

namespace ker::process {

void exit(uint64_t status) {
	syscall(abi::callnums::process, (uint64_t)abi::process::procmgmt_ops::exit, status);
}

} // namespace ker::process
