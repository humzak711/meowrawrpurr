#include "include/vmcs.h"

#include "include/debug.h"
#include "include/segmentation.h"
#include "include/vcpu.h"
#include "include/ept.h"

#include <linux/slab.h>

struct vmcs *alloc_vmcs(void)
{
    /* vmcs mem */
    struct vmcs *vmcs = kzalloc(sizeof(struct vmcs), GFP_KERNEL);
    if (!vmcs)
        goto map_vmcs_failed;

    /* vmcs region */
    vmcs->vmcs_region = kzalloc(sizeof(struct vmcs_region), GFP_KERNEL);
    if (!vmcs)
        goto map_vmcs_region_failed;

    vmcs->host.stack = kzalloc(HOST_STACK_SIZE, GFP_KERNEL);
    if (!vmcs->host.stack)
        goto map_host_stack_mem_failed;

    vmcs->bmp.msr_bitmap = kzalloc(BITMAP_SIZE, GFP_KERNEL);
    if (!vmcs->bmp.msr_bitmap)
        goto map_msr_bitmap_failed;

    vmcs->bmp.io_bitmap_a = kzalloc(BITMAP_SIZE, GFP_KERNEL);
    if (!vmcs->bmp.io_bitmap_a)
        goto map_io_bitmap_a_failed;

    vmcs->bmp.io_bitmap_b = kzalloc(BITMAP_SIZE, GFP_KERNEL);
    if (!vmcs->bmp.io_bitmap_b)
        goto map_io_bitmap_b_failed;

    vmcs->vmcs_region->header.fields.revision_id = 
        __rdmsrl(IA32_VMX_BASIC);
        
    vmcs->vmcs_region_phys = __pa(vmcs->vmcs_region);

    vmcs->host.stack_size = HOST_STACK_SIZE;

    vmcs->bmp.msr_bitmap_size = BITMAP_SIZE;
    
    vmcs->bmp.io_bitmap_a_size = BITMAP_SIZE;
    vmcs->bmp.io_bitmap_b_size = BITMAP_SIZE;

    union ia32_vmx_basic_t basic = {0};
    basic.val = __rdmsrl(IA32_VMX_BASIC);

    vmcs->ctl_msr_cache.pin = IA32_VMX_PINBASED_CTLS;
    vmcs->ctl_msr_cache.proc = IA32_VMX_PROCBASED_CTLS;
    vmcs->ctl_msr_cache.proc2 = IA32_VMX_PROCBASED_CTLS2;
    vmcs->ctl_msr_cache.exit = IA32_VMX_EXIT_CTLS;
    vmcs->ctl_msr_cache.entry = IA32_VMX_ENTRY_CTLS;

    if (basic.fields.defaults_to_one_clear != 0) {
        vmcs->ctl_msr_cache.pin = IA32_VMX_TRUE_PINBASED_CTLS;
        vmcs->ctl_msr_cache.proc = IA32_VMX_TRUE_PROCBASED_CTLS;
        vmcs->ctl_msr_cache.exit = IA32_VMX_TRUE_EXIT_CTLS;
        vmcs->ctl_msr_cache.entry = IA32_VMX_TRUE_ENTRY_CTLS;
    }

    vmcs->selectors_cache.cs = __read_cs();
    vmcs->selectors_cache.ds = __read_ds();
    vmcs->selectors_cache.ss = __read_ss();
    vmcs->selectors_cache.es = __read_es();
    vmcs->selectors_cache.fs = __read_fs();
    vmcs->selectors_cache.gs = __read_gs();
    vmcs->selectors_cache.tr = __str();
    vmcs->selectors_cache.ldtr = __sldt();
    vmcs->selectors_cache.gdtr = __sgdt();
    vmcs->selectors_cache.idtr = __sidt();

    vmcs->crx_cache.cr0 = __do_read_cr0();
    vmcs->crx_cache.cr3 = __do_read_cr3();
    vmcs->crx_cache.cr4 = __do_read_cr4();

    vmcs->sysenter_cache.esp = __rdmsrl(IA32_SYSENTER_ESP);
    vmcs->sysenter_cache.eip = __rdmsrl(IA32_SYSENTER_EIP);
    vmcs->sysenter_cache.cs = __rdmsrl(IA32_SYSENTER_CS);

