#include <arpa/nameser.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

// The ns_get* and ns_put* functions are taken from musl.
unsigned ns_get16(const unsigned char *cp) { return cp[0] << 8 | cp[1]; }

unsigned long ns_get32(const unsigned char *cp) {
	return (unsigned)cp[0] << 24 | cp[1] << 16 | cp[2] << 8 | cp[3];
}

void ns_put16(unsigned s, unsigned char *cp) {
	*cp++ = s >> 8;
	*cp++ = s;
}

void ns_put32(unsigned long l, unsigned char *cp) {
	*cp++ = l >> 24;
	*cp++ = l >> 16;
	*cp++ = l >> 8;
	*cp++ = l;
}

namespace {

int fail_parse() {
	errno = EMSGSIZE;
	return -1;
}

bool has_bytes(const unsigned char *ptr, const unsigned char *eom, size_t count) {
	return ptr <= eom && static_cast<size_t>(eom - ptr) >= count;
}

int
checked_skip_name(const unsigned char *msg, const unsigned char *eom, const unsigned char *src) {
	char scratch[NS_MAXDNAME];
	return ns_name_uncompress(msg, eom, src, scratch, sizeof(scratch));
}

int skip_question(const unsigned char *msg, const unsigned char *eom, const unsigned char *&ptr) {
	int consumed = checked_skip_name(msg, eom, ptr);
	if (consumed < 0)
		return -1;
	ptr += consumed;
	if (!has_bytes(ptr, eom, NS_QFIXEDSZ))
		return fail_parse();
	ptr += NS_QFIXEDSZ;
	return 0;
}

int skip_rr(const unsigned char *msg, const unsigned char *eom, const unsigned char *&ptr) {
	int consumed = checked_skip_name(msg, eom, ptr);
	if (consumed < 0)
		return -1;
	ptr += consumed;
	if (!has_bytes(ptr, eom, NS_RRFIXEDSZ))
		return fail_parse();
	uint16_t const rdlength = ns_get16(ptr + 8);
	ptr += NS_RRFIXEDSZ;
	if (!has_bytes(ptr, eom, rdlength))
		return fail_parse();
	ptr += rdlength;
	return 0;
}

int copy_dns_label(
    char *&dst, size_t &remaining, const unsigned char *src, size_t length, bool need_dot
) {
	if (remaining == 0)
		return fail_parse();
	if (need_dot) {
		if (remaining < 2)
			return fail_parse();
		*dst++ = '.';
		remaining--;
	}
	if (length + 1 > remaining)
		return fail_parse();
	memcpy(dst, src, length);
	dst += length;
	remaining -= length;
	return 0;
}

} // namespace

int ns_initparse(const unsigned char *msg, int msglen, ns_msg *handle) {
	if (!msg || !handle || msglen < NS_HFIXEDSZ)
		return fail_parse();

	memset(handle, 0, sizeof(*handle));
	handle->_msg = msg;
	handle->_eom = msg + msglen;
	handle->_id = ns_get16(msg);
	handle->_flags = ns_get16(msg + 2);
	for (int i = 0; i < ns_s_max; ++i)
		handle->_counts[i] = ns_get16(msg + 4 + (i * NS_INT16SZ));

	const unsigned char *ptr = msg + NS_HFIXEDSZ;
	for (int section = 0; section < ns_s_max; ++section) {
		handle->_sections[section] = ptr;
		for (int rr = 0; rr < handle->_counts[section]; ++rr) {
			if (section == ns_s_qd) {
				if (skip_question(msg, handle->_eom, ptr) < 0)
					return -1;
			} else {
				if (skip_rr(msg, handle->_eom, ptr) < 0)
					return -1;
			}
		}
	}

	handle->_sect = ns_s_max;
	handle->_rrnum = -1;
	handle->_msg_ptr = msg + NS_HFIXEDSZ;
	return 0;
}

int ns_parserr(ns_msg *handle, ns_sect section, int rrnum, ns_rr *rr) {
	if (!handle || !rr || section < ns_s_qd || section >= ns_s_max)
		return fail_parse();
	if (rrnum < 0) {
		rrnum = handle->_sect == section ? handle->_rrnum + 1 : 0;
	}
	if (rrnum < 0 || rrnum >= handle->_counts[section])
		return fail_parse();

	const unsigned char *ptr = handle->_sections[section];
	for (int current = 0; current <= rrnum; ++current) {
		int consumed =
		    ns_name_uncompress(handle->_msg, handle->_eom, ptr, rr->name, sizeof(rr->name));
		if (consumed < 0)
			return -1;
		ptr += consumed;

		if (section == ns_s_qd) {
			if (!has_bytes(ptr, handle->_eom, NS_QFIXEDSZ))
				return fail_parse();
			rr->type = ns_get16(ptr);
			rr->rr_class = ns_get16(ptr + NS_INT16SZ);
			rr->ttl = 0;
			rr->rdlength = 0;
			rr->rdata = nullptr;
			ptr += NS_QFIXEDSZ;
		} else {
			if (!has_bytes(ptr, handle->_eom, NS_RRFIXEDSZ))
				return fail_parse();
			rr->type = ns_get16(ptr);
			rr->rr_class = ns_get16(ptr + NS_INT16SZ);
			rr->ttl = ns_get32(ptr + (2 * NS_INT16SZ));
			rr->rdlength = ns_get16(ptr + (2 * NS_INT16SZ) + NS_INT32SZ);
			ptr += NS_RRFIXEDSZ;
			if (!has_bytes(ptr, handle->_eom, rr->rdlength))
				return fail_parse();
			rr->rdata = ptr;
			ptr += rr->rdlength;
		}
	}

	handle->_sect = section;
	handle->_rrnum = rrnum;
	handle->_msg_ptr = ptr;
	return 0;
}

int ns_name_uncompress(
    const unsigned char *msg,
    const unsigned char *eom,
    const unsigned char *src,
    char *dst,
    size_t dstsize
) {
	if (!msg || !eom || !src || !dst || src >= eom || msg > src || msg >= eom)
		return fail_parse();

	ptrdiff_t const message_size = eom - msg;
	const unsigned char *ptr = src;
	int consumed = 0;
	int jumps = 0;
	bool jumped = false;
	bool wrote_label = false;
	char *out = dst;
	size_t remaining = dstsize;

	while (ptr < eom) {
		unsigned char const label = *ptr++;
		if (!jumped)
			consumed++;

		if (label == 0) {
			if (!wrote_label) {
				if (remaining < 2)
					return fail_parse();
				*out++ = '.';
				remaining--;
			}
			if (remaining == 0)
				return fail_parse();
			*out = '\0';
			return consumed;
		}

		if ((label & NS_CMPRSFLGS) == NS_CMPRSFLGS) {
			if (ptr >= eom)
				return fail_parse();
			uint16_t const offset = static_cast<uint16_t>(((label & ~NS_CMPRSFLGS) << 8U) | *ptr++);
			if (!jumped)
				consumed++;
			if (offset >= message_size)
				return fail_parse();
			if (++jumps > message_size)
				return fail_parse();
			ptr = msg + offset;
			jumped = true;
			continue;
		}

		if ((label & NS_CMPRSFLGS) != 0 || label > NS_MAXLABEL || !has_bytes(ptr, eom, label))
			return fail_parse();
		if (copy_dns_label(out, remaining, ptr, label, wrote_label) < 0)
			return -1;
		wrote_label = true;
		ptr += label;
		if (!jumped)
			consumed += label;
	}

	return fail_parse();
}
