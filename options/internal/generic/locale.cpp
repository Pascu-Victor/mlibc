#include <bits/ensure.h>
#include <mlibc/debug.hpp>
#include <mlibc/locale-data.h>
#include <mlibc/locale.hpp>
#include <string.h>

namespace mlibc {

/* ====================================================================
 *  Global / thread-local locale state
 * ==================================================================== */

__mlibc_locale_struct __mlibc_global_locale = {
    .category_names = {"C", "C", "C", "C", "C", "C", "C", "C", "C", "C", "C", "C", "C"},
    .lc_ctype = &c_ctype_locale,
    .lc_numeric = &c_numeric_locale,
    .lc_time = &c_time_locale,
    .lc_monetary = &c_monetary_locale,
    .lc_messages = &c_messages_locale,
};

thread_local __mlibc_locale_struct *__mlibc_thread_locale = nullptr;

__mlibc_locale_struct *__mlibc_get_effective_locale() {
	if (__mlibc_thread_locale)
		return __mlibc_thread_locale;
	return &__mlibc_global_locale;
}

/* ====================================================================
 *  Built-in locale registry
 * ==================================================================== */

struct locale_entry {
	const char *name;
	const __mlibc_locale_struct *obj;
};

static constexpr locale_entry builtin_locales[] = {
    {"C", &__mlibc_c_locale_obj},
    {"POSIX", &__mlibc_c_locale_obj},
    {"en_US.UTF-8", &__mlibc_en_us_utf8_locale_obj},
    {"en_US.utf8", &__mlibc_en_us_utf8_locale_obj},
    {"de_DE.UTF-8", &__mlibc_de_de_utf8_locale_obj},
    {"de_DE.utf8", &__mlibc_de_de_utf8_locale_obj},
    {"fr_FR.UTF-8", &__mlibc_fr_fr_utf8_locale_obj},
    {"fr_FR.utf8", &__mlibc_fr_fr_utf8_locale_obj},
    {nullptr, nullptr},
};

const __mlibc_locale_struct *__mlibc_lookup_builtin_locale(const char *name) {
	if (!name || !*name)
		return &__mlibc_c_locale_obj;
	for (auto *e = builtin_locales; e->name; ++e) {
		if (!strcmp(e->name, name))
			return e->obj;
	}
	return nullptr;
}

/* ====================================================================
 *  Facet accessors (handle nullptr / LC_GLOBAL_LOCALE)
 * ==================================================================== */

const __mlibc_time_locale *__mlibc_get_time_locale(locale_t loc) {
	auto *l = loc;
	if (!l || l == (locale_t)-1L)
		l = __mlibc_get_effective_locale();
	return l->lc_time;
}

const __mlibc_numeric_locale *__mlibc_get_numeric_locale(locale_t loc) {
	auto *l = loc;
	if (!l || l == (locale_t)-1L)
		l = __mlibc_get_effective_locale();
	return l->lc_numeric;
}

const __mlibc_monetary_locale *__mlibc_get_monetary_locale(locale_t loc) {
	auto *l = loc;
	if (!l || l == (locale_t)-1L)
		l = __mlibc_get_effective_locale();
	return l->lc_monetary;
}

const __mlibc_messages_locale *__mlibc_get_messages_locale(locale_t loc) {
	auto *l = loc;
	if (!l || l == (locale_t)-1L)
		l = __mlibc_get_effective_locale();
	return l->lc_messages;
}

const __mlibc_ctype_locale *__mlibc_get_ctype_locale(locale_t loc) {
	auto *l = loc;
	if (!l || l == (locale_t)-1L)
		l = __mlibc_get_effective_locale();
	return l->lc_ctype;
}

/* ====================================================================
 *  nl_langinfo implementation using locale facet data
 * ==================================================================== */

static char *nl_langinfo_from_facets(
    const __mlibc_ctype_locale *ct,
    const __mlibc_time_locale *tm,
    const __mlibc_numeric_locale *num,
    const __mlibc_messages_locale *msg,
    nl_item item
) {
	if (item == CODESET) {
		return const_cast<char *>(ct->codeset);
	} else if (item >= ABMON_1 && item <= ABMON_12) {
		return const_cast<char *>(tm->abmon[item - ABMON_1]);
	} else if (item >= MON_1 && item <= MON_12) {
		return const_cast<char *>(tm->mon[item - MON_1]);
	} else if (item == AM_STR) {
		return const_cast<char *>(tm->am_pm[0]);
	} else if (item == PM_STR) {
		return const_cast<char *>(tm->am_pm[1]);
	} else if (item >= DAY_1 && item <= DAY_7) {
		return const_cast<char *>(tm->day[item - DAY_1]);
	} else if (item >= ABDAY_1 && item <= ABDAY_7) {
		return const_cast<char *>(tm->abday[item - ABDAY_1]);
	} else if (item == D_FMT) {
		return const_cast<char *>(tm->d_fmt);
	} else if (item == T_FMT) {
		return const_cast<char *>(tm->t_fmt);
	} else if (item == T_FMT_AMPM) {
		return const_cast<char *>(tm->t_fmt_ampm);
	} else if (item == D_T_FMT) {
		return const_cast<char *>(tm->d_t_fmt);
	} else if (item == ERA) {
		return const_cast<char *>(tm->era);
	} else if (item == ERA_D_FMT) {
		return const_cast<char *>(tm->era_d_fmt);
	} else if (item == ERA_T_FMT) {
		return const_cast<char *>(tm->era_t_fmt);
	} else if (item == ERA_D_T_FMT) {
		return const_cast<char *>(tm->era_d_t_fmt);
	} else if (item == ALT_DIGITS) {
		return const_cast<char *>(tm->alt_digits);
	} else if (item == RADIXCHAR) {
		return const_cast<char *>(num->decimal_point);
	} else if (item == THOUSEP) {
		return const_cast<char *>(num->thousands_sep);
	} else if (item == YESEXPR) {
		return const_cast<char *>(msg->yesexpr);
	} else if (item == NOEXPR) {
		return const_cast<char *>(msg->noexpr);
	} else if (item == __YESSTR) {
		return const_cast<char *>(msg->yesstr);
	} else if (item == __NOSTR) {
		return const_cast<char *>(msg->nostr);
	} else {
		mlibc::infoLogger() << "mlibc: nl_langinfo item " << item << " is not implemented properly"
		                    << frg::endlog;
		return const_cast<char *>("");
	}
}

/* Public nl_langinfo: uses the effective (thread/global) locale. */
char *nl_langinfo(nl_item item) {
	auto *loc = __mlibc_get_effective_locale();
	return nl_langinfo_from_facets(
	    loc->lc_ctype, loc->lc_time, loc->lc_numeric, loc->lc_messages, item
	);
}

/* Locale-aware nl_langinfo_l. */
char *nl_langinfo_l(nl_item item, locale_t loc) {
	auto *l = loc;
	if (!l || l == (locale_t)-1L)
		l = __mlibc_get_effective_locale();
	return nl_langinfo_from_facets(l->lc_ctype, l->lc_time, l->lc_numeric, l->lc_messages, item);
}

char *__mlibc_nl_langinfo_l(int item, locale_t loc) {
	return nl_langinfo_l(static_cast<nl_item>(item), loc);
}

} // namespace mlibc
