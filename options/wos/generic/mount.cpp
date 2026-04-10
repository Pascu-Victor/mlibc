#include <errno.h>
#include <sys/mount.h>

#include <mlibc/all-sysdeps.hpp>

int mount(
    const char *source,
    const char *target,
    const char *fstype,
    unsigned long flags,
    const void *data
) {
	if (int e = mlibc::sysdep_or_enosys<Mount>(source, target, fstype, flags, data); e) {
		errno = e;
		return -1;
	}
	return 0;
}

int umount(const char *target) { return umount2(target, 0); }

int umount2(const char *target, int flags) {
	if (int e = mlibc::sysdep_or_enosys<Umount2>(target, flags); e) {
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
