#include "include/vmexit_dispatcher.h"

#include "include/vmexit_handlers.h"
#include "include/arch.h"
#include "include/debug.h"
#include "include/vmcs_err.h"

struct vmexit_handler vmexit_dispatch_table[] = {
    {handle_cpuid, EXIT_REASON_CPUID},
    {handle_ept_fault, EXIT_REASON_EPT_FAULT},
    {handle_ept_misconfig, EXIT_REASON_EPT_MISCONFIG}
};

/* if we return false, the vmexit entry point should 
   cleanup and restore dis bihh */
bool vmexit_dispatcher(struct vcpu_ctx *ctx, struct regs *guest_regs)
{    
    union vmexit_reason_t reason = {0};
    if (!__vmread32(VMCS_RO_EXIT_REASON, &reason.val) || 
        reason.fields.vmentry_failure != 0) {
        return false;
    }

    for (u32 i = 0; i < ARRAY_LEN(vmexit_dispatch_table); i++) {
        if (vmexit_dispatch_table[i].reason == reason.fields.basic_reason)
            return vmexit_dispatch_table[i].func(ctx, guest_regs);
    }

    /*for (;;) {
        
    }*/
    return guest_rip_next();
}
