#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
	long page_size = sysconf(_SC_PAGESIZE);
	assert(page_size > 3);

	char *pages = mmap(
	    NULL, (size_t)page_size * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
	);
	assert(pages != MAP_FAILED);
	assert(mprotect(pages + page_size, (size_t)page_size, PROT_NONE) == 0);

	char *source = pages + page_size - 3;
	memcpy(source, "abc", 3);

	char dest[5] = {'x', 'x', 'x', 'x', 'x'};
	assert(strncpy(dest, source, 3) == dest);
	assert(memcmp(dest, "abc", 3) == 0);
	assert(dest[3] == 'x');

	assert(strncpy(dest, "d", 4) == dest);
	assert(dest[0] == 'd');
	assert(dest[1] == '\0');
	assert(dest[2] == '\0');
	assert(dest[3] == '\0');
	assert(dest[4] == 'x');

	assert(munmap(pages, (size_t)page_size * 2) == 0);
	return 0;
}
