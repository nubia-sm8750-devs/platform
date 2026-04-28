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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>


#include "vi530x.h"
#include "vi530x_platform.h"
#include "vi530x_firmware.h"
#include "vi530x_api.h"

#define STATUS_TOF_CONFIDENT 1
#define STATUS_TOF_SEMI_CONFIDENT 2
#define STATUS_TOF_NOT_CONFIDENT 0
#define STATUS_TOF_NO_OBJECT 3

static inline void getnstimeofday(struct timespec *tv)
{
	struct timespec64 now;

	ktime_get_real_ts64(&now);
	tv->tv_sec = now.tv_sec;
	tv->tv_nsec = now.tv_nsec/1000;
}

struct timespec timespec_add_ns(struct timespec ts, int64_t ns)
{
	struct timespec res;
	res.tv_nsec = ts.tv_nsec + ns;
	res.tv_sec = ts.tv_sec + res.tv_nsec / 1000000000LL;
	res.tv_nsec %= 1000000000LL;
	return res;
}

int64_t timespec_to_ns(struct timespec ts)
{
	return (int64_t)ts.tv_nsec + 1000000000LL * (int64_t)ts.tv_sec;
}

 struct timespec timespec_sub(struct timespec ts1, struct timespec ts2)
{
	int64_t ns1 = timespec_to_ns(ts1);
  	int64_t ns2 = timespec_to_ns(ts2);
  	return timespec_add_ns((struct timespec){0}, ns1 - ns2);
}
VI530X_Error VI530X_Chip_PowerON(VI530X_DEV dev)
{
    VI530X_Error Status = VI530X_ERROR_NONE;

#ifdef CONFIG_TOF_VDIG_SUPPLY
    Status=regulator_set_voltage(dev->power, 3000000, 3000000);//mV
    Status=regulator_enable(dev->power);
	Status = gpio_direction_output(dev->xshut_gpio, 0);
	mdelay(5);
	Status = gpio_direction_output(dev->xshut_gpio, 1);
    vi530x_errmsg("vi530x tof vdig power on regulator");

#else
	Status = gpio_direction_output(dev->pwren_gpio, 0);
	Status = gpio_direction_output(dev->xshut_gpio, 0);

	Status = gpio_direction_output(dev->pwren_gpio, 1);
	mdelay(5);
	Status = gpio_direction_output(dev->xshut_gpio, 1);
	mdelay(5);
    vi530x_errmsg("vi530x tof vdig power on gpio");

#endif

	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("Chip Power ON Failed Status = %d\n", Status);
		return VI530X_ERROR_POWER_ON;
	}

	return Status;
}

VI530X_Error VI530X_Chip_PowerOFF(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;

	Status = gpio_direction_output(dev->xshut_gpio, 0);
	mdelay(1);
#ifdef CONFIG_TOF_VDIG_SUPPLY
    Status = regulator_disable(dev->power);
    vi530x_errmsg("vi530x tof vdig power off regulator");
#else
    Status = gpio_direction_output(dev->pwren_gpio, 0);
    vi530x_errmsg("vi530x tof vdig power off gpio");
#endif

	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("Chip Power OFF Failed Status = %d\n", Status);
		return VI530X_ERROR_POWER_OFF;
	}

	return Status;
}

VI530X_Error VI530X_Wait_For_CPU_Ready(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;
	uint8_t stat;
	int retry = 0;

	do {
		mdelay(1);
/* Started by AICoder, pid:3ef26x04eem7f6f143ea0be5e038a703ebd610e2 */
		Status = vi530x_read_byte(dev, VI530X_REG_DEV_STAT, &stat);
		if(Status == -22)
		{
			vi530x_errmsg("i2c_transfer has some problems! Status = %d\n",Status);
			return Status;
		}
/* Ended by AICoder, pid:3ef26x04eem7f6f143ea0be5e038a703ebd610e2 */
	}while((retry++ < VI530X_MAX_WAIT_RETRY)
		&&(stat & 0x01));
	if(retry >= VI530X_MAX_WAIT_RETRY)
	{
		vi530x_errmsg("CPU Busy stat = %d\n", stat);
		return VI530X_ERROR_CPU_BUSY;
	}

	return Status;
}

