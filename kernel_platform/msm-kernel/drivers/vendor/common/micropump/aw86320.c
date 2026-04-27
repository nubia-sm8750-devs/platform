// SPDX-License-Identifier: GPL-2.0
/*
 * Awinic high voltage haptic driver
 *
 * Copyright (c) 2025 awinic. All Rights Reserved.
 *
 * Author: Joseph <zhangzetao@awinic.com>
 */

#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/firmware.h>
#include <linux/proc_fs.h>
#include <linux/mman.h>

#include "aw86320.h"
#include "aw86320_reg.h"

#define PROC_MICRO_PUMP_DIR    "driver/micropump"
#define PROC_MICRO_PUMP_ID    "icid"
#define PROC_MICRO_PUMP_ENABLE "enable"
#define PROC_MICRO_PUMP_SPEED  "speed"
#define PROC_MICRO_PUMP_FREQ   "freq"
#define PROC_MICRO_PUMP_REG    "reg"
#define PROC_MICRO_PUMP_UPDATE "update"
#define PROC_MICRO_PUMP_TIME   "time"

#define AW86320_DRIVER_VERSION	"V0.0.1"

static char *aw_ram_name = "micop_ram.bin"; //"haptic_ram.bin";
static char g_chip_id[32] = "-1";
static struct aw_haptic *g_aw86320;
static struct proc_dir_entry *micro_pump_proc_entry = NULL;
/*********************************************************
 *
 * I2C Read/Write
 *
 *********************************************************/
static int haptic_i2c_writes(struct aw_haptic *aw_haptic, uint8_t reg_addr, uint8_t *buf, uint32_t len)
{
	uint8_t *data = NULL;
	int ret = -1;

	data = kmalloc(len + 1, GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	data[0] = reg_addr;
	memcpy(&data[1], buf, len);
	ret = i2c_master_send(aw_haptic->i2c, data, len + 1);
	if (ret < 0)
		aw_err("i2c master send 0x%02x err", reg_addr);
	kfree(data);

	return ret;
}

static int haptic_i2c_reads(struct aw_haptic *aw_haptic, uint8_t reg_addr, uint8_t *buf, uint32_t len)
{
	int ret;
	struct i2c_msg msg[] = {
		[0] = {
			.addr = aw_haptic->i2c->addr,
			.flags = 0,
			.len = sizeof(uint8_t),
			.buf = &reg_addr,
			},
		[1] = {
			.addr = aw_haptic->i2c->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
			},
	};

	ret = i2c_transfer(aw_haptic->i2c->adapter, msg, ARRAY_SIZE(msg));
	if (ret < 0) {
		aw_err("transfer failed.");
		return ret;
	} else if (ret != AW_I2C_READ_MSG_NUM) {
		aw_err("transfer failed(size error).");
		return -ENXIO;
	}

	return ret;
}


static int haptic_i2c_write(struct aw_haptic *aw_haptic, uint8_t reg_addr, uint16_t reg_data)
{
	int ret = -1;
	uint8_t cnt = 0;
	uint8_t buf[2];

	buf[0] = (reg_data&0xff00)>>8;
	buf[1] = (reg_data&0x00ff)>>0;

	while (cnt < AW_I2C_RETRIES) {
		ret = haptic_i2c_writes(aw_haptic, reg_addr, buf, 2);
		/*ret = i2c_smbus_write_word_data(aw_haptic->i2c, reg_addr, reg_data);*/
		if (ret < 0) {
			aw_err("i2c_write cnt=%d error=%d", cnt, ret);
		} else {
			/* if (g_print_dbg)*/
			/*aw_info("reg_addr: 0x%02x, reg_data :0x%04x", reg_addr, reg_data);*/
			break;
		}
		cnt++;
	}

	return ret;
}

static int i2c_reads(struct aw_haptic *aw_haptic, uint8_t reg_addr, uint8_t *buf, uint32_t len)
{
	int ret;
	struct i2c_msg msg[] = {
		[0] = {
			.addr = aw_haptic->i2c->addr,
			.flags = 0,
			.len = sizeof(uint8_t),
			.buf = &reg_addr,
			},
		[1] = {
			.addr = aw_haptic->i2c->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
			},
	};

	ret = i2c_transfer(aw_haptic->i2c->adapter, msg, ARRAY_SIZE(msg));
	if (ret < 0) {
		aw_err("transfer failed.");
		return ret;
	} else if (ret != AW_I2C_READ_MSG_NUM) {
		aw_err("transfer failed(size error).");
		return -ENXIO;
	}

	return ret;
}

static int haptic_i2c_read(struct aw_haptic *aw_haptic, unsigned char reg_addr, uint16_t *reg_data)
{
	int ret = -1;
	unsigned char cnt = 0;
	unsigned char buf[2];

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_reads(aw_haptic, reg_addr, buf, 2);
		if (ret < 0) {
			aw_err("i2c_read cnt=%d error=%d", cnt, ret);
		} else {
			*reg_data = (buf[0]<<8) | (buf[1]<<0);
			/*aw_info("reg_addr: 0x%02x, reg_data :0x%04x", (uint8_t)reg_addr, (uint16_t)(*reg_data));*/
			/*if (g_print_dbg) */
			/*	aw_info("reg_addr: 0x%02x, reg_data :0x%04x",*/
			/*			(uint8_t)reg_addr, (uint16_t)(*reg_data));*/
			/*aw_err("i2c_read ret=%d", ret);*/
			break;
		}
		cnt++;
	}

	return ret;
}
static int haptic_i2c_write_bits(struct aw_haptic *aw_haptic, uint8_t reg_addr, uint32_t mask, uint16_t reg_data)
{
	uint16_t reg_val = 0;
	int ret = -1;

	ret = haptic_i2c_read(aw_haptic, reg_addr, &reg_val);
	if (ret < 0) {
		aw_err("i2c read error, ret=%d", ret);
		return ret;
	}

	reg_val &= mask;
	reg_val |= (reg_data & (~mask));
	ret = haptic_i2c_write(aw_haptic, reg_addr, reg_val);

	if (ret < 0) {
		aw_err("i2c write error, ret=%d", ret);
		return ret;
	}

	return 0;
}

