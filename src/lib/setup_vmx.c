#include "include/setup_vmx.h"
#include "include/arch.h"
#include "include/debug.h"

bool is_vmx_supported()
{
    union cpuid_feature_bits_ecx_t ecx = get_cpuid_feature_bits_ecx();
    return ecx.fields.vmx != 0;
}

bool setup_ia32_feature_ctrl(void)
{
    union ia32_feature_control_t ctrl = {0};
    ctrl.val = __rdmsrl(IA32_FEATURE_CONTROL);

    if (ctrl.fields.locked != 0) 
        return ctrl.fields.vmx_outside_smx != 0;

    ctrl.fields.vmx_outside_smx = 1;
    ctrl.fields.locked = 1;
    __wrmsrl(IA32_FEATURE_CONTROL, ctrl.val);
    return true;
}

void clear_cr0_conflicts(void)
{
    u64 cr0 = __do_read_cr0();
    cr0 |= __rdmsrl(IA32_VMX_CR0_FIXED0);
    cr0 &= __rdmsrl(IA32_VMX_CR0_FIXED1);
    __do_write_cr0(cr0);
}

void clear_cr4_conflicts(void)
{
    u64 cr4 = __do_read_cr4();
    cr4 |= __rdmsrl(IA32_VMX_CR4_FIXED0);
    cr4 &= __rdmsrl(IA32_VMX_CR4_FIXED1);
    __do_write_cr4(cr4);
}

inline void toggle_cr4_vmxe(bool on)
{   
    union cr4_t cr4 = {0};
    cr4.val = __do_read_cr4();
    cr4.fields.vmxe = 1;
    __do_write_cr4(cr4.val);
}

int setup_vmx(void)
{
    if (!is_cpu_intel()) {
        HV_LOG(KERN_ERR, "cpu is not intel");
        return -EOPNOTSUPP;
    }

    if (!is_vmx_supported()) {
        HV_LOG(KERN_ERR, "vmx is not supported");
        return -EOPNOTSUPP;
    }

    if (!setup_ia32_feature_ctrl()) {
        HV_LOG(KERN_ERR, "couldn't setup ia32 feature ctrl");
        return -EFAULT;
    }

    toggle_cr4_vmxe(true);
    clear_cr0_conflicts();
    clear_cr4_conflicts();

    return 0;
}