void VI530X_Read_ChipVersion(VI530X_DEV dev)
{
	uint8_t chipid[3] = {0};
	uint32_t ChipVersion = 0;

	vi530x_read_multibytes(dev, VI530X_REG_CHIPID_BASE, chipid, 3);
	ChipVersion = (chipid[1] << 16) + (chipid[0] << 8) + chipid[2];
	vi530x_errmsg("VI530X ChipVersion: 0x%x\n", ChipVersion);
}

VI530X_Error VI530X_Init_FirmWare(VI530X_DEV dev)
{
	uint8_t  sys_cfg_data = 0;
	uint16_t fw_size = 0;
	uint16_t fw_send = 0;
	uint8_t val;
	VI530X_Error Status = VI530X_ERROR_NONE;

	fw_size = LoadFirmware(dev);
	if(!fw_size)
	{
		vi530x_errmsg("Firmware Load Failed!\n");
		return VI530X_ERROR_FW_FAILURE;
	}
	vi530x_errmsg("Firmware Load begin!!!\n");
	vi530x_write_byte(dev, VI530X_REG_PW_CTRL, 0x08);
	vi530x_write_byte(dev, VI530X_REG_PW_CTRL, 0x0a);
	vi530x_write_byte(dev, VI530X_REG_MCU_CFG, 0x06);
	vi530x_read_byte(dev, VI530X_REG_SYS_CFG, &sys_cfg_data);
	vi530x_write_byte(dev, VI530X_REG_SYS_CFG, sys_cfg_data | (0x01 << 0));
	vi530x_write_byte(dev, VI530X_REG_CMD, 0x01);
	vi530x_write_byte(dev, VI530X_REG_SIZE, 0x02);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0, 0x0);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x01, 0x0);
	while(fw_size >= 32)
	{
		vi530x_write_reg_offset(dev, VI530X_REG_CMD, 0, VI530X_WRITEFW_CMD);
		vi530x_write_reg_offset(dev, VI530X_REG_SIZE, 0, 0x20);
		vi530x_write_multibytes(dev, VI530X_REG_SCRATCH_PAD_BASE, Firmware+fw_send*32, 32);
		udelay(10);
		fw_send += 1;
		fw_size -= 32;
	}
	if(fw_size > 0)
	{
		vi530x_write_reg_offset(dev, VI530X_REG_CMD, 0, VI530X_WRITEFW_CMD);
		vi530x_write_reg_offset(dev, VI530X_REG_SIZE, 0, (uint8_t)fw_size);
		vi530x_write_multibytes(dev, VI530X_REG_SCRATCH_PAD_BASE, Firmware+fw_send*32, fw_size);
	}
	vi530x_write_byte(dev, VI530X_REG_SYS_CFG, sys_cfg_data & ~(0x01 << 0));
	vi530x_write_byte(dev, VI530X_REG_MCU_CFG, 0x06);
	vi530x_write_byte(dev, VI530X_REG_PD_RESET, 0xA0);
	vi530x_write_byte(dev, VI530X_REG_PD_RESET, 0x80);
	vi530x_write_byte(dev, VI530X_REG_MCU_CFG, 0x07);
	vi530x_write_byte(dev, VI530X_REG_PW_CTRL, 0x02);
	vi530x_write_byte(dev, VI530X_REG_PW_CTRL, 0x00);
	mdelay(5);
	vi530x_read_byte(dev, VI530X_REG_SPCIAL_PURP, &val);
	if(val != 0x66)
	{
		vi530x_errmsg("Download Firmware Failed, value = %d\n", val);
		Status = VI530X_ERROR_FW_FAILURE;
	}

	return Status;
}

