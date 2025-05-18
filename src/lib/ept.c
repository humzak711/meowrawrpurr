#include "include/ept.h"

#include <linux/slab.h>

/* map 3 level page table here to save memory lowkey */

struct ept *map_ept(void)
{
    struct ept *ept = kzalloc(sizeof(struct ept), GFP_KERNEL);
    if (!ept)
        return ERR_PTR(-ENOMEM);

    /* finna map in dis pml4 */
    ept->pml4 = kzalloc(PT_SIZE, GFP_KERNEL);
    if (!ept->pml4)
        goto pml4_failed;

    /* map in da pml3 skrrr skrrrrrrrrr raaaaahh */
    ept->pml3 = kzalloc(PT_SIZE, GFP_KERNEL);
    if (!ept->pml3)
        goto pml3_failed;

    /* meow meow point pml4 first entry tp pml3 we
       legit only need one pml4 entry here bc 
       it accounts for like 512gb of memory
       or something i forgot cant remember dont
       care */
    ept->pml4[0].fields.r = 1;
    ept->pml4[0].fields.w = 1;
    ept->pml4[0].fields.x = 1;
    ept->pml4[0].fields.pml3_pfn = __pa(ept->pml3) >> 12;

    /* now we go iterate through our pml3 entries and 
      for each of em map the pml2 */
    u32 pml3e = 0;
    for (; pml3e < ARRAY_LEN(ept->pml2_arr); pml3e++) {

        ept->pml2_arr[pml3e] = kzalloc(PT_SIZE, GFP_KERNEL);
        if (!ept->pml2_arr[pml3e])
            goto pml2_failed;

        ept->pml3[pml3e].fields.r = 1;
        ept->pml3[pml3e].fields.w = 1;
        ept->pml3[pml3e].fields.x = 1;
        
        ept->pml3[pml3e].fields.pml2_pfn = 
            __pa(ept->pml2_arr[pml3e]) >> 12;
    }

    ept->eptp.fields.memtype = EPT_WB;
    ept->eptp.fields.walklength = 3;
    ept->eptp.fields.pfn = __pa(ept->pml4) >> 12;

    return ept;

pml2_failed:
    for (u32 i = 0; i < pml3e; i++) {
        if (ept->pml2_arr[i])
            kfree(ept->pml2_arr[i]);
    }

    kfree(ept->pml3);

pml3_failed:
    kfree(ept->pml4);

pml4_failed:
    kfree(ept);
    return ERR_PTR(-ENOMEM);
}

void unmap_ept(struct ept *ept)
{
    if (!ept)
        return;

    for (u32 i = 0; i < ARRAY_LEN(ept->pml2_arr); i++) {
        if (ept->pml2_arr[i])
            kfree(ept->pml2_arr[i]);
    }

    if (ept->pml3)
        kfree(ept->pml3);

    if (ept->pml4)
        kfree(ept->pml4);

    kfree(ept);
}

/* basically in this function 
   were gonna setup the caching 
   policy for our epts based on mtrrs 
   because if we dont do that, that isnt
   very good lmao, and we will set our pages
   to their corresponding pfn's too */
void init_ept_pages(struct ept *ept)
{
    u64 pfn = 0;


}