    return vmcs;


map_io_bitmap_b_failed:
    kfree(vmcs->bmp.io_bitmap_a);

map_io_bitmap_a_failed:
    kfree(vmcs->bmp.msr_bitmap);

map_msr_bitmap_failed:
    kfree(vmcs->host.stack);

map_host_stack_mem_failed:
    kfree(vmcs->vmcs_region);

map_vmcs_region_failed:
    kfree(vmcs);

map_vmcs_failed:
    return ERR_PTR(-ENOMEM);
}

void free_vmcs(struct vmcs *vmcs)
{
    if (!vmcs)
        return;

    if (vmcs->bmp.io_bitmap_b)
        kfree(vmcs->bmp.io_bitmap_b);

    if (vmcs->bmp.io_bitmap_a)
        kfree(vmcs->bmp.io_bitmap_a);

    if (vmcs->bmp.msr_bitmap)
        kfree(vmcs->bmp.msr_bitmap);

    if (vmcs->host.stack)
        kfree(vmcs->host.stack);

    if (vmcs->vmcs_region)
        kfree(vmcs->vmcs_region);    
       
    kfree(vmcs);
    vmcs = NULL;
}

u32 adjust_ctl(u32 msr, u32 ctl)
{
    u64 caps = __rdmsrl(msr);
 
    ctl |= (u32)(caps);
    ctl &= (u32)(caps >> 32);

    return ctl;
}

bool check_ctl(u32 msr, u32 ctl)
{
    u64 cap = __rdmsrl(msr);

    u32 low = (cap & 0xffffffff);
    return (ctl & ~(cap >> 32)) == 0 &&
           (ctl & low) == low;
}

bool vmwrite_adjusted(u64 field, u32 msr, u32 ctl)
{
    return __vmwrite(field, adjust_ctl(msr, ctl));
}

/* todo: check if features r supported by cpu and their status 
    before initialising corresponding vmcs fields */

bool setup_vmcs_ctls(struct vcpu_ctx *ctx)
{
    int ret = 1;

    /* pinbased ctls */

    union vmcs_vmx_pinbased_ctls_t pinbased_ctls = {0};
    ret &= vmwrite_adjusted(VMCS_CTRL_PINBASED_CONTROLS, 
           ctx->vmcs->ctl_msr_cache.pin, pinbased_ctls.ctl);

    /* procbased ctls */

   union vmcs_vmx_procbased_ctls_t procbased_ctls = {0};

    procbased_ctls.fields.use_msr_bitmaps = 1;
    procbased_ctls.fields.use_io_bitmaps = 1;
    procbased_ctls.fields.activate_secondary_controls = 1;
        
    ret &= vmwrite_adjusted(VMCS_CTRL_PROCBASED_CTLS, 
           ctx->vmcs->ctl_msr_cache.proc, procbased_ctls.ctl);
    
    /* secondary procbased ctls */

    union vmcs_vmx_procbased_ctls2_t procbased_ctls2 = {0};

    procbased_ctls2.fields.enable_rdtscp = 1;
    procbased_ctls2.fields.enable_invpcid = 1;
    procbased_ctls2.fields.enable_xsaves_xrstors = 1;
    procbased_ctls2.fields.conceal_vmx_from_pt = 1;

    ret &= vmwrite_adjusted(VMCS_CTRL_PROCBASED_CTLS2,
           ctx->vmcs->ctl_msr_cache.proc2, procbased_ctls2.ctl);

    /* tertiary procbased ctls */


    /* exit ctls */

    union vmcs_vmx_exit_ctls_t exit_ctls = {0};

    exit_ctls.fields.host_address_space_size = 1;
    exit_ctls.fields.save_debug_controls = 1;
    exit_ctls.fields.acknowledge_interrupt_on_exit = 1;
    exit_ctls.fields.conceal_vmx_from_pt = 1;

    ret &= vmwrite_adjusted(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, 
           ctx->vmcs->ctl_msr_cache.exit, exit_ctls.ctl);

    /* secondary exit ctls */


    /* entry ctls */

    union vmcs_vmx_entry_ctls_t entry_ctls = {0};

    entry_ctls.fields.ia32e_mode_guest = 1;
    entry_ctls.fields.load_debug_controls = 1;
    entry_ctls.fields.conceal_vmx_from_pt = 1;

    ret &= vmwrite_adjusted(VMCS_CTRL_VMENTRY_CONTROLS, 
           ctx->vmcs->ctl_msr_cache.entry, entry_ctls.ctl);


    /* other ctrls */

    ret &= __vmwrite(VMCS_CTRL_MSR_BITMAPS, __pa(ctx->vmcs->bmp.msr_bitmap));
    
    ret &= __vmwrite(VMCS_CTRL_IO_BITMAP_A, __pa(ctx->vmcs->bmp.io_bitmap_a));
    ret &= __vmwrite(VMCS_CTRL_IO_BITMAP_B, __pa(ctx->vmcs->bmp.io_bitmap_b));
    
    ret &= __vmwrite(VMCS_CTRL_EXCEPTION_BITMAP, 0);

    ret &= __vmwrite(VMCS_CTRL_CR0_GUEST_HOST_MASK, 0);
    ret &= __vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __do_read_cr0());
    ret &= __vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0);
    ret &= __vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __do_read_cr4());

    return ret;
}

