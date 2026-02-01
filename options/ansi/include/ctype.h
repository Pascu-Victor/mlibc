#ifndef _CTYPE_H
#define _CTYPE_H

#include <mlibc-config.h>

#ifdef __cplusplus
extern "C" {
#endif

/* glibc-compatible character type bit definitions for libcxx locale support */
#define _ISbit(bit) ((bit) < 8 ? ((1 << (bit)) << 8) : ((1 << (bit)) >> 8))

#define _ISupper _ISbit(0)  /* Uppercase letter */
#define _ISlower _ISbit(1)  /* Lowercase letter */
#define _ISalpha _ISbit(2)  /* Alphabetic */
#define _ISdigit _ISbit(3)  /* Numeric digit */
#define _ISxdigit _ISbit(4) /* Hexadecimal digit */
#define _ISspace _ISbit(5)  /* Whitespace */
#define _ISprint _ISbit(6)  /* Printable */
#define _ISgraph _ISbit(7)  /* Graphical (visible) */
#define _ISblank _ISbit(8)  /* Blank (space or tab) */
#define _IScntrl _ISbit(9)  /* Control character */
#define _ISpunct _ISbit(10) /* Punctuation */
#define _ISalnum _ISbit(11) /* Alphanumeric */

#ifndef __MLIBC_ABI_ONLY

/* Character classification function [7.4.1] */
int isalnum(int __c);
int isalpha(int __c);
int isblank(int __c);
int iscntrl(int __c);
int isdigit(int __c);
int isgraph(int __c);
int islower(int __c);
int isprint(int __c);
int ispunct(int __c);
int isspace(int __c);
int isupper(int __c);
int isxdigit(int __c);

/* glibc extensions. */
int isascii(int __c);

/* Character case mapping functions [7.4.2] */
int tolower(int __c);
int toupper(int __c);

#endif /* !__MLIBC_ABI_ONLY */

/* Borrowed from glibc */
#define toascii(c) ((c) & 0x7f)

#ifdef __cplusplus
}
#endif

#if __MLIBC_POSIX_OPTION
#include <bits/posix/posix_ctype.h>
#endif

#endif /* _CTYPE_H */
