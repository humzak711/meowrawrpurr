#ifndef _SETUP_VMX_H_
#define _SETUP_VMX_H_

#include "arch.h"
#include "debug.h"

bool is_vmx_supported(void);

bool setup_ia32_feature_ctrl(void);

void clear_cr0_conflicts(void);
void clear_cr4_conflicts(void);

inline void toggle_cr4_vmxe(bool on);

int setup_vmx(void);

#endif