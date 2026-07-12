
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <bits/ensure.h>
#include <frg/allocation.hpp>
#include <mlibc-config.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/allocator.hpp>
#include <mlibc/debug.hpp>

namespace {

DIR *allocate_dir() {
	auto dir = static_cast<DIR *>(getAllocator().allocate(sizeof(DIR)));
	__ensure(dir);
	dir->__handle = -1;
	dir->__ent_next = 0;
	dir->__ent_limit = 0;
	dir->__seek_offset = 0;
	return dir;
}

void free_dir(DIR *dir) { getAllocator().deallocate(dir, sizeof(DIR)); }

} // namespace

// Code taken from musl
int alphasort(const struct dirent **a, const struct dirent **b) {
	return strcoll((*a)->d_name, (*b)->d_name);
}

int closedir(DIR *dir) {
	close(dir->__handle);
	free_dir(dir);
	return 0;
}

int dirfd(DIR *dir) { return dir->__handle; }

DIR *fdopendir(int fd) {
	struct stat st;

	if (fstat(fd, &st) < 0) {
		return nullptr;
	}
	// Musl implements this, but O_PATH is only declared on the linux abi
	/*if(fcntl(fd, F_GETFL) & O_PATH) {
	    errno = EBADF;
	    return nullptr;
	}*/
	if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		return nullptr;
	}
	auto dir = allocate_dir();
	int flags = fcntl(fd, F_GETFD);
	fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
	dir->__handle = fd;
	return dir;
}

DIR *opendir(const char *path) {
	auto dir = allocate_dir();

	if (int e = mlibc::sysdep_or_enosys<OpenDir>(path, &dir->__handle); e) {
		errno = e;
		free_dir(dir);
		return nullptr;
	} else {
		return dir;
	}
}

struct dirent *readdir(DIR *dir) {
	__ensure(dir->__ent_next <= dir->__ent_limit);
	if (dir->__ent_next == dir->__ent_limit) {
		if (int e = mlibc::sysdep_or_enosys<ReadEntries>(
		        dir->__handle, dir->__ent_buffer, sizeof(dir->__ent_buffer), &dir->__ent_limit
		    );
		    e)
			__ensure(!"mlibc::sys_read_entries() failed");
		dir->__ent_next = 0;
		if (!dir->__ent_limit)
			return nullptr;
	}

	auto entp = reinterpret_cast<struct dirent *>(dir->__ent_buffer + dir->__ent_next);
	dir->__seek_offset = entp->d_off;
	__ensure(entp->d_reclen);
	__ensure(entp->d_reclen <= dir->__ent_limit - dir->__ent_next);
	dir->__ent_next += entp->d_reclen;

	if ((reinterpret_cast<uintptr_t>(entp) & (alignof(struct dirent) - 1)) == 0)
		return entp;

	__ensure(entp->d_reclen <= sizeof(dir->__current));
	memcpy(&dir->__current, entp, entp->d_reclen);
	return &dir->__current;
}

ssize_t posix_getdents(int fildes, void *buf, size_t nbyte, int flags) {
	if (flags) {
		errno = EINVAL;
		return -1;
	}

	size_t bytes_read = 0;
	if (int e = mlibc::sysdep_or_enosys<ReadEntries>(fildes, buf, nbyte, &bytes_read); e) {
		errno = e;
		return -1;
	}
	return bytes_read;
}

#if __MLIBC_LINUX_OPTION
[[gnu::alias("readdir")]] struct dirent64 *readdir64(DIR *dir);
#endif /* !__MLIBC_LINUX_OPTION */

int readdir_r(DIR *dir, struct dirent *entry, struct dirent **result) {
	if constexpr (!mlibc::IsImplemented<ReadEntries>) {
		MLIBC_MISSING_SYSDEP();
		return ENOSYS;
	}

	__ensure(dir->__ent_next <= dir->__ent_limit);
	if (dir->__ent_next == dir->__ent_limit) {
		if (int e = mlibc::sysdep_or_panic<ReadEntries>(
		        dir->__handle, dir->__ent_buffer, sizeof(dir->__ent_buffer), &dir->__ent_limit
		    );
		    e)
			__ensure(!"mlibc::sys_read_entries() failed");
		dir->__ent_next = 0;
		if (!dir->__ent_limit) {
			*result = nullptr;
			return 0;
		}
	}

	auto entp = reinterpret_cast<struct dirent *>(dir->__ent_buffer + dir->__ent_next);
	dir->__seek_offset = entp->d_off;
	__ensure(entp->d_reclen <= sizeof(*entry));
	memcpy(entry, entp, entp->d_reclen);
	dir->__ent_next += entp->d_reclen;
	*result = entry;
	return 0;
}

