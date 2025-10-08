.section .init,"ax",@progbits
    pop %rbp
    ret

.section .fini,"ax",@progbits
    .globl _fini
    .type _fini, @function
_fini:
    push %rbp
    mov %rsp, %rbp
    pop %rbp
    ret
