#ifndef _WOS_FD_H
#define _WOS_FD_H

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

// Collect stat metadata and consume fd with one WOS syscall. On return,
// fstat_error contains zero or a positive errno independent of the close result.
int wos_fstat_close(int fd, struct stat *statbuf, int *fstat_error);

#ifdef __cplusplus
}
#endif

#endif
