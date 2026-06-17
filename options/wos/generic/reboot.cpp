#include <errno.h>
#include <sys/power.h>
#include <sys/reboot.h>

int reboot(int cmd) {
	int64_t result = ker::abi::power::reboot(static_cast<uint64_t>(cmd));
	if (result == 0) {
		return 0;
	}
	errno = static_cast<int>(-result);
	return -1;
}
