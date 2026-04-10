#ifndef MLIBC_GLOBAL_CONFIG
#define MLIBC_GLOBAL_CONFIG

namespace mlibc {

struct GlobalConfig {
	void init();

	bool debugMalloc;
	bool debugPrintf;
	bool debugLocale;
	bool debugPthreadTrace;
	bool debugPathResolution;
	bool debugMonetaryLengths;
};

// Defined in global-config.cpp. Uses a global (not function-local static)
// to avoid __cxa_guard_acquire which depends on pthread_mutex_lock.
const GlobalConfig &globalConfig();

} // namespace mlibc

#endif // MLIBC_GLOBAL_CONFIG