static void sw_reset(struct aw_haptic *aw_haptic)
{
	aw_dbg("enter");
	haptic_i2c_write(aw_haptic, AW86320_CLK_REG1_REG, 0xAAAA);
	usleep_range(3000, 3500);
}

static int read_chipid(struct aw_haptic *aw_haptic, uint16_t *reg_val)
{
	int ret = -1;

	ret = haptic_i2c_read(aw_haptic, AW_REG_CHIPID, reg_val);
	
	if (ret < 0) {
		aw_err("failed to read REG_ID: %d", ret);
		return -EIO;
	}

	return ret;
}

static int parse_chipid(struct aw_haptic *aw_haptic)
{
	int ret = -1;
	uint16_t reg = 0;
	uint8_t cnt = 0;

	sw_reset(aw_haptic);
	for (cnt = 0; cnt < AW_READ_CHIPID_RETRIES; cnt++) {
		ret = read_chipid(aw_haptic, &reg);
		if (ret < 0)
			aw_err("read chip id fail: %d", ret);
		switch (reg) {
		case AW86230_CHIPID:
			aw_haptic->chipid = AW86230_CHIPID;
			strcpy(g_chip_id,"aw86320");
			aw_info("detected aw86320.");
			return ret;
		default:
			aw_info("unsupport device revision (0x%04X)", reg);
			break;
		}
		usleep_range(2000, 2500);
	}

	return -EINVAL;
}

static void aw86320_set_pwm(struct aw_haptic *aw_haptic, uint8_t mode)
{
	switch (mode) {
	case AW_PWM_48K:
		haptic_i2c_write_bits(aw_haptic, AW86320_CLK_REG2_REG,
					 AW86320_SLOT_FS_MASK,
					 AW86320_SLOT_FS_48KHZ_VALUE);
		break;
	case AW_PWM_24K:
		haptic_i2c_write_bits(aw_haptic, AW86320_CLK_REG2_REG,
					 AW86320_SLOT_FS_MASK,
					 AW86320_SLOT_FS_24KHZ_VALUE);
		break;
	case AW_PWM_12K:
		haptic_i2c_write_bits(aw_haptic, AW86320_CLK_REG2_REG,
					 AW86320_SLOT_FS_MASK,
					 AW86320_SLOT_FS_12KHZ_VALUE);
		break;
	default:
		break;
	}
}

static void aw86320_ram_init(struct aw_haptic *aw_haptic, bool flag)
{
	if (flag == true) {
		haptic_i2c_write_bits(aw_haptic, AW86320_GLB_REG2_REG, (~(1<<4)), (1<<4));
		usleep_range(1000, 1050);
	} else {
		haptic_i2c_write_bits(aw_haptic, AW86320_GLB_REG2_REG, (~(1<<4)), (0<<4));
	}
}

static void aw86320_misc_para_init(struct aw_haptic *aw_haptic)
{
	/*init for power on*/
	haptic_i2c_write(aw_haptic, 0x00, 0xAAAA);
	haptic_i2c_write(aw_haptic, 0x40, 0x7D7D);
	haptic_i2c_write(aw_haptic, 0x12, 0x1108);
	haptic_i2c_write(aw_haptic, 0x36, 0x2825);
	haptic_i2c_write(aw_haptic, 0x42, 0x1800);
	haptic_i2c_write(aw_haptic, 0x46, 0x103A);
	haptic_i2c_write(aw_haptic, 0x47, 0x13e5);
	haptic_i2c_write(aw_haptic, 0x48, 0x869C);
	haptic_i2c_write(aw_haptic, 0x4A, 0x0805);
	haptic_i2c_write(aw_haptic, 0x4B, 0x8000);
}

static void aw86320_play_mode(struct aw_haptic *aw_haptic, uint8_t play_mode)
{
	switch (play_mode) {
	case AW_STANDBY_MODE:
		aw_info("enter standby mode");
		haptic_i2c_write(aw_haptic, AW86320_SRAM_REG5_REG, 0X0E0E);
		haptic_i2c_write(aw_haptic, AW86320_REG_REG1_REG, 0X0000);
		mdelay(3);
		break;
	case AW_ATSIN0_MODE:
		aw_info("enter atsin0 mode");
		if (aw_haptic->info.use_reg_method_play == true)
			aw_haptic->play_type = AW_REG_PLAY_TYPE;
		if (aw_haptic->info.use_fifo_method_play == true)
			aw_haptic->play_type = AW_FIFO_PLAY_TYPE;
		break;
	default:
		aw_err("play mode %d err", play_mode);
		break;
	}
}

