// SPDX-License-Identifier: GPL-2.0-only
/*
 * bos1921.c
 * micropump
 *
 * Copyright (c) 2012 Marvell International Ltd.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>

#include "bos1921.h"

#define PROC_MICRO_PUMP_DIR "micropump"
#define PROC_PUMP_ENABLEAA "enableaa"
#define PROC_MICRO_PUMP_ENABLE "driver/micropump_enable"
#define PROC_MICRO_PUMP_SPEED  "driver/micropump_speed"
#define PROC_MICRO_PUMP_FREQ  "driver/micropump_freq"

#define BOS1921_I2C_NAME "bos1921"
#define BOS1921_DRIVER_VERSION "v0.1"

#define LOW_SYNTHESIZER_AMPLITUDE_V (180)
#define DEFAULT_SYNTHESIZER_AMPLITUDE_V (190)
#define DEFAULT_SYNTHESIZER_FREQUENCY_HZ (200)
#define DEFAULT_SYNTHESIZER_SHAPE_UP_MS (32)
#define DEFAULT_SYNTHESIZER_SHAPE_DOWN_MS (32)

struct proc_dir_entry *micro_pump_proc_entry = NULL;

static Bos1921RegisterStruct bos1921RegsArray[] = {
	{ BOS1921_ADDRESS_REFERENCE_REG, 0 },
	{ BOS1921_ADDRESS_ION_BL_REG, 0 },
	{ BOS1921_ADDRESS_DEADTIME_REG, 0 },
	{ BOS1921_ADDRESS_KP_REG, 0 },
	{ BOS1921_ADDRESS_KPA_KI_REG, 0 },
	{ BOS1921_ADDRESS_CONFIG_REG, 0 },
	{ BOS1921_ADDRESS_PARCAP_REG, 0 },
	{ BOS1921_ADDRESS_SUP_RISE_REG, 0 },
	{ BOS1921_ADDRESS_INT_ENABLE_REG, 0 },
	{ BOS1921_ADDRESS_SENSING_REG, 0 },
	{ BOS1921_ADDRESS_TRIM_REG, 0 },
	{ BOS1921_ADDRESS_COMM_REG, 0 },
	{ BOS1921_ADDRESS_VFEEDBACK_REG, 0 },
	{ BOS1921_ADDRESS_IC_STATUS_REG, 0 },
	{ BOS1921_ADDRESS_FIFO_STATE_REG, 0 },
	{ BOS1921_ADDRESS_SENSE_VALUE_REG, 0 },
	{ BOS1921_ADDRESS_RAM_DATA_REG, 0 },
	{ BOS1921_ADDRESS_CHIP_ID_REG, 0 },
	{ BOS1921_ADDRESS_INT_STATUS_REG, 0 },
};

struct bos1921 *mBOS1921 = NULL;

static void dump_data(uint8_t *data, uint16_t len)
{
	int i = 0;
	char str[1024] = { 0 };
	char s[10] = { 0 };
	for (i = 0; i < len; ++i) {
		sprintf(s, "%02X ", data[i]);
		strcat(str, s);
	}
	DBG("%s\n", str);
}

static void dump_data16(uint16_t *data, uint16_t len)
{
	int i = 0;
	char str[1024] = { 0 };
	char s[10] = { 0 };
	for (i = 0; i < len; ++i) {
		sprintf(s, "%02X ", data[i]);
		strcat(str, s);
	}
	DBG("%s\n", str);
}

static int32_t i2c_write(struct i2c_client *i2c, uint8_t *tr_data, uint16_t len)
{
	struct i2c_msg msg;

	msg.addr = i2c->addr;
	msg.flags = I2C_WR;
	msg.len = len;
	msg.buf = tr_data;

	dump_data(tr_data, len);
	return (i2c_transfer(i2c->adapter, &msg, 1) == 1) ? 0 : -1;
}

static void htoBe16(uint16_t value, uint8_t *data)
{
	data[0] = (value & 0xFF00) >> 8;
	data[1] = (value & 0x00FF);
}

static int32_t aw9620x_i2c_write(struct bos1921 *bos1921, uint16_t reg_addr16,
				 uint16_t reg_data16)
{
	int8_t cnt = AW_RETRIES;
	int32_t ret = -1;
	uint8_t w_buf[WRITE_REG_LENGTH] = { 0 };

	/*reg_addr*/
	w_buf[I2C_ADDRESS_INDEX] = (uint8_t)(reg_addr16 & 0xFF);
	/*data*/
	htoBe16(reg_data16, &w_buf[I2C_ADDRESS_REG_DATA_INDEX]);

	do {
		ret = i2c_write(bos1921->i2c, w_buf, ARRAY_SIZE(w_buf));
		if (ret < 0) {
			DBG_ERR("bos1921 i2c write error reg: 0x%04x data: 0x%04x, ret= %d cnt= %d",
				reg_addr16, reg_data16, ret, cnt);
		} else {
			break;
		}
		usleep_range(2000, 3000);
	} while (cnt--);

	if (cnt < 0) {
		DBG_ERR("i2c write error!");
		return -1;
	}

	return 0;
}

#if 1
static int32_t i2c_read(struct i2c_client *i2c, uint8_t *data,
			uint16_t data_len)
{
	struct i2c_msg msg[1];

	msg[0].addr = i2c->addr;
	msg[0].flags = I2C_RD;
	msg[0].len = data_len;
	msg[0].buf = data;

	return i2c_transfer(i2c->adapter, msg, 1);
}
#else
static int32_t i2c_read(struct i2c_client *i2c, uint8_t *addr, uint8_t addr_len,
			uint8_t *data, uint16_t data_len)
{
	struct i2c_msg msg[2];

	msg[0].addr = i2c->addr;
	msg[0].flags = I2C_WR;
	msg[0].len = addr_len;
	msg[0].buf = addr;

	msg[1].addr = i2c->addr;
	msg[1].flags = I2C_RD;
	msg[1].len = data_len;
	msg[1].buf = data;

	return i2c_transfer(i2c->adapter, msg, 2);
}
#endif