bool setup_vmcs_host_regs(struct vcpu_ctx *ctx, u64 rip)
{
    u16 es = ctx->vmcs->selectors_cache.es;
    u16 cs = ctx->vmcs->selectors_cache.cs;
    u16 ss = ctx->vmcs->selectors_cache.ss;
    u16 ds = ctx->vmcs->selectors_cache.ds;
    u16 fs = ctx->vmcs->selectors_cache.fs;
    u16 gs = ctx->vmcs->selectors_cache.gs;
    u16 tr = ctx->vmcs->selectors_cache.tr;

    int ret = 1;

    ret &= __vmwrite(VMCS_HOST_ES_SELECTOR, es & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_CS_SELECTOR, cs & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_SS_SELECTOR, ss & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_DS_SELECTOR, ds & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_FS_SELECTOR, fs & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_GS_SELECTOR, gs & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_TR_SELECTOR, tr & HOST_SELECTOR_MASK);

    struct __descriptor_table gdtr = ctx->vmcs->selectors_cache.gdtr;
    struct __descriptor_table idtr = ctx->vmcs->selectors_cache.idtr;

    ret &= __vmwrite(VMCS_HOST_TR_BASE, get_segment_base(gdtr, tr));
    ret &= __vmwrite(VMCS_HOST_FS_BASE, __rdmsrl(IA32_FS_BASE));
    ret &= __vmwrite(VMCS_HOST_GS_BASE, __rdmsrl(IA32_GS_BASE));
    ret &= __vmwrite(VMCS_HOST_GDTR_BASE, gdtr.base);
    ret &= __vmwrite(VMCS_HOST_IDTR_BASE, idtr.base);

    ret &= __vmwrite(VMCS_HOST_CR0, ctx->vmcs->crx_cache.cr0);
    ret &= __vmwrite(VMCS_HOST_CR3, ctx->vmcs->crx_cache.cr3);
    ret &= __vmwrite(VMCS_HOST_CR4, ctx->vmcs->crx_cache.cr4);

    /* move the vcpu into the top of the hosts stack mem,
       move rsp to top of the stack */

    u64 stack_top = (u64)ctx->vmcs->host.stack + ctx->vmcs->host.stack_size;

    struct vcpu_ctx **vcpu_ctx_pos = (struct vcpu_ctx **)(
        stack_top - sizeof(struct vcpu_ctx *));

    *vcpu_ctx_pos = ctx;

    ret &= __vmwrite(VMCS_HOST_RSP, stack_top);

    /* should be vmexit handler entry point */
    ret &= __vmwrite(VMCS_HOST_RIP, rip);

    ret &= __vmwrite(VMCS_HOST_IA32_SYSENTER_CS, 
            ctx->vmcs->sysenter_cache.cs);

    ret &= __vmwrite(VMCS_HOST_IA32_SYSENTER_ESP, 
            ctx->vmcs->sysenter_cache.esp);

    ret &= __vmwrite(VMCS_HOST_IA32_SYSENTER_EIP, 
            ctx->vmcs->sysenter_cache.eip);

    /*ret &= __vmwrite(VMCS_HOST_IA32_PERF_GLOBAL_CTRL, 
            __rdmsrl(IA32_PERF_GLOBAL_CTRL));*/

    ret &= __vmwrite(VMCS_HOST_IA32_PAT, 
            __rdmsrl(IA32_PAT));

    ret &= __vmwrite(VMCS_HOST_IA32_EFER, 
            __rdmsrl(IA32_EFER));

    /*ret &= __vmwrite(VMCS_HOST_IA32_S_CET, 
            __rdmsrl(IA32_S_CET));

    ret &= __vmwrite(VMCS_HOST_IA32_INTERRUPT_SSP_TABLE_ADDR, 
            __rdmsrl(IA32_INTERRUPT_SSP_TABLE_ADDR));

    ret &= __vmwrite(VMCS_HOST_IA32_PKRS, 
            __rdmsrl(IA32_PKRS));*/

    return ret;
}

