#ifndef _ABIBITS_ROUTE_H
#define _ABIBITS_ROUTE_H

#include <sys/socket.h>

#define RTF_UP 0x0001
#define RTF_GATEWAY 0x0002
#define RTF_HOST 0x0004
#define RTF_REINSTATE 0x0008
#define RTF_DYNAMIC 0x0010
#define RTF_MODIFIED 0x0020
#define RTF_MTU 0x0040
#define RTF_MSS RTF_MTU
#define RTF_WINDOW 0x0080
#define RTF_IRTT 0x0100
#define RTF_REJECT 0x0200

struct rtentry {
	unsigned long int rt_pad1;
	struct sockaddr rt_dst;
	struct sockaddr rt_gateway;
	struct sockaddr rt_genmask;
	unsigned short int rt_flags;
	short int rt_pad2;
	unsigned long int rt_pad3;
	unsigned char rt_tos;
	unsigned char rt_class;
#if __INTPTR_WIDTH__ == 64
	short int rt_pad4[3];
#else
	short int rt_pad4;
#endif
	short int rt_metric;
	char *rt_dev;
	unsigned long int rt_mtu;
#define rt_mss rt_mtu
	unsigned long int rt_window;
	unsigned short int rt_irtt;
};

#endif /* _ABIBITS_ROUTE_H */
