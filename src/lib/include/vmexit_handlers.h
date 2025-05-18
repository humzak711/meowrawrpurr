#ifndef _VMEXIT_HANDLERS_H_
#define _VMEXIT_HANDLERS_H_

#include "vcpu.h"
#include "vmcs.h"

union vmexit_reason_t
{
   u32 val;
   struct
   {
        u32 basic_reason : 16;
        u32 cleared_to_0 : 1;
        u32 undefined0  : 8;
        u32 shadow_stack_busy : 1;
        u32 bus_lock_assert : 1;
        u32 enclave_mode : 1;
        u32 pending_mtf : 1;
        u32 exit_from_root : 1;
        u32 undefined1 : 1;
        u32 vmentry_failure : 1;
   } fields;
};

typedef enum {
    EXIT_REASON_EXCEPTION,
    EXIT_REASON_EXT_INTR,
    EXIT_REASON_TRIPLE_FAULT,
    EXIT_REASON_INIT,
    EXIT_REASON_SIPI,
    EXIT_REASON_IO_SMI,
    EXIT_REASON_SMI,
    EXIT_REASON_INTR_WINDOW,
    EXIT_REASON_NMI_WINDOW,
    EXIT_REASON_TASK_SWITCH,
    EXIT_REASON_CPUID,
    EXIT_REASON_GETSEC,
    EXIT_REASON_HLT,
    EXIT_REASON_INVD,
    EXIT_REASON_INVLPG,
    EXIT_REASON_RDPMC,
    EXIT_REASON_RDTSC,
    EXIT_REASON_RSM,
    EXIT_REASON_VMCALL,
    EXIT_REASON_VMCLEAR,
    EXIT_REASON_VMLAUNCH,
    EXIT_REASON_VMPTRLD,
    EXIT_REASON_VMPTRST,
    EXIT_REASON_VMREAD,
    EXIT_REASON_VMRESUME,
    EXIT_REASON_VMWRITE,
    EXIT_REASON_VMXOFF,
    EXIT_REASON_VMXON,
    EXIT_REASON_CR_ACCESS,
    EXIT_REASON_DR_ACCESS,
    EXIT_REASON_INOUT,
    EXIT_REASON_RDMSR,
    EXIT_REASON_WRMSR,
    EXIT_REASON_INVAL_VMCS,
    EXIT_REASON_INVAL_MSR,
    EXIT_REASON_MWAIT = 36,
    EXIT_REASON_MTF,
    EXIT_REASON_MONITOR = 39,
    EXIT_REASON_PAUSE,
    EXIT_REASON_MCE_DURING_ENTRY,
    EXIT_REASON_TPR = 43,
    EXIT_REASON_APIC_ACCESS,
    EXIT_REASON_VIRTUALIZED_EOI,
    EXIT_REASON_GDTR_IDTR,
    EXIT_REASON_LDTR_TR,
    EXIT_REASON_EPT_FAULT,
    EXIT_REASON_EPT_MISCONFIG,
    EXIT_REASON_INVEPT,
    EXIT_REASON_RDTSCP,
    EXIT_REASON_VMX_PREEMPT,
    EXIT_REASON_INVVPID,
    EXIT_REASON_WBINVD,
    EXIT_REASON_XSETBV,
    EXIT_REASON_APIC_WRITE,
    EXIT_REASON_RDRAND,
    EXIT_REASON_INVPCID,
    EXIT_REASON_VMFUNC,
    EXIT_REASON_ENCLS,
    EXIT_REASON_RDSEED,
    EXIT_REASON_PGMOD_LOG_FULL,
    EXIT_REASON_XSAVES,
    EXIT_REASON_XRSTORS,
    EXIT_REASON_PCONFIG,
    EXIT_REASON_SPP,
    EXIT_REASON_UMWAIT,
    EXIT_REASON_TPAUSE,
    EXIT_REASON_LOADIWKEY,
    EXIT_REASON_ENCLV,
    EXIT_REASON_ENQCMD_PASID_FAILURE = 72,
    EXIT_REASON_ENQCMDS_FAILURE,
    EXIT_REASON_BUSLOCK,
    EXIT_REASON_INSTRUCTION_TIMEOUT,
    EXIT_REASON_SEAMCALL,
    EXIT_REASON_TDCALL,
    EXIT_REASON_RDMSRLIST,
    EXIT_REASON_WRMSRLIST,
} basic_vmexit_reason_t;

#define VMENTRY_INVAL_GUEST_STATE 33
#define VMENTRY_MSR_LOADING 34
#define VMENTRY_MCE 41

union ept_violation_qualification_t
{
    u64 val;
    struct 
    {
        u64 r : 1;
        u64 w : 1;
        u64 x : 1;
        u64 gpa_r : 1;
        u64 gpa_w : 1;
        u64 gpa_x : 1;
        u64 gpa_user_x : 1;
        u64 gla_valid : 1;
        u64 access_gpa : 1;
        u64 adv_user_linear_addr : 1;
        u64 adv_rw : 1;
        u64 adv_nx : 1;
        u64 nmi_unblocking_iret : 1;
        u64 shadow_stack_access : 1;
        u64 reserved0 : 2;
        u64 pt : 1;
        u64 reserved1 : 47;
    } fields;
};

#define FEATURE_BITS_ECX_VMX (1 << 5)
#define FEATURE_BITS_ECX_HV (1 << 31)

#define CPUID_HV_ID 0x40000001
#define HV_ID 'meow'

typedef bool (*vmexit_handler_func_t)(struct vcpu_ctx *ctx, 
                                      struct regs *guest_regs);
struct vmexit_handler
{
    vmexit_handler_func_t func;
    basic_vmexit_reason_t reason;
};

bool handle_cpuid(struct vcpu_ctx *ctx, struct regs *guest_regs);
bool handle_ept_fault(struct vcpu_ctx *ctx, struct regs *guest_regs);
bool handle_ept_misconfig(struct vcpu_ctx *ctx, struct regs *guest_regs);

#endif