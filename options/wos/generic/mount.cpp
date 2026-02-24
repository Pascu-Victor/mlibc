#include <errno.h>
#include <sys/mount.h>

#include <bits/ensure.h>
#include <mlibc/wos-sysdeps.hpp>

int mount(
    const char *source,
    const char *target,
    const char *fstype,
    unsigned long flags,
    const void *data
) {
	MLIBC_CHECK_OR_ENOSYS(mlibc::sys_mount, -1);
	if (int e = mlibc::sys_mount(source, target, fstype, flags, data); e) {
		errno = e;
		return -1;
	}
	return 0;
}

int umount(const char *target) { return umount2(target, 0); }

int umount2(const char *target, int flags) {
	MLIBC_CHECK_OR_ENOSYS(mlibc::sys_umount2, -1);
	if (int e = mlibc::sys_umount2(target, flags); e) {
		errno = e;
		return -1;
	}
	return 0;
}

int pivot_root(const char *new_root, const char *put_old) {
	(void)new_root;
	(void)put_old;
	// TODO: Implement via WOS syscall
	errno = ENOSYS;
	return -1;
}
