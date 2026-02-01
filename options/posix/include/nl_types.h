#ifndef NL_TYPES_H
#define NL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* nl_catd is an opaque type for message catalog descriptors */
typedef void *nl_catd;

/* NL_CAT_LOCALE is used for nl_catopen to select the default locale */
#define NL_CAT_LOCALE 1

#ifndef __MLIBC_ABI_ONLY

/* Message catalog functions (stub implementations) */
nl_catd nl_catopen(const char *name, int oflag);
char *nl_catgets(nl_catd catd, int set_id, int msg_id, const char *s);
int nl_catclose(nl_catd catd);

/* Aliases without nl_ prefix (also POSIX standard) */
#define catopen(name, oflag) nl_catopen(name, oflag)
#define catgets(catd, set_id, msg_id, s) nl_catgets(catd, set_id, msg_id, s)
#define catclose(catd) nl_catclose(catd)

#endif /* !__MLIBC_ABI_ONLY */

#ifdef __cplusplus
}
#endif

#endif /* NL_TYPES_H */