__attribute__((unused)) static void VI530X_Integral_Counts_Write(VI530X_DEV dev, uint32_t inte_counts)
{
	union inte_data {
		uint32_t intecnts;
		uint8_t buf[4];
	} intedata;

	intedata.intecnts = inte_counts;
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0, 0x01);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 1, 0x03);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 2, 0x14);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 3, intedata.buf[0]);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 4, intedata.buf[1]);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 5, intedata.buf[2]);
	vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_USER_CFG_CMD);
	mdelay(5);
}

__attribute__((unused)) static void VI530X_Delay_Count_Write(VI530X_DEV dev, uint16_t delay_count)
{
	union delay_data {
		uint16_t delay;
		uint8_t buf[2];
	} delaydata;

	delaydata.delay = delay_count;
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0, 0x01);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 1, 0x02);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 2, 0x17);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 3, delaydata.buf[0]);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 4, delaydata.buf[1]);
	vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_USER_CFG_CMD);
	mdelay(5);
}

__attribute__((unused)) static void VI530X_Set_Integralcounts_Frame(VI530X_DEV dev, uint8_t fps, uint32_t intecoutns)
{
	uint32_t inte_time = 0;
	uint32_t fps_time = 0;
	uint32_t delay_time = 0;
	uint16_t delay_counts = 0;

	inte_time = intecoutns *1463/10;
	fps_time = 1000000000/fps;
	delay_time = fps_time - inte_time - 5200000;
	delay_counts = (uint16_t)(delay_time/3400);
	VI530X_Integral_Counts_Write(dev, intecoutns);
	VI530X_Delay_Count_Write(dev, delay_counts);
}

void VI530X_Set_Period(VI530X_DEV dev, uint32_t period)
{
	uint32_t inte_time = 0;
	uint32_t fps_time = 0;
	uint32_t delay_time = 0;
	uint16_t delay_counts = 0;
	union inte_data pdata = {0};
	union delay_data {
		uint16_t delay;
		uint8_t buf[2];
	} delaydata;

	if (period == 0) {
		vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0, 0x00);
		vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 1, 0x02);
		vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 2, 0x17);
		vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_USER_CFG_CMD);
		mdelay(5);
		vi530x_read_multibytes(dev, VI530X_REG_SCRATCH_PAD_BASE, delaydata.buf, 2);
	} else {
		vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0, 0x00);
		vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 1, 0x03);
		vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 2, 0x14);
		vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_USER_CFG_CMD);
		mdelay(5);
		vi530x_read_multibytes(dev, VI530X_REG_SCRATCH_PAD_BASE, pdata.buf, 3);
		inte_time = pdata.intecnts*1463/10;
		fps_time = 1000000000 / period;
		delay_time = fps_time - inte_time - 5200000;
		delay_counts = (uint16_t)(delay_time/3400);
		delaydata.delay = delay_counts;
	}
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0, 0x01);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 1, 0x02);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 2, 0x17);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 3, delaydata.buf[0]);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 4, delaydata.buf[1]);
	vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_USER_CFG_CMD);
	mdelay(5);
}

static void VI530X_Temperature_Enable(VI530X_DEV dev, uint8_t enable)
{
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0, 0x01);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x01, 0x01);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x02, 0x24);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x03, enable);
	vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_USER_CFG_CMD);
	mdelay(5);
}

static uint8_t VI530X_Get_MA_Window_Data(VI530X_DEV dev)
{
	uint8_t sum_ma = 0;
	int i = 0;
	uint8_t ma_val[8];

	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x00, 0x00);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x01, 0x08);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x02, 0x1A);
	vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_USER_CFG_CMD);
	mdelay(5);
	vi530x_read_multibytes(dev, VI530X_REG_SCRATCH_PAD_BASE, ma_val, 8);
	for(i = 0; i < 8; i++)
		sum_ma += ((ma_val[i] & 0x0F)+((ma_val[i] >> 4) & 0x0F));
	return sum_ma;
}

