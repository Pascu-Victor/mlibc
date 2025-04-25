
#include <errno.h>
#include <string.h>
#include <sys/utsname.h>

#include <bits/ensure.h>
#include <internal-config.h>
#include <mlibc/debug.hpp>
#include <mlibc/posix-sysdeps.hpp>

int uname(struct utsname *p) {
	if (p == nullptr) {
		errno = EFAULT;
		return -1;
	}

	MLIBC_CHECK_OR_ENOSYS(mlibc::sys_uname, -1);
	if (int e = mlibc::sys_uname(p); e) {
		errno = e;
		return -1;
	}
	return 0;
}
