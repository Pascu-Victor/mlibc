#include <mlibc/resolv_conf.hpp>
#include <mlibc/allocator.hpp>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <utility>

namespace mlibc {

frg::optional<struct resolv_conf_data> get_resolv_conf() {
	auto file = fopen("/etc/resolv.conf", "r");
	if (!file)
		return frg::null_opt;

	char line[128];
	struct resolv_conf_data ret;
	while (fgets(line, 128, file)) {
		char *pos;
		if (!strchr(line, '\n') && !feof(file)) {
			// skip truncated lines
			for (int c = getc(file); c != '\n' && c != EOF; c = getc(file));
			continue;
		}

		if ((pos = strchr(line, '#')))
			*pos = '\0';

		for (pos = line; isspace(*pos); pos++);
		if (!*pos)
			continue;

		char *key = pos;
		while (*pos && !isspace(*pos))
			pos++;
		if (*pos)
			*pos++ = '\0';

		for (; isspace(*pos); pos++);
		if (!*pos)
			continue;

		if (!strcmp(key, "nameserver")) {
			if (ret.name.empty()) {
				char *end = pos;
				while (*end && !isspace(*end))
					end++;
				ret.name = frg::string<MemoryAllocator>(pos, end - pos, getAllocator());
			}
		} else if (!strcmp(key, "search")) {
			ret.search.clear();
			while (*pos) {
				while (isspace(*pos))
					pos++;
				if (!*pos)
					break;

				char *end = pos;
				while (*end && !isspace(*end))
					end++;
				ret.search.push(frg::string<MemoryAllocator>(pos, end - pos, getAllocator()));
				pos = end;
			}
		} else if (!strcmp(key, "domain") && ret.search.empty()) {
			char *end = pos;
			while (*end && !isspace(*end))
				end++;
			ret.search.push(frg::string<MemoryAllocator>(pos, end - pos, getAllocator()));
		}
	}

	fclose(file);
	if(ret.name.empty() && ret.search.empty())
		return frg::null_opt;
	return ret;
}

frg::optional<struct nameserver_data> get_nameserver() {
	auto conf = get_resolv_conf();
	if(!conf || conf->name.empty())
		return frg::null_opt;

	struct nameserver_data ret;
	ret.name = std::move(conf->name);
	return ret;
}

} // namespace mlibc