static int aw86320_container_update(struct aw_haptic *aw_haptic,
	struct aw_haptic_container *aw_fw)
{
	uint16_t ae_thr = 0;
	uint16_t af_thr = 0;
	#ifdef AW_CHECK_RAM_DATA
	uint8_t ram_data[700] = {0};
	int i = 0;
	uint32_t check_sum = 0;
	#endif

	mutex_lock(&aw_haptic->lock);
	aw86320_ram_init(aw_haptic, true);
	aw_haptic->ram.base_addr = aw_haptic->ram_data.ram_base_addr;
	af_thr = aw_haptic->ram_data.fifo_af_addr;
	ae_thr = aw_haptic->ram_data.fifo_ae_addr;
	aw_info("base_addr=0x%04x,af_thr=0x%04x,ae_thr=0x%04x",
			aw_haptic->ram.base_addr, af_thr, ae_thr);
	haptic_i2c_write_bits(aw_haptic,
				AW86320_SRAM_REG1_REG,
				AW86320_BASE_ADDR_MASK,
				aw_haptic->ram.base_addr);
	haptic_i2c_write(aw_haptic, AW86320_SRAM_REG2_REG, af_thr);
	haptic_i2c_write(aw_haptic, AW86320_SRAM_REG3_REG, ae_thr);
	/* ram */
	aw_info("aw_fw->len = %d", aw_fw->len);
	/* for (i = 0; i < aw_fw->len; i++)*/
	/*	aw_info("write_ram_data[%d] = 0x%x",i, aw_fw->data[i]);*/
	haptic_i2c_writes(aw_haptic, AW86320_RAMDATA_REG, aw_fw->data, aw_fw->len);
	mdelay(5);
	#ifdef AW_CHECK_RAM_DATA
	/* ram check */
	haptic_i2c_write(aw_haptic, AW86320_RAM_ADDR_REG, 0x200);
	haptic_i2c_reads(aw_haptic, AW86320_RAMDATA_REG, ram_data, aw_fw->len);
	for (i = 0; i < aw_fw->len; i++)
		check_sum += ram_data[i];
	if (aw_haptic->ram.check_sum == check_sum)
		aw_info("ram data check sum pass");
	else {
		/* RAMINIT Disable */
		aw86320_ram_init(aw_haptic, false);
		mutex_unlock(&aw_haptic->lock);
		aw_err("ram data check sum error");
		return -ERANGE;
	}
	#endif
	/* RAMINIT Disable */
	aw86320_ram_init(aw_haptic, false);
	mutex_unlock(&aw_haptic->lock);

	return 0;
}

static void aw86320_interrupt_setup(struct aw_haptic *aw_haptic)
{
	haptic_i2c_write_bits(aw_haptic, AW86320_I2C_REG3_REG,
				AW86320_BST_OVPM_MASK, AW86320_BST_OVPM_OPEN_VALUE);
	haptic_i2c_write_bits(aw_haptic, AW86320_I2C_REG3_REG,
				AW86320_BST_SCPM_MASK, AW86320_BST_SCPM_OPEN_VALUE);
	haptic_i2c_write_bits(aw_haptic, AW86320_I2C_REG3_REG,
				AW86320_OTPM_MASK, AW86320_OTPM_NO_VALUE);
	haptic_i2c_write_bits(aw_haptic, AW86320_I2C_REG3_REG,
				AW86320_UVLOM_MASK, AW86320_UVLOM_NO_VALUE);
	haptic_i2c_write_bits(aw_haptic, AW86320_I2C_REG3_REG,
				AW86320_OCM_MASK, AW86320_OCM_OPEN_VALUE);
	haptic_i2c_write_bits(aw_haptic, AW86320_I2C_REG3_REG,
				AW86320_DONEM_MASK, AW86320_DONEM_OPEN_VALUE);
	haptic_i2c_write_bits(aw_haptic, AW86320_I2C_REG3_REG,
				AW86320_CMD_ERRM_MASK, AW86320_CMD_ERRM_OPEN_VALUE);
}

static void aw86320_get_irq_state(struct aw_haptic *aw_haptic)
{
	uint16_t reg_val = 0;

	haptic_i2c_read(aw_haptic, AW86320_I2C_REG2_REG, &reg_val);
	aw_info("interrupt state = 0x%04x", reg_val);

	if (reg_val & (1 << 9))
		aw_info("chip BST_OVPC");
	if (reg_val & (1 << 8))
		aw_info("chip BST_SCPC");
	if (reg_val & (1 << 7))
		aw_info("chip OTPC");
	if (reg_val & (1 << 6))
		aw_err("chip UVLOC error");
	if (reg_val & (1 << 5))
		aw_err("chip OCC eint error");
	if (reg_val & (1 << 4))
		aw_err("chip DONEC eint error");
	if (reg_val & (1 << 3))
		aw_err("chip CMD_ERRC eint error");
}

static irqreturn_t irq_handle(int irq, void *data)
{
	struct aw_haptic *aw_haptic = data;

	aw_info("irq_handle_enter");
	aw86320_get_irq_state(aw_haptic);

	return IRQ_HANDLED;
}

