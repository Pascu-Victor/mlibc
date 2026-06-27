#include <bits/ensure.h>
#include <mlibc/debug.hpp>
#include <resolv.h>

#include <arpa/nameser.h>
#include <string.h>

int dn_expand(
    const unsigned char *msg,
    const unsigned char *eomorig,
    const unsigned char *comp_dn,
    char *exp_dn,
    int length
) {
	if (length < 0)
		return -1;
	return ns_name_uncompress(msg, eomorig, comp_dn, exp_dn, static_cast<size_t>(length));
}

int res_mkquery(
    int op,
    const char *dname,
    int rr_class,
    int rr_type,
    const unsigned char *,
    int,
    const unsigned char *,
    unsigned char *buf,
    int buflen
) {
	int label_start;
	int label_end;
	unsigned char query[280];
	size_t const name_length = strnlen(dname, 255);
	size_t trimmed_length = name_length;

	if (trimmed_length && dname[trimmed_length - 1] == '.')
		trimmed_length--;
	if (trimmed_length && dname[trimmed_length - 1] == '.')
		return -1;

	int const packet_length = 17 + trimmed_length + !!trimmed_length;
	if (trimmed_length > 253 || buflen < packet_length || static_cast<unsigned int>(op) > 15U
	    || static_cast<unsigned int>(rr_class) > 255U || static_cast<unsigned int>(rr_type) > 255U)
		return -1;

	memset(query, 0, packet_length);
	query[2] = static_cast<unsigned char>((op * 8) + 1);
	query[3] = 32; // AD
	query[5] = 1;
	memcpy(query + 13, dname, trimmed_length);

	for (label_start = 13; query[label_start]; label_start = label_end + 1) {
		for (label_end = label_start; query[label_end] && query[label_end] != '.'; label_end++)
			;
		if (label_end - label_start - 1U > 62U)
			return -1;
		query[label_start - 1] = static_cast<unsigned char>(label_end - label_start);
	}

	query[label_start + 1] = static_cast<unsigned char>(rr_type);
	query[label_start + 3] = static_cast<unsigned char>(rr_class);
	memcpy(buf, query, packet_length);
	return packet_length;
}

int res_query(const char *, int, int, unsigned char *, int) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int res_init() {
	mlibc::infoLogger() << "mlibc: res_init is a stub!" << frg::endlog;
	return 0;
}

int res_ninit(res_state) {
	mlibc::infoLogger() << "mlibc: res_ninit is a stub!" << frg::endlog;
	return 0;
}

void res_nclose(res_state) { mlibc::infoLogger() << "mlibc: res_nclose is a stub!" << frg::endlog; }

int dn_comp(const char *, unsigned char *, int, unsigned char **, unsigned char **) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

/* Taken from musl */
int dn_skipname(const unsigned char *s, const unsigned char *end) {
	const unsigned char *p = s;
	while (p < end)
		if (!*p)
			return p - s + 1;
		else if (*p >= 192)
			if (p + 1 < end)
				return p - s + 2;
			else
				break;
		else if (end - p < *p + 1)
			break;
		else
			p += *p + 1;
	return -1;
}

/* This is completely unused, and exists purely to satisfy broken apps. */

struct __res_state *__res_state() {
	static struct __res_state res;
	return &res;
}
