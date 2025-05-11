#ifndef _SEGMENTATION_H_
#define _SEGMENTATION_H_

#include "arch.h"

#define TSS_AVAILABLE 0x9
#define TSS_BUSY 0xB

u64 get_segment_base(struct __descriptor_table gdtr, u16 __selector);
u32 get_segment_ar(u16 __selector);
u32 get_segment_limit(u16 __selector);

#endif