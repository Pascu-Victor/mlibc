#ifndef _MLIBC_LOCALE_DATA_H
#define _MLIBC_LOCALE_DATA_H

/*
 * WOS Locale System
 *
 * Provides a POSIX-conformant locale infrastructure for mlibc.
 * Built-in locale definitions: C/POSIX, en_US.UTF-8, de_DE.UTF-8, fr_FR.UTF-8
 *
 * The locale_t opaque handle points to a __mlibc_locale_struct which holds
 * per-category facets. Each facet contains the data needed by nl_langinfo(),
 * localeconv(), strftime(), etc.
 */

#include <bits/locale-internals.h>
#include <limits.h>
#include <locale.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Number of individually-settable categories (excluding LC_ALL). */
#define __MLIBC_NUM_LOCALE_CATEGORIES 13

/* ------------------------------------------------------------------
 *  Per-category facet: time / messages / numeric / monetary / ctype
 * ------------------------------------------------------------------ */

struct __mlibc_time_locale {
	const char *abday[7];   /* Sun..Sat  */
	const char *day[7];     /* Sunday..Saturday */
	const char *abmon[12];  /* Jan..Dec */
	const char *mon[12];    /* January..December */
	const char *am_pm[2];   /* AM, PM */
	const char *d_t_fmt;    /* D_T_FMT  e.g. "%a %b %e %T %Y" */
	const char *d_fmt;      /* D_FMT    e.g. "%m/%d/%y" */
	const char *t_fmt;      /* T_FMT    e.g. "%H:%M:%S" */
	const char *t_fmt_ampm; /* T_FMT_AMPM e.g. "%I:%M:%S %p" */
	const char *era;        /* ERA (empty for Gregorian) */
	const char *era_d_fmt;
	const char *era_t_fmt;
	const char *era_d_t_fmt;
	const char *alt_digits;
};

struct __mlibc_numeric_locale {
	const char *decimal_point;
	const char *thousands_sep;
	const char *grouping;
};

struct __mlibc_monetary_locale {
	const char *int_curr_symbol;
	const char *currency_symbol;
	const char *mon_decimal_point;
	const char *mon_thousands_sep;
	const char *mon_grouping;
	const char *positive_sign;
	const char *negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
	char int_p_cs_precedes;
	char int_p_sep_by_space;
	char int_n_cs_precedes;
	char int_n_sep_by_space;
	char int_p_sign_posn;
	char int_n_sign_posn;
};

struct __mlibc_messages_locale {
	const char *yesexpr;
	const char *noexpr;
	const char *yesstr;
	const char *nostr;
};

struct __mlibc_ctype_locale {
	const char *codeset;
	int mb_cur_max;
};

/* A single "locale object".
 * Each category slot points to a (possibly shared) facet table.
 * category_names[i] is the name string for category i (e.g. "C", "en_US.UTF-8").
 */
struct __mlibc_locale_struct {
	const char *category_names[__MLIBC_NUM_LOCALE_CATEGORIES];

	const struct __mlibc_ctype_locale *lc_ctype;
	const struct __mlibc_numeric_locale *lc_numeric;
	const struct __mlibc_time_locale *lc_time;
	/* lc_collate — not yet implemented; collation uses strcmp */
	const struct __mlibc_monetary_locale *lc_monetary;
	const struct __mlibc_messages_locale *lc_messages;
};

#ifdef __cplusplus
} /* extern "C" */

