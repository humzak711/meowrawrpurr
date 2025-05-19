#include "include/ept.h"

#include <linux/slab.h>

/* map 3 level page table here to save memory lowkey */

struct ept *map_ept(void)
{
    struct ept *ept = kzalloc(sizeof(struct ept), GFP_KERNEL);
    if (!ept)
        return ERR_PTR(-ENOMEM);

    union ia32_mtrr_def_type_t mtrr_def; 
    mtrr_def.val = __rdmsrl(IA32_MTRR_DEF_TYPE);
    if (mtrr_def.fields.mtrr_enable) {

        union ia32_mtrrcap_t cap; 
        cap.val = __rdmsrl(IA32_MTRRCAP);

        if (cap.fields.vcnt > 0) {
            ept->mtrr.mtrrs = kzalloc(cap.fields.vcnt, GFP_KERNEL);
            if (!ept->mtrr.mtrrs)
                goto mtrrs_failed;

            ept->mtrr.vcnt = cap.fields.vcnt;
            ept->mtrr.mtrrs_enabled = true;
        }
    }

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
    if (ept->mtrr.mtrrs)
        kfree(ept->mtrr.mtrrs);

mtrrs_failed:
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

    if (ept->mtrr.mtrrs)
        kfree(ept->mtrr.mtrrs);

    kfree(ept);
}

void __init_ept_mtrr(struct ept *ept)
{
    for (u32 mtrr_reg = 0; mtrr_reg < ept->mtrr.vcnt; mtrr_reg++) {

        union ia32_mtrr_physmask_t mask;
        mask.val = __rdmsrl(IA32_MTRR_PHYSMASK0 + (mtrr_reg + 2));

        if (!mask.fields.valid)
            continue;

        union ia32_mtrr_physbase_t base;
        base.val = __rdmsrl(IA32_MTRR_PHYSBASE0 + (mtrr_reg * 2));

        u64 bit = 0;
        if (!first_set_bit(&bit, mask.fields.physmask << 12))
            continue;

        u64 base_addr = base.fields.physbase << 12;
        u64 end_addr = base_addr + ((1ULL << bit) - 1ULL);

        ept->mtrr.mtrrs[mtrr_reg].base = base;
        ept->mtrr.mtrrs[mtrr_reg].mask = mask;
        ept->mtrr.mtrrs[mtrr_reg].base_addr = base_addr;
        ept->mtrr.mtrrs[mtrr_reg].end_addr = end_addr;
        ept->mtrr.mtrrs[mtrr_reg].type = base.fields.type;
    }
}

void __setup_ept_page_mtrr(union ept_pml2e_2mb_t *pml2e, u64 pfn,
                           struct mtrr_data *mtrrs, u32 vcnt)
{
    u32 type = EPT_WB;
    if (pfn == 0) {
        type = EPT_UC;
        goto apply;
    }
    
    u64 base = pfn << PAGE_SHIFT_2MB;
    u64 end = (base + PAGE_SHIFT_2MB - 1);
    
    for (u32 i = 0; i < vcnt; i++) {
        if (base <= mtrrs[i].end_addr && end >= mtrrs[i].base_addr) {
            
            type = mtrrs[i].type;
            if (type == EPT_UC)
                break;
        }
    }

apply:
    pml2e->fields.pfn = pfn;
    pml2e->fields.memtype = type;
    pml2e->fields.r = 1;
    pml2e->fields.w = 1;
    pml2e->fields.x = 1;
    pml2e->fields.page_2mb = 1;
}

void __setup_ept_page(union ept_pml2e_2mb_t *pml2e, u64 pfn)
{
    pml2e->fields.pfn = pfn;
    pml2e->fields.memtype = EPT_WB;
    pml2e->fields.r = 1;
    pml2e->fields.w = 1;
    pml2e->fields.x = 1;
    pml2e->fields.page_2mb = 1;
}

void __init_ept_pages_mtrr(struct ept *ept)
{
    __init_ept_mtrr(ept);

    u64 pfn = 0;
    for (u32 i = 0; i < ARRAY_LEN(ept->pml2_arr); i++) {
        for (u32 j = 0; j < PT_ENTRIES; j++) {

            __setup_ept_page_mtrr(&ept->pml2_arr[i][j], pfn,
                 ept->mtrr.mtrrs, ept->mtrr.vcnt);

            pfn++;
        }
    }
}

void init_ept_pages(struct ept *ept)
{
    if (ept->mtrr.mtrrs_enabled) 
        return __init_ept_pages_mtrr(ept);

    u64 pfn = 0;
    for (u32 i = 0; i < ARRAY_LEN(ept->pml2_arr); i++) {
        for (u32 j = 0; j < PT_ENTRIES; j++) {
            __setup_ept_page(&ept->pml2_arr[i][j], pfn);
            pfn++;
        }
    }
}
