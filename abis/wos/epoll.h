#ifndef _ABIBITS_EPOLL_H
#define _ABIBITS_EPOLL_H

#include <mlibc-config.h>

#if !__MLIBC_WOS_OPTION
#error "<sys/epoll.h> is inherently WOS specific. Enable the WOS option or do not use this header."
#endif /* !__MLIBC_WOS_OPTION */

#define EPOLL_CLOEXEC 02000000 /* Same as __MLIBC_O_CLOEXEC */

#endif /* _ABIBITS_EPOLL_H */
