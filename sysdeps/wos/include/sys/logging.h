#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <callnums/sys_log.h>
#include <sys/callnums.h>
#include <sys/syscall.h>

namespace ker::logging {

uint64_t log(const char *str, uint64_t len, abi::sys_log::sys_log_device device);

uint64_t logLine(const char *str, uint64_t len, abi::sys_log::sys_log_device device);

uint64_t
logEx(const char *module, abi::sys_log::sys_log_level level, const char *str, uint64_t len);

inline uint64_t
vlogExf(const char *module, abi::sys_log::sys_log_level level, const char *fmt, va_list args) {
	if (fmt == nullptr) {
		return 0;
	}

	char buf[abi::sys_log::JOURNAL_MESSAGE_MAX]{};
	int rc = vsnprintf(buf, sizeof(buf), fmt, args);
	if (rc < 0) {
		return 0;
	}

	return logEx(module, level, buf, strlen(buf));
}

template <typename... Args>
inline uint64_t
logExf(const char *module, abi::sys_log::sys_log_level level, const char *fmt, Args... args) {
	if (fmt == nullptr) {
		return 0;
	}

	char buf[abi::sys_log::JOURNAL_MESSAGE_MAX]{};
	int rc = snprintf(buf, sizeof(buf), fmt, args...);
	if (rc < 0) {
		return 0;
	}

	return logEx(module, level, buf, strlen(buf));
}

} // namespace ker::logging

namespace ker::mod::dbg {

template <size_t N>
struct fixed_string {
	char value[N]{};

	consteval fixed_string(const char (&str)[N]) {
		for (size_t i = 0; i < N; i++) {
			value[i] = str[i];
		}
	}

	[[nodiscard]] constexpr const char *c_str() const { return value; }
};

template <fixed_string Tag>
struct logger {
	static_assert(
	    (sizeof(Tag.value) - 1) <= abi::sys_log::JOURNAL_MODULE_MAX,
	    "logger tag exceeds JOURNAL_MODULE_MAX"
	);

	template <typename... Args>
	[[gnu::always_inline]] static inline void
	log(abi::sys_log::sys_log_level level, const char *fmt, Args... args) {
		ker::logging::logExf(Tag.c_str(), level, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void trace(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::trace, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void debug(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::debug, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void info(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::info, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void notice(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::notice, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void warn(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::warn, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void error(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::error, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void critical(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::critical, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void panic(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::panic, fmt, args...);
	}
};

} // namespace ker::mod::dbg

namespace wos {

template <ker::mod::dbg::fixed_string Tag>
using journal = ker::mod::dbg::logger<Tag>;

} // namespace wos
