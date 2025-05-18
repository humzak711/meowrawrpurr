#ifndef _EPT_H_
#define _EPT_H_

#include "arch.h"

#define PT_SIZE 4096

/* entries per pml3 and pml2 */
#define PT_ENTRIES 512

/* bcoz were doign 2mb pages */
#define EPT_PAGE_SIZE ((1024 * 1024) * 2)

#define PAGE_SHIFT_2MB 21

/*make dis shi jyst 3 level to save mem,
  and realistically we only need one pml4 entry
  so only need to map one pml3 in that case */
struct ept 
{
    union eptp_t eptp;
    union ept_pml4e_t *pml4;
    union ept_pml3e_t *pml3;
    union ept_pml2e_2mb_t *pml2_arr[PT_ENTRIES];
};

struct ept *map_ept(void);
void unmap_ept(struct ept *ept);

void __init_ept_memtypes(struct ept *ept);
void init_ept_pages(struct ept *ept);

#endif