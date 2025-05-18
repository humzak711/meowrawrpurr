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
    u64 pml2e_pfn = 0;
    u32 pml2_count = 0;
    for (; pml2_count < ARRAY_LEN(ept->pml2_arr); pml2_count++) {

        ept->pml2_arr[pml2_count] = kzalloc(PT_SIZE, GFP_KERNEL);
        if (!ept->pml2_arr[pml2_count])
            goto pml2_failed;

        ept->pml3[pml2_count].fields.r = 1;
        ept->pml3[pml2_count].fields.w = 1;
        ept->pml3[pml2_count].fields.x = 1;
        
        ept->pml3[pml2_count].fields.pml2_pfn = 
            __pa(ept->pml2_arr[pml2_count]) >> 12;
        
        for (u32 i = 0; i < PT_ENTRIES; i++) {
            ept->pml2_arr[pml2_count][i].fields.r = 1;
            ept->pml2_arr[pml2_count][i].fields.w = 1;
            ept->pml2_arr[pml2_count][i].fields.x = 1;
            ept->pml2_arr[pml2_count][i].fields.page_2mb = 1;
            ept->pml2_arr[pml2_count][i].fields.pfn = pml2e_pfn++;
        }
    }

    ept->eptp.fields.memtype = EPT_WB;
    ept->eptp.fields.walklength = 3;
    ept->eptp.fields.pfn = __pa(ept->pml4) >> 12;

    return ept;

pml2_failed:
    for (u32 i = 0; i < pml2_count; i++) {
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

void __init_ept_memtypes(struct ept *ept)
{
    union ia32_mtrrcap_t cap;
    cap.val = __rdmsrl(IA32_MTRRCAP);

    for (u32 mtrr_reg = 0; mtrr_reg < cap.fields.vcnt; mtrr_reg++) {

        union ia32_mtrr_physmask_t mask;
        mask.val = __rdmsrl(IA32_MTRR_PHYSMASK0 + (mtrr_reg + 2));

        if (!mask.fields.valid)
            continue;

        union ia32_mtrr_physbase_t base;
        base.val = __rdmsrl(IA32_MTRR_PHYSBASE0 + (mtrr_reg * 2));
        u64 type = base.fields.type;

        u64 bit = 0;
        if (!first_set_bit(&bit, mask.fields.physmask << 12))
            continue;

        u64 base_addr = base.fields.physbase << 12;
        u64 end_addr = base_addr + ((1ULL << bit) - 1ULL);

        u64 base_pfn = base_addr >> PAGE_SHIFT_2MB;
        u64 end_pfn = end_addr >> PAGE_SHIFT_2MB;

        for (u64 pfn = base_pfn; pfn <= end_pfn; pfn++) {
            u32 pml2_i = pfn / PT_ENTRIES;
            u32 pml2e_i = pfn % PT_ENTRIES;

            ept->pml2_arr[pml2_i][pml2e_i].fields.memtype = type;
        }
    }
}

void init_ept_pages(struct ept *ept)
{
    __init_ept_memtypes(ept);
}