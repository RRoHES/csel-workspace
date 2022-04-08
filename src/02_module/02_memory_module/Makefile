export PATH := /buildroot/output/host/usr/sbin$\
	:/buildroot/output/host/usr/bin/$\
	:/buildroot/output/host/sbin$\
	:/buildroot/output/host/bin/$\
	:$(PATH)

# Part executed when called from kernel build system:
ifneq ($(KERNELRELEASE),)
obj-m += memory_module.o		## name of the generated module

memory_module-objs := memory.o	## list of objects needed for that module
CFLAGS_skeleton.o := -DDEBUG

# Part executed when called from standard make in module source directory:
else
CVER     := aarch64-buildroot-linux-gnu-
KVER     := 5.15.21
CPU      := arm64

KDIR     := /buildroot/output/build/linux-$(KVER)/
TOOLS    := /buildroot/output/host/usr/bin/$(CVER)
MODPATH  := /rootfs

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(CPU) CROSS_COMPILE=$(TOOLS) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install

endif