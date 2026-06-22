#include <ctype.h>
#include <mlibc/allocator.hpp>
#include <mlibc/resolv_conf.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>

namespace mlibc {

namespace {
constexpr int MIN_RESOLV_TIMEOUT_SECS = 1;
constexpr int MAX_RESOLV_TIMEOUT_SECS = 30;
constexpr int MIN_RESOLV_ATTEMPTS = 1;
constexpr int MAX_RESOLV_ATTEMPTS = 5;

auto parse_positive_option_value(const char *value, long *out) -> bool {
	if (value == nullptr || *value == '\0' || out == nullptr)
		return false;

	char *end = nullptr;
	long parsed = strtol(value, &end, 10);
	if (end == value || *end != '\0' || parsed <= 0)
		return false;

	*out = parsed;
	return true;
}

auto clamp_resolv_option(long value, int min, int max) -> int {
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

void apply_resolv_option(resolv_conf_data &ret, char *token) {
	constexpr char TIMEOUT_PREFIX[] = "timeout:";
	constexpr char ATTEMPTS_PREFIX[] = "attempts:";
	long value = 0;

	if (!strncmp(token, TIMEOUT_PREFIX, sizeof(TIMEOUT_PREFIX) - 1)
	    && parse_positive_option_value(token + sizeof(TIMEOUT_PREFIX) - 1, &value)) {
		ret.timeout = clamp_resolv_option(value, MIN_RESOLV_TIMEOUT_SECS, MAX_RESOLV_TIMEOUT_SECS);
	} else if (
	    !strncmp(token, ATTEMPTS_PREFIX, sizeof(ATTEMPTS_PREFIX) - 1)
	    && parse_positive_option_value(token + sizeof(ATTEMPTS_PREFIX) - 1, &value)
	) {
		ret.attempts = clamp_resolv_option(value, MIN_RESOLV_ATTEMPTS, MAX_RESOLV_ATTEMPTS);
	}
}
} // namespace

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
			for (int c = getc(file); c != '\n' && c != EOF; c = getc(file))
				;
			continue;
		}

		if ((pos = strchr(line, '#')))
			*pos = '\0';

		for (pos = line; isspace(*pos); pos++)
			;
		if (!*pos)
			continue;

		char *key = pos;
		while (*pos && !isspace(*pos))
			pos++;
		if (*pos)
			*pos++ = '\0';

		for (; isspace(*pos); pos++)
			;
		if (!*pos)
			continue;

		if (!strcmp(key, "nameserver")) {
			char *end = pos;
			while (*end && !isspace(*end))
				end++;
			frg::string<MemoryAllocator> nameserver{
			    pos, static_cast<size_t>(end - pos), getAllocator()
			};
			if (ret.name.empty())
				ret.name = nameserver;
			ret.nameservers.push(std::move(nameserver));
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
		} else if (!strcmp(key, "options")) {
			while (*pos) {
				while (isspace(*pos))
					pos++;
				if (!*pos)
					break;

				char *end = pos;
				while (*end && !isspace(*end))
					end++;
				char saved = *end;
				*end = '\0';
				apply_resolv_option(ret, pos);
				if (!saved)
					break;
				pos = end + 1;
			}
		}
	}

	fclose(file);
	if (ret.name.empty() && ret.nameservers.empty() && ret.search.empty())
		return frg::null_opt;
	return ret;
}

frg::optional<struct nameserver_data> get_nameserver() {
	auto conf = get_resolv_conf();
	if (!conf || (conf->name.empty() && conf->nameservers.empty()))
		return frg::null_opt;

	struct nameserver_data ret;
	if (!conf->nameservers.empty())
		ret.name = std::move(conf->nameservers[0]);
	else
		ret.name = std::move(conf->name);
	return ret;
}

} // namespace mlibc
