#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <linux/kernel.h>

#define ON 1
#define OFF 0

#define LOGGING ON
#define DEBUG_MODE ON

#if LOGGING == ON

    #define HV_LOG(level, fmt, ...)                                        \
        printk(level "hypervisor (%s): " fmt "\n", __FUNCTION__,           \
               ##__VA_ARGS__)

#else

    #define HV_LOG(level, fmt, ...)

#endif

#endif