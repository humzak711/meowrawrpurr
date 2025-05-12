#include "include/vmcs.h"

#include "include/arch.h"
#include "include/debug.h"
#include "include/segmentation.h"
#include "include/vcpu.h"
#include "include/vmcs_encoding.h"

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

    /* host mem */
    vmcs->host.stack = kzalloc(HOST_STACK_SIZE, GFP_KERNEL);
    if (!vmcs->host.stack)
        goto map_host_stack_mem_failed;

    /* guest mem */
    vmcs->guest.msr_bitmap = kzalloc(BITMAP_SIZE, GFP_KERNEL);
    if (!vmcs->guest.msr_bitmap)
        goto map_msr_bitmap_failed;

    /*vmcs->guest.io_bitmap_a = kzalloc(BITMAP_SIZE, GFP_KERNEL);
    if (!vmcs->guest.io_bitmap_a)
        goto map_io_bitmap_a_failed;

    vmcs->guest.io_bitmap_b = kzalloc(BITMAP_SIZE, GFP_KERNEL);
    if (!vmcs->guest.io_bitmap_b)
        goto map_io_bitmap_b_failed;*/

    vmcs->vmcs_region->header.fields.revision_id = 
        __rdmsrl(IA32_VMX_BASIC);
        
    vmcs->vmcs_region_phys = __pa(vmcs->vmcs_region);

    vmcs->host.stack_size = HOST_STACK_SIZE;

    vmcs->guest.msr_bitmap_size = BITMAP_SIZE;
    /*
    vmcs->guest.io_bitmap_a_size = BITMAP_SIZE;
    vmcs->guest.io_bitmap_b_size = BITMAP_SIZE;
    */

    return vmcs;

/*
map_io_bitmap_b_failed:
    kfree(vmcs->guest.io_bitmap_a);

map_io_bitmap_a_failed:
    kfree(vmcs->guest.msr_bitmap);
*/

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

    /*if (vmcs->guest.io_bitmap_b)
        kfree(vmcs->guest.io_bitmap_b);

    if (vmcs->guest.io_bitmap_a)
        kfree(vmcs->guest.io_bitmap_a);*/

    if (vmcs->guest.msr_bitmap)
        kfree(vmcs->guest.msr_bitmap);

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

    ctl &= (caps >> 32); 
    ctl |= (caps & 0xffffffff);

    return ctl;
}

bool vmwrite_adjusted(u64 field, u32 msr, u32 ctl)
{
    return __vmwrite(field, adjust_ctl(msr, ctl));
}

/* todo: check if features r supported by cpu and their status 
    before initialising corresponding vmcs fields */

bool setup_vmcs_ctls(struct vmcs *vmcs)
{
    int ret = 1;

    /*
    if (!vmcs->guest.io_bitmap_a || !vmcs->guest.io_bitmap_b ||
        vmcs->guest.io_bitmap_a_size != BITMAP_SIZE ||
        vmcs->guest.io_bitmap_b_size != BITMAP_SIZE) {
            return -EINVAL
    }*/

    /* pinbased ctls */

    union vmcs_vmx_pinbased_ctls_t pinbased_ctls = {0};
    ret &= vmwrite_adjusted(VMCS_CTRL_PINBASED_CONTROLS, 
           IA32_VMX_PINBASED_CTLS, pinbased_ctls.ctl);

    /* procbased ctls */

   union vmcs_vmx_procbased_ctls_t procbased_ctls = {0};

    procbased_ctls.fields.use_msr_bitmaps = 1;
    // procbased_ctls.fields.use_io_bitmaps = 1;
    procbased_ctls.fields.activate_secondary_controls = 1;
        
    ret &= vmwrite_adjusted(VMCS_CTRL_PROCBASED_CTLS, 
           IA32_VMX_PROCBASED_CTLS, procbased_ctls.ctl);
    
    /* secondary procbased ctls */

    union vmcs_vmx_procbased_ctls2_t procbased_ctls2 = {0};

    procbased_ctls2.fields.enable_rdtscp = 1;
    procbased_ctls2.fields.enable_invpcid = 1;
    procbased_ctls2.fields.enable_xsaves_xrstors = 1;
    procbased_ctls2.fields.conceal_vmx_from_pt = 1;

    ret &= vmwrite_adjusted(VMCS_CTRL_PROCBASED_CTLS2, 
           IA32_VMX_PROCBASED_CTLS2, procbased_ctls2.ctl);

    /* tertiary procbased ctls */


    /* exit ctls */

    union vmcs_vmx_exit_ctls_t exit_ctls = {0};

    exit_ctls.fields.host_address_space_size = 1;
    exit_ctls.fields.save_debug_controls = 1;
    exit_ctls.fields.acknowledge_interrupt_on_exit = 1;
    exit_ctls.fields.conceal_vmx_from_pt = 1;

    ret &= vmwrite_adjusted(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, 
           IA32_VMX_EXIT_CTLS, exit_ctls.ctl);

    /* secondary exit ctls */


    /* entry ctls */

    union vmcs_vmx_entry_ctls_t entry_ctls = {0};

    entry_ctls.fields.ia32e_mode_guest = 1;
    entry_ctls.fields.load_debug_controls = 1;

    ret &= vmwrite_adjusted(VMCS_CTRL_VMENTRY_CONTROLS, 
           IA32_VMX_ENTRY_CTLS, entry_ctls.ctl);


    /* other ctrls */

    ret &= __vmwrite(VMCS_CTRL_MSR_BITMAPS, __pa(vmcs->guest.msr_bitmap));
    /*
    __vmwrite(VMCS_CTRL_IO_BITMAP_A, __pa(vmcs->io_bitmap_a));
    __vmwrite(VMCS_CTRL_IO_BITMAP_B, __pa(vmcs->io_bitmap_b));
    */
    ret &= __vmwrite(VMCS_CTRL_EXCEPTION_BITMAP, 0);

    ret &= __vmwrite(VMCS_CTRL_CR3_TARGET_COUNT, 0);
    ret &= __vmwrite(VMCS_CTRL_CR0_GUEST_HOST_MASK, 0);
    ret &= __vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __do_read_cr0());
    ret &= __vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0);
    ret &= __vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __do_read_cr4());

    return ret;
}