static uint16_t beToH16(uint8_t *data)
{
	uint32_t value = 0;

	value |= data[0] << 8;
	value |= data[1];

	return value;
}

static int32_t aw9620x_i2c_read(struct bos1921 *bos1921, uint16_t reg_addr16,
				uint16_t *reg_data16)
{
	int8_t cnt = AW_RETRIES;
	int32_t ret = -1;
	uint8_t r_buf[REG_VALUE_LENGTH] = { 0 };
	uint16_t commRegVal = 0;

	commRegVal = BOS1921_COMM_REG_CLEAR_RDADDR(BOS1921_ADDRESS_COMM_REG);
	commRegVal |= BOS1921_COMM_REG_WRITE_RDADDR(reg_addr16);
	ret = aw9620x_i2c_write(bos1921, BOS1921_ADDRESS_COMM_REG, commRegVal);
	if (ret) {
		DBG_ERR("write BOS1921_ADDRESS_COMM_REG error,ret=%d", ret);
		return ret;
	}

	do {
		ret = i2c_read(bos1921->i2c, &r_buf[0], REG_VALUE_LENGTH);
		if (ret < 0)
			DBG_ERR("i2c read error reg: 0x%04x, ret= %d cnt= %d",
				reg_addr16, ret, cnt);
		else
			break;
		usleep_range(2000, 3000);
	} while (cnt--);

	if (cnt < 0) {
		DBG_ERR("i2c read error!");
		return -1;
	}

	*reg_data16 = beToH16(&r_buf[0]);

	return 0;
}

#define ADDR(__reg) (bos1921RegsArray[__reg].addr)
#define VALUE(__reg) (bos1921RegsArray[__reg].value)
#define readReg(__reg, __value, __ret)                                         \
	do {                                                                   \
		wakeUpFromSleep(bos1921);                                      \
		__ret = aw9620x_i2c_read(bos1921, ADDR(__reg), &VALUE(__reg)); \
		__value = VALUE(__reg);                                        \
	} while (0)
#define writeReg(__reg, __value)                                  \
	do {                                                      \
		wakeUpFromSleep(bos1921);                         \
		aw9620x_i2c_write(bos1921, ADDR(__reg), __value); \
		VALUE(__reg) = __value;                           \
	} while (0)

static int32_t i2c_write_seq(struct bos1921 *bos1921)
{
	int8_t cnt = AW_RETRIES;
	int32_t ret = -1;

	do {
		ret = i2c_write(bos1921->i2c, bos1921->txBuffer,
				bos1921->txBufferLength);
		if (ret < 0) {
			DBG_ERR("bos1921 i2c write seq error %d", ret);
		} else {
			break;
		}
		usleep_range(2000, 3000);
	} while (cnt--);

	if (cnt < 0) {
		DBG_ERR("i2c write error!");
		return -1;
	}

	return ret;
}

static int wakeUpFromSleep(struct bos1921 *bos1921)
{
	DBG("enter,sleepEnabled=%d\n", bos1921->sleepEnabled);
	if (bos1921->sleepEnabled) {
		bos1921->sleepEnabled = 0;
		aw9620x_i2c_write(bos1921, ADDR(BOS1921_REGISTER_CHIP_ID_REG),
				  VALUE(BOS1921_REGISTER_CHIP_ID_REG));
		usleep_range(50, 50);
	}
	return !bos1921->sleepEnabled;
}

static int readAllRegister(struct bos1921 *bos1921)
{
	int res = 0;
	uint16_t dummy;
	for (enum BOS1921_REGISTER reg = BOS1921_REGISTER_FIRST;
	     reg < BOS1921_REGISTER_COUNT; reg++) {
		readReg(reg, dummy, res);
		if (res) {
			DBG_ERR("%s(),readReg fail,reg=0x%04X,res=%d", __func__,
				reg, res);
			break;
		}
	}
	return res;
}

static void dumpAllRegister(struct bos1921 *bos1921)
{
	for (enum BOS1921_REGISTER reg = BOS1921_REGISTER_FIRST;
	     reg < BOS1921_REGISTER_COUNT; reg++) {
		DBG("addr=0x%04X,value=0x%04X", ADDR(reg), VALUE(reg));
	}
}

static int32_t read_chipid(struct bos1921 *bos1921)
{
	int32_t ret = -1;
	uint16_t value = 0;

	DBG("enter\n");
	readReg(BOS1921_REGISTER_CHIP_ID_REG, value, ret);
	if (ret) {
		DBG_ERR("read CHIP ID failed: %d", ret);
	} else {
		DBG("read_chipid=0x%04X", value);
		if ((value & BOS1921_CHIP_ID_REG_CHIP_ID_MASK) ==
		    BOS1921_CHIP_ID_DEFAULT) {
			DBG("BOS1921 Rev A Detected");
			bos1921->chipId = BOS1921_CHIP_ID_DEFAULT;
			ret = 0;
		} else {
			DBG_ERR("BOS1921 No Valid Revision Detected");
			bos1921->chipId = INVALID_CHIP_ID;
			ret = -1;
		}
	}

	return ret;
}

static int bos1921GetState(struct bos1921 *bos1921, BosState *state)
{
	uint16_t value = 0;
	int res = 0;
	readReg(BOS1921_REGISTER_IC_STATUS_REG, value, res);
	if (0 == res) {
		BosState stateValue = BOS1921_IC_STATUS_REG_READ_STATE(value);
		if (stateValue > BOS_STATE_LENGTH) {
			res = -1;
		} else {
			*state = stateValue;
		}
	}
	return res;
}

