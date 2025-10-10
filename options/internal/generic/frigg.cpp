
#include <bits/ensure.h>
#include <mlibc/debug.hpp>
#include <mlibc/internal-sysdeps.hpp>

__attribute__((visibility("default"))) extern "C" void frg_panic(const char *mstr) {
	//	mlibc::sys_libc_log("mlibc: Call to frg_panic");
	mlibc::sys_libc_log(mstr);
	mlibc::sys_libc_panic();
}

__attribute__((visibility("default"))) extern "C" void frg_log(const char *mstr) {
	mlibc::sys_libc_log(mstr);
}
