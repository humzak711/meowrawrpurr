#ifndef _VMCS_H_
#define _VMCS_H_

#include "arch.h"
#include "vcpu.h"

#define VMCS_REGION_SIZE 4096
#define HOST_STACK_SIZE (4096 * 8)

#define BITMAP_SIZE 4096

#define HOST_SELECTOR_MASK 0xF8

typedef enum 
{
    active,
    hlt,
    shutdown,
    wait_for_sipi,
} activity_states;

union vmcs_vmx_exception_bitmap_t
{
    u32 val;
    struct
    {
        u32 divide_error : 1;
        u32 debug : 1;
        u32 nmi_interrupt : 1;
        u32 breakpoint : 1;
        u32 overflow : 1;
        u32 bound_range_exceeded : 1;
        u32 invalid_opcode : 1;
        u32 device_not_available : 1;
        u32 double_fault : 1;
        u32 coprocessor_segment_overrun : 1;
        u32 invalid_tss : 1;
        u32 segment_not_present : 1;
        u32 stack_segment_fault : 1;
        u32 general_protection : 1;
        u32 page_fault : 1;
        u32 reserved0 : 1;
        u32 floating_point_error : 1;
        u32 alignment_check : 1;
        u32 machine_check : 1;
        u32 simd_floating_point_exception : 1;
        u32 virtualization_exception : 1;
        u32 control_protection_exception : 1;
        u32 reserved1 : 10;
    } fields;
};

union vmcs_vmx_pinbased_ctls_t
{
    u32 ctl;
    struct
    {
        u32 external_interrupt_exiting : 1;
        u32 reserved0 : 2;
        u32 nmi_exiting : 1;
        u32 reserved1 : 1;
        u32 virtual_nmis : 1;
        u32 activate_vmx_preemption_timer : 1;
        u32 process_posted_interrupts : 1;
        u32 reserved2 : 24;
    } fields;
};

union vmcs_vmx_procbased_ctls_t
{
    u32 ctl;
    struct
    {
        u32 reserved0 : 2;
        u32 interrupt_window_exiting : 1;
        u32 use_tsc_offsetting : 1;
        u32 reserved1 : 3;
        u32 hlt_exiting : 1;
        u32 reserved2 : 1;
        u32 invlpg_exiting : 1;
        u32 mwait_exiting : 1;
        u32 rdpmc_exiting : 1;
        u32 rdtsc_exiting : 1;
        u32 reserved3 : 2;
        u32 cr3_load_exiting : 1;
        u32 cr3_store_exiting : 1;
        u32 activate_tertiary_controls : 1;
        u32 reserved4 : 1;
        u32 cr8_load_exiting : 1;
        u32 cr8_store_exiting : 1;
        u32 use_tpr_shadow : 1;
        u32 nmi_window_exiting : 1;
        u32 mov_dr_exiting : 1;
        u32 unconditional_io_exiting : 1;
        u32 use_io_bitmaps : 1;
        u32 reserved_5 : 1;
        u32 monitor_trap_flag : 1;
        u32 use_msr_bitmaps : 1;
        u32 monitor_exiting : 1;
        u32 pause_exiting : 1;
        u32 activate_secondary_controls : 1;
    } fields;
};

union vmcs_vmx_procbased_ctls2_t
{
    u32 ctl;
    struct
    {
        u32 virtualize_apic_accesses : 1;
        u32 enable_ept : 1;
        u32 descriptor_table_exiting : 1;
        u32 enable_rdtscp : 1;
        u32 virtualize_x2apic_mode : 1;
        u32 enable_vpid : 1;
        u32 wbinvd_exiting : 1;
        u32 unrestricted_guest : 1;
        u32 apic_register_virtualization : 1;
        u32 virtual_interrupt_delivery : 1;
        u32 pause_loop_exiting : 1;
        u32 rdrand_exiting : 1;
        u32 enable_invpcid : 1;
        u32 enable_vm_functions : 1;
        u32 vmcs_shadowing : 1;
        u32 enable_encls_exiting : 1;
        u32 rdseed_exiting : 1;
        u32 enable_pml : 1;
        u32 ept_violation_ve : 1;
        u32 conceal_vmx_from_pt : 1;
        u32 enable_xsaves_xrstors : 1;
        u32 pasid_translation : 1;
        u32 mode_based_ept_execute : 1;
        u32 ept_sub_page_write_permissions : 1;
        u32 intel_pt_use_guest_phys_addr : 1;
        u32 use_tsc_scaling : 1;
        u32 enable_user_wait_and_pause : 1;
        u32 enable_pconfig : 1;
        u32 enable_enclv_exiting : 1;
        u32 reserved0 : 1;
        u32 vmm_bus_lock_detection : 1;
        u32 instruction_timeout : 1;
    } fields;
};

