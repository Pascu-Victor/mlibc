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

uint64_t beginLogBlock();

uint64_t endLogBlock(uint64_t cookie);

uint64_t
logBlock(uint64_t cookie, const char *str, uint64_t len, abi::sys_log::sys_log_device device);

uint64_t
logLineBlock(uint64_t cookie, const char *str, uint64_t len, abi::sys_log::sys_log_device device);

uint64_t logExBlock(
    uint64_t cookie,
    const char *module,
    abi::sys_log::sys_log_level level,
    const char *str,
    uint64_t len
);

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

inline uint64_t logExfBlock(
    uint64_t cookie, const char *module, abi::sys_log::sys_log_level level, const char *fmt, ...
) {
	if (fmt == nullptr) {
		return 0;
	}

	char buf[abi::sys_log::JOURNAL_MESSAGE_MAX]{};
	va_list args;
	va_start(args, fmt);
	int rc = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	if (rc < 0) {
		return 0;
	}

	return logExBlock(cookie, module, level, buf, strlen(buf));
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

struct fixed_string {
	size_t size{};
	char value[abi::sys_log::JOURNAL_MODULE_MAX + 1]{};

	template <size_t N>
	consteval fixed_string(const char (&str)[N]) : size(N - 1) {
		static_assert(
		    N <= (abi::sys_log::JOURNAL_MODULE_MAX + 1), "logger tag exceeds JOURNAL_MODULE_MAX"
		);
		for (size_t i = 0; i < N; i++) {
			value[i] = str[i];
		}
	}

	[[nodiscard]] constexpr const char *c_str() const { return value; }
};

template <fixed_string Tag>
struct logger {
	static_assert(
	    Tag.size <= abi::sys_log::JOURNAL_MODULE_MAX, "logger tag exceeds JOURNAL_MODULE_MAX"
	);

	template <typename... Args>
	[[gnu::always_inline]] static inline void
	log(abi::sys_log::sys_log_level level, const char *fmt, Args... args) {
		ker::logging::logExf(Tag.c_str(), level, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void trace(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::TRACE, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void debug(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::DEBUG, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void info(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::INFO, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void notice(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::NOTICE, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void warn(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::WARN, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void error(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::ERROR, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void critical(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::CRITICAL, fmt, args...);
	}

	template <typename... Args>
	[[gnu::always_inline]] static inline void panic(const char *fmt, Args... args) {
		log(abi::sys_log::sys_log_level::PANIC, fmt, args...);
	}
};

} // namespace ker::mod::dbg

namespace wos {

template <ker::mod::dbg::fixed_string Tag>
using journal = ker::mod::dbg::logger<Tag>;

} // namespace wos
