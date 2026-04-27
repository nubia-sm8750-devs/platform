/*
 *  vi530x_module.c - Linux kernel modules for VI530X FlightSense TOF
 *						 sensor
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#ifndef VI530X_H
#define VI530X_H

#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/time.h>
#include <linux/timekeeping.h>
#include "vi530x_def.h"
#include <linux/regulator/driver.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_platform.h>


#define INPUT_TOF              ABS_HAT0X
#define INPUT_CONFIDENCE       ABS_HAT0Y
#define INPUT_PEAK             ABS_HAT1X
#define INPUT_NOISE            ABS_HAT1Y
#define INPUT_INTEGRAL_TIMES   ABS_HAT2X

#define VI530X_CHIP_ADDR 0xD8

#define VI530X_REG_MCU_CFG 0x00
#define VI530X_REG_SYS_CFG 0x01
#define VI530X_REG_DEV_STAT 0x02
#define VI530X_REG_INTR_STAT 0x03
#define VI530X_REG_INTR_MASK 0x04
#define VI530X_REG_I2C_IDLE_TIME 0x05
#define VI530X_REG_DEV_ADDR 0x06
#define VI530X_REG_PW_CTRL 0x07
#define VI530X_REG_SPCIAL_PURP 0x08
#define VI530X_REG_CMD 0x0A
#define VI530X_REG_SIZE 0x0B
#define VI530X_REG_SCRATCH_PAD_BASE 0x0C
#define VI530X_REG_CHIPID_BASE 0x2C
#define VI530X_REG_RCO_AO 0x37
#define VI530X_REG_DIGLDO_VREF 0x38
#define VI530X_REG_PLLLDO_VREF 0x39
#define VI530X_REG_ANALDO_VREF 0x3A
#define VI530X_REG_PD_RESET 0x3B
#define VI530X_REG_I2C_STOP_DELAY 0x3C
#define VI530X_REG_TRIM_MODE 0x3D
#define VI530X_REG_GPIO_SINGLE 0x3E
#define VI530X_REG_ANA_TEST_SINGLE 0x3F

#define VI530X_WRITEFW_CMD 0x03
#define VI530X_USER_CFG_CMD 0x09
#define VI530X_XTALK_TRIM_CMD 0x0D
#define VI530X_SINGLE_RANGE_CMD 0x0E
#define VI530X_CONTINOUS_RANGE_CMD 0x0F
#define VI530X_STOP_RANGE_CMD 0x01F

#define VI530X_OTPW_SUBCMD 0x02
#define VI530X_OTPR_SUBCMD 0x03
#define VI530X_MAX_WAIT_RETRY 5
#define DEFAULT_INTEGRAL_COUNTS 131072
#define DEFAULT_FRAME_COUNTS 30

#define VI530X_ERROR_NONE ((VI530X_Error) 0)
#define VI530X_ERROR_CPU_BUSY ((VI530X_Error) -1)
#define VI530X_ERROR_ENABLE_INTR ((VI530X_Error) -2)
#define VI530X_ERROR_XTALK_CALIB ((VI530X_Error) -3)
#define VI530X_ERROR_OFFSET_CALIB ((VI530X_Error) -4)
#define VI530X_ERROR_XTALK_CONFIG ((VI530X_Error) -5)
#define VI530X_ERROR_SINGLE_CMD ((VI530X_Error) -6)
#define VI530X_ERROR_CONTINUOUS_CMD ((VI530X_Error) -7)
#define VI530X_ERROR_GET_DATA ((VI530X_Error) -8)
#define VI530X_ERROR_STOP_CMD ((VI530X_Error) -9)
#define VI530X_ERROR_IRQ_STATE ((VI530X_Error) -10)
#define VI530X_ERROR_FW_FAILURE ((VI530X_Error) -11)
#define VI530X_ERROR_POWER_ON ((VI530X_Error) -12)
#define VI530X_ERROR_POWER_OFF ((VI530X_Error) -13)
#define VI530X_ERROR_OTP_SIZE ((VI530X_Error) -14)

struct timeval{
	time64_t 	tv_sec;		//Time in sec
	long		tv_usec;	//Time in micro seconds
};

struct timespec{
	/** Number of seconds. */
	long int tv_sec;
	/** Number of nanoseconds. Must be less than 1,000,000,000 . */
	long tv_nsec;
};
struct VI530X_Measurement_Data {
	int16_t tof;
	uint8_t confidence;
	uint32_t peak;
	uint32_t noise;
	uint32_t integral_times;
	uint8_t flag;
};

struct VI530X_XTALK_Calib_Data {
	int8_t xtalk_cal;
	uint16_t xtalk_peak;
	uint8_t maxratio;
};

struct VI530X_OFFSET_Calib_Data {
	int16_t offset_cal;
	int16_t offset_mili;
};

struct VI530X_XTALK_Config_Data {
	int8_t xtalk_config;
	uint8_t maxratio;
};

struct vi530x_data {
 	struct device *dev;
 	const char *dev_name;
	struct VI530X_Measurement_Data Rangedata;
	struct VI530X_XTALK_Calib_Data XtalkData;
	struct VI530X_OFFSET_Calib_Data OffsetData;
	struct VI530X_XTALK_Config_Data XtalkConfig;
 	struct mutex update_lock;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct miscdevice miscdev;
	struct timespec start_ts;
	int irq_gpio;
	int xshut_gpio;
	int pwren_gpio;
	int irq;
	uint32_t chip_enable;
	uint32_t enable_debug;
	uint8_t intr_state;
	uint8_t fwdl_status;
	int8_t xtalk_config;
	int16_t offset_config;
	const char *program_version;
	uint32_t integral_counts;
	uint32_t period;
	uint8_t ma_sum;
	struct mutex work_mutex;
	bool   pm_ctrl_client_enable;
#ifdef CONFIG_TOF_VDIG_SUPPLY
    struct regulator *power;
#endif
	struct miscdevice  subtofdev;
};

union inte_data {
	uint32_t intecnts;
	uint8_t buf[4];
};

enum VI530X_INT_STATUS {
	VI530X_INTR_DISABLED = 0,
	VI530X_INTR_ENABLED,
};

typedef struct VI530X_TOF
{
    struct  vi530x_data *tof_dev;
    struct  task_struct *tof_thread;
    struct  semaphore   tofsem;
    VI530X_Error        Status;
}VI530X_TOF;

#define DEBUG
#define vi530x_infomsg(str, args...) \
	 pr_info("%s: " str, __func__, ##args)
#define vi530x_dbgmsg(str, args...) \
	 pr_debug("%s: " str, __func__, ##args)
#define vi530x_errmsg(str, args...) \
	 pr_err("%s: " str, __func__, ##args)

#endif
