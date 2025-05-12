#ifndef _VCPU_H_
#define _VCPU_H_

#include "vmxon.h"

#include <linux/mutex.h>

struct vcpu_ctx
{
    struct 
    {
        struct vmxon_region *vmxon_region;
        u64 phys;
    } vmxon;

    struct vmcs *vmcs;
    u32 cpu_id;

    void *stashed_data;
    bool virtualised;
};

#define vcpu_ctx_stash_data(vcpu_ctx, data) \
    (((vcpu_ctx)->stashed_data) = (data))

#define get_vcpu_ctx_stash(vcpu_ctx) \
    ((vcpu_ctx)->stashed_data)

struct hv
{
    struct vcpu_ctx **vcpu_ctx_arr;
    u32 vcpu_ctx_arr_count;

    struct mutex vcpu_ctx_arr_lock;
};

extern struct hv hv_global;

int hv_global_add_vcpu(struct vcpu_ctx *ctx);
int hv_global_remove_vcpu(struct vcpu_ctx *ctx);
bool hv_global_vcpu_added(u32 cpu_id);

struct vcpu_ctx *alloc_vcpu(u32 cpu_id);
void free_vcpu(struct vcpu_ctx *vcpu);

struct vcpu_ctx *__virtualise_core(
    u32 cpu_id, u64 guest_rip, u64 guest_rsp, u64 guest_rflags);

void __devirtualise_core(struct vcpu_ctx *ctx);

u64 __guest_rip(void);
u64 __guest_rsp(void);

#endif