#ifndef _VMEXIT_DISPATCHER_H_
#define _VMEXIT_DISPATCHER_H_

#include "vmexit_handlers.h"

extern struct vmexit_handler vmexit_dispatch_table[];

bool vmexit_dispatcher(struct vcpu_ctx *ctx, struct regs *guest_regs);

#endif