#pragma once

#if !defined(__ASSEMBLER__)
#error "The early startup marker is assembly-only."
#endif
#include "../callnums/perf_trace.h"

// No calls, GOT/PLT, relocations, TLS, FPU, red-zone or allocation. This runs
// before rtld self-relocation. Preserve every syscall/input scratch register,
// the incoming flags and stack pointer. The timestamp is the numeric syscall
// receipt after this stub/entry-page faults, not a literal first-instruction TSC.
.macro WOS_PERF_STARTUP stage
	pushfq
	push %rax
	push %rcx
	push %rdx
	push %rsi
	push %rdi
	push %r8
	push %r9
	push %r10
	push %r11
	lea 80(%rsp), %rdi
	mov (%rdi), %rax
	lea 16(%rdi,%rax,8), %rdi
.Lwos_perf_env\@:
	cmpq $0, (%rdi)
	je .Lwos_perf_aux_start\@
	add $8, %rdi
	jmp .Lwos_perf_env\@
.Lwos_perf_aux_start\@:
	add $8, %rdi
	xor %ecx, %ecx
	xor %edx, %edx
	xor %r8d, %r8d
	xor %r9d, %r9d
	xor %r11d, %r11d
.Lwos_perf_aux\@:
	mov (%rdi), %rax
	test %rax, %rax
	jz .Lwos_perf_admit\@
	cmp $WOS_AT_PERF_TRACE_VERSION, %rax
	jne .Lwos_perf_session\@
	test $1, %ecx
	jnz .Lwos_perf_done\@
	or $1, %ecx
	mov 8(%rdi), %r9
	jmp .Lwos_perf_next\@
.Lwos_perf_session\@:
	cmp $WOS_AT_PERF_TRACE_SESSION, %rax
	jne .Lwos_perf_correlation\@
	test $2, %ecx
	jnz .Lwos_perf_done\@
	or $2, %ecx
	mov 8(%rdi), %rdx
	jmp .Lwos_perf_next\@
.Lwos_perf_correlation\@:
	cmp $WOS_AT_PERF_TRACE_CORRELATION, %rax
	jne .Lwos_perf_base\@
	test $4, %ecx
	jnz .Lwos_perf_done\@
	or $4, %ecx
	mov 8(%rdi), %r8
	jmp .Lwos_perf_next\@
.Lwos_perf_base\@:
	cmp $7, %rax
	jne .Lwos_perf_next\@
	mov 8(%rdi), %r11
.Lwos_perf_next\@:
	add $16, %rdi
	jmp .Lwos_perf_aux\@
.Lwos_perf_admit\@:
	cmp $7, %ecx
	jne .Lwos_perf_done\@
	cmp $WOS_PERF_TRACE_VERSION, %r9
	jne .Lwos_perf_done\@
	cmp $-1, %rdx
	je .Lwos_perf_done\@
	mov %r8, %rax
	shr $32, %rax
	jnz .Lwos_perf_done\@
.if \stage == WOS_PERF_STAGE_CRT_ENTRY
	// Static executables have no interpreter entry: mark the image here too.
	test %r11, %r11
	jnz .Lwos_perf_emit\@
	mov $WOS_PERF_TRACE_SYS_LOG, %eax
	mov $WOS_PERF_TRACE_LOG_OP, %edi
	mov $WOS_PERF_STAGE_IMAGE_ENTRY, %esi
	xor %r10d, %r10d
	syscall
.endif
.Lwos_perf_emit\@:
	mov $WOS_PERF_TRACE_SYS_LOG, %eax
	mov $WOS_PERF_TRACE_LOG_OP, %edi
	mov $\stage, %esi
	xor %r10d, %r10d
	syscall
.Lwos_perf_done\@:
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rdi
	pop %rsi
	pop %rdx
	pop %rcx
	pop %rax
	popfq
.endm
