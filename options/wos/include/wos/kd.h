#ifndef _WOS_KD_H
#define _WOS_KD_H

/* Keyboard mode constants */
#define K_RAW 0x00
#define K_XLATE 0x01
#define K_MEDIUMRAW 0x02
#define K_UNICODE 0x03
#define K_OFF 0x04

/* Keyboard ioctl commands */
#define KDGKBTYPE 0x4B33 /* get keyboard type */
#define KDGKBMODE 0x4B44 /* get keyboard mode */
#define KDSKBMODE 0x4B45 /* set keyboard mode */

/* Console sound ioctls */
#define KIOCSOUND 0x4B2F /* start/stop sound generation */
#define KDMKTONE 0x4B30  /* generate tone */

/* Keyboard type values (returned by KDGKBTYPE) */
#define KB_84 0x01
#define KB_101 0x02
#define KB_OTHER 0x03

/* Keyboard table entry ioctls */
#define KDGKBENT 0x4B46 /* get keyboard entry */
#define KDSKBENT 0x4B47 /* set keyboard entry */

/* Keyboard map sizes */
#define NR_KEYS 128
#define MAX_NR_KEYMAPS 256

/* Font/console map ioctls */
#define GIO_FONT 0x4B60
#define PIO_FONT 0x4B61
#define GIO_CMAP 0x4B70
#define PIO_CMAP 0x4B71
#define GIO_SCRNMAP 0x4B40
#define PIO_SCRNMAP 0x4B41

/* LED ioctls */
#define KDGETLED 0x4B31
#define KDSETLED 0x4B32
/* Unicode map ioctls */
#define PIO_UNIMAPCLR 0x4B74
#define PIO_UNIMAP 0x4B66
#define GIO_UNIMAP 0x4B66
#define PIO_UNISCRNMAP 0x4B6B
#define GIO_UNISCRNMAP 0x4B69

/* Unicode map size */
#define E_TABSZ 256

/* Unicode map structures */
struct unipair {
	unsigned short unicode;
	unsigned short fontpos;
};

struct unimapdesc {
	unsigned short entry_ct;
	struct unipair *entries;
};

struct unimapinit {
	unsigned short advised_hashsize;
	unsigned short advised_hashstep;
	unsigned short advised_hashlevel;
};
/* Clock tick rate for beep */
#define CLOCK_TICK_RATE 1193180

#endif /* _WOS_KD_H */