/* Started by AICoder, pid:rf7a7fb6f0y2ec814a40080a00cad58d70a82d76 */
static int32_t VI530X_Calculate_Pileup_Bias(VI530X_DEV dev, uint32_t peak, uint32_t noise, uint32_t integral_times)
{
	uint32_t peak_tmp = 0;
	const int16_t xth1[] = {0,500,800,1128,1734,2000};
	const int16_t pth1[] = {-15,-4,0,8,25,35};
	const int16_t xth2[] = {0,160,350,700,1200,2000};
	const int16_t pth2[] = {-6,-4,0,13,40,70};
	const int16_t *xth, *pth;
	int32_t bias = 0;
	int len = 0;
	uint8_t i = 0;

	if (noise > 8000) {
		xth = xth2;
		pth = pth2;
		len = sizeof(xth2) / sizeof(xth2[0]);
	} else {
		xth = xth1;
		pth = pth1;
		len = sizeof(xth1) / sizeof(xth1[0]);
	}

	if (integral_times == 0)
		return bias;

	noise = noise / 8;
	if (peak > noise * dev->ma_sum)
		peak_tmp = (peak - noise * dev->ma_sum) * 16 / integral_times;

	for (i = 0; i < len  - 1; i++) {
		if (peak_tmp < xth[i + 1]) {
			bias = (int32_t)(pth[i + 1] - pth[i]) * (int32_t)(peak_tmp - xth[i]) / (int32_t)(xth[i + 1] - xth[i]) + (int32_t)pth[i];
			return bias;
		}
	}
	if (peak_tmp >= xth[i])
		bias = (int32_t)(pth[i] - pth[i - 1]) * (int32_t)(peak_tmp - xth[i-1]) / (int32_t)(xth[i] - xth[i - 1])+(int32_t)pth[i - 1];
	return bias;
}

static uint32_t VI530X_Calculate_Confidence(VI530X_DEV dev, uint32_t peak, uint32_t noise, uint32_t integral_times)
{
	uint32_t confidence = 0;
	int64_t noise_r = 0;
	int64_t peak_r = 0;
	int64_t lower = 0;
	int64_t upper = 0;
	int i;
	int len;
	const int32_t xth[] = {4, 32, 114, 175, 313, 482, 539, 657, 1472, 2421, 3223, 6777, 7217, 12326, 14946, 20906, 25976, 32287, 41121, 44258, 51439, 56032, 80216};
	const int32_t ylower1[] = {5,  10,  21,  27, 34, 49, 54, 66, 136, 211, 279, 566, 600, 1025, 1221, 1682, 2086, 2559, 3336, 3779, 4338, 4796, 6806};
	const int32_t yupper1[] = {7, 13,  25,  32, 43, 60, 66, 80, 162, 243, 321, 630, 666, 1138, 1338, 1828, 2266, 2743, 3542, 3995, 4580, 5050, 7171};

	const int32_t ylower2[] = {5,  10,  21,  27, 34, 49, 54, 66, 136, 211, 279, 566, 600, 1025, 1221, 1682, 1986, 2309, 3036, 3500, 4338, 4896, 5706};
	const int32_t yupper2[] = {7, 13,  25,  32, 43, 60, 66, 80, 162, 243, 321, 630, 666, 1138, 1338, 1828, 2116, 2499, 3442, 3686, 4480, 5050, 6871};

	const int32_t *ylower, *yupper;
	len = sizeof(xth) / sizeof(xth[0]);
	if (noise > 8000) {
		ylower = ylower2;
		yupper = yupper2;
	} else {
		ylower = ylower1;
		yupper = yupper1;
	}

	peak_r = (int64_t)peak * 1024 * 100 / integral_times;
	noise_r = (int64_t)noise * 131072 / integral_times;
	for (i = 0; i < (len - 1); i++) {
		if (noise_r < xth[i + 1]) {
			lower = 100 * (int64_t)(ylower[i + 1] - ylower[i]) * (int64_t)abs(noise_r - xth[i]) / (int64_t)(xth[i + 1] - xth[i]) + 100 * (int64_t)ylower[i];
			upper = 100 * (int64_t)(yupper[i + 1] - yupper[i]) * (int64_t)abs(noise_r - xth[i]) / (int64_t)(xth[i + 1] - xth[i]) + 100 * (int64_t)yupper[i];
			break;
		} else if (noise_r >= xth[len - 1]) {
			lower = 100 * (int64_t)(ylower[len - 1] - ylower[len - 2]) * (int64_t)abs(noise_r - xth[len - 2]) / (int64_t)(xth[len - 1] - xth[len - 2]) + 100 * (int64_t)ylower[len - 2];
			upper = 100 * (int64_t)(yupper[len - 1] - yupper[len - 2]) * (int64_t)abs(noise_r - xth[len - 2]) / (int64_t)(xth[len - 1] - xth[len - 2]) + 100 * (int64_t)yupper[len - 2];
			break;
		}
	}
	if (peak_r < lower)
		confidence = 0;
	else if (peak_r > upper)
		confidence = 100;
	else
		confidence = 100 * (peak_r - lower) / (upper - lower);

	return confidence;
}
/* Ended by AICoder, pid:rf7a7fb6f0y2ec814a40080a00cad58d70a82d76 */

