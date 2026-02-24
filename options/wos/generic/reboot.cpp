#include <errno.h>
#include <sys/reboot.h>

int reboot(int cmd) {
	(void)cmd;
	// TODO: Implement via WOS syscall
	errno = ENOSYS;
	return -1;
}
