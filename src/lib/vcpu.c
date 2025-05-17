#include "include/vcpu.h"

#include "include/debug.h"
#include "include/vmcs.h"
#include "include/vmcs_encoding.h"
#include "include/vmcs_err.h"
#include "include/setup_vmx.h"

#include <linux/slab.h>

extern void __vmexit_entry(void);

struct hv *alloc_hv(void)
{
    struct hv *hv = kzalloc(sizeof(struct hv), GFP_KERNEL);
    if (!hv)
        return ERR_PTR(-ENOMEM);

    mutex_init(&hv->vcpu_ctx_arr_lock);

    /* setup epts */

    mutex_init(&hv->epts.lock);

    return hv;
}

/* individual vcpus should be shutdown and
   freed before calling this */
void free_hv(struct hv *hv)
{
    if (!hv)
        return;

    mutex_lock(&hv->vcpu_ctx_arr_lock);

    if (hv->vcpu_ctx_arr && hv->vcpu_ctx_arr_count > 0) {
        kfree(hv->vcpu_ctx_arr);
        hv->vcpu_ctx_arr = NULL;
        hv->vcpu_ctx_arr_count = 0;
    }

    mutex_unlock(&hv->vcpu_ctx_arr_lock);

    kfree(hv);
}

int hv_add_vcpu(struct hv *hv, struct vcpu_ctx *ctx)
{
    if (!ctx)
        return -EINVAL;

    mutex_lock(&hv->vcpu_ctx_arr_lock);

    /* sanity check coz im paranoid, if u got this many cores tho then 
       fuckin ell mate */
    if (hv->vcpu_ctx_arr_count == ~0U) {
        mutex_unlock(&hv->vcpu_ctx_arr_lock);
        return -ENOENT;
    }

    u32 new_count = hv->vcpu_ctx_arr_count + 1;
    struct vcpu_ctx **new_ptr = krealloc(
        hv->vcpu_ctx_arr, 
        sizeof(struct vcpu_ctx *) * new_count, GFP_KERNEL
    );

    if (!new_ptr) {
        mutex_unlock(&hv->vcpu_ctx_arr_lock);
        return -ENOMEM;
    }

    hv->vcpu_ctx_arr = new_ptr;
    hv->vcpu_ctx_arr[hv->vcpu_ctx_arr_count] = ctx;
    hv->vcpu_ctx_arr_count = new_count;
    ctx->hv_global = hv;

    mutex_unlock(&hv->vcpu_ctx_arr_lock);
    return 0;
}

int hv_remove_vcpu(struct hv *hv, struct vcpu_ctx *ctx)
{
    if (!ctx || hv->vcpu_ctx_arr_count == 0)
        return -EINVAL;

    mutex_lock(&hv->vcpu_ctx_arr_lock);

    long index = -1;
    for (u32 i = 0; i < hv->vcpu_ctx_arr_count; i++) {
        if (hv->vcpu_ctx_arr[i] && 
            hv->vcpu_ctx_arr[i]->cpu_id == ctx->cpu_id) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        mutex_unlock(&hv->vcpu_ctx_arr_lock);
        return -ENOENT;
    }

    /* krealloc is pretty ass for removing elements coz
       icl it can be really bad if an error happens, kinda
       annoying to deal with, so we finna do it by just 
       mapping a new array and copying the relevent 
       elements into it */

    struct vcpu_ctx **new_arr = NULL;

    /* if the new count is 0, can just free and null the array */
    u32 new_count = hv->vcpu_ctx_arr_count - 1;
    if (new_count == 0) 
        goto success;
    
    /* allocate new array of new count size */
    new_arr = kzalloc(sizeof(struct vcpu_ctx *) * new_count, GFP_KERNEL);
    if (!new_arr) {
        mutex_unlock(&hv->vcpu_ctx_arr_lock);
        return -ENOMEM;
    }

    /* copy relevant elements back to the new array */
    for (u32 i = 0, j = 0; i < hv->vcpu_ctx_arr_count; i++) {
        if (i == index)
            continue;

        new_arr[j++] = hv->vcpu_ctx_arr[i];
    }

success:
    kfree(hv->vcpu_ctx_arr);
    hv->vcpu_ctx_arr = new_arr;
    hv->vcpu_ctx_arr_count = new_count;

    mutex_unlock(&hv->vcpu_ctx_arr_lock);
    return 0;
}