bool setup_vmcs_guest_regs(struct vcpu_ctx *ctx, u64 rip, u64 rsp, u64 rflags)
{
    u16 es = ctx->vmcs->selectors_cache.es;
    u16 cs = ctx->vmcs->selectors_cache.cs;
    u16 ss = ctx->vmcs->selectors_cache.ss;
    u16 ds = ctx->vmcs->selectors_cache.ds;
    u16 fs = ctx->vmcs->selectors_cache.fs;
    u16 gs = ctx->vmcs->selectors_cache.gs;
    u16 ldtr = ctx->vmcs->selectors_cache.ldtr;
    u16 tr = ctx->vmcs->selectors_cache.tr;

    int ret = 1;

    ret &= __vmwrite(VMCS_GUEST_ES_SELECTOR, es);
    ret &= __vmwrite(VMCS_GUEST_CS_SELECTOR, cs);
    ret &= __vmwrite(VMCS_GUEST_SS_SELECTOR, ss);
    ret &= __vmwrite(VMCS_GUEST_DS_SELECTOR, ds);
    ret &= __vmwrite(VMCS_GUEST_FS_SELECTOR, fs);
    ret &= __vmwrite(VMCS_GUEST_GS_SELECTOR, gs);
    ret &= __vmwrite(VMCS_GUEST_LDTR_SELECTOR, ldtr);
    ret &= __vmwrite(VMCS_GUEST_TR_SELECTOR, tr);

    ret &= __vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, get_segment_ar(es));
    ret &=  __vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, get_segment_ar(cs));
    ret &= __vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, get_segment_ar(ss));
    ret &= __vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, get_segment_ar(ds));
    ret &= __vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, get_segment_ar(fs));
    ret &= __vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, get_segment_ar(gs));
    ret &= __vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, get_segment_ar(ldtr));
    ret &= __vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, get_segment_ar(tr));

    struct __descriptor_table gdtr = ctx->vmcs->selectors_cache.gdtr;
    struct __descriptor_table idtr = ctx->vmcs->selectors_cache.idtr;

    ret &= __vmwrite(VMCS_GUEST_ES_BASE, get_segment_base(gdtr, es));
    ret &= __vmwrite(VMCS_GUEST_CS_BASE, get_segment_base(gdtr, cs));
    ret &= __vmwrite(VMCS_GUEST_SS_BASE, get_segment_base(gdtr, ss));
    ret &= __vmwrite(VMCS_GUEST_DS_BASE, get_segment_base(gdtr, ds));
    ret &= __vmwrite(VMCS_GUEST_FS_BASE, __rdmsrl(IA32_FS_BASE));
    ret &= __vmwrite(VMCS_GUEST_GS_BASE, __rdmsrl(IA32_GS_BASE));
    ret &= __vmwrite(VMCS_GUEST_LDTR_BASE, get_segment_base(gdtr, ldtr));
    ret &= __vmwrite(VMCS_GUEST_TR_BASE, get_segment_base(gdtr, tr));
    ret &= __vmwrite(VMCS_GUEST_GDTR_BASE, gdtr.base);
    ret &= __vmwrite(VMCS_GUEST_IDTR_BASE, idtr.base);

    ret &= __vmwrite(VMCS_GUEST_ES_LIMIT, get_segment_limit(es));
    ret &= __vmwrite(VMCS_GUEST_CS_LIMIT, get_segment_limit(cs));
    ret &= __vmwrite(VMCS_GUEST_SS_LIMIT, get_segment_limit(ss));
    ret &= __vmwrite(VMCS_GUEST_DS_LIMIT, get_segment_limit(ds));
    ret &= __vmwrite(VMCS_GUEST_FS_LIMIT, get_segment_limit(fs));
    ret &= __vmwrite(VMCS_GUEST_GS_LIMIT, get_segment_limit(gs));
    ret &= __vmwrite(VMCS_GUEST_LDTR_LIMIT, get_segment_limit(ldtr));
    ret &= __vmwrite(VMCS_GUEST_TR_LIMIT, get_segment_limit(tr));
    ret &= __vmwrite(VMCS_GUEST_GDTR_LIMIT, gdtr.limit);
    ret &= __vmwrite(VMCS_GUEST_IDTR_LIMIT, idtr.limit);

    ret &= __vmwrite(VMCS_GUEST_CR0, ctx->vmcs->crx_cache.cr0);
    ret &= __vmwrite(VMCS_GUEST_CR3, ctx->vmcs->crx_cache.cr3);
    ret &= __vmwrite(VMCS_GUEST_CR4, ctx->vmcs->crx_cache.cr4);

    ret &= __vmwrite(VMCS_GUEST_DR7, __read_dr7());
    ret &= __vmwrite(VMCS_GUEST_RFLAGS, rflags);

    /* should be original guest rip/rsp to bluepill */
    ret &= __vmwrite(VMCS_GUEST_RIP, rip);
    ret &= __vmwrite(VMCS_GUEST_RSP, rsp);

    ret &= __vmwrite(VMCS_GUEST_IA32_DEBUGCTL, 
            __rdmsrl(IA32_DEBUG_CTL));

    ret &= __vmwrite(VMCS_GUEST_IA32_SYSENTER_CS,
            ctx->vmcs->sysenter_cache.cs);

    ret &= __vmwrite(VMCS_GUEST_IA32_SYSENTER_ESP, 
            ctx->vmcs->sysenter_cache.esp);

    ret &= __vmwrite(VMCS_GUEST_IA32_SYSENTER_EIP, 
            ctx->vmcs->sysenter_cache.eip);

    /*ret &= __vmwrite(VMCS_GUEST_IA32_PERF_GLOBAL_CTRL, 
            __rdmsrl(IA32_PERF_GLOBAL_CTRL));*/
    
    ret &= __vmwrite(VMCS_GUEST_IA32_PAT, 
            __rdmsrl(IA32_PAT));

    ret &= __vmwrite(VMCS_GUEST_IA32_EFER, 
            __rdmsrl(IA32_EFER));

    /*ret &= __vmwrite(VMCS_GUEST_IA32_BNDCFGS, 
            __rdmsrl(IA32_BNDCFGS));*/

    /*ret &= __vmwrite(VMCS_GUEST_IA32_RTIT_CTL, 
            __rdmsrl(IA32_RTIT_CTL));

    ret &= __vmwrite(VMCS_GUEST_IA32_LBR_CTL, 
            __rdmsrl(IA32_LBR_CTL));

    ret &= __vmwrite(VMCS_GUEST_IA32_S_CET, 
            __rdmsrl(IA32_S_CET));

    ret &= __vmwrite(VMCS_GUEST_IA32_INTERRUPT_SSP_TABLE_ADDR, 
            __rdmsrl(IA32_INTERRUPT_SSP_TABLE_ADDR));

    ret &= __vmwrite(VMCS_GUEST_IA32_PKRS, 
            __rdmsrl(IA32_PKRS));*/

    ret &= __vmwrite(VMCS_GUEST_ACTIVITY_STATE, active);
