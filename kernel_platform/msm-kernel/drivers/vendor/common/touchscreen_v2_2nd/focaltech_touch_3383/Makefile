# Makefile for the focaltech touchscreen drivers.



obj-$(CONFIG_TOUCHSCREEN_FOCALTECH_TOUCH_V4_2_2nd) += focaltech_tp.o
focaltech_tp-y := focaltech_core.o \
		focaltech_ex_fun.o \
		focaltech_ex_mode.o \
		focaltech_gesture.o \
		focaltech_esdcheck.o \
		focaltech_point_report_check.o \
		focaltech_proximity.o



focaltech_tp-y += focaltech_i2c.o






focaltech_tp-y += focaltech_flash.o
focaltech_tp-y += focaltech_flash/focaltech_upgrade_ft3383.o
focaltech_tp-y += focaltech_test/focaltech_test.o
focaltech_tp-y += focaltech_test/focaltech_test_ini.o
focaltech_tp-y += focaltech_test/supported_ic/focaltech_test_ft3383.o