static int haptic_parsing_bin_file(struct aw_haptic *aw_haptic, const struct firmware *cont)
{
	int i = 0;

	aw_haptic->ram_data.ram_base_addr = cont->data[5] << 8 | cont->data[4];
	aw_haptic->ram_data.fifo_ae_addr = cont->data[7] << 8 | cont->data[6];
	aw_haptic->ram_data.fifo_af_addr = cont->data[9] << 8 | cont->data[8];
	aw_haptic->ram_data.ram_u8data_size = cont->data[11] << 8 | cont->data[10];

	aw_haptic->ram_data.wavseq_cnt = cont->data[12];
	for (i = 1; i <= aw_haptic->ram_data.wavseq_cnt; i++) {
		aw_haptic->ram_data.wavseq_type[i] = cont->data[12 + i];
		aw_info("wavseq_cnt = %d,The %d wavseq_type is %d",
			aw_haptic->ram_data.wavseq_cnt, i, aw_haptic->ram_data.wavseq_type[i]);
	}

	return 0;
}

static void ram_load(const struct firmware *cont, void *context)
{
	uint32_t check_sum = 0;
	int i = 0;
	int ret = 0;
	int wave_cnt = 0;
	struct aw_haptic *aw_haptic = context;
	struct aw_haptic_container *aw_fw;

	if (!cont) {
		aw_err("failed to read %s", aw_ram_name);
		release_firmware(cont);
		return;
	}
	aw_info("loaded %s - size: %zu", aw_ram_name, cont ? cont->size : 0);
	/* check sum */
	wave_cnt = cont->data[12];
	for (i = wave_cnt + 21; i < cont->size; i++) {
		check_sum += cont->data[i];
		//aw_info("cont->data[%d] = 0x%x",i,cont->data[i]);
	}

	if (check_sum != (uint32_t)((cont->data[3] << 24) | (cont->data[2] << 16)
		| (cont->data[1] << 8) | (cont->data[0]))) {
		aw_err("check sum err: check_sum=0x%04x", check_sum);
		release_firmware(cont);
		return;
	}
	aw_info("check sum pass : 0x%04x", check_sum);
	aw_haptic->ram.check_sum = check_sum;

	haptic_parsing_bin_file(aw_haptic, cont);

	/* aw ram update */
	aw_fw = kzalloc(aw_haptic->ram_data.ram_u8data_size + sizeof(int), GFP_KERNEL);
	if (!aw_fw) {
		release_firmware(cont);
		aw_err("Error allocating memory");
		return;
	}

	aw_fw->len = aw_haptic->ram_data.ram_u8data_size;
	aw_info("aw_fw_len = %d", aw_fw->len);
	memcpy(aw_fw->data, cont->data + wave_cnt + 21, aw_fw->len);
	release_firmware(cont);
	ret = aw86320_container_update(aw_haptic, aw_fw);
	if (ret) {
		aw_err("ram firmware update failed!");
	} else {
		aw_haptic->ram_init = true;
		aw_info("ram firmware update complete!");

	}
	kfree(aw_fw);
}

static int ram_update(struct aw_haptic *aw_haptic)
{
	aw_haptic->ram_init = false;
	return request_firmware_nowait(THIS_MODULE, 1, aw_ram_name, aw_haptic->dev, GFP_KERNEL,
				       aw_haptic, ram_load);
}

static void ram_work_routine(struct work_struct *work)
{
	struct aw_haptic *aw_haptic = container_of(work, struct aw_haptic, ram_work.work);

	ram_update(aw_haptic);
}

static void ram_work_init(struct aw_haptic *aw_haptic)
{
	int ram_timer_val = AW_RAM_WORK_DELAY_INTERVAL;

	INIT_DELAYED_WORK(&aw_haptic->ram_work, ram_work_routine);
	schedule_delayed_work(&aw_haptic->ram_work, msecs_to_jiffies(ram_timer_val));
}
static void aw86320_timecheck_work_handle(struct work_struct *work)
{
	struct aw_haptic *aw_haptic = g_aw86320;

	aw_haptic->enabled = 0;
	aw_info("enter");
	haptic_i2c_write(aw_haptic, 0x25, 0x0000);
}
static void aw86320_timecheck_work_init(struct aw_haptic *aw_haptic)
{
    if((aw_haptic->duration > 1023)){
		cancel_delayed_work_sync(&aw_haptic->time_check_work);
		INIT_DELAYED_WORK(&aw_haptic->time_check_work, aw86320_timecheck_work_handle);
		schedule_delayed_work(&aw_haptic->time_check_work, msecs_to_jiffies(aw_haptic->duration));
	}
}

static uint16_t aw86320_get_glb_state(struct aw_haptic *aw_haptic)
{
	uint16_t state = 0;

	haptic_i2c_read(aw_haptic, AW86320_GLB_REG4_REG, &state);
	aw_dbg("glb state value is 0x%04X", state);

	return state;
}

static int aw86320_wait_enter_standby(struct aw_haptic *aw_haptic)
{
	uint16_t reg_val = 0;
	int count = 100;

	while (count--) {
		reg_val = aw86320_get_glb_state(aw_haptic);
		if ((reg_val & 0x0f00) == 0x0) {
			aw_info("entered standby!");
			return 0;
		}
		aw_dbg("wait for standby");
		usleep_range(2000, 2500);
	}
	aw_err("do not enter standby automatically");

	return -ERANGE;
}

