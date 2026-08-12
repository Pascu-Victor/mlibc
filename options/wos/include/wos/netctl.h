#ifndef _WOS_NETCTL_H
#define _WOS_NETCTL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WOS_NET_IF_NAME_LEN 16
#define WOS_NET_HWADDR_LEN 8
#define WOS_NET_ADDR_LEN 16

#define WOS_NETCTL_VERSION_1 1

#define WOS_NET_ADDR_F_DADFAILED 0x00000008u
#define WOS_NET_ADDR_F_DEPRECATED 0x00000020u
#define WOS_NET_ADDR_F_TENTATIVE 0x00000040u
#define WOS_NET_ADDR_F_PERMANENT 0x00000080u
#define WOS_NET_ADDR_F_NOPREFIXROUTE 0x00000200u
#define WOS_NET_ADDR_F_AUTOCONF 0x00010000u
#define WOS_NET_ADDR_F_NODAD 0x00020000u

#define WOS_NET_ROUTE_F_GATEWAY (1u << 0)
#define WOS_NET_ROUTE_F_AUTOCONF (1u << 1)

#define WOS_NET_LINK_SET_FLAGS (1u << 0)
#define WOS_NET_LINK_SET_MTU (1u << 1)
#define WOS_NET_LINK_SET_TXQLEN (1u << 2)
#define WOS_NET_LINK_SET_NAME (1u << 3)
#define WOS_NET_LINK_SET_HWADDR (1u << 4)

struct wos_net_if_info {
	uint32_t ifindex;
	char name[WOS_NET_IF_NAME_LEN];
	uint32_t flags;
	uint32_t mtu;
	uint32_t tx_queue_len;
	uint16_t type;
	uint8_t addr_len;
	uint8_t operstate;
	uint8_t addr[WOS_NET_HWADDR_LEN];
	uint8_t broadcast[WOS_NET_HWADDR_LEN];
};

struct wos_net_addr_info {
	uint32_t ifindex;
	uint16_t family;
	uint8_t prefix_len;
	uint8_t scope;
	uint32_t flags;
	char label[WOS_NET_IF_NAME_LEN];
	uint8_t address[WOS_NET_ADDR_LEN];
	uint8_t local[WOS_NET_ADDR_LEN];
	uint8_t broadcast[WOS_NET_ADDR_LEN];
};

struct wos_net_addr_req {
	uint32_t ifindex;
	uint16_t family;
	uint8_t prefix_len;
	uint8_t scope;
	uint32_t flags;
	uint8_t address[WOS_NET_ADDR_LEN];
	uint8_t local[WOS_NET_ADDR_LEN];
	uint8_t replace;
};

struct wos_net_addr_req_v2 {
	uint32_t size;
	uint16_t version;
	uint16_t reserved;
	struct wos_net_addr_req address;
	uint32_t preferred_lifetime_s;
	uint32_t valid_lifetime_s;
};

struct wos_net_route_record {
	uint32_t size;
	uint16_t version;
	uint16_t family;
	uint32_t ifindex;
	uint32_t metric;
	uint32_t flags;
	uint8_t prefix_len;
	uint8_t scope;
	uint16_t reserved16;
	uint8_t destination[WOS_NET_ADDR_LEN];
	uint8_t gateway[WOS_NET_ADDR_LEN];
	uint32_t lifetime_s;
	uint32_t reserved32;
};

struct wos_net_link_set_req {
	uint32_t ifindex;
	char ifname[WOS_NET_IF_NAME_LEN];
	uint32_t fields;
	uint32_t flags;
	uint32_t flag_mask;
	uint32_t mtu;
	uint32_t tx_queue_len;
	char new_name[WOS_NET_IF_NAME_LEN];
	uint8_t hwaddr[WOS_NET_HWADDR_LEN];
	uint8_t hwaddr_len;
};

int wos_net_if_list(struct wos_net_if_info *out, size_t *count);
int wos_net_addr_list(struct wos_net_addr_info *out, size_t *count);
int wos_net_addr_set(const struct wos_net_addr_req *req);
int wos_net_addr_del(const struct wos_net_addr_req *req);
int wos_net_link_set(const struct wos_net_link_set_req *req);
int wos_net_addr_set_v2(const struct wos_net_addr_req_v2 *req);
int wos_net_route_list(struct wos_net_route_record *out, size_t *count);
int wos_net_route_set(const struct wos_net_route_record *req);
int wos_net_route_del(const struct wos_net_route_record *req);

#ifdef __cplusplus
static_assert(sizeof(wos_net_if_info) == 52);
static_assert(sizeof(wos_net_addr_info) == 76);
static_assert(sizeof(wos_net_addr_req) == 48);
static_assert(sizeof(wos_net_addr_req_v2) == 64);
static_assert(sizeof(wos_net_link_set_req) == 68);
static_assert(sizeof(wos_net_route_record) == 64);
#endif

#ifdef __cplusplus
}
#endif

#endif