VI530X_Error VI530X_Single_Measure(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;

	Status = VI530X_Wait_For_CPU_Ready(dev);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("CPU Abnormal Single Measure!Status = %d\n", Status);
		return VI530X_ERROR_SINGLE_CMD;
	}
	Status = vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_SINGLE_RANGE_CMD);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("Single measure Failed Status = %d\n", Status);
		return VI530X_ERROR_SINGLE_CMD;
	}

	return Status;
}
VI530X_Error VI530X_Start_Continuous_Measure(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;

	Status = VI530X_Wait_For_CPU_Ready(dev);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("CPU Abnormal Continuous Measure!Status = %d\n", Status);
		return VI530X_ERROR_CONTINUOUS_CMD;
	}

	Status = vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_CONTINOUS_RANGE_CMD);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("Continuous Measure Failed Status = %d\n", Status);
		return VI530X_ERROR_CONTINUOUS_CMD;
	}

	return Status;
}

VI530X_Error VI530X_Stop_Continuous_Measure(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;

	Status = vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_STOP_RANGE_CMD);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("Stop Measure Failed Status = %d\n", Status);
		return VI530X_ERROR_STOP_CMD;
	}

	return Status;
}

VI530X_Error VI530X_Get_Measure_Data(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;
	uint8_t buf[32];
	int16_t millimeter = 0;
	uint32_t noise = 0;
	uint32_t peak1 = 0, peak2 = 0;
	uint32_t peak = 0;
	uint32_t integral_times = 0;
	int32_t bias = 0;
	uint32_t confidence = 0;
	uint32_t ratio = 0;
	int16_t tof1 = 0, tof2 = 0;
	uint32_t tmp_peak = 0, tmp_bin = 0;
	uint8_t tof1_bin = 0, tof2_bin = 0;
	uint8_t reftof_bin = 0, flag = 0;;
	int8_t xtalk_bin = 0;

	Status = vi530x_read_multibytes(dev, VI530X_REG_SCRATCH_PAD_BASE, buf, 32);
	if (Status != VI530X_ERROR_NONE) {
		vi530x_errmsg("Get Range Data Failed Status = %d\n", Status);
		return VI530X_ERROR_GET_DATA;
	}

	tof2_bin = buf[0];
	tof1_bin = buf[17];
	reftof_bin = buf[6];
	tof2 = ((int16_t)buf[2] << 8) | ((int16_t)buf[1]);//*((int16_t *)(buf + 1));
	tof1 = ((int16_t)buf[13] << 8) | ((int16_t)buf[12]);//*((int16_t *)(buf + 12));
	integral_times = (((uint32_t )buf[24]) << 16) | (((uint32_t )buf[23]) << 8) | ((uint32_t)buf[22]);
	peak2 = (((uint32_t )buf[11]) << 24) | (((uint32_t )buf[10]) << 16) | (((uint32_t )buf[9]) << 8) | ((uint32_t)buf[8]);//*((uint32_t *)(buf + 8));
	peak1 = (((uint32_t )buf[31]) << 24) | (((uint32_t )buf[30]) << 16) | (((uint32_t )buf[29]) << 8) | ((uint32_t)buf[28]);//*((uint32_t *)(buf + 28));
	integral_times = integral_times & 0x00ffffff;
	noise = (((uint32_t )buf[27]) << 16) | (((uint32_t )buf[26]) << 8) | ((uint32_t)buf[25]);//*((uint32_t *)(buf + 25));
	noise = noise & 0x00ffffff;

	xtalk_bin = (int8_t)reftof_bin + dev->XtalkConfig.xtalk_config;
	if (tof1_bin <= xtalk_bin + 2) {
		if (peak1 >= 3000 * dev->ma_sum) {
			millimeter = tof1;
			peak = peak1;
			tmp_peak = peak2;
			tmp_bin = tof2_bin;
		} else {
			millimeter = tof2;
			peak = peak2;
			tmp_peak = peak1;
			tmp_bin = tof1_bin;
		}
	} else {
		millimeter = tof1;
		peak = peak1;
		tmp_peak = peak2;
		tmp_bin = tof2_bin;
	}

	bias = VI530X_Calculate_Pileup_Bias(dev, peak, noise, integral_times);
	confidence = VI530X_Calculate_Confidence(dev, peak, noise, integral_times);

	if (tof1 > 3500)
		confidence = 0;
	ratio = 100 * noise * dev->ma_sum / peak;
	if (noise > 90000 && buf[3] <= 50 && ratio < 850)
		confidence = 0;

	millimeter = millimeter + (int16_t)bias;
	millimeter = millimeter - (int16_t)dev->offset_config;
	if (millimeter < 0)
		confidence = 0;

	if (millimeter >= 300) {
		if ((tmp_peak > 10000) && (tmp_bin >= xtalk_bin - 2)
			&& (tmp_bin <= xtalk_bin + 2))
			flag = 1;
	}

	dev->Rangedata.tof = millimeter;
	dev->Rangedata.confidence = confidence;
	dev->Rangedata.peak = peak;
	dev->Rangedata.noise = noise;
	dev->Rangedata.integral_times = integral_times;
	dev->Rangedata.flag = flag;//stain flag

	if (dev->enable_debug)
		vi530x_infomsg("tof: %d, confidence: %d, peak: %u, noise: %u, integral_times: %u, bias: %d, flag: %d\n",
			millimeter, confidence, peak, noise, integral_times, bias, flag);

	return Status;
}

