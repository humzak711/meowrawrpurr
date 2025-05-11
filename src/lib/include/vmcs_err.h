#ifndef _VMCS_ERR_H_
#define _VMCS_ERR_H_

#include "arch.h"

#define UNKNOWN_ERR "unknown"

struct vmcs_err 
{
    u64 errcode;
    char *err;
};

extern const struct vmcs_err vmcs_err_table[];

u64 vmcs_get_errcode(void);
char *vmcs_get_err(u64 errcode); 

#endif