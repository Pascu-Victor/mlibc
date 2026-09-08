#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

int main() {
	struct rlimit getlim, setlim;

	setlim.rlim_cur = 16;
	assert(!getrlimit(RLIMIT_NOFILE, &getlim));
	setlim.rlim_max = getlim.rlim_max;
	if (setlim.rlim_cur > setlim.rlim_max)
		setlim.rlim_cur = setlim.rlim_max;

	int ret = setrlimit(RLIMIT_NOFILE, &setlim);

	if (ret == -1) {
		fprintf(stderr, "%s\n", strerror(errno));
		assert(!ret);
	}

	assert(!getrlimit(RLIMIT_NOFILE, &getlim));

	assert(setlim.rlim_cur == getlim.rlim_cur);
	assert(setlim.rlim_max == getlim.rlim_max);

	return 0;
}
