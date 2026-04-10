#include <mlibc/global-config.hpp>
#include <stdlib.h>
#include <string.h>

namespace mlibc {

// Use a global with controlled initialization order instead of a
// function-local static. Function-local statics go through
// __cxa_guard_acquire which calls pthread_mutex_lock, creating a
// circular dependency during early init on WOS.
static GlobalConfig globalConfigInstance;

const GlobalConfig &globalConfig() { return globalConfigInstance; }

struct GlobalConfigGuard {
	GlobalConfigGuard();
};

GlobalConfigGuard guard;

GlobalConfigGuard::GlobalConfigGuard() {
	// Force the config to be initialized during startup.
	globalConfigInstance.init();
}

static bool envEnabled(const char *env) {
	auto value = getenv(env);
	return value && *value && *value != '0';
}

void GlobalConfig::init() {
	debugMalloc = envEnabled("MLIBC_DEBUG_MALLOC");
	debugPrintf = envEnabled("MLIBC_DEBUG_PRINTF");
	debugLocale = envEnabled("MLIBC_DEBUG_LOCALE");
	debugPthreadTrace = envEnabled("MLIBC_DEBUG_PTHREAD_TRACE");
	debugPathResolution = envEnabled("MLIBC_DEBUG_PATH_RESOLUTION");
	debugMonetaryLengths = envEnabled("MLIBC_DEBUG_MONETARY_LENGTHS");
}

} // namespace mlibc