static int writeRam(struct bos1921 *bos1921, WSFCommand bank, uint16_t *data,
		    size_t length)
{
	int res = 0;

	DBG("enter,bank=%d,length=%zu,sleepEnabled=%d\n", bank, length,
	    bos1921->sleepEnabled);
	if (bos1921->sleepEnabled) {
		wakeUpFromSleep(bos1921);
	}
	dump_data16(data, length / sizeof(uint16_t));

	memset(bos1921->txBuffer, 0, sizeof(bos1921->txBuffer));
	bos1921->txBuffer[I2C_ADDRESS_INDEX] =
		BOS1921_ADDRESS_REFERENCE_REG; // [0] Main register map address
	htoBe16(bank,
		&bos1921->txBuffer[WRITE_RAM_BANK_INDEX]); // [1-2] WFS bank address
	for (size_t i = 0; i < length;
	     i += 2) // [3..] RAM Data to be forwarded to WFS
	{
		htoBe16(*(data++),
			&bos1921->txBuffer[i + WRITE_RAM_DATA_INDEX]);
	}

	bos1921->txBufferLength = WRITE_RAM_DATA_INDEX + length;
	res = i2c_write_seq(bos1921);
	DBG("exit,res=%d\n", res);
	return res;
}

static int bos1921SynthesizerPlay(struct bos1921 *bos1921, WaveformId start,
				  WaveformId stop)
{
	int res = 0;
	uint16_t startStopAddr = 0;

	DBG("enter,start=%d,stop=%d\n", start, stop);
	if (bos1921 == NULL || start >= MAXIMUM_NUMBER_OF_WAVE ||
	    stop >= MAXIMUM_NUMBER_OF_WAVE) {
		DBG("bos1921=%p,start=%d,stop=%d\n", bos1921, start, stop);
		return -1;
	}

	startStopAddr = (stop << RAM_SYNTHESIS_END_WAVE_ADDRESS_SHIFT) |
			(start << RAM_SYNTHESIS_START_WAVE_ADDRESS_SHIFT);

	res = writeRam(bos1921, WSF_COMMAND_RAM_SYNTHESIS, &startStopAddr,
		       sizeof(startStopAddr));

	return res;
}

static int bos1921SynthesizerStop(struct bos1921 *bos1921)
{
	uint16_t startStopAddr = (1 << RAM_SYNTHESIS_STOP_ADDRESS_SHIFT);
	int res = writeRam(bos1921, WSF_COMMAND_RAM_SYNTHESIS, &startStopAddr,
			   sizeof(startStopAddr));
	return res;
}

static int32_t soft_reset(struct bos1921 *bos1921)
{
	int32_t ret = -1;
	uint16_t config_value = 0;
	unsigned long start = 0, end = 0;

	DBG("enter\n");

	readReg(BOS1921_REGISTER_CONFIG_REG, config_value, ret);
	if (ret < 0) {
		DBG_ERR("read BOS1921_REGISTER_CONFIG_REG failed: %d", ret);
		return -1;
	}

	start = jiffies;
	DBG("!!!!!!!!!!!!!!!!!!!! Reset IC !!!!!!!!!!!!!!!!!!!!!");
	if (BOS1921_CONFIG_REG_READ_OE(config_value) == 1) {
		BosState state = -1;
		int clearOe = 1;
		DBG_ERR("CONFIG.OE is 1");
		if ((0 == bos1921GetState(bos1921, &state)) &&
		    (state == BOS_STATE_RUN)) {
			Bos1921HapticMode mode =
				BOS1921_CONFIG_REG_READ_PLAY_MODE(config_value);
			DBG("IcStatus.State = %u, Config.PlayMode = %u", state,
			    mode);
			switch (mode) {
			case Bos1921HapticMode_Direct:
				writeReg(BOS1921_ADDRESS_REFERENCE_REG, 0);
				break;
			case Bos1921HapticMode_Fifo:
				//Do nothing, Wait for the FIFO to be empty
				break;
			case Bos1921HapticMode_RamPlayback:
				config_value =
					BOS1921_CONFIG_REG_CLEAR_PLAY_MODE(
						config_value);
				config_value |=
					BOS1921_CONFIG_REG_WRITE_PLAY_MODE(
						Bos1921HapticMode_Direct);
				DBG("Set playmode to 0");
				writeReg(BOS1921_ADDRESS_CONFIG_REG,
					 config_value);
				DBG("Write 0 in REFERENCE");
				writeReg(BOS1921_ADDRESS_REFERENCE_REG, 0);
				break;
			case Bos1921HapticMode_RamSynthesis:
				DBG("Stop ram synthesis,mode=%d", mode);
				bos1921SynthesizerStop(bos1921);
				clearOe = 0;
				break;
			default:
				break;
			}
		}

		if (clearOe) {
			DBG("Set OE to 0");
			config_value =
				BOS1921_CONFIG_REG_CLEAR_OE(config_value);
			writeReg(BOS1921_ADDRESS_CONFIG_REG, config_value);
		}

		{
			int timeout = 0;
			uint16_t sense_value = 0;
			DBG("Wait for Idle state...");
			do {
				bos1921GetState(bos1921, &state);
				readReg(BOS1921_REGISTER_SENSING_REG,
					sense_value, ret);
				DBG("Ic.Status = 0x%04X, sense value=0x%04X",
				    state, sense_value);
				msleep(10);
			} while ((state != BOS_STATE_IDLE) && (timeout < 1000));

			if (state != BOS_STATE_IDLE) {
				DBG_ERR("Bos1921: Timeout while waiting for idle state");
				ret = -1;
			}
		}
	}

	config_value = BOS1921_CONFIG_REG_CLEAR_RST(config_value);
	config_value |= BOS1921_CONFIG_REG_WRITE_RST(1);
	writeReg(BOS1921_ADDRESS_CONFIG_REG, config_value);

	VALUE(BOS1921_REGISTER_CONFIG_REG) =
		BOS1921_CONFIG_REG_CLEAR_RST(config_value);

	msleep(BOS1921_RESET_SOFTWARE_DELAY_MS);

	bos1921->sleepEnabled = 1;
	wakeUpFromSleep(bos1921);

	end = jiffies;
	DBG("!!!!!!!!!!!!!!!!!!!! Reset IC end: %s, delay=%u ms !!!!!!!!!!!!!!!!!!!!!",
	    ret == 0 ? "SUCCESS" : "FAILURE", jiffies_to_msecs(end - start));

	return ret;
}

