#include <mlibc/debug.hpp>
#include <nl_types.h>

namespace {

bool nl_catopen_seen = false;

} // namespace

nl_catd nl_catopen(const char *name, int oflag) {
	// Stub implementation: message catalogs are not implemented
	if (!nl_catopen_seen) {
		mlibc::infoLogger() << "mlibc: nl_catopen() is a no-op" << frg::endlog;
		nl_catopen_seen = true;
	}
	return nullptr;
}

char *nl_catgets(nl_catd catd, int set_id, int msg_id, const char *s) {
	// Stub implementation: return the default string
	return const_cast<char *>(s);
}

int nl_catclose(nl_catd catd) {
	// Stub implementation: nothing to close
	return 0;
}
