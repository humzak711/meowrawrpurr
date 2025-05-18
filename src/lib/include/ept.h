#ifndef _EPT_H_
#define _EPT_H_

#include "arch.h"

#define PT_SIZE 4096

/* entries per pml3 and pml2 */
#define PT_ENTRIES 512

/* bcoz were doign 2mb pages */
#define EPT_PAGE_SIZE ((1024 * 1024) * 2)

#define PAGE_SHIFT_2MB 21

struct mtrr_data
{
    union ia32_mtrr_physbase_t base;
    union ia32_mtrr_physmask_t mask;

    u64 base_addr;
    u64 end_addr;

    u32 type;
};

/*make dis shi jyst 3 level to save mem,
  and realistically we only need one pml4 entry
  so only need to map one pml3 in that case */
struct ept 
{
    union eptp_t eptp;
    union ept_pml4e_t *pml4;
    union ept_pml3e_t *pml3;
    union ept_pml2e_2mb_t *pml2_arr[PT_ENTRIES];

    struct 
    {
        struct mtrr_data *mtrrs;
        u32 vcnt;
        bool mtrrs_enabled;
    } mtrr;
};

struct ept *map_ept(void);
void unmap_ept(struct ept *ept);

void __init_ept_mtrr(struct ept *ept);

void __setup_ept_page_mtrr(union ept_pml2e_2mb_t *pml2e, u64 pfn,
                           struct mtrr_data *mtrrs, u32 vcnt);

void __setup_ept_page(union ept_pml2e_2mb_t *pml2e, u64 pfn);

void __init_ept_pages_mtrr(struct ept *ept);

void init_ept_pages(struct ept *ept);

#endif