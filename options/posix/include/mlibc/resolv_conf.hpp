#ifndef _MLIBC_RESOLV_CONF
#define _MLIBC_RESOLV_CONF

#include <frg/string.hpp>
#include <frg/optional.hpp>
#include <frg/vector.hpp>
#include <mlibc/allocator.hpp>

namespace mlibc {

struct nameserver_data {
	nameserver_data()
	: name(getAllocator()) {}
	frg::string<MemoryAllocator> name;
};

struct resolv_conf_data {
	resolv_conf_data()
	: name(getAllocator()), search(getAllocator()) {}
	frg::string<MemoryAllocator> name;
	frg::vector<frg::string<MemoryAllocator>, MemoryAllocator> search;
};

frg::optional<struct resolv_conf_data> get_resolv_conf();
frg::optional<struct nameserver_data> get_nameserver();

} // namespace mlibc

#endif // _MLIBC_RESOLV_CONF
