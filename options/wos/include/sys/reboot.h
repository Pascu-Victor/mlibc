#ifndef _WOS_SYS_REBOOT_H
#define _WOS_SYS_REBOOT_H

#define RB_AUTOBOOT 0x01234567
#define RB_HALT_SYSTEM 0xcdef0123
#define RB_ENABLE_CAD 0x89abcdef
#define RB_DISABLE_CAD 0
#define RB_POWER_OFF 0x4321fedc

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __MLIBC_ABI_ONLY
int reboot(int __cmd);
#endif /* !__MLIBC_ABI_ONLY */

#ifdef __cplusplus
}
#endif

#endif /* _WOS_SYS_REBOOT_H */