static int setDefaultConfig(struct bos1921 *bos1921)
{
	int res = 0;
	DBG("enter\n");
	res = readAllRegister(bos1921);

	if (0 == res) {
		VALUE(BOS1921_REGISTER_CONFIG_REG) =
			BOS1921_CONFIG_REG_CLEAR_SENSE(
				VALUE(BOS1921_REGISTER_CONFIG_REG));

		VALUE(BOS1921_REGISTER_ION_BL_REG) =
			BOS1921_ION_BL_REG_CLEAR_FSWMAX(
				VALUE(BOS1921_REGISTER_ION_BL_REG));
		VALUE(BOS1921_REGISTER_ION_BL_REG) |=
			BOS1921_ION_BL_REG_WRITE_FSWMAX(0x3);
		VALUE(BOS1921_REGISTER_ION_BL_REG) =
			BOS1921_ION_BL_REG_CLEAR_ION_SCALE(
				VALUE(BOS1921_REGISTER_ION_BL_REG));
		VALUE(BOS1921_REGISTER_ION_BL_REG) |=
			BOS1921_ION_BL_REG_WRITE_ION_SCALE(0x5D);
		VALUE(BOS1921_REGISTER_ION_BL_REG) =
			BOS1921_ION_BL_REG_CLEAR_SB(
				VALUE(BOS1921_REGISTER_ION_BL_REG));
		VALUE(BOS1921_REGISTER_ION_BL_REG) |=
			BOS1921_ION_BL_REG_WRITE_SB(0x3);

		VALUE(BOS1921_REGISTER_DEADTIME_REG) =
			BOS1921_DEADTIME_REG_CLEAR_DHS(
				VALUE(BOS1921_REGISTER_DEADTIME_REG));
		VALUE(BOS1921_REGISTER_DEADTIME_REG) |=
			BOS1921_DEADTIME_REG_WRITE_DHS(0x5B);

		VALUE(BOS1921_REGISTER_KP_REG) =
			BOS1921_KP_REG_CLEAR_KP(VALUE(BOS1921_REGISTER_KP_REG));
		VALUE(BOS1921_REGISTER_KP_REG) |= BOS1921_KP_REG_WRITE_KP(0x80);

		VALUE(BOS1921_REGISTER_KPA_KI_REG) =
			BOS1921_KPA_KI_REG_CLEAR_KPA(
				VALUE(BOS1921_REGISTER_KPA_KI_REG));
		VALUE(BOS1921_REGISTER_KPA_KI_REG) |=
			BOS1921_KPA_KI_REG_WRITE_KPA(0xA0);

		VALUE(BOS1921_REGISTER_PARCAP_REG) =
			BOS1921_PARCAP_REG_CLEAR_PARCAP(
				VALUE(BOS1921_REGISTER_PARCAP_REG));
		VALUE(BOS1921_REGISTER_PARCAP_REG) |=
			BOS1921_PARCAP_REG_WRITE_PARCAP(0x10);

		VALUE(BOS1921_REGISTER_SUP_RISE_REG) =
			BOS1921_SUP_RISE_REG_CLEAR_VDD(
				VALUE(BOS1921_REGISTER_SUP_RISE_REG));
		VALUE(BOS1921_REGISTER_SUP_RISE_REG) |=
			BOS1921_SUP_RISE_REG_WRITE_VDD(0x1F);
		VALUE(BOS1921_REGISTER_SUP_RISE_REG) =
			BOS1921_SUP_RISE_REG_CLEAR_TI_RISE(
				VALUE(BOS1921_REGISTER_SUP_RISE_REG));
		VALUE(BOS1921_REGISTER_SUP_RISE_REG) |=
			BOS1921_SUP_RISE_REG_WRITE_TI_RISE(0x01);

		//writeReg(BOS1921_REGISTER_CONFIG_REG, VALUE(BOS1921_REGISTER_CONFIG_REG));
		//writeReg(BOS1921_REGISTER_ION_BL_REG, VALUE(BOS1921_REGISTER_ION_BL_REG));
		//writeReg(BOS1921_REGISTER_DEADTIME_REG, VALUE(BOS1921_REGISTER_DEADTIME_REG));
		//writeReg(BOS1921_REGISTER_KP_REG, VALUE(BOS1921_REGISTER_KP_REG));
		//writeReg(BOS1921_REGISTER_KPA_KI_REG, VALUE(BOS1921_REGISTER_KPA_KI_REG));
		//writeReg(BOS1921_REGISTER_PARCAP_REG, VALUE(BOS1921_REGISTER_PARCAP_REG));
		//writeReg(BOS1921_REGISTER_SUP_RISE_REG, VALUE(BOS1921_REGISTER_SUP_RISE_REG));
		writeReg(BOS1921_REGISTER_ION_BL_REG, 0x0F5D);
		writeReg(BOS1921_REGISTER_DEADTIME_REG, 0x0B6A);
		writeReg(BOS1921_REGISTER_KP_REG, 0x00B0);
		writeReg(BOS1921_REGISTER_KPA_KI_REG, 0x0260);
		writeReg(BOS1921_REGISTER_PARCAP_REG, 0x0010);
		writeReg(BOS1921_REGISTER_SUP_RISE_REG, 0x4FC1);
		//writeReg(BOS1921_REGISTER_SENSE_VALUE_REG, 0x0000);
	}

	DBG("exit\n");
	return res;
}

