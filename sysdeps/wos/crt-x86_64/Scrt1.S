#include "../include/mlibc/perf-startup-asm.h"
.section .text
.global _start
_start:
	WOS_PERF_STARTUP WOS_PERF_STAGE_CRT_ENTRY
	mov %rsp, %rdi
	lea main(%rip), %rsi
	call __mlibc_entry
.section .note.GNU-stack,"",%progbits