VI530X_Error VI530X_Get_Interrupt_State(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;
	uint8_t stat;

	Status = vi530x_read_byte(dev, VI530X_REG_INTR_STAT, &stat);
	if(!(stat & 0x01))
	{
		vi530x_errmsg("Get Interrupt State Failed Status = %d\n", Status);
		return VI530X_ERROR_IRQ_STATE;
	}

	return Status;
}

VI530X_Error VI530X_Interrupt_Enable(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;
	int loop = 0;
	uint8_t enable = 0;

	do
	{
		vi530x_read_byte(dev, VI530X_REG_INTR_MASK, &enable);
		enable |=  0x01;
		vi530x_write_byte(dev, VI530X_REG_INTR_MASK, enable);
		vi530x_read_byte(dev, VI530X_REG_INTR_MASK, &enable);
		loop++;
	} while((loop < VI530X_MAX_WAIT_RETRY)
		&& (!(enable & 0x01)));
	if(loop >= VI530X_MAX_WAIT_RETRY)
	{
		vi530x_errmsg("Enable interrupt Failed Status = %d\n", Status);
		return VI530X_ERROR_ENABLE_INTR;
	}

	return Status;
}

VI530X_Error VI530X_Chip_Init(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;

	VI530X_Read_ChipVersion(dev);
	Status = VI530X_Wait_For_CPU_Ready(dev);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("Internal CPU busy!\n");
		return Status;
	}
	Status = VI530X_Interrupt_Enable(dev);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("Clear Interrupt Mask failed!\n");
		return Status;
	}
	Status = VI530X_Init_FirmWare(dev);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("Download Firmware Failed!\n");
		return Status;
	}
	dev->ma_sum = VI530X_Get_MA_Window_Data(dev);
	VI530X_Read_FW_Version(dev);


	return Status;
}

