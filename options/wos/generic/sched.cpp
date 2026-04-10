#include <bits/ensure.h>
#include <errno.h>
#include <sched.h>

#include <mlibc/all-sysdeps.hpp>

using namespace mlibc;

int sched_getcpu(void) {
	int cpu;
	if (int e = sysdep_or_enosys<Getcpu>(&cpu); e) {
		errno = e;
		return -1;
	}
	return cpu;
}

int setns(int fd, int nstype) {
	if (int e = sysdep_or_enosys<SetNs>(fd, nstype); e) {
		errno = e;
		return -1;
	}
	return 0;
}

int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask) {
	if (int e = sysdep_or_enosys<GetAffinity>(pid, cpusetsize, mask); e) {
		errno = e;
		return -1;
	}
	return 0;
}

int unshare(int flags) {
	if (int e = sysdep_or_enosys<Unshare>(flags); e) {
		errno = e;
		return -1;
	}
	return 0;
}

int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask) {
	if (int e = sysdep_or_enosys<SetAffinity>(pid, cpusetsize, mask); e) {
		errno = e;
		return -1;
	}
	return 0;
}

int clone(int (*)(void *), void *, int, void *, ...) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}
