KERNEL_SRC := /lib/modules/$(shell uname -r)/build
SRC_DIR := .

LIB_DIR := lib
OBJ_FILES := $(LIB_DIR)/arch.o $(LIB_DIR)/setup_vmx.o $(LIB_DIR)/vmxon.o \
	$(LIB_DIR)/vmcs.o $(LIB_DIR)/vcpu.o $(LIB_DIR)/segmentation.o \
	$(LIB_DIR)/vmcs_err.o $(LIB_DIR)/vmexit_dispatcher.o \
	$(LIB_DIR)/__entry_points.o

SRC_OBJ := driver.o 

ALL_OBJS := $(SRC_OBJ) $(OBJ_FILES)
ccflags-y := -I$(SRC_DIR) -I$(LIB_DIR)/include

MODULE_NAME := hypervisor
obj-m := $(MODULE_NAME).o
$(MODULE_NAME)-objs := $(ALL_OBJS)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
	rm -f $(ALL_OBJS)

.PHONY: all clean