union vmcs_vmx_procbased_ctls3_t
{
    u64 ctl;
    struct
    {
        u64 loadiwkey_exiting : 1;
        u64 enable_hlat : 1;
        u64 ept_paging_write_control : 1;
        u64 guest_paging_verification : 1;
        u64 ipi_virtualization : 1;
        u64 reserved0 : 1;
        u64 enable_msr_list_instructions : 1;
        u64 virtualize_ia32_spec_ctrl : 1;
        u64 reserved1 : 56;
    } fields;
};

union vmcs_vmx_exit_ctls_t
{
    u32 ctl;
    struct
    {
        u32 reserved0 : 2;
        u32 save_debug_controls : 1;
        u32 reserved1 : 6;
        u32 host_address_space_size : 1;
        u32 reserved2 : 2;
        u32 load_ia32_perf_global_ctrl : 1;
        u32 reserved3 : 2;
        u32 acknowledge_interrupt_on_exit : 1;
        u32 reserved4 : 2;
        u32 save_ia32_pat : 1;
        u32 load_ia32_pat : 1;
        u32 save_ia32_efer : 1;
        u32 load_ia32_efer : 1;
        u32 save_vmx_preemption_timer_value : 1;
        u32 clear_ia32_bndcfgs : 1;
        u32 conceal_vmx_from_pt : 1;
        u32 clear_ia32_rtit_ctl : 1;
        u32 clear_ia32_lbr_ctl : 1;
        u32 clear_uinv : 1;
        u32 load_cet_state : 1;
        u32 load_pkrs : 1;
        u32 save_ia32_perf_global_ctl : 1;
        u32 activate_secondary_controls : 1;
    } fields;
};

union vmcs_vmx_exit_ctls2_t
{
    u32 ctl;
    struct
    {
        u32 reserved0 : 3;
        u32 prematurely_busy_shadow_stack : 1;
        u32 reserved1 : 28;
    } fields;
};

union vmcs_vmx_entry_ctls_t
{
    u32 ctl;
    struct
    {
        u32 reserved0 : 2;
        u32 load_debug_controls : 1;
        u32 reserved1 : 6;
        u32 ia32e_mode_guest : 1;
        u32 entry_to_smm : 1;
        u32 deactivate_dual_monitor_treatment : 1;
        u32 reserved2 : 1;
        u32 load_ia32_perf_global_ctrl : 1;
        u32 load_ia32_pat : 1;
        u32 load_ia32_efer : 1;
        u32 load_ia32_bndcfgs : 1;
        u32 conceal_vmx_from_pt : 1;
        u32 load_ia32_rtit_ctl : 1;
        u32 load_uinv : 1;
        u32 load_cet_state : 1;
        u32 load_ia32_lbr_ctl : 1;
        u32 load_pkrs : 1;
        u32 reserved3 : 9;
    } fields;
};

struct vmcs_region 
{
    union 
    {
        u32 val;
        struct 
        {
            u32 revision_id : 31;
            u32 shadow_vmcs_indicator : 1;
        } fields;
    } header;
    u32 abort_id;

    char data[VMCS_REGION_SIZE - sizeof(u64)];
} __pack;
size_assert(struct vmcs_region, VMCS_REGION_SIZE);

struct regs
{
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rdi;
    u64 rsi;
    u64 rdx;
    u64 rcx;
    u64 rbx;
    u64 rax;
    u64 rflags;
    u64 rbp;
} __pack;
size_assert(struct regs, 128);

struct vmcs
{
    struct vmcs_region *vmcs_region;
    u64 vmcs_region_phys;

    u64 eptp;

    struct 
    { 
        char *stack;
        size_t stack_size;
    } host;

    struct 
    {
        char *msr_bitmap;
        size_t msr_bitmap_size;

        char *io_bitmap_a;
        size_t io_bitmap_a_size;

        char *io_bitmap_b;
        size_t io_bitmap_b_size;
    } bmp;

    struct 
    {
        u32 pin;
        u32 proc;
        u32 proc2;
        u32 exit;
        u32 entry;
    } ctl_msr_cache;

    struct 
    {
        u16 cs;
        u16 ds;
        u16 ss;
        u16 es;
        u16 fs;
        u16 gs;
        u16 tr;
        u16 ldtr;
        struct __descriptor_table gdtr;
        struct __descriptor_table idtr;
    } selectors_cache;

    struct 
    {
        u64 cr0;
        u64 cr3;
        u64 cr4;
    } crx_cache;

    struct 
    {
        u64 esp;
        u64 eip;
        u16 cs; 
    } sysenter_cache;
};

struct vmcs *alloc_vmcs(void);
void free_vmcs(struct vmcs *vmcs);

u32 adjust_ctl(u32 msr, u32 ctl);
bool check_ctl(u32 msr, u32 ctl);

bool vmwrite_adjusted(u64 field, u32 msr, u32 ctl);

bool setup_vmcs_ctls(struct vcpu_ctx *ctx);
bool setup_vmcs_host_regs(struct vcpu_ctx *ctx, u64 rip);
bool setup_vmcs_guest_regs(struct vcpu_ctx *ctx, u64 rip, 
                           u64 rsp, u64 rflags);

bool do_vmcs_checks(struct vcpu_ctx *ctx);

#endif