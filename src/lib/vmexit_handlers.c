#include "include/vmexit_handlers.h"

#include "include/arch.h"
#include "include/debug.h"
#include "include/vmcs_err.h"

bool handle_cpuid(struct vcpu_ctx *ctx, struct regs *guest_regs)
{
    u32 input_eax = (u32)guest_regs->rax;
    u32 input_ecx = (u32)guest_regs->rcx;
    
    u32 regs[4] = {input_eax, (u32)guest_regs->rbx,
                   input_ecx, (u32)guest_regs->rdx};

    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    if (input_eax == CPUID_HV_ID)
        regs[0] = HV_ID;

    guest_regs->rax = regs[0];
    guest_regs->rbx = regs[1];
    guest_regs->rcx = regs[2];
    guest_regs->rdx = regs[3];

    return guest_rip_next();
}