VI530X_Error VI530X_Start_XTalk_Calibration(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;

	Status = VI530X_Wait_For_CPU_Ready(dev);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("CPU Abnormal XTALK Calibrating Status = %d\n", Status);
		return VI530X_ERROR_XTALK_CALIB;
	}
	VI530X_Temperature_Enable(dev, 0);
	Status = vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_XTALK_TRIM_CMD);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("XTALK Calibration Failed Status = %d\n", Status);
		return VI530X_ERROR_XTALK_CALIB;
	}

	return Status;
}

VI530X_Error VI530X_Start_Offset_Calibration(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;
	uint8_t buf[32];
	uint32_t noise = 0;
	uint32_t peak1 = 0, peak2 = 0;
	uint32_t peak = 0;
	uint32_t integral_times = 0;
	int32_t bias = 0;
	int16_t tof1 = 0, tof2 = 0;
	int16_t millimeter = 0;
	int16_t total = 0;
	int16_t offset = 0;
	int cnt = 0;
	uint8_t stat = 0;

	VI530X_Temperature_Enable(dev, 0);
	Status = VI530X_Start_Continuous_Measure(dev);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("Offset Calibtration Start Failed!\n");
		Status = VI530X_ERROR_OFFSET_CALIB;
		goto err_exit;
	}
	while(1)
	{
		mdelay(35);
		Status = vi530x_read_byte(dev, VI530X_REG_INTR_STAT, &stat);
		if(Status == VI530X_ERROR_NONE)
		{
			if((stat & 0x01) == 0x01)
			{
				Status = vi530x_read_multibytes(dev, VI530X_REG_SCRATCH_PAD_BASE, buf, 32);
				if(Status != VI530X_ERROR_NONE)
				{
					vi530x_errmsg("Get Range Data Failed Status = %d\n", Status);
					break;
				}
				tof2 = ((int16_t)buf[2] << 8) | ((int16_t)buf[1]);//*((int16_t *)(buf + 1));
				tof1 = ((int16_t)buf[13] << 8) | ((int16_t)buf[12]);//*((int16_t *)(buf + 12));
				integral_times = (((uint32_t )buf[24]) << 16) | (((uint32_t )buf[23]) << 8) | ((uint32_t)buf[22]);
				peak2 = (((uint32_t )buf[11]) << 24) | (((uint32_t )buf[10]) << 16) | (((uint32_t )buf[9]) << 8) | ((uint32_t)buf[8]);//*((uint32_t *)(buf + 8));
				peak1 = (((uint32_t )buf[31]) << 24) | (((uint32_t )buf[30]) << 16) | (((uint32_t )buf[29]) << 8) | ((uint32_t)buf[28]);//*((uint32_t *)(buf + 28));
				integral_times = integral_times & 0x00ffffff;
				noise = (((uint32_t )buf[27]) << 16) | (((uint32_t )buf[26]) << 8) | ((uint32_t)buf[25]);//*((uint32_t *)(buf + 25));
				noise = noise & 0x00ffffff;
				if (tof1 <= 35) {
					if (peak1 >= 10000 * dev->ma_sum) {
						millimeter = tof1;
						peak = peak1;
					} else {
						millimeter = tof2;
						peak = peak2;
					}
				} else {
					millimeter = tof1;
					peak = peak1;
				}
				bias = VI530X_Calculate_Pileup_Bias(dev, peak, noise, integral_times);
				millimeter = millimeter + (int16_t)bias;
				total += millimeter;
				++cnt;
			} else
				continue;
		} else {
			vi530x_errmsg("Can't Get irq State!Status = %d\n", Status);
			break;
		}
		if(cnt >= 30)
			break;
	}
	Status = VI530X_Stop_Continuous_Measure(dev);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("Offset Calibtration Stop Failed!\n");
		Status = VI530X_ERROR_OFFSET_CALIB;
		goto err_exit;
	}
	offset = total / 30;
	dev->OffsetData.offset_cal = offset - dev->OffsetData.offset_mili;

	if (dev->enable_debug)
		vi530x_infomsg("offset mili: %d, offset cal: %d\n", dev->OffsetData.offset_mili, dev->OffsetData.offset_cal);

