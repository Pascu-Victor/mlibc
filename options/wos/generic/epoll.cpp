#include <errno.h>
#include <sys/epoll.h>

#include <mlibc/all-sysdeps.hpp>

int epoll_create(int flags) {
	int fd;
	if (int e = mlibc::sysdep_or_enosys<EpollCreate>(flags, &fd); e) {
		errno = e;
		return -1;
	}
	return fd;
}

int epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask) {
	int raised;
	if (int e =
	        mlibc::sysdep_or_enosys<EpollPwait>(epfd, events, maxevents, timeout, sigmask, &raised);
	    e) {
		errno = e;
		return -1;
	}
	return raised;
}

int epoll_create1(int flags) {
	int fd;
	if (int e = mlibc::sysdep_or_enosys<EpollCreate>(flags, &fd); e) {
		errno = e;
		return -1;
	}
	return fd;
}

int epoll_ctl(int epfd, int mode, int fd, struct epoll_event *event) {
	if (int e = mlibc::sysdep_or_enosys<EpollCtl>(epfd, mode, fd, event); e) {
		errno = e;
		return -1;
	}
	return 0;
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
	int raised;
	if (int e = mlibc::sysdep_or_enosys<EpollPwait>(epfd, events, maxevents, timeout, nullptr, &raised);
	    e) {
		errno = e;
		return -1;
	}
	return raised;
}
