
#include <limits.h>
#include <locale.h>
#include <string.h>

#include <bits/ensure.h>

#include <frg/optional.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/locale-data.h>

namespace {
// Values of the C locale are defined by the C standard.
constexpr lconv c_lconv = {
    const_cast<char *>("."), // decimal_point
    const_cast<char *>(""),  // thousands_sep
    const_cast<char *>(""),  // grouping
    const_cast<char *>(""),  // mon_decimal_point
    const_cast<char *>(""),  // mon_thousands_sep
    const_cast<char *>(""),  // mon_grouping
    const_cast<char *>(""),  // positive_sign
    const_cast<char *>(""),  // negative_sign
    const_cast<char *>(""),  // currency_symbol
    CHAR_MAX,                // frac_digits
    CHAR_MAX,                // p_cs_precedes
    CHAR_MAX,                // n_cs_precedes
    CHAR_MAX,                // p_sep_by_space
    CHAR_MAX,                // n_sep_by_space
    CHAR_MAX,                // p_sign_posn
    CHAR_MAX,                // n_sign_posn
    const_cast<char *>(""),  // int_curr_symbol
    CHAR_MAX,                // int_frac_digits
    CHAR_MAX,                // int_p_cs_precedes
    CHAR_MAX,                // int_n_cs_precedes
    CHAR_MAX,                // int_p_sep_by_space
    CHAR_MAX,                // int_n_sep_by_space
    CHAR_MAX,                // int_p_sign_posn
    CHAR_MAX                 // int_n_sign_posn
};
} // namespace

char *setlocale(int category, const char *name) {
	auto *glob = &mlibc::__mlibc_global_locale;

	if (category == LC_ALL) {
		/* Query: return current LC_CTYPE name as representative. */
		if (!name)
			return const_cast<char *>(glob->category_names[__LC_CTYPE]);

		/* Empty string → default to C. */
		if (!*name)
			name = "C";

		auto *src = mlibc::__mlibc_lookup_builtin_locale(name);
		if (!src) {
			mlibc::infoLogger() << "mlibc: Locale " << name << " is not supported" << frg::endlog;
			return nullptr;
		}

		/* Set all categories. */
		for (int i = 0; i < __MLIBC_NUM_LOCALE_CATEGORIES; i++)
			glob->category_names[i] = src->category_names[i];
		glob->lc_ctype = src->lc_ctype;
		glob->lc_numeric = src->lc_numeric;
		glob->lc_time = src->lc_time;
		glob->lc_monetary = src->lc_monetary;
		glob->lc_messages = src->lc_messages;

		return const_cast<char *>(glob->category_names[__LC_CTYPE]);
	}

	/* Single-category case. */
	if (category < 0 || category >= __MLIBC_NUM_LOCALE_CATEGORIES) {
		mlibc::infoLogger() << "mlibc: Unexpected value " << category
		                    << " for category in setlocale()" << frg::endlog;
		return nullptr;
	}

	/* Query only. */
	if (!name)
		return const_cast<char *>(glob->category_names[category]);

	if (!*name)
		name = "C";

	auto *src = mlibc::__mlibc_lookup_builtin_locale(name);
	if (!src) {
		mlibc::infoLogger() << "mlibc: Locale " << name << " is not supported" << frg::endlog;
		return nullptr;
	}

	/* Apply the single category from the looked-up locale. */
	glob->category_names[category] = src->category_names[category];
	switch (category) {
		case __LC_CTYPE:
			glob->lc_ctype = src->lc_ctype;
			break;
		case __LC_NUMERIC:
			glob->lc_numeric = src->lc_numeric;
			break;
		case __LC_TIME:
			glob->lc_time = src->lc_time;
			break;
		case __LC_MONETARY:
			glob->lc_monetary = src->lc_monetary;
			break;
		case __LC_MESSAGES:
			glob->lc_messages = src->lc_messages;
			break;
		/* LC_COLLATE and extended categories: name is set, no facet data yet. */
		default:
			break;
	}

	return const_cast<char *>(glob->category_names[category]);
}

namespace {
lconv effective_lc;
} // namespace

struct lconv *localeconv(void) {
	auto *loc = mlibc::__mlibc_get_effective_locale();

	// Numeric facet
	const auto *num = loc->lc_numeric;
	effective_lc.decimal_point = const_cast<char *>(num->decimal_point);
	effective_lc.thousands_sep = const_cast<char *>(num->thousands_sep);
	effective_lc.grouping = const_cast<char *>(num->grouping);

	// Monetary facet
	const auto *mon = loc->lc_monetary;
	effective_lc.mon_decimal_point = const_cast<char *>(mon->mon_decimal_point);
	effective_lc.mon_thousands_sep = const_cast<char *>(mon->mon_thousands_sep);
	effective_lc.mon_grouping = const_cast<char *>(mon->mon_grouping);
	effective_lc.positive_sign = const_cast<char *>(mon->positive_sign);
	effective_lc.negative_sign = const_cast<char *>(mon->negative_sign);
	effective_lc.currency_symbol = const_cast<char *>(mon->currency_symbol);
	effective_lc.frac_digits = mon->frac_digits;
	effective_lc.p_cs_precedes = mon->p_cs_precedes;
	effective_lc.n_cs_precedes = mon->n_cs_precedes;
	effective_lc.p_sep_by_space = mon->p_sep_by_space;
	effective_lc.n_sep_by_space = mon->n_sep_by_space;
	effective_lc.p_sign_posn = mon->p_sign_posn;
	effective_lc.n_sign_posn = mon->n_sign_posn;
	effective_lc.int_curr_symbol = const_cast<char *>(mon->int_curr_symbol);
	effective_lc.int_frac_digits = mon->int_frac_digits;
	effective_lc.int_p_cs_precedes = mon->int_p_cs_precedes;
	effective_lc.int_n_cs_precedes = mon->int_n_cs_precedes;
	effective_lc.int_p_sep_by_space = mon->int_p_sep_by_space;
	effective_lc.int_n_sep_by_space = mon->int_n_sep_by_space;
	effective_lc.int_p_sign_posn = mon->int_p_sign_posn;
	effective_lc.int_n_sign_posn = mon->int_n_sign_posn;

	return &effective_lc;
}
