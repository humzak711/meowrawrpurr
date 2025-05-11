#include "include/vmexit_dispatcher.h"
#include "include/arch.h"
#include "include/debug.h"
#include "include/vmcs_encoding.h"

struct vmexit_handler vmexit_dispatch_table[] = {

};

/* if we return false, the vmexit entry point should 
   cleanup and restore dis bihh */
bool vmexit_dispatcher(struct vcpu_ctx *ctx, struct regs *guest_regs)
{    
    union vmexit_reason_t reason;
    if (!__vmread32(VMCS_RO_EXIT_REASON, &reason.val)) {
        HV_LOG(KERN_ERR, "reading vmexit reason failed, core %u", 
               ctx->cpu_id);

        return false;
    }

    if (reason.fields.vmentry_failure != 0) {
        HV_LOG(KERN_ERR, "vmentry failed, core %u", ctx->cpu_id);
        return false;
    }   

    HV_LOG(KERN_DEBUG, "vmexit occured, reason: %u, core: %u",
           reason.fields.basic_reason, ctx->cpu_id);

    for (u32 i = 0; i < ARRAY_LEN(vmexit_dispatch_table); i++) {
        if (vmexit_dispatch_table[i].reason == reason.fields.basic_reason)
            return vmexit_dispatch_table[i].func(ctx, guest_regs);
    }

    return true;
}