#ifndef _WOS_SYS_KLOG_H
#define _WOS_SYS_KLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __MLIBC_ABI_ONLY

int klogctl(int __type, char *__bufp, int __len);

#endif /* !__MLIBC_ABI_ONLY */

#ifdef __cplusplus
}
#endif

#endif /* _WOS_SYS_KLOG_H */
