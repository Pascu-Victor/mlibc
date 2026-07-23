#include <assert.h>
#include <stdlib.h>

extern char **environ;

int main(void) {
	assert(setenv("MLIBC_ENVIRONMENT_TEST", "setenv", 1) == 0);
	assert(getenv("MLIBC_ENVIRONMENT_TEST"));

	assert(clearenv() == 0);
	assert(environ);
	assert(!environ[0]);
	assert(!getenv("MLIBC_ENVIRONMENT_TEST"));

	assert(setenv("MLIBC_ENVIRONMENT_TEST", "after-clear", 1) == 0);
	assert(getenv("MLIBC_ENVIRONMENT_TEST"));
	assert(unsetenv("MLIBC_ENVIRONMENT_TEST") == 0);
	assert(!getenv("MLIBC_ENVIRONMENT_TEST"));

	return 0;
}