bool setup_vmcs_host_regs(struct vcpu_ctx *ctx, u64 rip)
{
    u16 es = __read_es();
    u16 cs = __read_cs();
    u16 ss = __read_ss();
    u16 ds = __read_ds();
    u16 fs = __read_fs();
    u16 gs = __read_gs();
    u16 tr = __str();

    int ret = 1;

    ret &= __vmwrite(VMCS_HOST_ES_SELECTOR, es & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_CS_SELECTOR, cs & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_SS_SELECTOR, ss & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_DS_SELECTOR, ds & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_FS_SELECTOR, fs & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_GS_SELECTOR, gs & HOST_SELECTOR_MASK);
    ret &= __vmwrite(VMCS_HOST_TR_SELECTOR, tr & HOST_SELECTOR_MASK);

    struct __descriptor_table gdtr = __sgdt();
    struct __descriptor_table idtr = __sidt();

    ret &= __vmwrite(VMCS_HOST_TR_BASE, get_segment_base(gdtr, tr));
    ret &= __vmwrite(VMCS_HOST_FS_BASE, __rdmsrl(IA32_FS_BASE));
    ret &= __vmwrite(VMCS_HOST_GS_BASE, __rdmsrl(IA32_GS_BASE));
    ret &= __vmwrite(VMCS_HOST_GDTR_BASE, gdtr.base);
    ret &= __vmwrite(VMCS_HOST_IDTR_BASE, idtr.base);

    ret &= __vmwrite(VMCS_HOST_CR0, __do_read_cr0());
    ret &= __vmwrite(VMCS_HOST_CR3, __do_read_cr3());
    ret &= __vmwrite(VMCS_HOST_CR4, __do_read_cr4());

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
            __rdmsrl(IA32_SYSENTER_CS));
    ret &= __vmwrite(VMCS_HOST_IA32_SYSENTER_ESP, 
            __rdmsrl(IA32_SYSENTER_ESP));
    ret &= __vmwrite(VMCS_HOST_IA32_SYSENTER_EIP, 
            __rdmsrl(IA32_SYSENTER_EIP));

    ret &= __vmwrite(VMCS_HOST_IA32_PERF_GLOBAL_CTRL, 
            __rdmsrl(IA32_PERF_GLOBAL_CTRL));

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

bool setup_vmcs_guest_regs(u64 rip, u64 rsp, u64 rflags)
{
    u16 es = __read_es();
    u16 cs = __read_cs();
    u16 ss = __read_ss();
    u16 ds = __read_ds();
    u16 fs = __read_fs();
    u16 gs = __read_gs();
    u16 ldtr = __sldt();
    u16 tr = __str();

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

    struct __descriptor_table gdtr = __sgdt();
    struct __descriptor_table idtr = __sidt();

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

    ret &= __vmwrite(VMCS_GUEST_CR0, __do_read_cr0());
    ret &= __vmwrite(VMCS_GUEST_CR3, __do_read_cr3());
    ret &= __vmwrite(VMCS_GUEST_CR4, __do_read_cr4());

    ret &= __vmwrite(VMCS_GUEST_DR7, __read_dr7());
    ret &= __vmwrite(VMCS_GUEST_RFLAGS, rflags);

    /* should be original guest rip/rsp to bluepill */
    ret &= __vmwrite(VMCS_GUEST_RIP, rip);
    ret &= __vmwrite(VMCS_GUEST_RSP, rsp);

    ret &= __vmwrite(VMCS_GUEST_IA32_DEBUGCTL, 
            __rdmsrl(IA32_DEBUG_CTL));

    ret &= __vmwrite(VMCS_GUEST_IA32_SYSENTER_CS,
            __rdmsrl(IA32_SYSENTER_CS));
    ret &= __vmwrite(VMCS_GUEST_IA32_SYSENTER_ESP, 
            __rdmsrl(IA32_SYSENTER_ESP));
    ret &= __vmwrite(VMCS_GUEST_IA32_SYSENTER_EIP, 
            __rdmsrl(IA32_SYSENTER_EIP));

    ret &= __vmwrite(VMCS_GUEST_IA32_PERF_GLOBAL_CTRL, 
            __rdmsrl(IA32_PERF_GLOBAL_CTRL));
    
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
    ret &= __vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, ~0ULL);
   
    return ret;
}