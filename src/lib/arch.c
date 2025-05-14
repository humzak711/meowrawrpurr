#include "include/arch.h"

/* ctrls */

inline u64 __do_read_cr0(void)
{
    u64 val = 0;
    __asm__ __volatile__ (
        "lfence;"
        "movq %%cr0, %0;" 
        :"=r"(val) 
        : 
        :"memory"
    );

    return val;
}

inline void __do_write_cr0(u64 val)
{
    __asm__ __volatile__ (
        "sfence;"
        "movq %0, %%cr0;" 
        : 
        :"r"(val) 
        :"memory"
    );
}

inline u64 __do_read_cr2(void)
{
    u64 val = 0;
    __asm__ __volatile__ (
        "lfence;"
        "movq %%cr2, %0;" 
        :"=r"(val) 
        : 
        :"memory"
    );

    return val;
}

inline void __do_write_cr2(u64 val)
{
    __asm__ __volatile__ (
        "sfence;"
        "movq %0, %%cr2;" 
        : 
        :"r"(val) 
        :"memory"
    );
}

inline u64 __do_read_cr3(void)
{
    u64 val = 0;
    __asm__ __volatile__ (
        "lfence;"
        "movq %%cr3, %0;" 
        :"=r"(val) 
        : 
        :"memory"
    );

    return val;
}

inline void __do_write_cr3(u64 val)
{
    __asm__ __volatile__ (
        "sfence;"
        "movq %0, %%cr3;" 
        : 
        :"r"(val) 
        :"memory"
    );
}

inline u64 __do_read_cr4(void)
{
    u64 val = 0;
    __asm__ __volatile__ (
        "lfence;"
        "movq %%cr4, %0;" 
        :"=r"(val) 
        : 
        :"memory"
    );

    return val;
}

inline void __do_write_cr4(u64 val)
{
    __asm__ __volatile__ (
        "sfence;"
        "movq %0, %%cr4;" 
        : 
        :"r"(val) 
        :"memory"
    );
}

inline u64 __do_read_cr8(void)
{
    u64 val = 0;
    __asm__ __volatile__ (
        "lfence;"
        "movq %%cr8, %0;" 
        :"=r"(val) 
        : 
        :"memory"
    );

    return val;
}

inline void __do_write_cr8(u64 val)
{
    __asm__ __volatile__ (
        "sfence;"
        "movq %0, %%cr8;" 
        : 
        :"r"(val) 
        :"memory"
    );
}

/* cpuid */