static int setHapticMode(struct bos1921 *bos1921, Bos1921HapticMode mode)
{
	DBG("enter,mode=%d\n", mode);
	VALUE(BOS1921_REGISTER_CONFIG_REG) = BOS1921_CONFIG_REG_CLEAR_PLAY_MODE(
		VALUE(BOS1921_REGISTER_CONFIG_REG));
	VALUE(BOS1921_REGISTER_CONFIG_REG) |=
		BOS1921_CONFIG_REG_WRITE_PLAY_MODE(mode);

	writeReg(BOS1921_REGISTER_CONFIG_REG,
		 VALUE(BOS1921_REGISTER_CONFIG_REG));

	return 0;
}

static int getShapeBitFieldValue(uint16_t shapeValue, uint8_t *shapeField)
{
	int res = 0;
	const uint16_t shaperTable[] = { 0,    32,   64,   96,	128, 160,
					 192,  224,  256,  512, 768, 1024,
					 1280, 1536, 1792, 2048 };
	for (uint32_t index = 0; index < DATA_ARRAY_LENGTH(shaperTable);
	     index++) {
		if (shapeValue >= shaperTable[index]) {
			*shapeField = index;
			res = 1;
		} else {
			break;
		}
	}
	return res;
}

static int setSlice(struct bos1921 *bos1921)
{
	int res = 0;
	SynthSlice *slice = &bos1921->slice;
	uint8_t shapeUp = 0, shapeDown = 0;
	uint16_t buffer[SLICE_BLOCK_SIZE + RAM_ADDRESS_LENGTH] = { 0 };
	uint32_t amplitude_computed = 0;
	uint16_t freq = 0;

	DBG("SetSlice id %d Frequency %d Hz Amplitude %d (mV) cycle %d",
	    slice->sliceId, slice->mHzFreq, slice->mVAmp, slice->cycle);

	res = getShapeBitFieldValue(slice->shapeUpMs, &shapeUp);
	res = !res && getShapeBitFieldValue(slice->shapeDownMs, &shapeDown);

	amplitude_computed = (slice->mVAmp * AMPLITUDE_MAX_VALUE) /
			     BOS1921_OUT_PEAK_TO_PEAK_VOLTAGE_MILLI_VOLT;
	freq = (uint16_t)(slice->mHzFreq / FREQUENCY_RESOLUTION_MILLI_HZ);

	buffer[RAM_ADDRESS_INDEX] =
		FIRST_RAM_SLICE_ADDRESS + (slice->sliceId * SLICE_BLOCK_SIZE);
	buffer[SLICE_AMPLITUDE_INDEX] = (uint16_t)(amplitude_computed);
	buffer[SLICE_FREQ_CYCLE_INDEX] = (uint16_t)freq;
	buffer[SLICE_FREQ_CYCLE_INDEX] |= (0xFF & slice->cycle)
					  << SLICE_CYCLE_SHIFT;
	buffer[SLICE_SHAPE_INDEX] =
		((slice->continuous ? 1 : 0) << SLICE_CONT_SHIFT) |
		(shapeUp << SHAPE_UP_SHIFT) | shapeDown | 1;
	return !res && writeRam(bos1921, WSF_COMMAND_RAM_ACCESS, buffer,
				sizeof(buffer));
}

static int bos1921SetSlice(struct bos1921 *bos1921, const uint8_t outputChannel)
{
	int res = 0;
	DBG("enter,outputChannel=%d\n", outputChannel);
	if (bos1921 == NULL ||
	    bos1921->slice.sliceId >= MAXIMUM_NUMBER_OF_SLICE) {
		DBG_ERR("bos1921=%p,sliceId=%d\n", bos1921,
			bos1921->slice.sliceId);
		return -1;
	}

	res = setHapticMode(bos1921, Bos1921HapticMode_RamSynthesis);
	res = !res && setSlice(bos1921);

	return res;
}

static int bos1921SetWaveforms(struct bos1921 *bos1921, WaveformId id,
			       uint8_t startSliceId, size_t nbrOfSlices,
			       uint16_t cycle, uint8_t outputChannel)
{
	int res = 0;
	uint16_t waveformAddr = FIRST_RAM_WAVE_ADDRESS + (id * WAVE_BLOCK_SIZE);
	uint16_t command[] = {
		waveformAddr,
		FIRST_RAM_SLICE_ADDRESS + (startSliceId * SLICE_BLOCK_SIZE),
		FIRST_RAM_SLICE_ADDRESS +
			((startSliceId + nbrOfSlices) * SLICE_BLOCK_SIZE) - 1,
		(uint16_t)cycle
	};

	(void)outputChannel;
	DBG("enter,id=%d,startSliceId=%d,nbrOfSlices=%zu,cycle=%d,outputChannel=%d\n",
	    id, startSliceId, nbrOfSlices, cycle, outputChannel);

	if (bos1921 == NULL || id >= MAXIMUM_NUMBER_OF_WAVE ||
	    nbrOfSlices >= MAXIMUM_NUMBER_OF_SLICE) {
		DBG("bos1921=%p,id=%d,nbrOfSlices=%zu\n", bos1921, id,
		    nbrOfSlices);
		res = -1;
	} else {
		res = writeRam(bos1921, WSF_COMMAND_RAM_ACCESS, command,
			       sizeof(command));
	}

	return res;
}

