.section .text

.extern __virtualise_core
.extern __devirtualise_core
.extern __guest_rip
.extern __guest_rsp

.extern vmexit_dispatcher

.globl __virtualise_core_entry
.type __virtualise_core_entry, @function

.globl __vmexit_entry
.type __vmexit_entry, @function

.macro pushall
    pushq %rbp
    movq %rsp, %rbp
    pushfq
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm

.macro popall
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    popfq
    popq %rbp
.endm

/* bool __virtualise_core_entry(u32 cpu_id) */
__virtualise_core_entry:
    pushall

    movq $.__guest_entry, %rsi
    movq %rsp, %rdx
    pushfq 
    popq %rcx

    call __virtualise_core

    /* only reached if  virtualising the core fails */
    popall
    xor %rax, %rax
    ret
    int3

    /* guest entry should start here if virtualised */
.__guest_entry:
    popall
    movl $1, %eax
    ret
    int3
.size __virtualise_core_entry, . - __virtualise_core_entry

/* entry point for vmexit dispatcher */
__vmexit_entry:

    /* sub 8 from rsp so its pointing to vcpu ptr */
    subq $8, %rsp 
    pushall

    /* pushall subs %rsp by 128, so we add it to point to vcpu ptr again */
    movq 128(%rsp), %rdi

    /* rsp currently holds struct regs *, so pass that as second arg */
    movq %rsp, %rsi

    pushq %rdi //  preserve vcpu ptr
    subq $32, %rsp
    call vmexit_dispatcher
    addq $32, %rsp
    popq %rdi
 
    /* if the dispatcher returns 0 then cleanup */
    test %eax, %eax
    je .__vmxoff

    popall
    addq $8, %rsp 
    vmresume
    int3

/* todo: proper shutdown of the hv */
.__vmxoff:
    popall
    int3
.size __vmexit_entry, . - __vmexit_entry