void rewinddir(DIR *dir) {
	dir->__seek_offset = lseek(dir->__handle, 0, SEEK_SET);
	dir->__ent_next = 0;
	dir->__ent_limit = 0;
}

int scandir(
    const char *path,
    struct dirent ***res,
    int (*select)(const struct dirent *),
    int (*compare)(const struct dirent **, const struct dirent **)
) {
	int handle = -1;
	if (int e = mlibc::sysdep_or_enosys<OpenDir>(path, &handle); e) {
		errno = e;
		return -1;
	}

	// we should save the errno
	int old_errno = errno;

	alignas(struct dirent) char ent_buffer[sizeof(static_cast<DIR *>(nullptr)->__ent_buffer)];
	size_t ent_next = 0;
	size_t ent_limit = 0;
	struct dirent **array = nullptr, **tmp = nullptr;
	int length = 0;
	int count = 0;
	int scan_errno = 0;

	while (!scan_errno) {
		if (ent_next == ent_limit) {
			if (int e = mlibc::sysdep_or_enosys<ReadEntries>(
			        handle, ent_buffer, sizeof(ent_buffer), &ent_limit
			    );
			    e) {
				scan_errno = e;
				break;
			}
			ent_next = 0;
			if (!ent_limit)
				break;
		}

		auto dir_ent = reinterpret_cast<struct dirent *>(ent_buffer + ent_next);
		if (!dir_ent->d_reclen || dir_ent->d_reclen > ent_limit - ent_next) {
			scan_errno = EINVAL;
			break;
		}
		ent_next += dir_ent->d_reclen;

		if (select && !select(dir_ent))
			continue;

		if (count >= length) {
			length = 2 * length + 1;
			tmp = static_cast<struct dirent **>(realloc(array, length * sizeof(struct dirent *)));
			// we need to check the call actually goes through
			// before we overwrite array so that we can
			// deallocate the already written entries should realloc()
			// have failed
			if (!tmp) {
				scan_errno = ENOMEM;
				break;
			}
			array = tmp;
		}
		array[count] = static_cast<struct dirent *>(malloc(dir_ent->d_reclen));
		if (!array[count]) {
			scan_errno = ENOMEM;
			break;
		}

		memcpy(array[count], dir_ent, dir_ent->d_reclen);
		count++;
	}

	close(handle);

	if (scan_errno) {
		if (array)
			while (count-- > 0)
				free(array[count]);
		free(array);
		errno = scan_errno;
		return -1;
	}

	// from here we can set the old errno back
	errno = old_errno;

	if (compare)
		qsort(array, count, sizeof(struct dirent *), (int (*)(const void *, const void *))compare);
	*res = array;
	return count;
}

#if __MLIBC_LINUX_OPTION
[[gnu::alias("scandir")]] int scandir64(
    const char *path,
    struct dirent64 ***res,
    int (*select)(const struct dirent64 *),
    int (*compare)(const struct dirent64 **, const struct dirent64 **)
);
[[gnu::alias("versionsort")]] int
versionsort64(const struct dirent64 **__a, const struct dirent64 **__b);
#endif /* !__MLIBC_LINUX_OPTION */

void seekdir(DIR *d, long off) {
	d->__seek_offset = lseek(d->__handle, off, SEEK_SET);
	d->__ent_next = 0;
	d->__ent_limit = 0;
}

long telldir(DIR *d) { return d->__seek_offset; }

#if __MLIBC_GLIBC_OPTION

int versionsort(const struct dirent **a, const struct dirent **b) {
	return strverscmp((*a)->d_name, (*b)->d_name);
}

ssize_t getdents64(int fd, void *dirp, size_t count) { return posix_getdents(fd, dirp, count, 0); }

#endif // __MLIBC_GLIBC_OPTION
