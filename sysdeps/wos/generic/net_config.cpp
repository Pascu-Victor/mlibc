#include <abi-bits/in.h>
#include <abi-bits/socket.h>
#include <errno.h>
#include <ifaddrs.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/allocator.hpp>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <sys/net.h>
#include <wos/netctl.h>

namespace mlibc {
namespace {

struct WosIfaddrAllocation {
	struct ifaddrs entry;
	char name[WOS_NET_IF_NAME_LEN];
	struct sockaddr_storage address;
	struct sockaddr_storage netmask;
	struct sockaddr_storage broadcast;
};

bool any_address(const uint8_t *address, size_t length) {
	for (size_t i = 0; i < length; ++i) {
		if (address[i] != 0)
			return true;
	}
	return false;
}

bool usable_global_ipv6(const struct wos_net_addr_info &address) {
	return address.family == AF_INET6 && address.scope == 0
	       && !(address.flags & (WOS_NET_ADDR_F_TENTATIVE | WOS_NET_ADDR_F_DADFAILED));
}

const struct wos_net_if_info *
find_interface(const struct wos_net_if_info *interfaces, size_t count, uint32_t ifindex) {
	for (size_t i = 0; i < count; ++i) {
		if (interfaces[i].ifindex == ifindex)
			return &interfaces[i];
	}
	return nullptr;
}

void fill_prefix(uint8_t *bytes, size_t byte_count, uint8_t prefix_len) {
	memset(bytes, 0, byte_count);
	for (size_t i = 0; i < byte_count && prefix_len != 0; ++i) {
		uint8_t bits = prefix_len > 8 ? 8 : prefix_len;
		bytes[i] = static_cast<uint8_t>(0xFFu << (8 - bits));
		prefix_len -= bits;
	}
}

int load_netctl_rows(
    struct wos_net_if_info **interfaces_out,
    size_t *interface_count_out,
    struct wos_net_addr_info **addresses_out,
    size_t *address_count_out
) {
	*interfaces_out = nullptr;
	*interface_count_out = 0;
	*addresses_out = nullptr;
	*address_count_out = 0;

	size_t interface_count = 0;
	int r = ker::abi::net::netctl_if_list(nullptr, &interface_count);
	if (r < 0)
		return -r;
	if (interface_count > SIZE_MAX / sizeof(struct wos_net_if_info))
		return EOVERFLOW;
	if (interface_count != 0) {
		auto *interfaces = static_cast<struct wos_net_if_info *>(
		    getAllocator().allocate(interface_count * sizeof(struct wos_net_if_info))
		);
		if (!interfaces)
			return ENOMEM;
		size_t capacity = interface_count;
		r = ker::abi::net::netctl_if_list(interfaces, &interface_count);
		if (r < 0) {
			getAllocator().free(interfaces);
			return -r;
		}
		*interfaces_out = interfaces;
		*interface_count_out = interface_count < capacity ? interface_count : capacity;
	}

	size_t address_count = 0;
	r = ker::abi::net::netctl_addr_list(nullptr, &address_count);
	if (r < 0) {
		getAllocator().free(*interfaces_out);
		*interfaces_out = nullptr;
		return -r;
	}
	if (address_count > SIZE_MAX / sizeof(struct wos_net_addr_info)) {
		getAllocator().free(*interfaces_out);
		*interfaces_out = nullptr;
		return EOVERFLOW;
	}
	if (address_count != 0) {
		auto *addresses = static_cast<struct wos_net_addr_info *>(
		    getAllocator().allocate(address_count * sizeof(struct wos_net_addr_info))
		);
		if (!addresses) {
			getAllocator().free(*interfaces_out);
			*interfaces_out = nullptr;
			return ENOMEM;
		}
		size_t capacity = address_count;
		r = ker::abi::net::netctl_addr_list(addresses, &address_count);
		if (r < 0) {
			getAllocator().free(addresses);
			getAllocator().free(*interfaces_out);
			*interfaces_out = nullptr;
			return -r;
		}
		*addresses_out = addresses;
		*address_count_out = address_count < capacity ? address_count : capacity;
	}
	return 0;
}

void fill_ipv4(struct WosIfaddrAllocation *allocation, const struct wos_net_addr_info &info) {
	auto *address = reinterpret_cast<struct sockaddr_in *>(&allocation->address);
	auto *netmask = reinterpret_cast<struct sockaddr_in *>(&allocation->netmask);
	auto *broadcast = reinterpret_cast<struct sockaddr_in *>(&allocation->broadcast);
	address->sin_family = AF_INET;
	netmask->sin_family = AF_INET;
	broadcast->sin_family = AF_INET;
	const uint8_t *local = any_address(info.local, 4) ? info.local : info.address;
	memcpy(&address->sin_addr, local, 4);
	fill_prefix(reinterpret_cast<uint8_t *>(&netmask->sin_addr), 4, info.prefix_len);
	memcpy(&broadcast->sin_addr, info.broadcast, 4);
	allocation->entry.ifa_broadaddr = reinterpret_cast<struct sockaddr *>(broadcast);
}

void fill_ipv6(struct WosIfaddrAllocation *allocation, const struct wos_net_addr_info &info) {
	auto *address = reinterpret_cast<struct sockaddr_in6 *>(&allocation->address);
	auto *netmask = reinterpret_cast<struct sockaddr_in6 *>(&allocation->netmask);
	address->sin6_family = AF_INET6;
	netmask->sin6_family = AF_INET6;
	const uint8_t *local = any_address(info.local, 16) ? info.local : info.address;
	memcpy(&address->sin6_addr, local, 16);
	fill_prefix(reinterpret_cast<uint8_t *>(&netmask->sin6_addr), 16, info.prefix_len);
	if (info.scope == 253)
		address->sin6_scope_id = info.ifindex;
}

} // namespace

int Sysdeps<Getifaddrs>::operator()(struct ifaddrs **out) {
	if (!out)
		return EINVAL;
	*out = nullptr;

	struct wos_net_if_info *interfaces = nullptr;
	struct wos_net_addr_info *addresses = nullptr;
	size_t interface_count = 0;
	size_t address_count = 0;
	int e = load_netctl_rows(&interfaces, &interface_count, &addresses, &address_count);
	if (e)
		return e;

	struct ifaddrs **tail = out;
	for (size_t i = 0; i < address_count; ++i) {
		const auto &info = addresses[i];
		if (info.family != AF_INET && info.family != AF_INET6)
			continue;
		const auto *interface = find_interface(interfaces, interface_count, info.ifindex);
		if (!interface)
			continue;

		auto *allocation = static_cast<WosIfaddrAllocation *>(
		    getAllocator().allocate(sizeof(WosIfaddrAllocation))
		);
		if (!allocation) {
			getAllocator().free(addresses);
			getAllocator().free(interfaces);
			while (*out) {
				auto *next = (*out)->ifa_next;
				getAllocator().free(*out);
				*out = next;
			}
			return ENOMEM;
		}
		memset(allocation, 0, sizeof(*allocation));
		memcpy(allocation->name, interface->name, WOS_NET_IF_NAME_LEN);
		allocation->name[WOS_NET_IF_NAME_LEN - 1] = '\0';
		allocation->entry.ifa_name = allocation->name;
		allocation->entry.ifa_flags = interface->flags;
		allocation->entry.ifa_addr = reinterpret_cast<struct sockaddr *>(&allocation->address);
		allocation->entry.ifa_netmask = reinterpret_cast<struct sockaddr *>(&allocation->netmask);
		if (info.family == AF_INET)
			fill_ipv4(allocation, info);
		else
			fill_ipv6(allocation, info);
		*tail = &allocation->entry;
		tail = &allocation->entry.ifa_next;
	}

	getAllocator().free(addresses);
	getAllocator().free(interfaces);
	return 0;
}

int Sysdeps<InetConfigured>::operator()(bool *ipv4, bool *ipv6) {
	if (!ipv4 || !ipv6)
		return EINVAL;
	*ipv4 = false;
	*ipv6 = false;

	struct wos_net_if_info *interfaces = nullptr;
	struct wos_net_addr_info *addresses = nullptr;
	size_t interface_count = 0;
	size_t address_count = 0;
	int e = load_netctl_rows(&interfaces, &interface_count, &addresses, &address_count);
	if (e)
		return e;
	for (size_t i = 0; i < address_count; ++i) {
		const auto *interface = find_interface(interfaces, interface_count, addresses[i].ifindex);
		if (!interface || !strncmp(interface->name, "lo", WOS_NET_IF_NAME_LEN))
			continue;
		if (addresses[i].family == AF_INET)
			*ipv4 = true;
		else if (usable_global_ipv6(addresses[i]))
			*ipv6 = true;
	}
	getAllocator().free(addresses);
	getAllocator().free(interfaces);
	return 0;
}

} // namespace mlibc
