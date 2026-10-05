#pragma once

#include <callnums/perf_trace.h>
#include <mlibc/all-sysdeps.hpp>
#include <stdint.h>

namespace mlibc {

struct PerfStartupToken {
	uint64_t version = 0;
	uint64_t session = UINT64_MAX;
	uint64_t correlation = 0;
};

// Entry-stack parsing needs no allocator, errno or TLS. Missing, partial,
// duplicate or unknown capability data disables the optional operation.
inline auto perf_startup_token(const uintptr_t *entry_stack) -> PerfStartupToken {
	if (!entry_stack)
		return {};
	auto aux = entry_stack + entry_stack[0] + 2;
	while (*aux)
		++aux;
	++aux;
	PerfStartupToken token;
	unsigned present = 0;
	for (; aux[0]; aux += 2) {
		unsigned bit = 0;
		switch (aux[0]) {
			case WOS_AT_PERF_TRACE_VERSION:
				bit = 1;
				token.version = aux[1];
				break;
			case WOS_AT_PERF_TRACE_SESSION:
				bit = 2;
				token.session = aux[1];
				break;
			case WOS_AT_PERF_TRACE_CORRELATION:
				bit = 4;
				token.correlation = aux[1];
				break;
			default:
				break;
		}
		if (present & bit)
			return {};
		present |= bit;
	}
	if (present != 7 || token.version != WOS_PERF_TRACE_VERSION || token.session == UINT64_MAX
	    || token.correlation > UINT32_MAX)
		return {};
	return token;
}

inline void perf_startup_point(const PerfStartupToken &token, uint64_t phase) {
	if (token.version == WOS_PERF_TRACE_VERSION)
		(void)mlibc::sysdep<PerfTrace>(phase, token.session, token.correlation, token.version);
}

} // namespace mlibc