namespace mlibc {

/* ====================================================================
 *  Built-in locale facet data  (C locale)
 * ==================================================================== */

inline constexpr __mlibc_ctype_locale c_ctype_locale = {
    .codeset = "ANSI_X3.4-1968",
    .mb_cur_max = 1,
};

// We use a more descriptive codeset for UTF-8 locales
inline constexpr __mlibc_ctype_locale utf8_ctype_locale = {
    .codeset = "UTF-8",
    .mb_cur_max = 4,
};

inline constexpr __mlibc_time_locale c_time_locale = {
    .abday = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
    .day = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"},
    .abmon = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"},
    .mon =
        {"January",
         "February",
         "March",
         "April",
         "May",
         "June",
         "July",
         "August",
         "September",
         "October",
         "November",
         "December"},
    .am_pm = {"AM", "PM"},
    .d_t_fmt = "%a %b %e %T %Y",
    .d_fmt = "%m/%d/%y",
    .t_fmt = "%H:%M:%S",
    .t_fmt_ampm = "%I:%M:%S %p",
    .era = "",
    .era_d_fmt = "",
    .era_t_fmt = "",
    .era_d_t_fmt = "",
    .alt_digits = "",
};

inline constexpr __mlibc_numeric_locale c_numeric_locale = {
    .decimal_point = ".",
    .thousands_sep = "",
    .grouping = "",
};

inline constexpr __mlibc_monetary_locale c_monetary_locale = {
    .int_curr_symbol = "",
    .currency_symbol = "",
    .mon_decimal_point = "",
    .mon_thousands_sep = "",
    .mon_grouping = "",
    .positive_sign = "",
    .negative_sign = "",
    .int_frac_digits = CHAR_MAX,
    .frac_digits = CHAR_MAX,
    .p_cs_precedes = CHAR_MAX,
    .p_sep_by_space = CHAR_MAX,
    .n_cs_precedes = CHAR_MAX,
    .n_sep_by_space = CHAR_MAX,
    .p_sign_posn = CHAR_MAX,
    .n_sign_posn = CHAR_MAX,
    .int_p_cs_precedes = CHAR_MAX,
    .int_p_sep_by_space = CHAR_MAX,
    .int_n_cs_precedes = CHAR_MAX,
    .int_n_sep_by_space = CHAR_MAX,
    .int_p_sign_posn = CHAR_MAX,
    .int_n_sign_posn = CHAR_MAX,
};

inline constexpr __mlibc_messages_locale c_messages_locale = {
    .yesexpr = "^[yY]",
    .noexpr = "^[nN]",
    .yesstr = "yes",
    .nostr = "no",
};

/* ====================================================================
 *  Built-in locale facet data  (en_US.UTF-8)
 * ==================================================================== */

inline constexpr __mlibc_time_locale en_us_time_locale = {
    .abday = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
    .day = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"},
    .abmon = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"},
    .mon =
        {"January",
         "February",
         "March",
         "April",
         "May",
         "June",
         "July",
         "August",
         "September",
         "October",
         "November",
         "December"},
    .am_pm = {"AM", "PM"},
    .d_t_fmt = "%a %d %b %Y %r %Z",
    .d_fmt = "%m/%d/%Y",
    .t_fmt = "%r",
    .t_fmt_ampm = "%I:%M:%S %p",
    .era = "",
    .era_d_fmt = "",
    .era_t_fmt = "",
    .era_d_t_fmt = "",
    .alt_digits = "",
};

inline constexpr __mlibc_numeric_locale en_us_numeric_locale = {
    .decimal_point = ".",
    .thousands_sep = ",",
    .grouping = "\x03\x03",
};

inline constexpr __mlibc_monetary_locale en_us_monetary_locale = {
    .int_curr_symbol = "USD ",
    .currency_symbol = "$",
    .mon_decimal_point = ".",
    .mon_thousands_sep = ",",
    .mon_grouping = "\x03\x03",
    .positive_sign = "",
    .negative_sign = "-",
    .int_frac_digits = 2,
    .frac_digits = 2,
    .p_cs_precedes = 1,
    .p_sep_by_space = 0,
    .n_cs_precedes = 1,
    .n_sep_by_space = 0,
    .p_sign_posn = 1,
    .n_sign_posn = 1,
    .int_p_cs_precedes = 1,
    .int_p_sep_by_space = 1,
    .int_n_cs_precedes = 1,
    .int_n_sep_by_space = 1,
    .int_p_sign_posn = 1,
    .int_n_sign_posn = 1,
};

inline constexpr __mlibc_messages_locale en_us_messages_locale = {
    .yesexpr = "^[yY]",
    .noexpr = "^[nN]",
    .yesstr = "yes",
    .nostr = "no",
};

/* ====================================================================
 *  Built-in locale facet data  (de_DE.UTF-8)
 * ==================================================================== */

inline constexpr __mlibc_time_locale de_de_time_locale = {
    .abday = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"},
    .day = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"},
    .abmon = {"Jan", "Feb", "Mär", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"},
    .mon =
        {"Januar",
         "Februar",
         "März",
         "April",
         "Mai",
         "Juni",
         "Juli",
         "August",
         "September",
         "Oktober",
         "November",
         "Dezember"},
    .am_pm = {"", ""},
    .d_t_fmt = "%a %d %b %Y %T %Z",
    .d_fmt = "%d.%m.%Y",
    .t_fmt = "%T",
    .t_fmt_ampm = "",
    .era = "",
    .era_d_fmt = "",
    .era_t_fmt = "",
    .era_d_t_fmt = "",
    .alt_digits = "",
};

inline constexpr __mlibc_numeric_locale de_de_numeric_locale = {
    .decimal_point = ",",
    .thousands_sep = ".",
    .grouping = "\x03\x03",
};

inline constexpr __mlibc_monetary_locale de_de_monetary_locale = {
    .int_curr_symbol = "EUR ",
    .currency_symbol = "€",
    .mon_decimal_point = ",",
    .mon_thousands_sep = ".",
    .mon_grouping = "\x03\x03",
    .positive_sign = "",
    .negative_sign = "-",
    .int_frac_digits = 2,
    .frac_digits = 2,
    .p_cs_precedes = 0,
    .p_sep_by_space = 1,
    .n_cs_precedes = 0,
    .n_sep_by_space = 1,
    .p_sign_posn = 1,
    .n_sign_posn = 1,
    .int_p_cs_precedes = 0,
    .int_p_sep_by_space = 1,
    .int_n_cs_precedes = 0,
    .int_n_sep_by_space = 1,
    .int_p_sign_posn = 1,
    .int_n_sign_posn = 1,
};

inline constexpr __mlibc_messages_locale de_de_messages_locale = {
    .yesexpr = "^[jJyY]",
    .noexpr = "^[nN]",
    .yesstr = "ja",
    .nostr = "nein",
};

/* ====================================================================
 *  Built-in locale facet data  (fr_FR.UTF-8)
 * ==================================================================== */

inline constexpr __mlibc_time_locale fr_fr_time_locale = {
    .abday = {"dim.", "lun.", "mar.", "mer.", "jeu.", "ven.", "sam."},
    .day = {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"},
    .abmon =
        {"janv.",
         "févr.",
         "mars",
         "avr.",
         "mai",
         "juin",
         "juil.",
         "août",
         "sept.",
         "oct.",
         "nov.",
         "déc."},
    .mon =
        {"janvier",
         "février",
         "mars",
         "avril",
         "mai",
         "juin",
         "juillet",
         "août",
         "septembre",
         "octobre",
         "novembre",
         "décembre"},
    .am_pm = {"", ""},
    .d_t_fmt = "%a %d %b %Y %T %Z",
    .d_fmt = "%d/%m/%Y",
    .t_fmt = "%T",
    .t_fmt_ampm = "",
    .era = "",
    .era_d_fmt = "",
    .era_t_fmt = "",
    .era_d_t_fmt = "",
    .alt_digits = "",
};

inline constexpr __mlibc_numeric_locale fr_fr_numeric_locale = {
    .decimal_point = ",",
    .thousands_sep = "\u202F", /* narrow no-break space */
    .grouping = "\x03\x03",
};

inline constexpr __mlibc_monetary_locale fr_fr_monetary_locale = {
    .int_curr_symbol = "EUR ",
    .currency_symbol = "€",
    .mon_decimal_point = ",",
    .mon_thousands_sep = "\u202F",
    .mon_grouping = "\x03\x03",
    .positive_sign = "",
    .negative_sign = "-",
    .int_frac_digits = 2,
    .frac_digits = 2,
    .p_cs_precedes = 0,
    .p_sep_by_space = 1,
    .n_cs_precedes = 0,
    .n_sep_by_space = 1,
    .p_sign_posn = 1,
    .n_sign_posn = 1,
    .int_p_cs_precedes = 0,
    .int_p_sep_by_space = 1,
    .int_n_cs_precedes = 0,
    .int_n_sep_by_space = 1,
    .int_p_sign_posn = 1,
    .int_n_sign_posn = 1,
};

inline constexpr __mlibc_messages_locale fr_fr_messages_locale = {
    .yesexpr = "^[oOyY]",
    .noexpr = "^[nN]",
    .yesstr = "oui",
    .nostr = "non",
};

/* ====================================================================
 *  Built-in composite locale objects
 * ==================================================================== */

/* The C/POSIX locale object — all categories set to C defaults. */
inline constexpr __mlibc_locale_struct __mlibc_c_locale_obj = {
    .category_names = {"C", "C", "C", "C", "C", "C", "C", "C", "C", "C", "C", "C", "C"},
    .lc_ctype = &c_ctype_locale,
    .lc_numeric = &c_numeric_locale,
    .lc_time = &c_time_locale,
    .lc_monetary = &c_monetary_locale,
    .lc_messages = &c_messages_locale,
};

inline constexpr __mlibc_locale_struct __mlibc_en_us_utf8_locale_obj = {
    .category_names =
        {"en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8",
         "en_US.UTF-8"},
    .lc_ctype = &utf8_ctype_locale,
    .lc_numeric = &en_us_numeric_locale,
    .lc_time = &en_us_time_locale,
    .lc_monetary = &en_us_monetary_locale,
    .lc_messages = &en_us_messages_locale,
};

inline constexpr __mlibc_locale_struct __mlibc_de_de_utf8_locale_obj = {
    .category_names =
        {"de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8",
         "de_DE.UTF-8"},
    .lc_ctype = &utf8_ctype_locale,
    .lc_numeric = &de_de_numeric_locale,
    .lc_time = &de_de_time_locale,
    .lc_monetary = &de_de_monetary_locale,
    .lc_messages = &de_de_messages_locale,
};

inline constexpr __mlibc_locale_struct __mlibc_fr_fr_utf8_locale_obj = {
    .category_names =
        {"fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8",
         "fr_FR.UTF-8"},
    .lc_ctype = &utf8_ctype_locale,
    .lc_numeric = &fr_fr_numeric_locale,
    .lc_time = &fr_fr_time_locale,
    .lc_monetary = &fr_fr_monetary_locale,
    .lc_messages = &fr_fr_messages_locale,
};

/* ------------------------------------------------------------------
 * Look up a built-in locale by name.
 * Returns nullptr if the name is not a known built-in.
 * ------------------------------------------------------------------ */
const __mlibc_locale_struct *__mlibc_lookup_builtin_locale(const char *name);

/* ------------------------------------------------------------------
 * Global locale (the per-thread current locale and the process-wide
 * global locale used by setlocale()).
 * ------------------------------------------------------------------ */

/* The process-wide global locale object (mutable, used by setlocale). */
extern __mlibc_locale_struct __mlibc_global_locale;

/* Per-thread locale set by uselocale(). nullptr means "use global". */
extern thread_local __mlibc_locale_struct *__mlibc_thread_locale;

/* Get the effective locale for the calling thread.
 * Returns the thread-local locale if set, otherwise the global locale. */
__mlibc_locale_struct *__mlibc_get_effective_locale();

/* Get the time facet from a locale_t (handling LC_GLOBAL_LOCALE and nullptr). */
const __mlibc_time_locale *__mlibc_get_time_locale(locale_t loc);

/* Get the numeric facet from a locale_t. */
const __mlibc_numeric_locale *__mlibc_get_numeric_locale(locale_t loc);

/* Get the monetary facet from a locale_t. */
const __mlibc_monetary_locale *__mlibc_get_monetary_locale(locale_t loc);

/* Get the messages facet from a locale_t. */
const __mlibc_messages_locale *__mlibc_get_messages_locale(locale_t loc);

/* Get the ctype facet from a locale_t. */
const __mlibc_ctype_locale *__mlibc_get_ctype_locale(locale_t loc);

/* nl_langinfo implementation that uses a specific locale's data. */
char *__mlibc_nl_langinfo_l(int item, locale_t loc);

} /* namespace mlibc */

#endif /* __cplusplus */

#endif /* _MLIBC_LOCALE_DATA_H */