err_exit:
	VI530X_Temperature_Enable(dev, 1);
	return Status;
}

VI530X_Error VI530X_Get_XTalk_Parameter(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;
	uint8_t val;
	uint8_t cg_buf[5];

	Status = vi530x_read_byte(dev, VI530X_REG_SPCIAL_PURP, &val);
	if(Status == VI530X_ERROR_NONE && val == 0xaa)
	{
		Status = vi530x_read_multibytes(dev, VI530X_REG_SCRATCH_PAD_BASE, cg_buf, 4);
		if(Status != VI530X_ERROR_NONE)
		{
			vi530x_errmsg("Get XTALK parameter Failed Status = %d\n", Status);
			Status = VI530X_ERROR_XTALK_CALIB;
			goto err_exit;
		}
		dev->XtalkData.xtalk_cal = *((int8_t *)(cg_buf + 0));
		dev->XtalkData.xtalk_peak = *((uint16_t *)(cg_buf + 1));
		dev->XtalkData.maxratio = *((uint8_t *)(cg_buf + 3));
	} else {
		vi530x_errmsg("XTALK Calibration Failed Status = %d, val = 0x%02x\n", Status, val);
		Status = VI530X_ERROR_XTALK_CALIB;
		goto err_exit;
	}

err_exit:
	VI530X_Temperature_Enable(dev, 1);
	return Status;
}

VI530X_Error VI530X_Config_XTalk_Parameter(VI530X_DEV dev)
{
	VI530X_Error Status = VI530X_ERROR_NONE;

	Status = VI530X_Wait_For_CPU_Ready(dev);
	if(Status != VI530X_ERROR_NONE)
	{
		vi530x_errmsg("CPU Abnormal Configing XTALK Failed Status = %d\n", Status);
		return VI530X_ERROR_XTALK_CONFIG;
	}
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0, 0x01);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x01, 0x01);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x02, 0x26);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x03, *((uint8_t *)(&dev->XtalkConfig.xtalk_config)));
	vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_USER_CFG_CMD);
	mdelay(5);

	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0, 0x01);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x01, 0x01);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x02, 0x25);
	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0x03, dev->XtalkConfig.maxratio);
	vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_USER_CFG_CMD);
	mdelay(5);

	return Status;
}
void VI530X_Read_FW_Version(VI530X_DEV dev)
{
	uint8_t fw_version[4];

	vi530x_write_reg_offset(dev, VI530X_REG_SCRATCH_PAD_BASE, 0, 0x06);
	vi530x_write_byte(dev, VI530X_REG_CMD, VI530X_USER_CFG_CMD);
	mdelay(5);
	vi530x_read_multibytes(dev, VI530X_REG_SCRATCH_PAD_BASE, fw_version, 4);

	vi530x_infomsg("fw version: %#x %#x %#x %#x\n", fw_version[0], fw_version[1], fw_version[2], fw_version[3]);
}
