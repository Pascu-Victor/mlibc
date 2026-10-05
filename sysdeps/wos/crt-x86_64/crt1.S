#include "../include/mlibc/perf-startup-asm.h"
.section .text

.global _start
_start:
	WOS_PERF_STARTUP WOS_PERF_STAGE_CRT_ENTRY
    mov %rsp, %rdi
    mov $main, %rsi
    call __mlibc_entry

.size _start, . - _start
.section .note.GNU-stack,"",%progbits
