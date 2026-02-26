/*
 * WOS Independent Timekeeping / Timezone Library — Implementation
 *
 * Embeds POSIX TZ strings for common world timezones covering every
 * UTC offset from -12 to +14.  The table is sorted by UTC offset.
 *
 * This file is self-contained and does not depend on the locale system.
 */

#include <mlibc/wos-tz.h>
#include <string.h>

/* ====================================================================
 *  Built-in timezone table
 *
 *  Each entry: { IANA name, POSIX TZ string, UTC offset (sec), DST?, description }
 *
 *  POSIX TZ format recap:
 *    std offset [dst [offset] [,start[/time],end[/time]]]
 *  Offset sign: POSIX convention is west-positive (opposite of ISO).
 *    EST5EDT  → UTC-5 standard, UTC-4 DST
 *    CET-1CEST → UTC+1 standard, UTC+2 DST
 * ==================================================================== */

const struct wos_tz_entry wos_tz_table[] = {
    /* UTC-12 */
    {"Etc/GMT+12", "GMT+12", -43200, 0, "Baker Island, Howland Island"},

    /* UTC-11 */
    {"Pacific/Pago_Pago", "SST11", -39600, 0, "American Samoa"},

    /* UTC-10 */
    {"Pacific/Honolulu", "HST10", -36000, 0, "Hawaii"},

    /* UTC-9 */
    {"America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0", -32400, 1, "Alaska"},

    /* UTC-8 */
    {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0", -28800, 1, "US Pacific Time"},

    /* UTC-7 */
    {"America/Denver", "MST7MDT,M3.2.0,M11.1.0", -25200, 1, "US Mountain Time"},

    {"America/Phoenix", "MST7", -25200, 0, "Arizona (no DST)"},

    /* UTC-6 */
    {"America/Chicago", "CST6CDT,M3.2.0,M11.1.0", -21600, 1, "US Central Time"},

    {"America/Mexico_City", "CST6", -21600, 0, "Mexico City (no DST since 2022)"},

    /* UTC-5 */
    {"America/New_York", "EST5EDT,M3.2.0,M11.1.0", -18000, 1, "US Eastern Time"},

    {"America/Bogota", "COT5", -18000, 0, "Colombia"},

    /* UTC-4 */
    {"America/Halifax", "AST4ADT,M3.2.0,M11.1.0", -14400, 1, "Atlantic Time (Canada)"},

    {"America/Caracas", "VET4", -14400, 0, "Venezuela"},

    /* UTC-3:30 */
    {"America/St_Johns", "NST3:30NDT,M3.2.0,M11.1.0", -12600, 1, "Newfoundland"},

    /* UTC-3 */
    {"America/Sao_Paulo", "BRT3", -10800, 0, "Brazil (Brasilia)"},

    {"America/Argentina/Buenos_Aires", "ART3", -10800, 0, "Argentina"},

    /* UTC-2 */
    {"Etc/GMT+2", "GST2", -7200, 0, "South Georgia"},

    /* UTC-1 */
    {"Atlantic/Azores", "AZOT1AZOST,M3.5.0/0,M10.5.0/1", -3600, 1, "Azores"},

    /* UTC+0 */
    {"Etc/UTC", "UTC0", 0, 0, "Coordinated Universal Time"},

    {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0", 0, 1, "United Kingdom"},

    /* UTC+1 */
    {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3", 3600, 1, "Central European Time"},

    {"Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3", 3600, 1, "France"},

    {"Europe/Rome", "CET-1CEST,M3.5.0,M10.5.0/3", 3600, 1, "Italy"},

    {"Europe/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3", 3600, 1, "Spain"},

    {"Africa/Lagos", "WAT-1", 3600, 0, "West Africa Time"},

    /* UTC+2 */
    {"Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4", 7200, 1, "Eastern European Time"},

    {"Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4", 7200, 1, "Greece"},

    {"Africa/Cairo", "EET-2", 7200, 0, "Egypt"},

    {"Africa/Johannesburg", "SAST-2", 7200, 0, "South Africa"},

    /* UTC+3 */
    {"Europe/Moscow", "MSK-3", 10800, 0, "Moscow"},

    {"Asia/Riyadh", "AST-3", 10800, 0, "Saudi Arabia"},

    {"Africa/Nairobi", "EAT-3", 10800, 0, "East Africa Time"},

    /* UTC+3:30 */
    {"Asia/Tehran", "IRST-3:30", 12600, 0, "Iran (no DST since 2022)"},

    /* UTC+4 */
    {"Asia/Dubai", "GST-4", 14400, 0, "UAE"},

    /* UTC+4:30 */
    {"Asia/Kabul", "AFT-4:30", 16200, 0, "Afghanistan"},

    /* UTC+5 */
    {"Asia/Karachi", "PKT-5", 18000, 0, "Pakistan"},

    {"Asia/Tashkent", "UZT-5", 18000, 0, "Uzbekistan"},

    /* UTC+5:30 */
    {"Asia/Kolkata", "IST-5:30", 19800, 0, "India"},

    /* UTC+5:45 */
    {"Asia/Kathmandu", "NPT-5:45", 20700, 0, "Nepal"},

    /* UTC+6 */
    {"Asia/Dhaka", "BDT-6", 21600, 0, "Bangladesh"},

    {"Asia/Almaty", "ALMT-6", 21600, 0, "Kazakhstan (East)"},

    /* UTC+6:30 */
    {"Asia/Yangon", "MMT-6:30", 23400, 0, "Myanmar"},

    /* UTC+7 */
    {"Asia/Bangkok", "ICT-7", 25200, 0, "Thailand, Vietnam, Cambodia"},

    {"Asia/Jakarta", "WIB-7", 25200, 0, "Indonesia (West)"},

    /* UTC+8 */
    {"Asia/Shanghai", "CST-8", 28800, 0, "China"},

    {"Asia/Hong_Kong", "HKT-8", 28800, 0, "Hong Kong"},

    {"Asia/Taipei", "CST-8", 28800, 0, "Taiwan"},

    {"Asia/Singapore", "SGT-8", 28800, 0, "Singapore"},

    {"Australia/Perth", "AWST-8", 28800, 0, "Western Australia"},

    /* UTC+8:45 */
    {"Australia/Eucla", "ACWST-8:45", 31500, 0, "Australia (Central Western)"},

    /* UTC+9 */
    {"Asia/Tokyo", "JST-9", 32400, 0, "Japan"},

    {"Asia/Seoul", "KST-9", 32400, 0, "South Korea"},

    /* UTC+9:30 */
    {"Australia/Darwin", "ACST-9:30", 34200, 0, "Australia (Northern Territory)"},

    {"Australia/Adelaide",
     "ACST-9:30ACDT,M10.1.0,M4.1.0/3",
     34200,
     1,
     "Australia (South Australia)"},

    /* UTC+10 */
    {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3", 36000, 1, "Australia (East, DST)"},

    {"Australia/Brisbane", "AEST-10", 36000, 0, "Australia (Queensland, no DST)"},

    {"Pacific/Guam", "ChST-10", 36000, 0, "Guam"},

    /* UTC+11 */
    {"Pacific/Noumea", "NCT-11", 39600, 0, "New Caledonia"},

    /* UTC+12 */
    {"Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3", 43200, 1, "New Zealand"},

    {"Pacific/Fiji", "FJT-12", 43200, 0, "Fiji"},

    /* UTC+13 */
    {"Pacific/Tongatapu", "TOT-13", 46800, 0, "Tonga"},

    /* UTC+14 */
    {"Pacific/Kiritimati", "LINT-14", 50400, 0, "Line Islands (Kiribati)"},

    /* Sentinel */
    {nullptr, nullptr, 0, 0, nullptr},
};

/* ====================================================================
 *  Locale → timezone mapping
 * ==================================================================== */

struct locale_tz_map {
	const char *locale_prefix; /* matched against start of locale name */
	const char *iana_name;     /* IANA timezone to use */
};

static const struct locale_tz_map locale_tz_table[] = {
    /* English-speaking */
    {"en_US", "America/New_York"},
    {"en_GB", "Europe/London"},
    {"en_AU", "Australia/Sydney"},
    {"en_NZ", "Pacific/Auckland"},
    {"en_CA", "America/New_York"},
    {"en_IN", "Asia/Kolkata"},
    {"en_ZA", "Africa/Johannesburg"},
    {"en_SG", "Asia/Singapore"},
    {"en_HK", "Asia/Hong_Kong"},

    /* European */
    {"de_DE", "Europe/Berlin"},
    {"de_AT", "Europe/Berlin"},
    {"de_CH", "Europe/Berlin"},
    {"fr_FR", "Europe/Paris"},
    {"fr_BE", "Europe/Paris"},
    {"fr_CH", "Europe/Paris"},
    {"fr_CA", "America/New_York"},
    {"it_IT", "Europe/Rome"},
    {"es_ES", "Europe/Madrid"},
    {"es_MX", "America/Mexico_City"},
    {"es_AR", "America/Argentina/Buenos_Aires"},
    {"pt_BR", "America/Sao_Paulo"},
    {"pt_PT", "Europe/London"},
    {"nl_NL", "Europe/Berlin"},
    {"nl_BE", "Europe/Berlin"},
    {"sv_SE", "Europe/Berlin"},
    {"da_DK", "Europe/Berlin"},
    {"nb_NO", "Europe/Berlin"},
    {"nn_NO", "Europe/Berlin"},
    {"fi_FI", "Europe/Helsinki"},
    {"el_GR", "Europe/Athens"},
    {"pl_PL", "Europe/Berlin"},
    {"cs_CZ", "Europe/Berlin"},
    {"sk_SK", "Europe/Berlin"},
    {"hu_HU", "Europe/Berlin"},
    {"ro_RO", "Europe/Helsinki"},
    {"bg_BG", "Europe/Helsinki"},
    {"uk_UA", "Europe/Helsinki"},
    {"hr_HR", "Europe/Berlin"},
    {"sl_SI", "Europe/Berlin"},

    /* Eastern Europe / Russia */
    {"ru_RU", "Europe/Moscow"},
    {"tr_TR", "Europe/Moscow"},

    /* Middle East */
    {"ar_SA", "Asia/Riyadh"},
    {"ar_EG", "Africa/Cairo"},
    {"he_IL", "Europe/Helsinki"},
    {"fa_IR", "Asia/Tehran"},

    /* Asia */
    {"zh_CN", "Asia/Shanghai"},
    {"zh_TW", "Asia/Taipei"},
    {"zh_HK", "Asia/Hong_Kong"},
    {"ja_JP", "Asia/Tokyo"},
    {"ko_KR", "Asia/Seoul"},
    {"hi_IN", "Asia/Kolkata"},
    {"bn_IN", "Asia/Kolkata"},
    {"ta_IN", "Asia/Kolkata"},
    {"th_TH", "Asia/Bangkok"},
    {"vi_VN", "Asia/Bangkok"},
    {"id_ID", "Asia/Jakarta"},
    {"ms_MY", "Asia/Singapore"},

    /* Africa */
    {"sw_KE", "Africa/Nairobi"},

    /* C / POSIX locales → UTC */
    {"C", "Etc/UTC"},
    {"POSIX", "Etc/UTC"},

    /* Sentinel */
    {nullptr, nullptr},
};

/* ====================================================================
 *  API implementation
 * ==================================================================== */

const struct wos_tz_entry *wos_tz_find(const char *iana_name) {
	if (!iana_name)
		return nullptr;
	for (const auto *e = wos_tz_table; e->iana_name; ++e) {
		if (!strcmp(e->iana_name, iana_name))
			return e;
	}
	return nullptr;
}

const struct wos_tz_entry *wos_tz_find_by_offset(int utc_offset_sec, int want_dst) {
	/* First pass: exact match on offset + DST preference. */
	for (const auto *e = wos_tz_table; e->iana_name; ++e) {
		if (e->utc_offset == utc_offset_sec && e->has_dst == want_dst)
			return e;
	}
	/* Second pass: match offset regardless of DST. */
	for (const auto *e = wos_tz_table; e->iana_name; ++e) {
		if (e->utc_offset == utc_offset_sec)
			return e;
	}
	return nullptr;
}

const char *wos_tz_for_locale(const char *locale_name) {
	if (!locale_name || !*locale_name)
		return "UTC0";

	/* Match the longest prefix in the locale_tz_table. */
	const struct locale_tz_map *best = nullptr;
	size_t best_len = 0;

	for (const auto *m = locale_tz_table; m->locale_prefix; ++m) {
		size_t plen = strlen(m->locale_prefix);
		if (plen > best_len && !strncmp(locale_name, m->locale_prefix, plen)) {
			best = m;
			best_len = plen;
		}
	}

	if (best) {
		const auto *tz = wos_tz_find(best->iana_name);
		if (tz)
			return tz->posix_tz;
	}

	return "UTC0";
}

int wos_tz_count(void) {
	int n = 0;
	for (const auto *e = wos_tz_table; e->iana_name; ++e)
		n++;
	return n;
}
