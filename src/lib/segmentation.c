#include "include/segmentation.h"
#include "include/arch.h"

u64 get_segment_base(struct __descriptor_table gdtr, u16 __selector)
{
    union __selector_t selector;
    selector.val = __selector;

    if (selector.fields.ti == 0 && selector.fields.index == 0) 
        return 0;
    
    u16 max_segments = (gdtr.limit + 1) / 8;
    if (selector.fields.index >= max_segments)
        return 0;

    struct __segment_descriptor_32 *gdt = 
        (struct __segment_descriptor_32 *)gdtr.base;

    struct __segment_descriptor_32 *descriptor = 
        &gdt[selector.fields.index];

    u64 base = (u64)((descriptor->bitfield.fields.base_high << 24) |
                    (descriptor->bitfield.fields.base_mid << 16) |
                    descriptor->base_low);

    if ((selector.fields.index + 1) >= max_segments)
        return base;

    if (descriptor->bitfield.fields.available_for_system == 0 && 
        (descriptor->bitfield.fields.segment_type == TSS_AVAILABLE ||
         descriptor->bitfield.fields.segment_type == TSS_BUSY)) {

        struct __segment_descriptor_64 *expanded_descriptor =
            (struct __segment_descriptor_64 *)descriptor;
                
        base |= (u64)expanded_descriptor->base_upper << 32;
    }

    return base;
}

u32 get_segment_ar(u16 __selector)
{

    union __selector_t selector;
    selector.val = __selector;

    union access_rights_t ar = {0};

    if ((selector.fields.ti == 0 && selector.fields.index == 0)) {
	    ar.val = 0;
        ar.fields.segment_unusable = 1;
        return ar.val;
    }

    __lar(selector.val, &ar.val);
    ar.val >>= 8;
    ar.fields.segment_unusable = 0;
    ar.fields.reserved0 = 0;
    ar.fields.reserved1 = 0;

    return ar.val;
}

u32 get_segment_limit(u16 __selector)
{
    u32 limit = 0;
    __lsl(__selector, &limit);
    return limit;
}
