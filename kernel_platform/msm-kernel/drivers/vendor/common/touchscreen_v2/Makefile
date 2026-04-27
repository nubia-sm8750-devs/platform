#
# Makefile for zte touchscreen_v2 driver.
#
subdir-ccflags-y += -I$(srctree)/drivers/vendor/common/zlog/zlog_common
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/goodix_berlin_driver_v2/firmware_config/$(CONFIG_TOUCHSCREEN_BOARD_NAME)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/focaltech_touch_3681/focaltech_test/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/focaltech_touch_3681/firmware_config/$(CONFIG_TOUCHSCREEN_BOARD_NAME)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/goodix_berlin_driver_9916r/firmware_config/$(CONFIG_TOUCHSCREEN_BOARD_NAME)/
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2/goodix_berlin_driver_9966/firmware_config/$(CONFIG_TOUCHSCREEN_BOARD_NAME)/

zte_tpd-y := ztp_core.o ztp_report_algo.o ztp_state_change.o
zte_tpd-$(CONFIG_TOUCHSCREEN_UFP_MAC) +=  ztp_ufp.o

######################################################
ifneq ($(filter y, $(CONFIG_TOUCHSCREEN_GOODIX_BRL_V2)),)
	ifneq ($(filter y, $(CONFIG_TOUCHSCREEN_GOODIX_BRL_I2C)),)
		zte_tpd-y += goodix_berlin_driver_v2/goodix_brl_i2c.o
	else
		zte_tpd-y += goodix_berlin_driver_v2/goodix_brl_spi.o
	endif
endif
zte_tpd-$(CONFIG_TOUCHSCREEN_GOODIX_BRL_V2) += \
	goodix_berlin_driver_v2/goodix_ts_core.o \
	goodix_berlin_driver_v2/goodix_brl_hw.o \
	goodix_berlin_driver_v2/goodix_cfg_bin.o \
	goodix_berlin_driver_v2/goodix_ts_utils.o \
	goodix_berlin_driver_v2/goodix_brl_fwupdate.o \
	goodix_berlin_driver_v2/goodix_ts_gesture.o \
	goodix_berlin_driver_v2/goodix_ts_inspect.o \
	goodix_berlin_driver_v2/goodix_ts_tools.o \
	goodix_berlin_driver_v2/goodix_common_interface.o

######################################################
ifneq ($(filter y, $(CONFIG_TOUCHSCREEN_GOODIX_BRL_9916R)),)
	ifneq ($(filter y, $(CONFIG_TOUCHSCREEN_GOODIX_BRL_I2C)),)
		zte_tpd-y += goodix_berlin_driver_9916r/goodix_brl_i2c.o
	else
		zte_tpd-y += goodix_berlin_driver_9916r/goodix_brl_spi.o
	endif
endif
zte_tpd-$(CONFIG_TOUCHSCREEN_GOODIX_BRL_9916R) += \
	goodix_berlin_driver_9916r/goodix_brl_fwupdate.o \
	goodix_berlin_driver_9916r/goodix_brl_hw.o \
	goodix_berlin_driver_9916r/goodix_cfg_bin.o \
	goodix_berlin_driver_9916r/goodix_ts_core.o \
	goodix_berlin_driver_9916r/goodix_ts_gesture.o \
	goodix_berlin_driver_9916r/goodix_ts_inspect.o \
	goodix_berlin_driver_9916r/goodix_ts_tools.o \
	goodix_berlin_driver_9916r/goodix_ts_utils.o \
	goodix_berlin_driver_9916r/goodix_common_interface.o		

######################################################
ifneq ($(filter y, $(CONFIG_TOUCHSCREEN_GOODIX_BRL_9966)),)
	ifneq ($(filter y, $(CONFIG_TOUCHSCREEN_GOODIX_BRL_I2C)),)
		zte_tpd-y += goodix_berlin_driver_9966/goodix_brl_i2c.o
	else
		zte_tpd-y += goodix_berlin_driver_9966/goodix_brl_spi.o
	endif
endif
zte_tpd-$(CONFIG_TOUCHSCREEN_GOODIX_BRL_9966) += \
	goodix_berlin_driver_9966/goodix_brl_fwupdate.o \
	goodix_berlin_driver_9966/goodix_brl_hw.o \
	goodix_berlin_driver_9966/goodix_cfg_bin.o \
	goodix_berlin_driver_9966/goodix_ts_core.o \
	goodix_berlin_driver_9966/goodix_ts_gesture.o \
	goodix_berlin_driver_9966/goodix_ts_inspect.o \
	goodix_berlin_driver_9966/goodix_ts_tools.o \
	goodix_berlin_driver_9966/goodix_ts_utils.o \
	goodix_berlin_driver_9966/goodix_common_interface.o

######################################################

zte_tpd-$(CONFIG_TOUCHSCREEN_FTS_3681) += \
	focaltech_touch_3681/focaltech_core.o \
	focaltech_touch_3681/focaltech_ex_fun.o \
	focaltech_touch_3681/focaltech_ex_mode.o \
	focaltech_touch_3681/focaltech_gesture.o \
	focaltech_touch_3681/focaltech_esdcheck.o \
	focaltech_touch_3681/focaltech_point_report_check.o \
	focaltech_touch_3681/focaltech_flash.o \
	focaltech_touch_3681/focaltech_common_interface.o \
	focaltech_touch_3681/focaltech_i2c.o \
	focaltech_touch_3681/focaltech_test/focaltech_test.o \
	focaltech_touch_3681/focaltech_test/focaltech_test_ini.o \
	focaltech_touch_3681/focaltech_flash/focaltech_upgrade_ft3681.o \
	focaltech_touch_3681/focaltech_test/supported_ic/focaltech_test_ft3681.o \

