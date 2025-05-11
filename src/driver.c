#include <linux/module.h>

#include "lib/include/arch.h"
#include "lib/include/debug.h"
#include "lib/include/vcpu.h"
#include "lib/include/vmxon.h"

#include <linux/smp.h>
#include <linux/kthread.h>

MODULE_AUTHOR("humzak711");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("Cachekill bluepill");
MODULE_LICENSE("GPL");

#include <linux/fs.h>

extern bool __virtualise_core_entry(u32 cpu_id);

static int __init driver_entry(void)
{
    HV_LOG(KERN_DEBUG, "driver loaded");

    u32 this_cpu_id = smp_processor_id();

    bool virtualised = __virtualise_core_entry(this_cpu_id);
    if (virtualised) 
        HV_LOG(KERN_DEBUG, "core virtualised: %u", this_cpu_id);
    
    else 
        HV_LOG(KERN_DEBUG, "failed to virtualise core: %u", this_cpu_id);

    return 0;   
}

static void __exit driver_exit(void)
{
    HV_LOG(KERN_DEBUG, "driver unloaded");
}

module_init(driver_entry);
module_exit(driver_exit);