static int setConfig(struct bos1921 *bos1921)
{
	int res = 0;
	DBG("enter,mode=%d\n", bos1921->mode);
	if (bos1921->mode == MICRO_PUMP_MODE_SYNTHESIZER) {
		res = bos1921SetSlice(bos1921, 0);
		res = !res && bos1921SetWaveforms(bos1921, 0, 0, 1, 1, 1);
		if (res) {
			DBG_ERR("bos1921SetSlice or bos1921SetWaveforms fail\n");
		}
	} else if (bos1921->mode == MICRO_PUMP_MODE_WAV) {
		DBG_ERR("Never Implement\n");
	}

	return res;
}

static int bos1921CtrlOutput(struct bos1921 *bos1921, int activate)
{
	int res = 0;
	VALUE(BOS1921_REGISTER_CONFIG_REG) =
		BOS1921_CONFIG_REG_CLEAR_OE(VALUE(BOS1921_REGISTER_CONFIG_REG));
	VALUE(BOS1921_REGISTER_CONFIG_REG) |=
		BOS1921_CONFIG_REG_WRITE_OE((activate ? 1 : 0));
	writeReg(BOS1921_REGISTER_CONFIG_REG,
		 VALUE(BOS1921_REGISTER_CONFIG_REG));
	return res;
}

static int start(struct bos1921 *bos1921)
{
	int res = -1;
	DBG("enter,mode=%d\n", bos1921->mode);
	if (bos1921->mode == MICRO_PUMP_MODE_SYNTHESIZER) {
		res = bos1921SynthesizerPlay(bos1921, 0, 0);
		res = !res && bos1921CtrlOutput(bos1921, 1);
	} else if (bos1921->mode == MICRO_PUMP_MODE_WAV) {
		DBG_ERR("Never Implement\n");
	} else {
		DBG_ERR("unsupport mode=%d\n", bos1921->mode);
	}
	return res;
}

static int stop(struct bos1921 *bos1921)
{
	int res = 0;
	DBG("enter,mode=%d\n", bos1921->mode);
	if (bos1921->mode == MICRO_PUMP_MODE_SYNTHESIZER) {
		res = bos1921SynthesizerStop(bos1921);
	} else if (bos1921->mode == MICRO_PUMP_MODE_WAV) {
		DBG_ERR("Never Implement\n");
	} else {
		res = -1;
	}
	return res;
}

static int enableMicroPump(struct bos1921 *bos1921)
{
	int res = 0;
	DBG("enter\n");
	res = setConfig(bos1921);
	res = !res && start(bos1921);
	if (0 == res) {
		bos1921->enabled = 1;
	}
	return res;
}

static int disableMicroPump(struct bos1921 *bos1921)
{
	int res = 0;
	DBG("enter\n");
	res = stop(bos1921);
	if (0 == res) {
		bos1921->enabled = 0;
	}
	return res;
}

static ssize_t enable_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	ssize_t len = 0;
	struct bos1921 *bos1921 = mBOS1921;
	len += snprintf(buf + len, PAGE_SIZE - len, "%d\n", bos1921->enabled);
	return len;
}

//1:Low sensitivity, 2: Medium sensitivity, 3: high
static ssize_t enable_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	uint32_t value = 0;
	struct bos1921 *bos1921 = mBOS1921;
	if (sscanf(buf, "%d", &value) != 1) {
		DBG_ERR("sscanf parse err");
		return count;
	}
	DBG("value = %d", value);
	if (value) {
		enableMicroPump(bos1921);
	} else {
		disableMicroPump(bos1921);
	}
	return count;
}

static DEVICE_ATTR(enableaa, 0664, enable_show, enable_store);

static struct attribute *bos1921_attributes[] = { &dev_attr_enableaa.attr,
						  NULL };

static struct attribute_group bos1921_attribute_group = {
	.attrs = bos1921_attributes
};

static ssize_t enable_state_show(struct file *file, char __user *buffer,
				 size_t count, loff_t *offset)
{
	ssize_t len = 0;
	struct bos1921 *bos1921 = mBOS1921;
	uint8_t data_buf[30] = { 0 };

	if (*offset != 0) {
		return 0;
	}

	DBG("%s val:%d.\n", __func__, bos1921->enabled);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", bos1921->enabled);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t enable_state_store(struct file *file, const char __user *buffer,
				  size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = { 0 };
	struct bos1921 *bos1921 = mBOS1921;

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;
	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;
	input = input > 0 ? 1 : 0;
	bos1921->enabled = input;
	DBG("%s val %d.\n", __func__, input);
	if (input) {
		enableMicroPump(bos1921);
	} else {
		disableMicroPump(bos1921);
	}
	return len;
}
static const struct proc_ops proc_ops_micropump = {
	.proc_read = enable_state_show,
	.proc_write = enable_state_store,
};