struct vcpu_ctx *hv_get_vcpu(struct hv *hv, u32 cpu_id)
{
    mutex_lock(&hv->vcpu_ctx_arr_lock);

    for (u32 i = 0; i < hv->vcpu_ctx_arr_count; i++) {

        struct vcpu_ctx *ctx = hv->vcpu_ctx_arr[i];

        if (ctx && ctx->cpu_id == cpu_id) {
            mutex_unlock(&hv->vcpu_ctx_arr_lock);
            return ctx;
        }
    }

    mutex_unlock(&hv->vcpu_ctx_arr_lock);
    return NULL;
}

struct vcpu_ctx *alloc_vcpu(u32 cpu_id)
{
    struct vcpu_ctx *ctx = kzalloc(sizeof(struct vcpu_ctx), GFP_KERNEL);
    if (!ctx) {

        HV_LOG(KERN_ERR, "couldnt map vcpu structure, core: %u", cpu_id); 
        
        return ERR_PTR(-ENOMEM);
    }

    void *ptr_err = NULL;

    struct vmxon_region *vmxon_region = alloc_vmxon_region();
    if (IS_ERR(vmxon_region)) {
        HV_LOG(KERN_ERR, "couldnt map vmxon structure, core: %u", cpu_id);
        ptr_err = vmxon_region;
        goto alloc_vmxon_region_failed;
    }

    struct vmcs *vmcs = alloc_vmcs();
    if (IS_ERR(vmcs)) {
        HV_LOG(KERN_ERR, "couldnt map vmcs structure, core: %u", cpu_id);
        ptr_err = vmcs;
        goto alloc_vmcs_failed;
    }

    ctx->vmxon.vmxon_region = vmxon_region;
    ctx->vmxon.phys = __pa(vmxon_region);
    ctx->vmcs = vmcs;
    ctx->cpu_id = cpu_id;

    return ctx;

alloc_vmcs_failed:
    free_vmxon_region(vmxon_region);

alloc_vmxon_region_failed:
    kfree(ctx);
    return ptr_err;
}

void free_vcpu(struct vcpu_ctx *vcpu)
{
    if (!vcpu)
        return;

    if (vcpu->vmcs) 
        free_vmcs(vcpu->vmcs);

    if (vcpu->vmxon.vmxon_region)
        free_vmxon_region(vcpu->vmxon.vmxon_region);

    vcpu = NULL;
}