static void aw86320_play_stop(struct aw_haptic *aw_haptic)
{
	aw86320_play_mode(aw_haptic, AW_STANDBY_MODE);
	aw86320_wait_enter_standby(aw_haptic);
}

void aw86320_play_atsin0(uint16_t time, uint16_t amplitude, uint16_t freq)
{
	uint16_t cmd_time;
	uint16_t data_cfg;

	aw_info("enter time=%d,amplitude=%d,freq=%d",time,amplitude,freq);

	if (amplitude > 90 || amplitude < 0)
		return;
	else
		amplitude = AW_ATSIN0_AMPLITUDE_TO_CODE(amplitude);

	if (freq > 500 || freq < 0)
		return;

	if(time == 0)
	   return;

	if (time >= 1023){
		time = 1023;
		g_aw86320->enabled = 1;
	}else{
		g_aw86320->enabled = 1;
	}

	cmd_time = 0x0A << 10 | time;
	data_cfg = g_aw86320->info.atsin0_polor << 14 | g_aw86320->info.atsin0_phase << 12 | freq;
	aw_info("cmd_time=0x%04x,amplitude=0x%04x,freq=0x%04x", cmd_time, amplitude, data_cfg);
	aw86320_play_mode(g_aw86320, AW_ATSIN0_MODE);

	if (g_aw86320->play_type == AW_REG_PLAY_TYPE) {
		aw_info(" is reg play type %d",AW_REG_PLAY_TYPE);
		aw86320_play_stop(g_aw86320);
		if (g_aw86320->enabled == 1) {
			aw_info(" g_aw86320->enabled = 1");
			aw86320_timecheck_work_init(g_aw86320);
			haptic_i2c_write(g_aw86320, 0x25, 0x0000);
			haptic_i2c_write(g_aw86320, 0x23, amplitude);
			haptic_i2c_write(g_aw86320, 0x24, data_cfg);
			haptic_i2c_write(g_aw86320, 0x25, cmd_time);
		}
	}
}

/*****************************************************
 *
 * device tree
 *
 *****************************************************/
static void
aw86320_parse_dt(struct device *dev, struct aw_haptic *aw_haptic, struct device_node *np)
{
	uint32_t val = 0;
	int rc = 0;
    aw_haptic->power_en = of_get_named_gpio(np, "reset-gpios", 0);
	if (!gpio_is_valid(aw_haptic->power_en)){
		aw_err("no power-en gpio in dtsi.");
	}else{
		rc = gpio_request(aw_haptic->power_en, "awp86320_power_en");
		aw_info("request GPIO:%d, rc:%d", aw_haptic->power_en, rc);
		if (rc < 0){
			aw_err("Failed to request GPIO:%d, ERRNO:%d", aw_haptic->power_en, rc);
		}else{
			aw_info("set power_en 1");
			gpio_direction_output(aw_haptic->power_en, 1);
		}
	}
	/* aw_haptic rst & int */

	aw_haptic->irq_gpio = of_get_named_gpio(np, "irq-gpio", 0);
	if (aw_haptic->irq_gpio < 0) {
		aw_err("no irq gpio provided.");
		aw_haptic->irq_gpio = -1;
	} else {
		aw_info("irq gpio provide ok irq = %d.", aw_haptic->irq_gpio);
	}

    aw_haptic->amplitude = 80;
	aw_haptic->duration = 1023;
	aw_haptic->freq = 450;
	aw_info("default:duration=%d,amplitude=%d,freq=%d",aw_haptic->duration, aw_haptic->amplitude,aw_haptic->freq);

	aw_haptic->info.use_fifo_method_play = of_property_read_bool(np, "is_fifo_method_play");
	aw_info("aw8624x->info.use_fifo_method_play=%d", aw_haptic->info.use_fifo_method_play);

	aw_haptic->info.use_reg_method_play = of_property_read_bool(np, "is_reg_method_play");
	aw_info("aw8624x->info.use_reg_method_play=%d", aw_haptic->info.use_reg_method_play);

	val = of_property_read_u32(np, "atsin0_polor", &aw_haptic->info.atsin0_polor);
	if (val != 0)
		aw_info("atsin0_polor not found");
	val = of_property_read_u32(np, "atsin0_phase", &aw_haptic->info.atsin0_phase);
	if (val != 0)
		aw_info("atsin0_phase not found");
}
static int enableMicroPump(struct aw_haptic *aw_haptic, int on)
{
	int res = 0;
	unsigned int gpio_value = 0;
	bool b_poweron_flag = false;
	
	aw_info("enter,on=%d\n", on);
	
	if (gpio_is_valid(aw_haptic->power_en)){
		gpio_value = gpio_get_value(aw_haptic->power_en);
		if((on > 0) && (1 == gpio_value)){
           b_poweron_flag = false;
		}else{
		  // gpio_direction_output(aw_haptic->power_en, (on ? 1 : 0));
		}
	}

	if(on){
		if(b_poweron_flag){
			aw86320_misc_para_init(aw_haptic);
			aw86320_set_pwm(aw_haptic, AW_PWM_24K);
			aw86320_play_mode(aw_haptic, AW_STANDBY_MODE);
			aw86320_interrupt_setup(aw_haptic);
			ram_work_init(aw_haptic);
		}
		aw86320_play_atsin0(aw_haptic->duration, aw_haptic->amplitude, aw_haptic->freq);
	}else{
		aw_haptic->enabled = 0;
	}

	if(on){
       aw86320_play_atsin0(aw_haptic->duration, aw_haptic->amplitude, aw_haptic->freq);
	}else{
       aw_haptic->enabled = 0;
	   aw86320_play_mode(aw_haptic, AW_STANDBY_MODE);
	}
	return res;
}

