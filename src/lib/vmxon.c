#include "include/vmxon.h"
#include "include/arch.h"

#include <linux/slab.h>

struct vmxon_region *alloc_vmxon_region(void)
{
    struct vmxon_region *vmxon_region = 
        kzalloc(sizeof(struct vmxon_region), GFP_KERNEL);

    if (!vmxon_region)
        return ERR_PTR(-ENOMEM);

    union ia32_vmx_basic_t basic = {0};
    basic.val = __rdmsrl(IA32_VMX_BASIC);

    vmxon_region->header.fields.revision_id = basic.fields.revision_id;

    return vmxon_region;
}

void free_vmxon_region(struct vmxon_region *vmxon_region)
{
    if (vmxon_region) {
        kfree(vmxon_region);
        vmxon_region = NULL;
    }
}