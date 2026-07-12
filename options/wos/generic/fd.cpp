#include <errno.h>
#include <sys/vfs.h>
#include <wos/fd.h>

extern "C" int wos_fstat_close(int fd, struct stat *statbuf, int *fstat_error) {
	if (!statbuf || !fstat_error) {
		errno = EINVAL;
		return -1;
	}

	int stat_result = -EINVAL;
	int const close_result = ker::abi::vfs::fstat_close_fd(fd, statbuf, &stat_result);
	*fstat_error = stat_result < 0 ? -stat_result : 0;
	if (close_result < 0) {
		errno = -close_result;
		return -1;
	}
	return 0;
}
