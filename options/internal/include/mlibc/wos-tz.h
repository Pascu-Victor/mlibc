/*
 * WOS Independent Timekeeping / Timezone Library
 *
 * A standalone, self-contained timezone database that embeds POSIX TZ rule
 * strings for common timezones. This library is independent of the locale
 * system and can be used on its own, but is also integrated with the WOS
 * locale infrastructure for locale-aware time formatting.
 *
 * Usage:
 *   #include <mlibc/wos-tz.h>
 *
 *   // Lookup a timezone by IANA name
 *   const wos_tz_entry *tz = wos_tz_find("America/New_York");
 *   if (tz) setenv("TZ", tz->posix_tz, 1);
 *
 *   // Get the default timezone for a locale name
 *   const char *tz_str = wos_tz_for_locale("de_DE.UTF-8");
 *   // Returns "CET-1CEST,M3.5.0,M10.5.0/3"
 *
 *   // Iterate all known timezones
 *   for (const auto *e = wos_tz_table; e->iana_name; ++e) { ... }
 */

#ifndef _MLIBC_WOS_TZ_H
#define _MLIBC_WOS_TZ_H

#ifdef __cplusplus
extern "C" {
#endif

struct wos_tz_entry {
	const char *iana_name;   /* IANA timezone name (e.g. "America/New_York") */
	const char *posix_tz;    /* POSIX TZ string (e.g. "EST5EDT,M3.2.0,M11.1.0") */
	int utc_offset;          /* Standard-time offset from UTC in seconds (east = positive) */
	int has_dst;             /* 1 if DST is observed, 0 otherwise */
	const char *description; /* Human-readable description */
};

/*
 * The built-in timezone table, terminated by a sentinel entry with
 * iana_name == NULL.
 */
extern const struct wos_tz_entry wos_tz_table[];

/*
 * Look up a timezone by its IANA name. Returns NULL if not found.
 * The search is case-sensitive.
 */
const struct wos_tz_entry *wos_tz_find(const char *iana_name);

/*
 * Look up a timezone by approximate UTC offset (in seconds, east = positive).
 * If `want_dst` is non-zero, prefer zones that observe DST.
 * Returns the first match, or NULL if no zone matches.
 */
const struct wos_tz_entry *wos_tz_find_by_offset(int utc_offset_sec, int want_dst);

/*
 * Return the POSIX TZ string associated with the given locale name.
 * Falls back to "UTC0" if the locale is not recognized.
 */
const char *wos_tz_for_locale(const char *locale_name);

/*
 * Return the number of entries in the built-in timezone table
 * (excluding the sentinel).
 */
int wos_tz_count(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _MLIBC_WOS_TZ_H */