struct vcpu_ctx *__virtualise_core(struct hv *hv, u32 cpu_id, 
    u64 guest_rip, u64 guest_rsp, u64 guest_rflags)
{
    void *ptr_err = NULL;
    int ret = 0;

    /* if the vcpu is already added, can just get to 
       setting up fields its all gud, we can reuse this mem */
    struct vcpu_ctx *ctx = hv_get_vcpu(hv, cpu_id);
    if (ctx)  
        goto vmx;

    ret = setup_vmx();
    if (ret < 0) {
        HV_LOG(KERN_ERR, "couldnt setup vmxe, core: %u", cpu_id);
        return ERR_PTR(ret);
    }

    ctx = alloc_vcpu(cpu_id);
    if (IS_ERR(ctx)) {
        HV_LOG(KERN_ERR, "couldnt allocate vcpu, core: %u", cpu_id);
        return ctx;
    }

    ret = hv_add_vcpu(hv, ctx);
    if (ret < 0) {
        HV_LOG(KERN_DEBUG, "failed to add vcpu to arr, core: %u", cpu_id);
        ptr_err = ERR_PTR(ret);
        goto vcpu_arr_add_failed;
    }

vmx:
    if (!__vmxon(ctx->vmxon.phys)) {
        HV_LOG(KERN_ERR, "couldnt vmxon, core: %u", cpu_id);
        ptr_err = ERR_PTR(-EFAULT);
        goto vmxon_failed;
    }

    HV_LOG(KERN_DEBUG, "vmxon successful, core: %u", cpu_id);

    if (!__vmclear(ctx->vmcs->vmcs_region_phys) || 
        !__vmptrld(ctx->vmcs->vmcs_region_phys)) {

        HV_LOG(KERN_ERR, "couldnt vmptrld, core: %u", cpu_id);
        ptr_err = ERR_PTR(-EFAULT);
        goto vmptrld_failed;
    }

    HV_LOG(KERN_DEBUG, "vmptrld successful, core: %u", cpu_id);

    if (!setup_vmcs_ctls(ctx)) { 
        HV_LOG(KERN_ERR, "setup vmcs ctls failed, core %u", cpu_id);
        goto virtualise_failed;
    }

    HV_LOG(KERN_DEBUG, "setup vmcs ctls on core %u", cpu_id);

    if (!setup_vmcs_host_regs(ctx, (u64)__vmexit_entry)) {
        HV_LOG(KERN_ERR, "setup vmcs host regs failed, core %u", cpu_id);
        goto virtualise_failed;
    }

    HV_LOG(KERN_DEBUG, "setup vmcs host regs on core %u", cpu_id);

    if (!setup_vmcs_guest_regs(ctx, guest_rip, guest_rsp, guest_rflags)) {
        HV_LOG(KERN_ERR, "setup vmcs guest regs failed, core %u", cpu_id);
        goto virtualise_failed;
    }

    HV_LOG(KERN_DEBUG, "setup guest regs on core %u", cpu_id);

    if (!do_vmcs_checks(ctx)) {
        HV_LOG(KERN_ERR, "vmcs checks failed on core %u", cpu_id);
        goto virtualise_failed;
    }

    HV_LOG(KERN_DEBUG, "vmcs checks passed on core %u", cpu_id);

    ctx->virtualised = true;
    HV_LOG(KERN_DEBUG, "vmlaunching on core %u", cpu_id);
    if (!__vmlaunch()) {
        ctx->virtualised = false;
        HV_LOG(KERN_ERR, "failed to vmlaunch, core: %u", cpu_id);
        goto virtualise_failed;
    }

    return ctx;

virtualise_failed:
    ptr_err = ERR_PTR(-EFAULT);
    
    char *reason = vmcs_get_err(vmcs_get_errcode());
    if (reason != NULL)
        HV_LOG(KERN_ERR, "core: %u, vmcs err %s", cpu_id, reason);

    __vmclear(ctx->vmcs->vmcs_region_phys);

vmptrld_failed:
    __vmxoff();

vmxon_failed:
    /* if we cant remove the ctx from the array, icl
       just straight up keep the vcpu in the array 
       so that memory can at least maybe get reused 
       if we try virtualise it again */
    if (hv_remove_vcpu(hv, ctx) < 0) {
        HV_LOG(KERN_ERR, "couldnt remove vcpu from arr, core: %u", cpu_id);
        return ptr_err;
    }

vcpu_arr_add_failed:
    free_vcpu(ctx);
    return ptr_err;
}

void __setup_vcpu_exit(struct vcpu_ctx *ctx)
{
    /* these better not fail or were fucked */
    __vmread(VMCS_GUEST_RIP, &ctx->exit_regs.rip);
    __vmread(VMCS_GUEST_RSP, &ctx->exit_regs.rsp);
    __vmread(VMCS_GUEST_CR3, &ctx->exit_regs.cr3);
}

/* keep its memory available for reusage, 
   this way, we can also still use its stack
   for clean shutdown */
void __devirtualise_core(struct vcpu_ctx *ctx)
{
    if (!ctx->virtualised)
        return;
     
    HV_LOG(KERN_DEBUG, "devirtualising core %u", ctx->cpu_id);

    __setup_vcpu_exit(ctx);
    __vmxoff(); //if this fails, we are fucked anyway lol
    ctx->virtualised = false;
    
    HV_LOG(KERN_DEBUG, "devirtualised core %u", ctx->cpu_id);
}