static ssize_t speed_show(struct file *file, char __user *buffer,
				 size_t count, loff_t *offset)
{
	ssize_t len = 0;
	struct bos1921 *bos1921 = mBOS1921;
	uint8_t data_buf[30] = { 0 };

	if (*offset != 0) {
		return 0;
	}

	DBG("%s val:%d.\n", __func__, bos1921->speed);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", bos1921->speed);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t speed_store(struct file *file, const char __user *buffer,
				  size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = { 0 };
	struct bos1921 *bos1921 = mBOS1921;

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;
	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;

	bos1921->speed = input;
	DBG("%s val %d.\n", __func__, input);


	if((bos1921->speed > 49) && (bos1921->speed < 191)){
	   bos1921->slice.mVAmp = bos1921->speed * 1000;
	}else{
	   bos1921->slice.mVAmp = LOW_SYNTHESIZER_AMPLITUDE_V * 1000;
	}
	DBG("speed =%d,mVAmp=%d",bos1921->speed,bos1921->slice.mVAmp);
    disableMicroPump(bos1921);
	//msleep(900);
	usleep_range(600000, 650000);
	enableMicroPump(bos1921);


	return len;
}
static const struct proc_ops proc_ops_speed = {
	.proc_read = speed_show,
	.proc_write = speed_store,
};

static ssize_t freq_show(struct file *file, char __user *buffer,
				 size_t count, loff_t *offset)
{
	ssize_t len = 0;
	struct bos1921 *bos1921 = mBOS1921;
	uint8_t data_buf[30] = { 0 };

	if (*offset != 0) {
		return 0;
	}

	DBG("%s val:%d.\n", __func__, bos1921->freq);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", bos1921->freq);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t freq_store(struct file *file, const char __user *buffer,
				  size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = { 0 };
	struct bos1921 *bos1921 = mBOS1921;

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;
	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;

	bos1921->freq = input;
	DBG("%s val %d.\n", __func__, input);


	if((bos1921->freq > 40) && (bos1921->freq < 201)){
	   bos1921->slice.mHzFreq = bos1921->freq * 1000;
	}else{
	   bos1921->slice.mHzFreq =	DEFAULT_SYNTHESIZER_FREQUENCY_HZ * 1000;
	}
	DBG("freq =%d,mHzFreq=%d",bos1921->freq,bos1921->slice.mHzFreq);
    disableMicroPump(bos1921);
	//msleep(500);
	usleep_range(600000, 650000);
	enableMicroPump(bos1921);


	return len;
}
static const struct proc_ops proc_ops_freq = {
	.proc_read = freq_show,
	.proc_write = freq_store,
};

static void create_pump_proc_entry(void)
{
	struct proc_dir_entry *pump_proc_entry = NULL;

	/*micro_pump_proc_entry = proc_mkdir(PROC_MICRO_PUMP_DIR, NULL);
	if (micro_pump_proc_entry == NULL) {
		pr_err("%s: mkdir micropump failed!\n",  __func__);
		return;
	}
	pump_enable_proc_entry = proc_create(PROC_PUMP_ENABLEAA, 0664,  micro_pump_proc_entry, &proc_ops_micropump);
	*/
	pump_proc_entry = proc_create(PROC_MICRO_PUMP_ENABLE, 0664, NULL,
					     &proc_ops_micropump);
	if (pump_proc_entry == NULL)
		pr_err("proc_create enable failed!\n");

	pump_proc_entry = proc_create(PROC_MICRO_PUMP_SPEED, 0664, NULL,
					     &proc_ops_speed);
	if (pump_proc_entry == NULL)
		pr_err("proc_create speed failed!\n");

	pump_proc_entry = proc_create(PROC_MICRO_PUMP_FREQ, 0664, NULL,
					     &proc_ops_freq);
	if (pump_proc_entry == NULL)
		pr_err("proc_create freq failed!\n");
}

static void pump_proc_deinit(void)
{
	/*if (micro_pump_proc_entry == NULL) {
		pr_err("%s: proc/micropump is NULL!\n",  __func__);
		return;
	}
	remove_proc_entry(PROC_PUMP_ENABLEAA, micro_pump_proc_entry);
	remove_proc_entry(PROC_MICRO_PUMP_DIR, NULL);*/

	remove_proc_entry(PROC_MICRO_PUMP_ENABLE, NULL);
	remove_proc_entry(PROC_MICRO_PUMP_SPEED, NULL);
	remove_proc_entry(PROC_MICRO_PUMP_FREQ, NULL);
}

static int32_t bos1921_parse_dt(struct device *dev, struct bos1921 *bos1921,
				struct device_node *np)
{
	bos1921->irq_gpio = of_get_named_gpio(np, "irq-gpio", 0);
	if (bos1921->irq_gpio < 0) {
		bos1921->irq_gpio = -1;
		DBG_ERR("no irq gpio provided.");
	} else {
		DBG("irq gpio provided ok.");
	}

	return 0;
}

static int32_t bos1921_power_init(struct bos1921 *bos1921)
{
	int32_t rc = 0;
	DBG("enter");

	bos1921->vcc2_1P8 = regulator_get(bos1921->dev, "pm_humu_l15");
	if (IS_ERR(bos1921->vcc2_1P8)) {
		rc = PTR_ERR(bos1921->vcc2_1P8);
		DBG_ERR("regulator get failed vcc2_1P8 rc = %d", rc);
		return rc;
	}

	if (regulator_count_voltages(bos1921->vcc2_1P8) > 0) {
		rc = regulator_set_voltage(bos1921->vcc2_1P8, 1800000, 1800000);
		if (rc) {
			DBG_ERR("regulator set vcc2_1P8 vol failed rc = %d",
				rc);
			goto reg_vcc_put;
		}
	} else {
		DBG_ERR("regulator_count_voltages vcc2_1P8 <= 0");
	}

	bos1921->vcc1_3P3 = regulator_get(bos1921->dev, "pm_humu_l9");
	if (IS_ERR(bos1921->vcc1_3P3)) {
		rc = PTR_ERR(bos1921->vcc1_3P3);
		DBG_ERR("regulator get failed vcc1_3P3 rc = %d", rc);
		return rc;
	}

	if (regulator_count_voltages(bos1921->vcc1_3P3) > 0) {
		rc = regulator_set_voltage(bos1921->vcc1_3P3, 3300000, 3300000);
		if (rc) {
			DBG_ERR("regulator set vcc1_3P3 vol failed rc = %d",
				rc);
			goto reg_vcc_put;
		}
	} else {
		DBG_ERR("regulator_count_voltages vcc1_3P3 <= 0");
	}

	return rc;

reg_vcc_put:
	regulator_put(bos1921->vcc1_3P3);
	regulator_put(bos1921->vcc2_1P8);
	return rc;
}

