ifeq ($(ZTE_FEATURE_SECONDARY_ENABLE),true)
  ccflags-y += -DZTE_FEATURE_SECONDARY_ENABLE
endif

obj-m += ah1898_hall2.o

ah1898_hall2-objs := ah1898.o

BOARD_VENDOR_KERNEL_MODULES +=  $(KERNEL_MODULES_OUT)/ah1898_hall2.ko