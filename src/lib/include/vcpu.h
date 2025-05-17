#ifndef _VCPU_H_
#define _VCPU_H_

#include "arch.h"
#include "vmxon.h"
#include "ept.h"

#include <linux/mutex.h>

struct vcpu_ctx
{
    struct 
    {   
        u64 rip;
        u64 rsp;
        u64 cr3;
    } exit_regs;

    struct 
    {
        struct vmxon_region *vmxon_region;
        u64 phys;
    } vmxon;

    struct vmcs *vmcs;
    u32 cpu_id;

    struct hv *hv_global;
    bool virtualised;
} __pack;
size_assert(struct vcpu_ctx, 61);

struct hv
{
    struct vcpu_ctx **vcpu_ctx_arr;
    u32 vcpu_ctx_arr_count;

    struct mutex vcpu_ctx_arr_lock;

    /* icl to save memory finna share epts, best 
       way for this icl */
    struct 
    {
        struct ept *ept;
        struct mutex lock;
    } epts;
};

struct hv *alloc_hv(void);
void free_hv(struct hv *hv);

int hv_add_vcpu(struct hv *hv, struct vcpu_ctx *ctx);
int hv_remove_vcpu(struct hv *hv, struct vcpu_ctx *ctx);
struct vcpu_ctx *hv_get_vcpu(struct hv *hv, u32 cpu_id);

struct vcpu_ctx *alloc_vcpu(u32 cpu_id);
void free_vcpu(struct vcpu_ctx *vcpu);

struct vcpu_ctx *__virtualise_core(struct hv *hv, u32 cpu_id, 
    u64 guest_rip, u64 guest_rsp, u64 guest_rflags);

void __setup_vcpu_exit(struct vcpu_ctx *ctx);
void __devirtualise_core(struct vcpu_ctx *ctx);

#endif