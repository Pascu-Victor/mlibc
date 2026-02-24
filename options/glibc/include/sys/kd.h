#ifndef _SYS_KD_H
#define _SYS_KD_H

#if __MLIBC_WOS_OPTION
#include <wos/kd.h>
#else
/* Make sure the <linux/types.h> header is not loaded.  */
#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H 1
#define __undef_LINUX_TYPES_H
#endif

#include <linux/kd.h>

#ifdef __undef_LINUX_TYPES_H
#undef _LINUX_TYPES_H
#undef __undef_LINUX_TYPES_H
#endif
#endif /* __MLIBC_WOS_OPTION */

#endif /* _SYS_KD_H */
