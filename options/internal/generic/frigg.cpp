
#include <bits/ensure.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>

__attribute__((visibility("default"))) extern "C" void frg_panic(const char *mstr) {
	//	mlibc::sysdep<LibcLog>("mlibc: Call to frg_panic");
	mlibc::sysdep<LibcLog>(mstr);
	mlibc::sysdep<LibcPanic>();
}

__attribute__((visibility("default"))) extern "C" void frg_log(const char *mstr) {
	mlibc::sysdep<LibcLog>(mstr);
}
