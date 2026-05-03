#include <errno.h>
#include <sys/net.h>
#include <wos/netctl.h>

extern "C" int wos_net_if_list(struct wos_net_if_info *out, size_t *count) {
	int r = ker::abi::net::netctl_if_list(out, count);
	if (r < 0) {
		errno = -r;
		return -1;
	}
	return 0;
}

extern "C" int wos_net_addr_list(struct wos_net_addr_info *out, size_t *count) {
	int r = ker::abi::net::netctl_addr_list(out, count);
	if (r < 0) {
		errno = -r;
		return -1;
	}
	return 0;
}

extern "C" int wos_net_addr_set(const struct wos_net_addr_req *req) {
	int r = ker::abi::net::netctl_addr_set(req);
	if (r < 0) {
		errno = -r;
		return -1;
	}
	return 0;
}

extern "C" int wos_net_addr_del(const struct wos_net_addr_req *req) {
	int r = ker::abi::net::netctl_addr_del(req);
	if (r < 0) {
		errno = -r;
		return -1;
	}
	return 0;
}

extern "C" int wos_net_link_set(const struct wos_net_link_set_req *req) {
	int r = ker::abi::net::netctl_link_set(req);
	if (r < 0) {
		errno = -r;
		return -1;
	}
	return 0;
}
