#include <errno.h>
#include <sys/klog.h>

int klogctl(int type, char *bufp, int len) {
	(void)type;
	(void)bufp;
	(void)len;
	// TODO: Implement via WOS syscall
	errno = ENOSYS;
	return -1;
}
