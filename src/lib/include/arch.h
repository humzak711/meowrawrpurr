#ifndef _ARCH_H_
#define _ARCH_H_

#include <linux/types.h>
#include <linux/processor.h>
#include "vmcs_encoding.h"

#define __noinline __attribute__ ((__noinline__))
#define __pack __attribute__ ((packed))

#define size_assert(obj, size) \
    _Static_assert(sizeof(obj) == size, "size mismatch: " #obj)

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

union access_rights_t
{
    u32 val;
    struct
    {
        u32 segment_type : 4;
        u32 descriptor_type : 1;
        u32 dpl : 2;
        u32 p : 1;
        u32 reserved0 : 4;
        u32 avl : 1;
        u32 longmode : 1;
        u32 db : 1;
        u32 g : 1;
        u32 segment_unusable : 1;
        u32 reserved1 : 15;
    } fields;
};

union __selector_t 
{
    u16 val;
    struct 
    {
        u16 rpl : 2;
        u16 ti : 1;
        u16 index : 13;
    } fields;
};
    
struct __descriptor_table 
{
    u16 limit;
    u64 base;
} __pack;
size_assert(struct __descriptor_table, 10);

struct __segment_descriptor_32
{
    u16 limit_low;
    u16 base_low;
    union
    {
        u32 val;
        struct
        {
            u32 base_mid : 8;
            u32 segment_type : 4;
            u32 descriptor_type : 1;
            u32 dpl : 2;
            u32 present : 1;
            u32 limit_high : 4;
            u32 avl : 1;
            u32 long_mode : 1;
            u32 db : 1;
            u32 granularity : 1;
            u32 base_high : 8;
        } fields;
    } bitfield;
} __pack;
size_assert(struct __segment_descriptor_32, 8);

struct __segment_descriptor_64
{
    u16 limit_low;
    u16 base_low;
    union
    {
        u32 val;
        struct
        {
            u32 base_mid : 8;
            u32 segment_type : 4;
            u32 reserved0 : 1;
            u32 dpl : 2;
            u32 present : 1;
            u32 limit_high : 4;
            u32 avl : 1;
            u32 reserved1 : 1;
            u32 reserved2 : 1;
            u32 granularity : 1;
            u32 base_high : 8;
        } fields;
    } descriptor_lower;

    u32 base_upper;
    union
    {
        u32 val;
        struct
        {
            u32 reserved0 : 8;
            u32 reserved1 : 5;
            u32 reserved2 : 19;
        } fields;
    } descriptor_upper;
} __pack;
size_assert(struct __segment_descriptor_64, 16);

/* ctrls */

union cr0_t
{
    u64 val;
    struct
    {
        u64 pe : 1;
        u64 mp : 1;
        u64 em : 1;
        u64 ts : 1;
        u64 et : 1;
        u64 ne : 1;
        u64 reserved0 : 10;
        u64 wp : 1;
        u64 reserved1 : 1;
        u64 am : 1;
        u64 reserved2 : 10;
        u64 nw : 1;
        u64 cd : 1;
        u64 pg : 1;
        u64 undefined0 : 32;
    } fields;
};

union cr4_t
{
    u64 val;
    struct
    {
        u64 vme : 1;
        u64 pvi : 1;
        u64 tsd : 1;
        u64 de : 1;
        u64 pse : 1;
        u64 pae : 1;
        u64 mce : 1;
        u64 pge : 1;
        u64 pce : 1;
        u64 osfxr : 1;
        u64 osxmmexcpt : 1;
        u64 umip : 1;
        u64 la57 : 1;
        u64 vmxe : 1;
        u64 smxe : 1;
        u64 reserved0 : 1;
        u64 fsgsbase : 1;
        u64 pcide : 1;
        u64 osxsave : 1;
        u64 kl : 1;
        u64 smep : 1;
        u64 smap : 1;
        u64 pke : 1;
        u64 cet : 1;
        u64 pks : 1;
        u64 uintr : 1;
        u64 reserved1 : 2;
        u64 lam_sup : 1;
    } fields;
};

inline u64 __do_read_cr0(void);
inline void __do_write_cr0(u64 val);

inline u64 __do_read_cr2(void);
inline void __do_write_cr2(u64 val);

inline u64 __do_read_cr3(void);
inline void __do_write_cr3(u64 val);

inline u64 __do_read_cr4(void);
inline void __do_write_cr4(u64 val);

inline u64 __do_read_cr8(void);
inline void __do_write_cr8(u64 val);

/* cpuid */

#define CPUID_MANUFACTURER_ID 0
#define GENUINE_INTEL_EBX 0x756E6547
#define GENUINE_INTEL_ECX 0x6C65746E
#define GENUINE_IOTEL_ECX 0x6C65746F
#define GENUINE_INTEL_EDX 0x49656E69

bool is_cpu_intel(void);

#define CPUID_FEATURE_BITS 1
union cpuid_feature_bits_eax_t
{
    u32 val;
    struct
    {
        u32 stepping_id : 4;
        u32 model : 4;
        u32 family_id : 4;
        u32 processor_type : 2;
        u32 reserved0 : 2;
        u32 extended_model_id : 4;
        u32 extended_family_id : 8;
        u32 reserved1 : 4;
    } fields;
};

union cpuid_feature_bits_ebx_t
{
    u32 val;
    struct
    {
        u32 brand_index : 8;
        u32 cache_line_size : 8;
        u32 max_addressible_ids : 8;
        u32 lapic_id : 8;
    } fields;
};

union cpuid_feature_bits_ecx_t
{
    u32 val;
    struct
    {
        u32 sse3 : 1;
        u32 pclmulqdq : 1;
        u32 dtes64 : 1;
        u32 monitor : 1;
        u32 ds_cpl : 1;
        u32 vmx : 1;
        u32 smx : 1;
        u32 est : 1;
        u32 tm2 : 1;
        u32 ssse3 : 1;
        u32 cnxt_id : 1;
        u32 sdbg : 1;
        u32 fma : 1;
        u32 cx16 : 1;
        u32 xptr : 1;
        u32 pdcm : 1;
        u32 reserved0 : 1;
        u32 pcid : 1;
        u32 dca : 1;
        u32 sse4_1 : 1;
        u32 sse4_2 : 1;
        u32 x2apic : 1;
        u32 movbe : 1;
        u32 popcnt : 1;
        u32 tsc_deadline : 1;
        u32 aes_ni : 1;
        u32 xsave : 1;
        u32 osxsave : 1;
        u32 avx : 1;
        u32 f16c : 1;
        u32 rdrnd : 1;
        u32 hypervisor : 1;
    } fields;
};

union cpuid_feature_bits_edx_t
{
    u32 val;
    struct
    {
        u32 fpu : 1;
        u32 vme : 1;
        u32 de : 1;
        u32 pse : 1;
        u32 tsc : 1;
        u32 msr : 1;
        u32 pae : 1;
        u32 mce : 1;
        u32 cx8 : 1;
        u32 apic : 1;
        u32 reserved0 : 1;
        u32 sep : 1;
        u32 mtrr : 1;
        u32 pge : 1;
        u32 mca : 1;
        u32 cmov : 1;
        u32 pat : 1;
        u32 pse_36 : 1;
        u32 psn : 1;
        u32 clfsh : 1;
        u32 nx : 1;
        u32 ds : 1;
        u32 acpi : 1;
        u32 mmx : 1;
        u32 fxsr : 1;
        u32 sse : 1;
        u32 sse2 : 1;
        u32 ss : 1;
        u32 htt : 1;
        u32 tm : 1;
        u32 ia64 : 1;
        u32 pbe : 1;
    } fields;
};

__noinline union cpuid_feature_bits_eax_t get_cpuid_feature_bits_eax(void);
__noinline union cpuid_feature_bits_ebx_t get_cpuid_feature_bits_ebx(void);
__noinline union cpuid_feature_bits_ecx_t get_cpuid_feature_bits_ecx(void);
__noinline union cpuid_feature_bits_edx_t get_cpuid_feature_bits_edx(void);

/* msr's */

inline u64 __rdmsrl(u32 msr);
inline void __wrmsrl(u32 msr, u64 val);

#define IA32_FEATURE_CONTROL 0x03A
union ia32_feature_control_t
{
    u64 val;
    struct
    {
        u64 locked : 1;
        u64 vmx_inside_smx : 1;
        u64 vmx_outside_smx : 1;
        u64 reserved0 : 5;
        u64 senter_local_enable : 7;
        u64 senter_global_enable : 1;
        u64 reserved1 : 1;
        u64 sgx_launch_control_enable : 1;
        u64 sgx_global_enable : 1;
        u64 reserved2 : 1;
        u64 lmce_on : 1;
        u64 reserved3 : 43;
    } fields;
};


#define IA32_MISC_ENABLE 0x1A0
union ia32_misc_enable_t
{
    u64 val;
    struct
    {
        u64 fast_strings_enable : 1;
        u64 reserved1 : 2;
        u64 tcc : 1;
        u64 reserved2 : 3;
        u64 performance_monitoring_enable : 1;
        u64 reserved3 : 3;
        u64 branch_trace_storage_unavailable : 1;
        u64 pebs_unavailable : 1;
        u64 reserved4 : 3;
        u64 enhanced_intel_speedstep_tech_enabled : 1;
        u64 reserved5 : 1;
        u64 monitor_fsm_enabled : 1;
        u64 reserved6 : 3;
        u64 limit_cpuid_maxval : 1;
        u64 xtrp_message_disable : 1;
        u64 reserved : 20;
    } fields;
};

#define IA32_SPEC_CTRL 0x48
union ia32_spec_ctrl_t
{
    u64 val;
    struct
    {
        u64 ibrs : 1;  // tags the btb or something, it prevents bti
        u64 stibp : 1; // btb logical core tagging when smt is enabled

        /*
            all loads with values speculatively forwarded stall until stores
            address is resolved and the dependancy is validated
        */
        u64 ssbd : 1;

        /*
            idk what these bits r for someone dm me on discord and
            explain them to me
        */
        u64 ipred_dis_u : 1;
        u64 ipred_dis_s : 1;

        // disable rsb alternate paths
        u64 rrsba_dis_u : 1;
        u64 rrsba_dis_s : 1;

        u64 psfd : 1;   // disable fsfp
        u64 ddpd_u : 1; // disable data dependancy predictor
        u64 reserved1 : 1;
        u64 bhi_dis_s : 1; // bhi mitigation

        u64 reserved2 : 53;
    } fields;
};

#define IA32_ARCH_CAPABILITIES 0x10A
union ia32_arch_capabilities_t
{
    u64 val;
    struct
    {
        u64 rdcl_no : 1;
        u64 ibrs_all : 1;
        u64 rsba : 1;
        u64 skip_l1dfl_vmentry : 1;
        u64 ssb_no : 1;
        u64 mds_no : 1;
        u64 if_pschange_mc_no : 1;
        u64 tsx_ctrl : 1;
        u64 taa_no : 1;
        u64 mcu_control : 1;
        u64 misc_package_ctls : 1;
        u64 energy_filtering_ctl : 1;
        u64 doitm : 1;
        u64 sbdr_ssdp_no : 1;
        u64 fbsdp_no : 1;
        u64 psdp_no : 1;
        u64 mcu_enumeration : 1;
        u64 fb_clear : 1;
        u64 fb_clear_ctrl : 1;
        u64 rrsba : 1;
        u64 bhi_no : 1;
        u64 xapic_disable_status : 1;
        u64 mcu_extended_service : 1;
        u64 overclocking_status : 1;
        u64 pbrsb_no : 1;
        u64 gds_ctrl : 1;
        u64 gds_no : 1;
        u64 rfds_no : 1;
        u64 rfds_clear : 1;
        u64 ign_umonitor_support : 1;
        u64 mon_umon_mitg_support : 1;
        u64 reserved : 33;
    } fields;
};

#define IA32_MCU_OPT_CTRL 0x123
union ia32_mcu_opt_ctrl_t
{
    u64 val;
    struct 
    {
        u64 rngds_mit_dis : 1;
        u64 rtm_allow : 1;
        u64 rtm_locked : 1;
        u64 fb_clear_dis : 1; // verw clears microarch buffers
        u64 gds_mit_dis : 1;
        u64 gds_mit_locked : 1;
        u64 ign_umonitor : 1;
        u64 mon_umon_mit : 1;
        u64 reserved0 : 56;
    } fields;
};

#define IA32_DEBUG_CTL 0x1D9
union ia32_debug_ctl_t
{
    u64 val;
    struct
    {
        u64 lbr : 1;
        u64 btf : 1;
        u64 bld : 1;
        u64 reserved1 : 3;
        u64 tr : 1;
        u64 bts : 1;
        u64 btint : 1;
        u64 bts_off_os : 1;
        u64 bts_off_usr : 1;
        u64 freeze_lbrs_on_pmi : 1;
        u64 freeze_perfmon_on_pmi : 1;
        u64 enable_unicore_pmi : 1;
        u64 freeze_while_smm : 1;
        u64 rtm_debug : 1;
        u64 reserved2 : 48;
    } fields;
};

#define IA32_SYSENTER_CS 0x174
union ia32_sysenter_cs_t 
{
    u64 val;
    struct 
    {
        u64 cs : 16;
        u64 unused : 48;
    } fields;
};

#define IA32_SYSENTER_ESP 0x175
#define IA32_SYSENTER_EIP 0x176

#define IA32_PERF_GLOBAL_CTRL 0x38F
union ia32_perf_global_ctrl_t
{
    u64 val;
    struct 
    {
        u64 en_pmc0 : 1;
        u64 en_pmc1 : 1;
        u64 en_pmc2 : 1;
        u64 en_pmcn : 28;
        u64 reserved0 : 1;
        u64 en_fixed_ctr0 : 1;
        u64 en_fixed_ctrm : 14;
        u64 reserved1 : 1;
        u64 en_perf_metrics : 1;
        u64 reserved2 : 15;
    } fields;
};

#define IA32_PAT 0x277
union ia32_pat_t 
{
    u64 val;
    struct 
    {
        u64 pa0 : 3;
        u64 reserved0 : 5;
        u64 pa1: 3;
        u64 reserved1 : 5;
        u64 pa2: 3;
        u64 reserved2 : 5;
        u64 pa3: 3;
        u64 reserved3 : 5;
        u64 pa4: 3;
        u64 reserved4 : 5;
        u64 pa5: 3;
        u64 reserved5 : 5;
        u64 pa6: 3;
        u64 reserved6 : 5;
        u64 pa7: 3;
        u64 reserved7 : 5;
    } fields;
};

#define IA32_EFER 0xC0000080
union ia32_efer_t
{
    u64 val;
    struct 
    {
        u64 syscall_enable : 1;
        u64 reserved0 : 7;
        u64 lme : 1;
        u64 reserved1 : 1;
        u64 lma : 1;
        u64 nxe : 1;
        u64 reserved2 : 52;
    } fields;
};

#define IA32_FS_BASE 0xC0000100
#define IA32_GS_BASE 0xC0000101
#define IA32_KERNEL_GS_BASE 0xC0000102

#define IA32_BNDCFGS 0xD90
union ia32_bndcfgs_t
{
    u64 val;
    struct
    {
        u64 en : 1;
        u64 bndpreserve : 1;
        u64 reserved0 : 10;
        u64 bound_dir_base_addr : 52;
    } fields;
};

#define IA32_S_CET 0x6A2

#define IA32_INTERRUPT_SSP_TABLE_ADDR 0X6A8

#define IA32_PKRS 0x6E1
union ia32_pkrs_t 
{
    u64 val;
    struct 
    {
        u64 permissions : 32;
        u64 reserved0 : 32;
    } fields;
};

#define IA32_RTIT_CTL 0x570
union ia32_rtit_ctl_t
{
    u64 val;
    struct 
    {
        u64 trace_en : 1;
        u64 cyc_en : 1;
        u64 os : 1;
        u64 usr : 1;
        u64 pwr_evt_en : 1;
        u64 fup_on_ptw : 1;
        u64 fabric_en : 1;
        u64 cr3_filter : 1;
        u64 to_pa : 1;
        u64 mtc_en : 1;
        u64 tsc_en : 1;
        u64 dis_retc : 1;
        u64 ptw_en : 1;
        u64 branch_en : 1;
        u64 mtc_freq : 4;
        u64 reserved0 : 1;
        u64 cyc_thresh : 4;
        u64 reserved1 : 1;
        u64 psb_freq : 4;
        u64 reserved2 : 3;
        u64 event_en : 1;
        u64 addr0_cfg : 4;
        u64 addr1_cfg : 4;
        u64 addr2_cfg : 4;
        u64 addr3_cfg : 4;
        u64 reserved3 : 7;
        u64 dis_tnt : 1;
        u64 inject_psb_pmi_on_enable : 1;
        u64 reserved4 : 7;
    } fields;
};

#define IA32_LBR_CTL 0x14CE
union ia32_lbr_ctl_t
{
    u64 val;
    struct
    {
        u64 lbr_enabled : 1;
        u64 os : 1;
        u64 usr : 1;
        u64 call_stack : 1;
        u64 reserved1 : 12;
        u64 cond : 1;
        u64 near_rel_jmp : 1;
        u64 near_ind_jmp : 1;
        u64 near_rel_call : 1;
        u64 near_ind_call : 1;
        u64 near_ret : 1;
        u64 other_branch : 1;
        u64 reserved2 : 41;
    } fields;
};

#define IA32_VMX_BASIC 0x480
union ia32_vmx_basic_t 
{
    u64 val;
    struct 
    {
        u64 revision_id : 31;
        u64 reserved0 : 1;
        u64 num_bytes : 13;
        u64 reserved1 : 3;
        u64 phys_addr_width : 1;
        u64 dual_monitor_smm_supported : 1;
        u64 vmcs_mem_type : 4;
        u64 vmcs_instruction_info_reports : 1;
        u64 defaults_to_one_clear : 1;
        u64 vmentry_hw_exceptions : 1;
        u64 reserved2 : 7;
    } fields;
};

#define IA32_VMX_PINBASED_CTLS 0x481
#define IA32_VMX_TRUE_PINBASED_CTLS 0x48D

#define IA32_VMX_PROCBASED_CTLS 0x482
#define IA32_VMX_TRUE_PROCBASED_CTLS 0x48E

#define IA32_VMX_PROCBASED_CTLS2 0x48B

#define IA32_VMX_PROCBASED_CTLS3 0x492

#define IA32_VMX_EXIT_CTLS 0x483
#define IA32_VMX_TRUE_EXIT_CTLS 0x48F

#define IA32_VMX_EXIT_CTLS2 0x493

#define IA32_VMX_ENTRY_CTLS 0x484
#define IA32_VMX_TRUE_ENTRY_CTLS 0x490

#define IA32_VMX_MISC 0x485
union ia32_vmx_misc_t 
{
    u64 val;
    struct 
    {
        u64 preempt_timer_tsc_relation : 5;
        u64 lma_store : 1;
        u64 activity_state : 3;
        u64 reserved0 : 5;
        u64 pt : 1;
        u64 smm_reads_smbase : 1;
        u64 cr3_target_num : 9;
        u64 msr_store_list_recommended_no : 3;
        u64 can_block_smi_vmxoff : 1;
        u64 vmwrite_supported : 1;
        u64 exception_injection_supported : 1;
        u64 reserved1 : 1;
        u64 mseg_revision_id : 32;
    } fields;
};

#define IA32_VMX_CR0_FIXED0 0x486
#define IA32_VMX_CR0_FIXED1 0x487
#define IA32_VMX_CR4_FIXED0 0x488
#define IA32_VMX_CR4_FIXED1 0x489

#define IA32_VMX_VMCS_ENUM 0x48A
union ia32_vmx_vmcs_enum_t 
{
    u32 val;
    struct 
    {
        u32 access_type : 1;
        u32 index : 9;
        u32 field_type : 2;
        u32 reserved0 : 1;
        u32 field_width : 2;
        u32 reserved1 : 17;
    } fields;
};

#define IA32_VMX_EPT_VPID_CAP 0x48C
union ia32_vmx_ept_vpid_cap_t 
{
    u64 val;
    struct 
    {
        u64 execute_only_transitions_supported : 1;
        u64 reserved0 : 5;
        u64 pagewalk_len_4_supported : 1;
        u64 pagewalk_len_5_supported : 1;
        u64 ept_uc_supported : 1;
        u64 reserved1 : 5;
        u64 ept_wb_supported : 1;
        u64 reserved2 : 1;
        u64 pde_2mb_page_supported : 1;
        u64 pdpte_1gb_page_supported : 1;
        u64 reserved3 : 2;
        u64 invept_supported : 1;
        u64 ept_accessed_dirty_supported : 1;
        u64 ept_violation_adv_reports : 1;
        u64 ept_s_cet_supported : 1;
        u64 reserved4 : 1;
        u64 single_ctx_invept_supported : 1;
        u64 all_ctx_invept_supported : 1;
        u64 reserved5 : 5;
        u64 invvpid_supported : 1;
        u64 reserved6 : 7;
        u64 individual_addr_invvpid_supported : 1;
        u64 single_ctx_invvpid_supported : 1;
        u64 all_ctx_invvpid_supported : 1;
        u64 single_ctx_remaining_globals_invvpid_supported : 1;
        u64 reserved7 : 4;
        u64 hlat_prefix_size : 6;
        u64 reserved8 : 10;
    } fields;
};

#define IA32_VMX_VMFUNC 0x491

enum ept_memory_types
{
    EPT_UC, 
    EPT_WC, 
    EPT_WT = 4,
    EPT_WP,
    EPT_WB
};

union ept_pml4e_t
{
    u64 val;
    struct 
    {
        u64 r : 1;
        u64 w : 1;
        u64 x : 1;
        u64 reserved0 : 5;
        u64 accessed : 1;
        u64 reserved1 : 1;
        u64 user_x : 1;
        u64 reserved2 : 1;
        u64 pml3_pfn : 52;
    } fields;
};

union ept_pml3e_1gb_t 
{
    u64 val;
    struct 
    {
        u64 r : 1;
        u64 w : 1;
        u64 x : 1;
        u64 memtype : 3;
        u64 ignore_pat : 1;
        u64 page_1gb : 1;
        u64 accessed : 1;
        u64 dirty : 1;
        u64 user_x : 1;
        u64 reserved0 : 1;
        u64 reserved1 : 18;
        u64 pfn : 22;
        u64 reserved2 : 5;
        u64 guest_pg_verification : 1;
        u64 pg_write_access : 1;
        u64 reserved3 : 1;
        u64 supervisor_shadow_stack : 1;
        u64 reserved4 : 2;
        u64 suppress_ve : 1;
    } fields;
};

union ept_pml3e_t
{
    u64 val;
    struct 
    {
        u64 r : 1;
        u64 w : 1;
        u64 x : 1;
        u64 reserved0 : 7;
        u64 user_x : 1;
        u64 reserved1 : 1;
        u64 pml2_pfn : 39;
        u64 reserved3 : 13;
    } fields;
};

union ept_pml2e_2mb_t 
{
    u64 val;
    struct 
    {
        u64 r : 1;
        u64 w : 1;
        u64 x : 1;
        u64 memtype : 3;
        u64 ignore_pat : 1;
        u64 page_2mb : 1;
        u64 accessed : 1;
        u64 dirty : 1;
        u64 user_x : 1;
        u64 reserved0 : 10;
        u64 pfn : 30;
        u64 reserved1 : 6;
        u64 guest_pg_verification : 1;
        u64 pg_write_accesste : 1;
        u64 reserved2 : 1;
        u64 supervisor_shadow_stack : 1;
        u64 reserved3 : 2;
        u64 suppress_ve : 1; 
    } fields;
};

union ept_pml2e_t
{
    u64 val;
    struct 
    {
        u64 r : 1;
        u64 w : 1;
        u64 x : 1;
        u64 reserved0 : 7;
        u64 user_x : 1;
        u64 reserved1 : 1;
        u64 pml1_pfn : 39;
        u64 reserved2 : 13;
    } fields;
};

union ept_pml1e_t 
{
    u64 val;
    struct 
    {
        u64 r : 1;
        u64 w : 1;
        u64 x : 1;
        u64 memtype : 3;
        u64 ignore_pat : 1;
        u64 reserved0 : 1;
        u64 accessed : 1;
        u64 dirty : 1;
        u64 user_x : 1;
        u64 reserved1 : 1;
        u64 pfn : 39;
        u64 reserved2 : 6;
        u64 guest_pg_verification : 1;
        u64 pg_write_access : 1;
        u64 reserved3 : 1;
        u64 supervisor_shadow_stack : 1;
        u64 sub_pg_write_permissions : 1;
        u64 reserved4 : 1;
        u64 suppress_ve : 1;
    } fields;
};

/* vmx operations */

inline bool __vmxon(u64 phys);
inline bool __vmxoff(void);

inline bool __vmread(u64 field, u64 *outp);
inline bool __vmwrite(u64 field, u64 inp);

inline bool __vmread32(u64 field, u32 *outp);

inline bool __vmptrld(u64 phys);
inline bool __vmptrst(u64 *outp);

inline bool __vmresume(void);
inline bool __vmclear(u64 phys);

inline bool __vmlaunch(void);

/* other intrinsics */

inline u16 __read_cs(void);
inline u16 __read_ds(void);
inline u16 __read_ss(void);
inline u16 __read_es(void);
inline u16 __read_fs(void);
inline u16 __read_gs(void);
inline u16 __str(void);
inline u16 __sldt(void);
inline struct __descriptor_table __sgdt(void);
inline struct __descriptor_table __sidt(void);
inline bool __lar(u16 segment, u32 *outp);
inline bool __lsl(u16 segment, u32 *outp);

inline u64 __read_rflags(void);
inline u64 __read_dr7(void);

/* vmcs ops */

bool guest_rip_add(u64 length);
bool guest_rip_next(void);

#endif