//    ret &= __vmwrite(VMCS_GUEST_INTERRUPT_STATUS, 0);
    ret &= __vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, ~0ULL);
    ret &= __vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0);

    return ret;
}

bool do_vmcs_checks(struct vcpu_ctx *ctx)
{
    union cr0_t cr0 = {0};
    union cr4_t cr4 = {0};
    cr0.val = __do_read_cr0();
    cr4.val = __do_read_cr4();

    int ret = 1;

    ret &= cr4.fields.cet == 0 || cr0.fields.wp != 0;
    ret &= ((u64)ctx->vmcs->bmp.msr_bitmap & 0xfff) == 0;

    ret &= ((u64)ctx->vmcs->bmp.io_bitmap_a & 0xfff) == 0;
    ret &= ((u64)ctx->vmcs->bmp.io_bitmap_b & 0xfff) == 0;

    u32 pin = 0;
    u32 proc = 0;
    u32 proc2 = 0;
    u32 exit = 0;
    u32 entry = 0;

    __vmread32(VMCS_CTRL_PINBASED_CONTROLS, &pin);
    __vmread32(VMCS_CTRL_PROCBASED_CTLS, &proc);
    __vmread32(VMCS_CTRL_PROCBASED_CTLS2, &proc2);
    __vmread32(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, &exit);
    __vmread32(VMCS_CTRL_VMENTRY_CONTROLS, &entry);

    ret &= check_ctl(ctx->vmcs->ctl_msr_cache.pin, pin);
    ret &= check_ctl(ctx->vmcs->ctl_msr_cache.proc, proc);
    ret &= check_ctl(ctx->vmcs->ctl_msr_cache.proc2, proc2);
    ret &= check_ctl(ctx->vmcs->ctl_msr_cache.exit, exit);
    ret &= check_ctl(ctx->vmcs->ctl_msr_cache.entry, entry);

    return ret;
}
