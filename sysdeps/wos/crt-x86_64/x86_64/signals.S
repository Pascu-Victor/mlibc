.section .text
.global __mlibc_signal_restore
.global __mlibc_signal_restore_rt
.type __mlibc_signal_restore, @function
.type __mlibc_signal_restore_rt, @function

// Signal handler return trampoline.
// Called when a signal handler does 'ret' — sa_restorer points here.
// Issues the WOS sigreturn syscall to restore the pre-signal context.
//
// WOS syscall convention:
//   RAX = callnum (3 = process)
//   RDI = sub-op  (9 = sigreturn)
//   syscall instruction
__mlibc_signal_restore:
__mlibc_signal_restore_rt:
    mov $3, %rax          // callnums::process
    mov $9, %rdi          // procmgmt_ops::sigreturn
    syscall
    ud2                   // should never return

.section .note.GNU-stack,"",@progbits
