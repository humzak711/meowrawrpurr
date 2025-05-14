#include <linux/module.h>

#include "lib/include/arch.h"
#include "lib/include/debug.h"
#include "lib/include/vcpu.h"
#include "lib/include/vmxon.h"

#include <linux/smp.h>

MODULE_AUTHOR("humzak711");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("shitty bluepill");
MODULE_LICENSE("GPL");

extern bool __virtualise_core_entry(u32 cpu_id);

#include <linux/kthread.h>

struct rawr
{
    struct hv *hv_global;
    struct task_struct *init_thread;
    struct mutex global_lock;
};

struct rawr rawr_global = {
    .hv_global = &hv_global,
    .init_thread = NULL,
    .global_lock = __MUTEX_INITIALIZER(rawr_global.global_lock),
};

static void virtualise(void *rawr_global)
{
    u32 this_cpu_id = smp_processor_id();

    bool virtualised = __virtualise_core_entry(this_cpu_id);
    if (virtualised) 
        HV_LOG(KERN_DEBUG, "core virtualised: %u", this_cpu_id);
    else 
        HV_LOG(KERN_DEBUG, "failed to virtualise core: %u", this_cpu_id);
}

static int bluepill(void *rawr_global)
{
    on_each_cpu(virtualise, rawr_global, 1);
    return 0;
}

static int __init driver_entry(void)
{
    HV_LOG(KERN_DEBUG, "driver loaded");

    rawr_global.init_thread = 
        kthread_run(bluepill, &rawr_global, "meowmeow");

    if (!rawr_global.init_thread) {
        HV_LOG(KERN_DEBUG, "failed to create thread");
        return -EFAULT;
    }

    HV_LOG(KERN_DEBUG, "thread created");
    return 0;   
}

//todo: proper cleanup
static void __exit driver_exit(void)
{
    HV_LOG(KERN_DEBUG, "driver unloaded");
}

module_init(driver_entry);
module_exit(driver_exit);