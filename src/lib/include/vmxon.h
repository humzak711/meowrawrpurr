#ifndef _VMXON_H_
#define _VMXON_H_

#include "arch.h"

#define VMXON_REGION_SIZE 4096

struct vmxon_region 
{
    union 
    {
        u32 val;
        struct 
        {
            u32 revision_id : 31;
            u32 reserved0 : 1;
        } fields;
    } header;
    u32 abort_id;

    char data[VMXON_REGION_SIZE - sizeof(u64)];
} __pack;
size_assert(struct vmxon_region, VMXON_REGION_SIZE);

struct vmxon_region *alloc_vmxon_region(void);
void free_vmxon_region(struct vmxon_region *vmxon_region);

#endif