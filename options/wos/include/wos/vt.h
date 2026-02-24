#ifndef _WOS_VT_H
#define _WOS_VT_H

/* VT ioctl commands */
#define VT_OPENQRY 0x5600     /* find available VT */
#define VT_GETMODE 0x5601     /* get VT mode */
#define VT_SETMODE 0x5602     /* set VT mode */
#define VT_GETSTATE 0x5603    /* get global VT state */
#define VT_SENDSIG 0x5604     /* signal to send to VT */
#define VT_RELDISP 0x5605     /* release display */
#define VT_ACTIVATE 0x5606    /* activate VT */
#define VT_WAITACTIVE 0x5607  /* wait for VT active */
#define VT_DISALLOCATE 0x5608 /* free VT memory */
#define VT_RESIZE 0x5609      /* set VT rows/cols */

/* VT mode */
#define VT_AUTO 0x00
#define VT_PROCESS 0x01
#define VT_ACKACQ 0x02

struct vt_mode {
	char mode;
	char waitv;
	short relsig;
	short acqsig;
	short frsig;
};

struct vt_stat {
	unsigned short v_active;
	unsigned short v_signal;
	unsigned short v_state;
};

struct vt_sizes {
	unsigned short v_rows;
	unsigned short v_cols;
	unsigned short v_scrollsize;
};

#endif /* _WOS_VT_H */
