#subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
#subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
#subdir-ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6755
#subdir-ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
#subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/

subdir-ccflags-y += -DCFG_VEB_MANAGE_INFO_ERASE
#subdir-ccflags-y += -DCFG_PLATFORM_MTK
subdir-ccflags-y += -Werror

#subdir-ccflags-y += -DCFG_MTK6572

subdir-ccflags-y += -DCFG_VEB_OF
subdir-ccflags-y += -DCFG_VEB_DEBUG
subdir-ccflags-y += -DCONFIG_SMARTCARD
ifeq ($(ZTE_CLOSE_VEB_A5),true)
ccflags-y += -DZTE_CLOSE_VEB_A5
endif
subdir-ccflags-$(CONFIG_VENDOR_ZTE_DEV_MONITOR_SYSTEM) += -I$(srctree)/drivers/vendor/common/zlog/zlog_common

obj-m += veb_a5spi.o
veb_a5spi-y := veb_spi.o
veb_a5spi-y += veb_platform.o
veb_a5spi-y += veb_base.o
veb_a5spi-y += veb_crc.o
veb_a5spi-y += veb_sha.o
veb_a5spi-y += veb_smartcard.o

veb_a5spi-y += veb_a5_upgrade.o
#veb_a5spi-y += veb_a5_upgrade_smart.o
BOARD_VENDOR_KERNEL_MODULES +=  $(KERNEL_MODULES_OUT)/veb_a5spi.ko