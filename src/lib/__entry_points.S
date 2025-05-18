.section .text

.extern __virtualise_core
.extern __devirtualise_core
.extern __setup_vcpu_exit
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

/* bool __virtualise_core_entry(struct hv *hv, u32 cpu_id) */
__virtualise_core_entry:
    pushall

    movq $.__guest_entry, %rdx
    movq %rsp, %rcx
    pushfq 
    popq %r8

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

    /* sub 8 from rsp so its pointing to vcpu ptr, then save regs to stack */
    subq $8, %rsp 
    pushall

    /* we finna pass vcpu ctx ptr as first arg and regs as second */
    movq 128(%rsp), %rdi
    movq %rsp, %rsi
    call vmexit_dispatcher
 
    /* if the dispatcher returns 0 then cleanup */
    test %eax, %eax
    je .__vmxoff

    popall
    addq $8, %rsp 
    vmresume
    
    /* todo: vmresume error handling */

    nop
    int3

/* todo: proper shutdown of the hv */
.__vmxoff:

    /* vmxoff dis shi and setup vcpu ctx */
    movq 128(%rsp), %rdi
    call __devirtualise_core

    popall // restore guests regs
    movq (%rsp), %rsi // store the vcpu ctx into rsi

    // restore guest rip into rax
    movq (%rsi), %rax 

    // restore the guests stack
    movq 8(%rsi), %rsp 

    // restore guests cr3
    movq 16(%rsi), %rdi
    movq %rdi, %cr3

    // zero out rsi before handing back control 
    xor %rsi, %rsi 

    /* #ud for debugging otherwise my shi crashes because 
       this isnt setup properly */
    vmresume 

    nop
    int3
.size __vmexit_entry, . - __vmexit_entry