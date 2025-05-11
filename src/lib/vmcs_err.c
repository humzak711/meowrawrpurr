#include "include/vmcs_err.h"

#include "include/arch.h"
#include "include/vmcs_encoding.h"

const struct vmcs_err vmcs_err_table[] = {
    {0, UNKNOWN_ERR},
    {1, "VMCALL executed in VMX root operation"},
    {2, "VMCLEAR with invalid physical address"},
    {3, "VMCLEAR with VMXON pointer"},
    {4, "VMLAUNCH with non-clear VMCS"},
    {5, "VMRESUME with non-launched VMCS"},
    {6, "VMRESUME after VMXOFF"},
    {7, "VM entry with invalid control field(s)"},
    {8, "VM entry with invalid host-state field(s)"},
    {9, "VMPTRLD with invalid physical address"},
    {10, "VMPTRLD with VMXON pointer"},
    {11, "VMPTRLD with incorrect VMCS revision identifier"},
    {12, "VMREAD / VMWRITE from / to unsupported VMCS component"},
    {14, UNKNOWN_ERR},
    {13, "VMWRITE to read-only VMCS component"},
    {15, "VMXON executed in VMX root operation"},
    {16, "VM entry with invalid executive-VMCS pointer"},
    {17, "VM entry with non-launched executive VMCS"},
    {18, "VM entry with executive-VMCS pointer not VMXON pointer"},
    {19, "VMCALL with non-clear VMCS (dual-monitor treatment)"},
    {20, "VMCALL with invalid VM-exit control fields"},
    {22, "VMCALL with incorrect MSEG revision identifier"},
    {23, "VMXOFF under dual-monitor treatment of SMIs and SMM"},
    {24, "VMCALL with invalid SMM-monitor features (dual-monitor treatment)"},
    {25, "VM entry with invalid VM-exec control fields in executive VMCS"},
    {26, "VM entry with events blocked by MOV SS"},
    {27, UNKNOWN_ERR},
    {28, "Invalid operand to INVEPT / INVVPID"}
};

u64 vmcs_get_errcode(void)
{
    u64 errcode = 0;
    __vmread(VMCS_RO_VM_INSTRUCTION_ERROR, &errcode);
    return errcode;
}

char *vmcs_get_err(u64 errcode)
{
    if (errcode >= ARRAY_LEN(vmcs_err_table))
        return NULL;

    return vmcs_err_table[errcode].err;
}