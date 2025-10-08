.section .init,"ax",@progbits
    .globl _init
    .type _init, @function
_init:
    push %rbp
    mov %rsp, %rbp
    # The rest of the initialization code goes here