######################################################

zte_tpd-$(CONFIG_TOUCHSCREEN_NT36XXX_TOUCH_36532W) += \
	nt36xxx_touch_36532w/nt36xxx.o \
	nt36xxx_touch_36532w/nt36xxx_fw_update.o \
	nt36xxx_touch_36532w/nt36xxx_ext_proc.o \
	nt36xxx_touch_36532w/nt36xxx_mp_ctrlram.o \

######################################################

zte_tpd-$(CONFIG_TOUCHSCREEN_GOODIX_BRL_THP) += \
	goodix_berlin_driver_THP/goodix_thp.o \
	goodix_berlin_driver_THP/goodix_thp_spi.o \
	goodix_berlin_driver_THP/goodix_common_interface.o \

######################################################

zte_tpd-$(CONFIG_TOUCHSCREEN_GOODIX_BRL_FOLD_THP) += \
	goodix_berlin_driver_FOLD_THP/goodix_thp.o \
	goodix_berlin_driver_FOLD_THP/goodix_thp_spi.o \
	goodix_berlin_driver_FOLD_THP/goodix_common_interface.o \

######################################################

zte_tpd-$(CONFIG_TOUCHSCREEN_FTS_3683G) += \
	focaltech_touch_3683g/focaltech_core.o \
	focaltech_touch_3683g/focaltech_ex_mode.o \
	focaltech_touch_3683g/focaltech_gesture.o \
	focaltech_touch_3683g/focaltech_esdcheck.o \
	focaltech_touch_3683g/focaltech_point_report_check.o \
	focaltech_touch_3683g/focaltech_flash.o \
	focaltech_touch_3683g/focaltech_spi_v2.o \
	focaltech_touch_3683g/focaltech_proximity.o \
	focaltech_touch_3683g/focaltech_common_interface.o \
	focaltech_touch_3683g/focaltech_test/focaltech_test.o \
	focaltech_touch_3683g/focaltech_test/focaltech_test_ini.o \
	focaltech_touch_3683g/focaltech_flash/focaltech_upgrade_ft3683g.o \
	focaltech_touch_3683g/focaltech_test/supported_ic/focaltech_test_ft3683g.o \
	focaltech_touch_3683g/focaltech_ex_fun.o \
######################################################

zte_tpd-$(CONFIG_TOUCHSCREEN_SYNA_TCM2_S3930T) += \
	synaptics_touch_tcm2_THP/syna_tcm2.o \
	synaptics_touch_tcm2_THP/syna_tcm2_cdev.o \
	synaptics_touch_tcm2_THP/syna_tcm2_platform_spi.o \
	synaptics_touch_tcm2_THP/syna_tcm2_sysfs.o \
	synaptics_touch_tcm2_THP/syna_tcm2_common_interface.o \
	synaptics_touch_tcm2_THP/tcm/synaptics_touchcom_func_reflash.o \
	synaptics_touch_tcm2_THP/tcm/synaptics_touchcom_core_v2.o \
	synaptics_touch_tcm2_THP/tcm/synaptics_touchcom_func_base.o \
	synaptics_touch_tcm2_THP/tcm/synaptics_touchcom_func_touch.o \
	synaptics_touch_tcm2_THP/tcm/synaptics_touchcom_image_parsing.o \
	synaptics_touch_tcm2_THP/syna_tcm2_testing.o \
	synaptics_touch_tcm2_THP/testing/synaptics_touchcom_testing_0A00_noise.o \
	synaptics_touch_tcm2_THP/testing/synaptics_touchcom_testing_0001_build_id.o \
	synaptics_touch_tcm2_THP/testing/synaptics_touchcom_testing_0002_config_id.o \
	synaptics_touch_tcm2_THP/testing/synaptics_touchcom_testing_0100_trxtrx_short.o \
	synaptics_touch_tcm2_THP/testing/synaptics_touchcom_testing_0500_full_raw.o
#	synaptics_touch_tcm2_THP/tcm/synaptics_touchcom_core_v1.o \

######################################################

zte_tpd-$(CONFIG_TOUCHSCREEN_SYNA_TCM2_S3930T_LBP) += \
	synaptics_tcm2/syna_tcm2.o \
	synaptics_tcm2/syna_tcm2_cdev.o \
	synaptics_tcm2/syna_tcm2_platform_spi.o \
	synaptics_tcm2/syna_tcm2_sysfs.o \
	synaptics_tcm2/tcm/synaptics_touchcom_func_reflash.o \
	synaptics_tcm2/tcm/synaptics_touchcom_core_v2.o \
	synaptics_tcm2/tcm/synaptics_touchcom_func_base.o \
	synaptics_tcm2/tcm/synaptics_touchcom_func_touch.o \
	synaptics_tcm2/tcm/synaptics_touchcom_image_parsing.o
#	synaptics_tcm2/syna_tcm2_testing.o \

######################################################

zte_tpd-$(CONFIG_TOUCHSCREEN_HIMAX_HX83122A_LBP) += \
	hxchipset/himax_ic_HX85200a.o \
	hxchipset/himax_inspection.o \
	hxchipset/himax_debug.o \
	hxchipset/himax_ic_core.o \
	hxchipset/himax_common.o \
	hxchipset/himax_platform.o

######################################################

obj-m += zte_tpd.o
