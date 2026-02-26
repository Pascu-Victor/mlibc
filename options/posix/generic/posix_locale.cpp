#include <bits/ensure.h>
#include <bits/locale.h>
#include <bits/posix/posix_locale.h>
#include <errno.h>
#include <mlibc/debug.hpp>
#include <mlibc/locale-data.h>
#include <stdlib.h>
#include <string.h>

/*
 * WOS locale system — POSIX newlocale / freelocale / uselocale / duplocale
 *
 * locale_t is a pointer to __mlibc_locale_struct.  We allocate new structs
 * from the heap for user-created locales and free them in freelocale().
 * Built-in locale objects (the constexpr ones in locale-data.h) are never freed.
 */

/* Helper: apply category_mask bits from a built-in locale to a mutable locale struct. */
static void
apply_categories(__mlibc_locale_struct *dest, const __mlibc_locale_struct *src, int category_mask) {
	if (category_mask & LC_CTYPE_MASK) {
		dest->lc_ctype = src->lc_ctype;
		dest->category_names[__LC_CTYPE] = src->category_names[__LC_CTYPE];
	}
	if (category_mask & LC_NUMERIC_MASK) {
		dest->lc_numeric = src->lc_numeric;
		dest->category_names[__LC_NUMERIC] = src->category_names[__LC_NUMERIC];
	}
	if (category_mask & LC_TIME_MASK) {
		dest->lc_time = src->lc_time;
		dest->category_names[__LC_TIME] = src->category_names[__LC_TIME];
	}
	if (category_mask & LC_COLLATE_MASK) {
		dest->category_names[__LC_COLLATE] = src->category_names[__LC_COLLATE];
	}
	if (category_mask & LC_MONETARY_MASK) {
		dest->lc_monetary = src->lc_monetary;
		dest->category_names[__LC_MONETARY] = src->category_names[__LC_MONETARY];
	}
	if (category_mask & LC_MESSAGES_MASK) {
		dest->lc_messages = src->lc_messages;
		dest->category_names[__LC_MESSAGES] = src->category_names[__LC_MESSAGES];
	}
	/* Extended categories — just propagate the name for now. */
	if (category_mask & LC_PAPER_MASK)
		dest->category_names[__LC_PAPER] = src->category_names[__LC_PAPER];
	if (category_mask & LC_NAME_MASK)
		dest->category_names[__LC_NAME] = src->category_names[__LC_NAME];
	if (category_mask & LC_ADDRESS_MASK)
		dest->category_names[__LC_ADDRESS] = src->category_names[__LC_ADDRESS];
	if (category_mask & LC_TELEPHONE_MASK)
		dest->category_names[__LC_TELEPHONE] = src->category_names[__LC_TELEPHONE];
	if (category_mask & LC_MEASUREMENT_MASK)
		dest->category_names[__LC_MEASUREMENT] = src->category_names[__LC_MEASUREMENT];
	if (category_mask & LC_IDENTIFICATION_MASK)
		dest->category_names[__LC_IDENTIFICATION] = src->category_names[__LC_IDENTIFICATION];
}

locale_t newlocale(int category_mask, const char *locale_name, locale_t base) {
	/* Look up the requested locale by name. */
	const auto *src = mlibc::__mlibc_lookup_builtin_locale(locale_name);
	if (!src) {
		mlibc::infoLogger() << "mlibc: newlocale(): unknown locale \""
		                    << (locale_name ? locale_name : "(null)") << "\"" << frg::endlog;
		errno = ENOENT;
		return nullptr;
	}

	__mlibc_locale_struct *result;

	if (base && base != LC_GLOBAL_LOCALE) {
		/* Modify the base locale in-place and return it. */
		result = base;
	} else {
		/* Allocate a fresh locale struct, starting from the C locale. */
		result = static_cast<__mlibc_locale_struct *>(malloc(sizeof(__mlibc_locale_struct)));
		if (!result) {
			errno = ENOMEM;
			return nullptr;
		}
		/* Start from C defaults. */
		memcpy(result, &mlibc::__mlibc_c_locale_obj, sizeof(__mlibc_locale_struct));
	}

	/* Apply the requested categories from the source locale. */
	apply_categories(result, src, category_mask);

	return result;
}

void freelocale(locale_t locobj) {
	if (!locobj || locobj == LC_GLOBAL_LOCALE)
		return;

	/* Don't free built-in (constexpr) locale objects. They live in .rodata. */
	if (locobj == const_cast<__mlibc_locale_struct *>(&mlibc::__mlibc_c_locale_obj)
	    || locobj == const_cast<__mlibc_locale_struct *>(&mlibc::__mlibc_en_us_utf8_locale_obj)
	    || locobj == const_cast<__mlibc_locale_struct *>(&mlibc::__mlibc_de_de_utf8_locale_obj)
	    || locobj == const_cast<__mlibc_locale_struct *>(&mlibc::__mlibc_fr_fr_utf8_locale_obj))
		return;

	free(locobj);
}

locale_t uselocale(locale_t locobj) {
	locale_t previous = mlibc::__mlibc_thread_locale;
	if (!previous)
		previous = LC_GLOBAL_LOCALE;

	if (locobj) {
		if (locobj == LC_GLOBAL_LOCALE) {
			/* Revert to using the global locale. */
			mlibc::__mlibc_thread_locale = nullptr;
		} else {
			mlibc::__mlibc_thread_locale = locobj;
		}
	}

	return previous;
}

locale_t duplocale(locale_t locobj) {
	const __mlibc_locale_struct *src;

	if (!locobj || locobj == LC_GLOBAL_LOCALE) {
		src = mlibc::__mlibc_get_effective_locale();
	} else {
		src = locobj;
	}

	auto *result = static_cast<__mlibc_locale_struct *>(malloc(sizeof(__mlibc_locale_struct)));
	if (!result) {
		errno = ENOMEM;
		return nullptr;
	}

	memcpy(result, src, sizeof(__mlibc_locale_struct));
	return result;
}
