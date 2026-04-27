/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _AW86320_H_
#define _AW86320_H_

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <linux/mutex.h>
#include <linux/input.h>
#include <linux/version.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>

#define AW_VIM3
#define AW_REG_CHIPID				(0x00)
#define AW86230_CHIPID				(0x6027)


#define AW_I2C_NAME				"aw86320"
#define AW_I2C_RETRIES				(5)
#define AW_READ_CHIPID_RETRIES		(1) //	(5)
#define AW_SEQUENCER_SIZE			(8)
#define AW_I2C_READ_MSG_NUM			(2)
#define AW_RAM_WORK_DELAY_INTERVAL		(8000)
#define AW_RAMDATA_RD_BUFFER_SIZE		(1024)
#define AW_CHECK_RAM_DATA
#define AW86320_DRIVER_ENABLE
#define AW_CHECK_RAM_DATA
//#define AW_ATSIN0_AMPLITUDE_TO_CODE	(640 * 108 * 100) /*(0~102.4V) = (0~0XFFFF)*/
#define AW_ATSIN0_AMPLITUDE_TO_CODE(code)		(((code) * 32767 * 108) / (1024 * 10)) /*(0~102.4V) = (0~0XFFFF)*/

#if KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE
#define KERNEL_OVER_5_10
#endif

#if KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE
#define KERNEL_OVER_6_1
#endif

#include <linux/leds.h>
typedef struct led_classdev cdev_t;

/*********************************************************
 *
 * Log Format
 *
 *********************************************************/
#define aw_err(format, ...) \
	pr_err("[%s][%04d]%s: " format "\n", AW_I2C_NAME, __LINE__, __func__, ##__VA_ARGS__)

#define aw_info(format, ...) \
	pr_info("[%s][%04d]%s: " format "\n", AW_I2C_NAME, __LINE__, __func__, ##__VA_ARGS__)

#define aw_dbg(format, ...) \
	pr_debug("[%s][%04d]%s: " format "\n", AW_I2C_NAME, __LINE__, __func__, ##__VA_ARGS__)


/*********************************************************
 *
 * Enum Define
 *
 *********************************************************/
enum aw_haptic_work_mode {
	AW_STANDBY_MODE,
	AW_ATSIN0_MODE,
	AW_ATSIN1_MODE,
};

enum aw_haptic_play_mode {
	AW_NONE_PLAY_TYPE = 0,
	AW_REG_PLAY_TYPE = 1,
	AW_FIFO_PLAY_TYPE  = 2,
};

enum aw_haptic_pwm_mode {
	AW_PWM_12K = 0,
	AW_PWM_24K = 1,
	AW_PWM_48K = 2,
};

enum wavseq_data_type {
	WAVSEQ_TPYE_U8 = 1,
	WAVSEQ_TPYE_U16 = 2,
	WAVSEQ_TPYE_SLICE = 3
};

/*********************************************************
 *
 * Struct Define
 *
 *********************************************************/
struct bin {
	uint16_t ram_base_addr;
	uint16_t fifo_ae_addr;
	uint16_t fifo_af_addr;
	uint16_t ram_u8data_size;
	uint8_t wavseq_cnt;
	uint8_t wavseq_type[10]; //max 256.Increase according to actual situation.
};

struct aw_haptic_ram {
	uint32_t len;
	uint32_t check_sum;
	uint16_t base_addr;
	uint8_t ram_num;
	uint8_t version;
	uint8_t ram_shift;
	uint8_t baseaddr_shift;
};

struct atsin1 {
	uint32_t duration;
	uint32_t start_a0;
	uint32_t end_a1;
	uint32_t start_f0;
	uint32_t end_f1;
};


struct aw_haptic_dts_info {
	uint8_t mode;
	uint8_t play_mode[4];
	bool use_fifo_method_play;
	bool use_reg_method_play;
	uint32_t atsin0_polor;
	uint32_t atsin0_phase;
};

struct aw_haptic {
	bool ram_init;
	uint8_t play_type;
	uint16_t chipid;
	int irq_gpio;
	int amplitude;
	int duration;
	//int state; //enabled

	int enabled;
    //int speed; amplitude
	uint16_t freq;
	int power_en;

	cdev_t vib_dev;
	struct device *dev;
	struct i2c_client *i2c;
	struct mutex lock;
	struct hrtimer timer;
	struct delayed_work ram_work;
	struct delayed_work time_check_work;
	struct aw_haptic_ram ram;
	struct aw_haptic_dts_info info;
	struct bin ram_data;
	struct atsin1 atsin1;
};

struct aw_haptic_container {
	int len;
	uint8_t data[];
};

#endif
