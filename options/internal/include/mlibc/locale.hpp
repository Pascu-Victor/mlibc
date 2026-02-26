#ifndef MLIBC_LOCALE
#define MLIBC_LOCALE

#include <bits/nl_item.h>
#include <bits/posix/locale_t.h>

namespace mlibc {

char *nl_langinfo(nl_item item);
char *nl_langinfo_l(nl_item item, locale_t loc);

} // namespace mlibc

#endif // MLIBC_LOCALE
