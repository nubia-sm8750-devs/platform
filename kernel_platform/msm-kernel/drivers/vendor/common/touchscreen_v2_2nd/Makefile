#
# Makefile for zte touchscreen_v2 driver.
#
subdir-ccflags-y += -I$(srctree)/drivers/vendor/common/zlog/zlog_common
ccflags-y += -I$(srctree)/drivers/vendor/common/touchscreen_v2_2nd/

#zte_tpd-y := ztp_core.o ztp_report_algo.o ztp_state_change.o
#zte_tpd-$(CONFIG_TOUCHSCREEN_UFP_MAC) +=  ztp_ufp.o

zte_tpd_2nd-y := ztp_core_2nd.o ztp_report_algo_2nd.o ztp_state_change_2nd.o
zte_tpd_2nd-$(CONFIG_TOUCHSCREEN_UFP_MAC_2nd) +=  ztp_ufp_2nd.o

######################################################
zte_tpd_2nd-$(CONFIG_TOUCHSCREEN_FTS_3383) += \
	focaltech_touch_3383/focaltech_core.o \
	focaltech_touch_3383/focaltech_esdcheck.o \
	focaltech_touch_3383/focaltech_ex_fun.o \
	focaltech_touch_3383/focaltech_ex_mode.o \
	focaltech_touch_3383/focaltech_flash.o \
	focaltech_touch_3383/focaltech_flash/focaltech_upgrade_ft3383.o \
	focaltech_touch_3383/focaltech_gesture.o \
	focaltech_touch_3383/focaltech_i2c.o \
	focaltech_touch_3383/focaltech_point_report_check.o \
	focaltech_touch_3383/focaltech_proximity.o \
	focaltech_touch_3383/focaltech_test/focaltech_test.o \
	focaltech_touch_3383/focaltech_test/focaltech_test_ini.o \
	focaltech_touch_3383/focaltech_test/supported_ic/focaltech_test_ft3383.o \
    focaltech_touch_3383/focaltech_common_interface.o \
######################################################

obj-m += zte_tpd_2nd.o