static ssize_t ram_update_store(struct file *file, const char __user *buffer,
				  size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = { 0 };
	struct aw_haptic *aw_haptic = g_aw86320;

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;
	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;

	aw_info("%s val %d.\n", __func__, input);

	if (input)
		ram_update(aw_haptic);

	return len;
}
static const struct proc_ops proc_ops_update = {
	.proc_write = ram_update_store,
};

static ssize_t get_ic_id(struct file *file, char __user *buffer, size_t count, loff_t *offset)
{
    if (*offset != 0) {
        return 0;
    }

    aw_info("get_ic_id=%s\n", g_chip_id);
    return simple_read_from_buffer(buffer, count, offset, g_chip_id, strlen(g_chip_id));
}
static const struct proc_ops proc_ops_icid = {
	.proc_read = get_ic_id,
};
static ssize_t enable_show(struct file *file, char __user *buffer,
				 size_t count, loff_t *offset)
{
	ssize_t len = 0;
	struct aw_haptic *aw_haptic = g_aw86320;
	uint8_t data_buf[30] = { 0 };

	if (*offset != 0) {
		return 0;
	}

	aw_info("%s val:%d.\n", __func__, aw_haptic->enabled);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", aw_haptic->enabled);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t enable_store(struct file *file, const char __user *buffer,
				  size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = { 0 };
	struct aw_haptic *aw_haptic = g_aw86320;

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;
	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;

	aw_info("%s val %d.\n", __func__, input);
	if (input > 1) {
		input = 1;
	} 
	if (input < 0) {
		input = 0;
	}
	enableMicroPump(aw_haptic, input);
	return len;
}
static const struct proc_ops proc_ops_enable = {
	.proc_read = enable_show,
	.proc_write = enable_store,
};