static int bos1921_i2c_probe(struct i2c_client *i2c)
{
	struct bos1921 *bos1921 = NULL;
	int32_t ret = -1;
	BosState state;

	pr_info("%s enter", __func__);

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		DBG_ERR("check_functionality failed");
		return -EIO;
	}

	mBOS1921 = bos1921 =
		devm_kzalloc(&i2c->dev, sizeof(struct bos1921), GFP_KERNEL);
	if (bos1921 == NULL) {
		DBG_ERR("failed to malloc memory!");
		ret = -1;
		goto err_init1;
	}

	bos1921->dev = &i2c->dev;
	bos1921->i2c = i2c;
	i2c_set_clientdata(i2c, bos1921);

	//1.parse dts
	ret = bos1921_parse_dt(&i2c->dev, bos1921, i2c->dev.of_node);
	if (ret) {
		DBG_ERR("irq gpio error!, ret = %d", ret);
		goto err_init1;
	}

	bos1921_power_init(bos1921);

	//3.Create file node
	ret = sysfs_create_group(&bos1921->dev->kobj, &bos1921_attribute_group);
	if (ret < 0) {
		DBG_ERR("error creating sysfs attr files");
		goto err_init1;
	}

	create_pump_proc_entry();

	if (0 != (ret = read_chipid(bos1921))) {
		DBG_ERR("read chipid failed, ret=%d, ", ret);
		goto err_init2;
	} else {
		DBG_ERR("read chipid ok!");
	}

	bos1921->sleepEnabled = 1;
	bos1921->speed =1;

	if (0 != (ret = soft_reset(bos1921))) {
		DBG_ERR("soft reset failed, ret=%d", ret);
		goto err_init2;
	}

	if (0 != (ret = setDefaultConfig(bos1921))) {
		DBG_ERR("setDefaultConfig failed, ret=%d", ret);
		goto err_init2;
	}
	dumpAllRegister(bos1921);

	if ((0 == (ret = bos1921GetState(bos1921, &state))) &&
	    (state != BOS_STATE_ERROR)) {
		DBG("init success");
		bos1921->slice.sliceId = 0;
		bos1921->slice.mVAmp = DEFAULT_SYNTHESIZER_AMPLITUDE_V * 1000;
		bos1921->slice.mHzFreq =
			DEFAULT_SYNTHESIZER_FREQUENCY_HZ * 1000;
		bos1921->slice.cycle =
			1; //Using continuous mode, this value will be ignored
		bos1921->slice.shapeUpMs = DEFAULT_SYNTHESIZER_SHAPE_UP_MS;
		bos1921->slice.shapeDownMs = DEFAULT_SYNTHESIZER_SHAPE_DOWN_MS;
		bos1921->slice.continuous = 1;
		bos1921->mode = MICRO_PUMP_MODE_SYNTHESIZER;

		//enableMicroPump(bos1921);
		disableMicroPump(bos1921);
		dumpAllRegister(bos1921);
		DBG("sleep 1000ms");
		//msleep(1000);
		usleep_range(450000, 500000);
		dumpAllRegister(bos1921);

		return 0;
	}
	DBG_ERR("bos1921GetState failed,ret=%d,state=%d", ret, state);

err_init2:
	sysfs_remove_group(&i2c->dev.kobj, &bos1921_attribute_group);
	pump_proc_deinit();
err_init1:
	return ret;
}

static void bos1921_i2c_remove(struct i2c_client *i2c)
{
	//struct bos1921* bos1921 = i2c_get_clientdata(i2c);
	sysfs_remove_group(&i2c->dev.kobj, &bos1921_attribute_group);
	pump_proc_deinit();
}

static void bos1921_i2c_shutdown(struct i2c_client *i2c)
{
	pr_info("%s enter", __func__);
	//bos1921Sleep(mBOS1921);
}

static const struct of_device_id bos1921_dt_match[] = {
	{ .compatible = "zte,bos1921" },
	{},
};

static const struct i2c_device_id bos1921_i2c_id[] = { { BOS1921_I2C_NAME, 0 },
						       {} };
MODULE_DEVICE_TABLE(i2c, bos1921_i2c_id);

static struct i2c_driver bos1921_i2c_driver = {
	.driver = {
		.name = BOS1921_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(bos1921_dt_match),
	},
	.probe = bos1921_i2c_probe,
	.remove = bos1921_i2c_remove,
	.shutdown = bos1921_i2c_shutdown,
	.id_table = bos1921_i2c_id,
};

static int __init bos1921_i2c_init(void)
{
	int32_t ret = 0;

	pr_info("bos1921 driver version %s\n", BOS1921_DRIVER_VERSION);

	ret = i2c_add_driver(&bos1921_i2c_driver);
	if (ret) {
		pr_err("fail to add bos1921 device into i2c\n");
		return ret;
	}

	return 0;
}

static void __exit bos1921_i2c_exit(void)
{
	i2c_del_driver(&bos1921_i2c_driver);
}

late_initcall(bos1921_i2c_init);
module_exit(bos1921_i2c_exit);
MODULE_DESCRIPTION("BOS1921 Driver");

MODULE_LICENSE("GPL v2");