bool is_cpu_intel(void)
{
    u32 regs[4] = {CPUID_MANUFACTURER_ID, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    return regs[1] == GENUINE_INTEL_EBX && regs[3] == GENUINE_INTEL_EDX &&
           (regs[2] == GENUINE_INTEL_ECX || regs[2] == GENUINE_IOTEL_ECX);
}

__noinline union cpuid_feature_bits_eax_t get_cpuid_feature_bits_eax(void)
{
    u32 regs[4] = {CPUID_FEATURE_BITS, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    union cpuid_feature_bits_eax_t eax;
    eax.val = regs[0];
    return eax;
}

__noinline union cpuid_feature_bits_ebx_t get_cpuid_feature_bits_ebx(void)
{
    u32 regs[4] = {CPUID_FEATURE_BITS, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    union cpuid_feature_bits_ebx_t ebx;
    ebx.val = regs[1];
    return ebx;
}

__noinline union cpuid_feature_bits_ecx_t get_cpuid_feature_bits_ecx(void)
{
    u32 regs[4] = {CPUID_FEATURE_BITS, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    union cpuid_feature_bits_ecx_t ecx;
    ecx.val = regs[2];
    return ecx;
}

__noinline union cpuid_feature_bits_edx_t get_cpuid_feature_bits_edx(void)
{
    u32 regs[4] = {CPUID_FEATURE_BITS, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    union cpuid_feature_bits_edx_t edx;
    edx.val = regs[3];
    return edx;
}

/* msr's */

inline u64 __rdmsrl(u32 msr)
{
    u32 eax = 0;
    u32 edx = 0;

    __asm__ __volatile__ (
        "lfence;"
        "rdmsr;"
        :"=a"(eax), "=d"(edx) 
        :"c"(msr)
        :"memory"
    );

    return ((u64)edx << 32) | eax;
}

inline void __wrmsrl(u32 msr, u64 val)
{
    __asm__ __volatile__(
        "movl %%esi, %%eax;"
        "movq %%rsi, %%rdx;"
        "shr $32, %%rdx;"
        "sfence;"
        "wrmsr;"
        :
        :"c"(msr), "S"(val));
}

/* vmx operations */

inline bool __vmxon(u64 phys)
{
    u8 ret = 0;
    __asm__ __volatile__ (
        "vmxon %[phys];"
        "seta %[ret];" 
        :[ret]"=r"(ret)
        :[phys]"m"(phys)
        :"cc", "memory"
    );

    return !!ret;
}

inline bool __vmxoff(void)
{
    u8 ret = 0;
    __asm__ __volatile__ (
        "vmxoff;"
        "seta %[ret];" 
        :[ret]"=r"(ret)
        :
        :"cc", "memory"
    );

    return !!ret;   
}

inline bool __vmread(u64 field, u64 *outp)
{
    u8 ret = 0;
    __asm__ __volatile__ (
        "vmread %[field], %[outp];"
        "seta %[ret];"
        :[ret]"=r"(ret), [outp]"=rm"(*outp)
        :[field]"r"(field)
        :"cc", "memory"
    );

    return !!ret;
}

inline bool __vmwrite(u64 field, u64 inp)
{
    u8 ret = 0;
    __asm__ __volatile__ (
        "vmwrite %[inp], %[field];"
        "seta %[ret];"
        :[ret]"=r"(ret)
        :[field]"r"(field), [inp]"rm"(inp)
        :"cc", "memory"
    );

    return !!ret;
}

inline bool __vmread32(u64 field, u32 *outp)
{
    u64 val = 0;
    bool ret = __vmread(field, &val);
    if (ret) 
        *outp = (u32)val;

    return ret;
}

inline bool __vmptrld(u64 phys)
{
    u8 ret = 0;
    __asm__ __volatile__ (
        "vmptrld %[phys];"
        "seta %[ret];"
        :[ret]"=r"(ret)
        :[phys]"m"(phys)
        :"cc", "memory"
    );

    return !!ret;
}

inline bool __vmptrst(u64 *outp)
{
    u8 ret = 0;
    __asm__ __volatile__ (
        "vmptrst %[phys];"
        "seta %[ret];"
        :[phys]"=m"(*outp), [ret]"=r"(ret)
        :
        :"cc", "memory"
    );

    return !!ret;
}

inline bool __vmresume(void)
{
    u8 ret = 0;
    __asm__ __volatile__ (
        "vmresume;"
        "seta %[ret];"
        :[ret]"=r"(ret)
        :
        :"cc"
    ); 

    return !!ret;
}

inline bool __vmclear(u64 phys)
{
    u8 ret = 0;
    __asm__ __volatile__ (
        "vmclear %[phys];"
        "seta %[ret];"
        :[ret]"=r"(ret)
        :[phys]"m"(phys)
        :"cc", "memory"
    );

    return !!ret;
}

inline bool __vmlaunch(void)
{
    u8 ret = 0;
    __asm__ __volatile__ (
        "vmlaunch;"
        "seta %[ret];"
        :[ret]"=r"(ret)
        :
        :"cc"
    );

    return !!ret;
}

/* other intrinsics */

inline u16 __read_cs(void)
{
    u16 cs = 0;
    __asm__ __volatile__ ("movw %%cs, %0;" :"=rm"(cs));
    return cs;
}

inline u16 __read_ds(void)
{
    u16 ds = 0;
    __asm__ __volatile__ ("movw %%ds, %0;" :"=rm"(ds));
    return ds;
}

inline u16 __read_ss(void)
{
    u16 ss = 0;
    __asm__ __volatile__ ("movw %%ss, %0;" :"=rm"(ss));
    return ss;
}

inline u16 __read_es(void)
{
    u16 es = 0;
    __asm__ __volatile__ ("movw %%es, %0;" :"=rm"(es));
    return es;
}

inline u16 __read_fs(void)
{
    u16 fs = 0;
    __asm__ __volatile__ ("movw %%fs, %0;" :"=rm"(fs));
    return fs;
}

inline u16 __read_gs(void)
{
    u16 gs = 0;
    __asm__ __volatile__ ("movw %%gs, %0;" :"=rm"(gs));
    return gs;
}

inline u16 __str(void)
{
    u16 tr = 0;
    __asm__ __volatile__ ("str %0;" :"=rm"(tr));
    return tr;
}

inline u16 __sldt(void)
{
    u16 ldt = 0;
    __asm__ __volatile__ ("sldt %0;" :"=rm"(ldt));
    return ldt;
}

inline struct __descriptor_table __sgdt(void)
{
    struct __descriptor_table gdt = {0};
    __asm__ __volatile__ ("sgdt %0;" :"=m"(gdt)::"memory");
    return gdt;
    
}

inline struct __descriptor_table __sidt(void)
{
    struct __descriptor_table idt = {0};
    __asm__ __volatile__ ("sidt %0;" :"=m"(idt)::"memory");
    return idt;
}

inline bool __lar(u16 segment, u32 *outp)
{
    u8 ret = 0;

    __asm__ __volatile__ (
        "lar %[segment], %[outp];"
        "sete %[ret];"
        :[outp]"=r"(*outp), [ret]"=r"(ret)
        :[segment]"r"(segment)
        :"cc"
    );

    return !!ret;
}

inline bool __lsl(u16 segment, u32 *outp)
{
    u8 ret = 0;

    __asm__ __volatile__ (
        "lsl %[segment], %[outp];"
        "sete %[ret];"
        :[outp]"=r"(*outp), [ret]"=r"(ret)
        :[segment]"r"(segment)
        :"cc"
    );

    return !!ret;
}

inline u64 __read_rflags(void)
{
    u64 rflags = 0;
    __asm__ __volatile__ (
        "pushfq; popq %0;" :"=r"(rflags)
    );

    return rflags;
}

inline u64 __read_dr7(void)
{
    u64 dr7 = 0;
    __asm__ __volatile__ (
        "movq %%dr7, %0;":"=r"(dr7)
    );

    return dr7;
}

#include "include/debug.h"

/* vmcs ops */
bool guest_rip_add(u64 length)
{
    u64 rip = 0;
    bool ret = __vmread(VMCS_GUEST_RIP, &rip);
    return ret ? __vmwrite(VMCS_GUEST_RIP, rip + length) : ret;
}

bool guest_rip_next(void)
{
    u64 len = 0;
    bool ret = __vmread(VMCS_RO_VMEXIT_INSTRUCTION_LENGTH, &len);
    return ret ? guest_rip_add(len) : ret;
}