static ssize_t speed_show(struct file *file, char __user *buffer,
				 size_t count, loff_t *offset)
{
	ssize_t len = 0;
	struct aw_haptic *aw_haptic = g_aw86320;
	uint8_t data_buf[30] = { 0 };

	if (*offset != 0) {
		return 0;
	}

	aw_info("speed:%d,[0-90].\n", aw_haptic->amplitude);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", aw_haptic->amplitude);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

#define HIGH_VOLTAGE_OUTPUT 80
#define MID_VOLTAGE_OUTPUT 60
#define LOW_VOLTAGE_OUTPUT 30
static ssize_t speed_store(struct file *file, const char __user *buffer,
				  size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = { 0 };
	struct aw_haptic *aw_haptic = g_aw86320;
	unsigned int max_voltage = 90;

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;

	aw_info("data_buf=%s\n", data_buf);
	if(0 == strcmp("high\n", data_buf)){
		input = HIGH_VOLTAGE_OUTPUT;
	}else if(0 == strcmp("mid\n", data_buf)){
		input = MID_VOLTAGE_OUTPUT;
	}else if(0 == strcmp("low\n", data_buf)){
		input = LOW_VOLTAGE_OUTPUT;
	}else{
		ret = kstrtouint(data_buf, 0, &input);
		if (ret)
			return -EINVAL;
	}

	// value: 0-31
	if(input > max_voltage){
		aw_haptic->amplitude = max_voltage;
	}else if(input < 0){
		aw_haptic->amplitude = 0;
	}else{
		aw_haptic->amplitude = input;
	}

	aw_info("input=%d,amplitude=%d\n", input, aw_haptic->amplitude);

	if(aw_haptic->enabled > 0)
		aw86320_play_atsin0(aw_haptic->duration, aw_haptic->amplitude, aw_haptic->freq);

	return len;
}
static const struct proc_ops proc_ops_speed = {
	.proc_read = speed_show,
	.proc_write = speed_store,
};
static ssize_t duration_show(struct file *file, char __user *buffer,
				 size_t count, loff_t *offset)
{
	ssize_t len = 0;
	struct aw_haptic *aw_haptic = g_aw86320;
	uint8_t data_buf[30] = { 0 };

	if (*offset != 0) {
		return 0;
	}

	aw_info("duration:%d,[0-1023].\n", aw_haptic->duration);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", aw_haptic->duration);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}
static ssize_t duration_store(struct file *file, const char __user *buffer,
				  size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = { 0 };
	struct aw_haptic *aw_haptic = g_aw86320;
	unsigned int max_duration = 1023;

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;

	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;

	// value: 0-1023
	if(input > max_duration){
		aw_haptic->duration = max_duration;
	}else if(input < 0){
		aw_haptic->duration = 0;
	}else{
		aw_haptic->duration = input;
	}

	aw_info("input=%d,amplitude=%d\n", input, aw_haptic->duration);

	if(aw_haptic->enabled > 0)
		aw86320_play_atsin0(aw_haptic->duration, aw_haptic->amplitude, aw_haptic->freq);

	return len;
}
static const struct proc_ops proc_ops_duration = {
	.proc_read = duration_show,
	.proc_write = duration_store,
};
static ssize_t reg_show(struct file *file, char __user *buffer,
				 size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t i = 0;
	uint16_t reg_val = 0;
	struct aw_haptic *aw_haptic = g_aw86320;
	uint8_t data_buf[1300] = { 0 };

	if (*offset != 0) {
		return 0;
	}
	for (i = 0; i < AW86320_REG_MAX; i++) {
		haptic_i2c_read(aw_haptic, i, &reg_val);
		len += snprintf(data_buf + len, PAGE_SIZE - len, "reg:0x%02x=0x%04x\n", i, reg_val);
	}

	aw_info("reg:%s\n", data_buf);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}
static ssize_t reg_store(struct file *file, const char __user *buffer,
				  size_t len, loff_t *off)
{
	int ret = 0;
	uint16_t val = 0;
	char buf[15] = { 0 };
	uint32_t databuf[2] = { 0, 0 };
	struct aw_haptic *aw_haptic = g_aw86320;

	len = len >= sizeof(buf) ? sizeof(buf) - 1 : len;
	aw_info("%s len=%zu,sizeof(buf)=%zu.\n", __func__,len, sizeof(buf));

	ret = copy_from_user(buf, buffer, len);
	if (ret)
		return -EINVAL;

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2) {
		val = (uint16_t)databuf[1];
		aw_info("%s reg=%d,val=%d.\n", __func__,(uint8_t)databuf[0], val);
		haptic_i2c_write(aw_haptic, (uint8_t)databuf[0], val);
	}

	return len;
}

static const struct proc_ops proc_ops_reg = {
    .proc_read = reg_show,
	.proc_write = reg_store,
};

static ssize_t freq_show(struct file *file, char __user *buffer,
				 size_t count, loff_t *offset)
{
	ssize_t len = 0;
	struct aw_haptic *aw_haptic = g_aw86320;
	uint8_t data_buf[30] = { 0 };

	if (*offset != 0) {
		return 0;
	}

	aw_info("freq:%d.\n", aw_haptic->freq);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", aw_haptic->freq);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t freq_store(struct file *file, const char __user *buffer,
				  size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = { 0 };
	//struct awp1921 *awp1921 = mAWP1921;
	struct aw_haptic *aw_haptic = g_aw86320;
	unsigned int max_freq_value = 500;

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;
	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;

	if(input > max_freq_value){
		aw_haptic->freq = max_freq_value;
	}else{
		aw_haptic->freq = input;
	}

	//aw_info("%s val %d.\n", __func__, input);

	aw_info("freq =%d",aw_haptic->freq);
	if(aw_haptic->enabled > 0)
		aw86320_play_atsin0(aw_haptic->duration, aw_haptic->amplitude, aw_haptic->freq);

	return len;
}
static const struct proc_ops proc_ops_freq = {
	.proc_read = freq_show,
	.proc_write = freq_store,
};

static void create_pump_proc_entry(void)
{
	struct proc_dir_entry *pump_proc_entry = NULL;

	micro_pump_proc_entry = proc_mkdir(PROC_MICRO_PUMP_DIR, NULL);
	if (micro_pump_proc_entry == NULL) {
		aw_err("%s: mkdir micropump failed!\n",  __func__);
		return;
	}


	pump_proc_entry = proc_create(PROC_MICRO_PUMP_ID, 0664, micro_pump_proc_entry, &proc_ops_icid);
	if (pump_proc_entry == NULL)
		aw_err("proc_create time failed!\n");

	pump_proc_entry = proc_create(PROC_MICRO_PUMP_ENABLE, 0664, micro_pump_proc_entry,
					     &proc_ops_enable);
	if (pump_proc_entry == NULL)
		aw_err("proc_create enable failed!\n");

	pump_proc_entry = proc_create(PROC_MICRO_PUMP_SPEED, 0664, micro_pump_proc_entry,
					     &proc_ops_speed);
	if (pump_proc_entry == NULL)
		aw_err("proc_create speed failed!\n");

	pump_proc_entry = proc_create(PROC_MICRO_PUMP_FREQ, 0664, micro_pump_proc_entry,
					     &proc_ops_freq);
	if (pump_proc_entry == NULL)
		aw_err("proc_create freq failed!\n");

	pump_proc_entry = proc_create(PROC_MICRO_PUMP_REG, 0222, micro_pump_proc_entry,
					     &proc_ops_reg);
	if (pump_proc_entry == NULL)
		aw_err("proc_create reg failed!\n");

	pump_proc_entry = proc_create(PROC_MICRO_PUMP_UPDATE, 0222, micro_pump_proc_entry,
					     &proc_ops_update);
	if (pump_proc_entry == NULL)
		aw_err("proc_create update failed!\n");

	pump_proc_entry = proc_create(PROC_MICRO_PUMP_TIME, 0222, micro_pump_proc_entry,
					     &proc_ops_duration);					
	if (pump_proc_entry == NULL)
		aw_err("proc_create time failed!\n");
}

static void pump_proc_deinit(void)
{
	remove_proc_entry(PROC_MICRO_PUMP_ENABLE, micro_pump_proc_entry);
	remove_proc_entry(PROC_MICRO_PUMP_SPEED, micro_pump_proc_entry);
	remove_proc_entry(PROC_MICRO_PUMP_FREQ, micro_pump_proc_entry);
	remove_proc_entry(PROC_MICRO_PUMP_REG, micro_pump_proc_entry);
	remove_proc_entry(PROC_MICRO_PUMP_UPDATE, micro_pump_proc_entry);
	remove_proc_entry(PROC_MICRO_PUMP_TIME, micro_pump_proc_entry);
	remove_proc_entry(PROC_MICRO_PUMP_DIR, NULL);
}
static int aw_i2c_probe(struct i2c_client *i2c)
{
	int ret = 0;
#ifdef AW_VIM3
	int irq_flags = 0;
#endif
	struct aw_haptic *aw_haptic;
	struct device_node *np = i2c->dev.of_node;

	pr_info("<%s>%s: enter\n", AW_I2C_NAME, __func__);
	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		pr_err("<%s>%s: check_functionality failed\n", AW_I2C_NAME, __func__);
		return -EIO;
	}

	aw_haptic = devm_kzalloc(&i2c->dev, sizeof(struct aw_haptic), GFP_KERNEL);
	if (aw_haptic == NULL)
		return -ENOMEM;

	aw_haptic->dev = &i2c->dev;
	aw_haptic->i2c = i2c;

	i2c_set_clientdata(i2c, aw_haptic);
	dev_set_drvdata(&i2c->dev, aw_haptic);
	aw86320_parse_dt(&i2c->dev, aw_haptic, np);

	irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
	ret = devm_request_threaded_irq(&i2c->dev, i2c->irq,
					NULL, irq_handle, irq_flags,
					"aw86320", aw_haptic);
	if (ret) {
		dev_err(&i2c->dev, "Unable to request IRQ.\n");
		//return ret;
	}

	/* aw_haptic chip id */
	ret = parse_chipid(aw_haptic);
	if (ret < 0) {
		aw_err("parse chipid failed ret=%d", ret);
		goto err_init1;
	}

	g_aw86320 = aw_haptic;
	//aw86320_parse_dt(&i2c->dev, aw_haptic, np);

	create_pump_proc_entry();
	//vibrator_init(aw_haptic);
	//hrtimer_init(&aw_haptic->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	//aw_haptic->timer.function = vibrator_timer_func;

	mutex_init(&aw_haptic->lock);
	aw86320_misc_para_init(aw_haptic);
	aw86320_set_pwm(aw_haptic, AW_PWM_24K);
	aw86320_play_mode(aw_haptic, AW_STANDBY_MODE);
	aw86320_interrupt_setup(aw_haptic);
	ram_work_init(aw_haptic);
	aw_info("probe completed successfully!");

	return 0;
err_init1:
	gpio_direction_output(aw_haptic->power_en, 0);
	gpio_free(aw_haptic->power_en);
	return ret;
}

