#include "mlibc-asm/helpers.h"
#include <mlibc-config.h>
#if __MLIBC_WOS_OPTION
#include <mlibc/perf-startup-asm.h>
#endif

PROC_START(_start)
.cfi_undefined rip
#if __MLIBC_WOS_OPTION
	WOS_PERF_STARTUP WOS_PERF_STAGE_IMAGE_ENTRY
#endif
	call relocateSelf

	mov %rsp, %rdi
	call interpreterMain

	jmp *%rax
PROC_END(_start)

GNU_STACK_NOTE()