#ifdef KERNEL_OVER_6_1
static void aw_remove(struct i2c_client *i2c)
#else
static int aw_remove(struct i2c_client *i2c)
#endif
{
	struct aw_haptic *aw_haptic = i2c_get_clientdata(i2c);

	aw_info("enter");
	pump_proc_deinit();
	//devm_led_classdev_unregister(&aw_haptic->i2c->dev, &aw_haptic->vib_dev);
	//hrtimer_cancel(&aw_haptic->timer);
	cancel_delayed_work_sync(&aw_haptic->ram_work);
	cancel_delayed_work_sync(&aw_haptic->time_check_work);
	mutex_destroy(&aw_haptic->lock);

#ifndef KERNEL_OVER_6_1
	return 0;
#endif
}

static void aw_shutdown(struct i2c_client *i2c)
{
	aw_info("enter");
	//return 0;
}

static int aw_i2c_suspend(struct device *dev)
{
	int ret = 0;
	struct aw_haptic *aw_haptic = dev_get_drvdata(dev);

	aw86320_play_stop(aw_haptic);

	return ret;
}

static int aw_i2c_resume(struct device *dev)
{
	int ret = 0;

	return ret;
}

static SIMPLE_DEV_PM_OPS(aw_pm_ops, aw_i2c_suspend, aw_i2c_resume);

static const struct i2c_device_id aw_i2c_id[] = {
	{AW_I2C_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, aw_i2c_id);

static const struct of_device_id aw_dt_match[] = {
	{.compatible = "awinic,aw86320"},
	{},
};

static struct i2c_driver aw_i2c_driver = {
	.driver = {
		   .name = AW_I2C_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(aw_dt_match),
#ifdef CONFIG_PM_SLEEP
		   .pm = &aw_pm_ops,
#endif
		   },
	.probe = aw_i2c_probe,
	.remove = aw_remove,
	.shutdown = aw_shutdown,
	.id_table = aw_i2c_id,
};

static int __init aw_i2c_init(void)
{
	int ret = 0;

	pr_info("<%s>%s: driver version%s\n", AW_I2C_NAME, __func__, AW86320_DRIVER_VERSION);
	ret = i2c_add_driver(&aw_i2c_driver);
	if (ret) {
		pr_err("<%s>%s: fail to add aw_haptic device into i2c\n", AW_I2C_NAME, __func__);
		return ret;
	}

	return 0;
}
//module_init(aw_i2c_init);
late_initcall(aw_i2c_init);
static void __exit aw_i2c_exit(void)
{
	i2c_del_driver(&aw_i2c_driver);
}
module_exit(aw_i2c_exit);

MODULE_DESCRIPTION("AWINIC Haptic Driver");
MODULE_LICENSE("GPL v2");
