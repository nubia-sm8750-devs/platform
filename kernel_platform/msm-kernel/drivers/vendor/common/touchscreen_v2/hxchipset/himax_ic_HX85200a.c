/* SPDX-License-Identifier: GPL-2.0 */
/*  Himax Android Driver Sample Code for HX85200A chipset
 *
 *  Copyright (C) 2024 Himax Corporation.
 *
 *  This software is licensed under the terms of the GNU General Public
 *  License version 2,  as published by the Free Software Foundation,  and
 *  may be copied,  distributed,  and modified under those terms.
 *
 *  This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include "himax_ic_HX85200a.h"
#include "himax_modular.h"

static void hx85200a_chip_init(void)
{
	hx_s_ts->chip_cell_type = CHIP_IS_IN_CELL;
	// hx_s_ts->chip_max_dsram_size = 73728;
	I("%s:IC cell type = %d\n", __func__, hx_s_ts->chip_cell_type);
	(hx_s_ic_data->checksum_type) = HX_TP_BIN_CHECKSUM_CRC;
	/*Himax: Set FW and CFG Flash Address*/
	(hx_s_fwbin->addr_fw_ver_maj) = 59397;  /*0x00E805*/
	(hx_s_fwbin->addr_fw_ver_min) = 59398;  /*0x00E806*/
	(hx_s_fwbin->addr_cfg_ver_maj) = 59648;  /*0x00E900*/
	(hx_s_fwbin->addr_cfg_ver_min) = 59649;  /*0x00E901*/
	(hx_s_fwbin->addr_cid_ver_maj) = 59394;  /*0x00E802*/
	(hx_s_fwbin->addr_cid_ver_min) = 59395;  /*0x00E803*/
	(hx_s_fwbin->addr_cfg_table) = 0x10000;
	/*PANEL_VERSION_ADDR = 59396;*/  /*0x00E804*/
}

struct ic_setup_collect g_hx85200a_setup = {

	._addr_psl = ADDR_PSL,
	._addr_cs_central_state = ADDR_CS_CENTRAL_STATE,

	._addr_system_reset = ADDR_SYSTEM_RESET,
	._addr_leave_safe_mode = ADDR_LEAVE_SAFE_MODE,
	._addr_ctrl_fw = ADDR_CTRL_FW,
	._addr_flag_reset_event = ADDR_FLAG_RESET_EVENT,
	._func_hsen = HX85200AFUNC_HSEN,
	._func_smwp = HX85200AFUNC_SMWP,
	._func_headphone = HX85200AFUNC_HEADPHONE,
	._func_usb_detect = HX85200AFUNC_USB_DETECT,
	._func_ap_notify_fw_sus = HX85200AFUNC_AP_NOTIFY_FW_SUS,
	._func_en  = FUNC_EN,
	._func_dis = FUNC_DIS,

	._addr_program_reload_from = ADDR_PROGRAM_RELOAD_FROM,
	._addr_program_reload_to = ADDR_PROGRAM_RELOAD_TO,
	._addr_program_reload_page_write = ADDR_PROGRAM_RELOAD_PAGE_WRITE,
	._addr_rawout_sel = HX85200A_ADDR_RAWOUT_SEL,
	._addr_reload_status = HX85200A_ADDR_RELOAD_STATUS,
	._addr_reload_crc32_result = HX85200A_ADDR_RELOAD_CRC32_RESULT,
	._addr_reload_addr_from = HX85200A_ADDR_RELOAD_ADDR_FROM,
	._addr_reload_addr_cmd_beat = HX85200A_ADDR_RELOAD_ADDR_CMD_BEAT,
	._data_system_reset = DATA_SYSTEM_RESET,
	._data_leave_safe_mode = DATA_LEAVE_SAFE_MODE,
	._data_fw_stop = DATA_FW_STOP,
	._data_program_reload_start = DATA_PROGRAM_RELOAD_START, //useless
	._data_program_reload_compare = DATA_PROGRAM_RELOAD_COMPARE, //useless
	._data_program_reload_break = DATA_PROGRAM_RELOAD_BREAK, //useless

	._addr_set_frame = HX85200A_ADDR_SET_FRAME,
	._data_set_frame = DATA_SET_FRAME,

	._para_idle_dis = PARA_IDLE_DIS,
	._para_idle_en = PARA_IDLE_EN,
	._addr_sorting_mode_en      = HX85200A_ADDR_SORTING_MODE_EN,
	._addr_fw_mode_status       = HX85200A_ADDR_FW_MODE_STATUS,
	._addr_fw_ver               = HX85200A_ADDR_FW_VER,
	._addr_fw_cfg               = HX85200A_ADDR_FW_CFG,
	._addr_fw_vendor            = HX85200A_ADDR_FW_VENDOR,
	._addr_cus_info             = HX85200A_ADDR_CUS_INFO,
	._addr_proj_info            = HX85200A_ADDR_PROJ_INFO,
	._addr_fw_state             = ADDR_FW_STATE, //useless
	._addr_fw_dbg_msg           = HX85200A_ADDR_FW_DBG_MSG,

	._data_rawdata_ready_hb = DATA_RAWDATA_READY_HB,
	._data_rawdata_ready_lb = DATA_RAWDATA_READY_LB,
	._addr_ahb              = ADDR_AHB,
	._data_ahb_dis          = DATA_AHB_DIS,
	._data_ahb_en           = DATA_AHB_EN,
	._addr_event_stack      = ADDR_EVENT_STACK,

	._data_handshaking_end = DATA_HANDSHAKING_END,
#if defined(HX_ULTRA_LOW_POWER)
	._addr_ulpm_33 = ADDR_ULPM_33,
	._addr_ulpm_34 = ADDR_ULPM_34,
	._data_ulpm_11 = DATA_ULPM_11,
	._data_ulpm_22 = DATA_ULPM_22,
	._data_ulpm_33 = DATA_ULPM_33,
	._data_ulpm_aa = DATA_ULPM_AA,
#endif
	._addr_ctrl_mpap_ovl    = HX85200A_ADDR_CTRL_MPAP_OVL,
	._data_ctrl_mpap_ovl_on = DATA_CTRL_MPAP_OVL_ON,

	._addr_flash_spi200_trans_fmt = (ADDR_FLASH_CTRL_BASE + 0x10),
	._addr_flash_spi200_trans_ctrl = (ADDR_FLASH_CTRL_BASE + 0x20),
	._addr_flash_spi200_cmd = (ADDR_FLASH_CTRL_BASE + 0x24),
	._addr_flash_spi200_addr = (ADDR_FLASH_CTRL_BASE + 0x28),
	._addr_flash_spi200_data = (ADDR_FLASH_CTRL_BASE + 0x2c),
	._addr_flash_spi200_flash_speed = (ADDR_FLASH_CTRL_BASE + 0x40),

	._data_flash_spi200_txfifo_rst   = DATA_FLASH_SPI200_TXFIFO_RST,
	._data_flash_spi200_rxfifo_rst   = DATA_FLASH_SPI200_RXFIFO_RST,
	._data_flash_spi200_trans_fmt    = DATA_FLASH_SPI200_TRANS_FMT,
	._data_flash_spi200_trans_ctrl_1 = DATA_FLASH_SPI200_TRANS_CTRL_1,
	._data_flash_spi200_trans_ctrl_2 = DATA_FLASH_SPI200_TRANS_CTRL_2,
	._data_flash_spi200_trans_ctrl_3 = DATA_FLASH_SPI200_TRANS_CTRL_3,
	._data_flash_spi200_trans_ctrl_4 = DATA_FLASH_SPI200_TRANS_CTRL_4,
	._data_flash_spi200_trans_ctrl_5 = DATA_FLASH_SPI200_TRANS_CTRL_5,
	._data_flash_spi200_trans_ctrl_6 = DATA_FLASH_SPI200_TRANS_CTRL_6,
	._data_flash_spi200_trans_ctrl_7 = DATA_FLASH_SPI200_TRANS_CTRL_7,

	._data_flash_spi200_cmd_1 = DATA_FLASH_SPI200_CMD_1,
	._data_flash_spi200_cmd_2 = DATA_FLASH_SPI200_CMD_2,
	._data_flash_spi200_cmd_3 = DATA_FLASH_SPI200_CMD_3,
	._data_flash_spi200_cmd_4 = DATA_FLASH_SPI200_CMD_4,
	._data_flash_spi200_cmd_5 = DATA_FLASH_SPI200_CMD_5,
	._data_flash_spi200_cmd_6 = DATA_FLASH_SPI200_CMD_6,
	._data_flash_spi200_cmd_7 = DATA_FLASH_SPI200_CMD_7,
	._data_flash_spi200_cmd_8 = DATA_FLASH_SPI200_CMD_8,


	._addr_mkey             = HX85200A_ADDR_MKEY,
	._addr_rawdata_buf      = HX85200A_ADDR_RAWDATA_BUF,
	._data_rawdata_end      = DATA_RAWDATA_END,
	._pwd_get_rawdata_start = PWD_GET_RAWDATA_START,
	._pwd_get_rawdata_end   = PWD_GET_RAWDATA_END,

	._addr_chk_fw_reload    = HX85200A_ADDR_CHK_FW_RELOAD,
	._addr_chk_fw_reload2   = HX85200A_ADDR_CHK_FW_RELOAD2,
	._data_fw_reload_dis    = DATA_FW_RELOAD_DIS,
	._data_fw_reload_en     = DATA_FW_RELOAD_EN,
	._addr_chk_irq_edge     = HX85200A_ADDR_CHK_IRQ_EDGE,
	._addr_info_channel_num = HX85200A_ADDR_INFO_CHANNEL_NUM,
	._addr_info_max_pt      = HX85200A_ADDR_INFO_MAX_PT,
	._addr_info_def_stylus  = HX85200A_ADDR_INFO_DEF_STYLUS,
	._addr_info_stylus_ratio = HX85200A_ADDR_INFO_STYLUS_RATIO,

	/* for inspection */
	._addr_normal_noise_thx   = HX85200A_ADDR_NORMAL_NOISE_THX,
	._addr_lpwug_noise_thx    = HX85200A_ADDR_LPWUG_NOISE_THX,
	._addr_noise_scale        = HX85200A_ADDR_NOISE_SCALE,
	._addr_recal_thx          = HX85200A_ADDR_RECAL_THX,
	._addr_palm_num           = HX85200A_ADDR_PALM_NUM,
	._addr_weight_sup         = HX85200A_ADDR_WEIGHT_SUP,
	._addr_normal_weight_a    = HX85200A_ADDR_NORMAL_WEIGHT_A,
	._addr_lpwug_weight_a     = HX85200A_ADDR_LPWUG_WEIGHT_A,
	._addr_weight_b           = HX85200A_ADDR_WEIGHT_B,
	._addr_max_dc             = HX85200A_ADDR_MAX_DC,
	._addr_skip_frame         = HX85200A_ADDR_SKIP_FRAME,
	._addr_neg_noise_sup      = HX85200A_ADDR_NEG_NOISE_SUP,
	._data_neg_noise          = DATA_NEG_NOISE,


};


void hx85200a_burst_enable(uint8_t auto_add_4_byte)
{
	uint8_t tmp_data[DATA_LEN_4];
	uint8_t tmp_addr[DATA_LEN_4];

	/*I("%s,Entering\n", __func__);*/
	hx_parse_assign_cmd(ADDR_AHB_CONTINOUS, tmp_addr, DATA_LEN_1);
	hx_parse_assign_cmd(PARA_AHB_CONTINOUS, tmp_data, DATA_LEN_1);


	if (himax_bus_write(tmp_addr[0], NUM_NULL, tmp_data, 1) < 0) {
		E("%s: bus access fail!\n", __func__);
		return;
	}

	hx_parse_assign_cmd(ADDR_AHB_INC4, tmp_addr, DATA_LEN_1);
	if (hx_s_ts->spi == NULL)
		hx_parse_assign_cmd(PARA_AHB_INC4, tmp_data, DATA_LEN_1);
	else
		hx_parse_assign_cmd(HX85200A_SPI_PARA_AHB_INC4,
			tmp_data, DATA_LEN_1);
	tmp_data[0] = (tmp_data[0] | auto_add_4_byte);
	if (himax_bus_write(tmp_addr[0], NUM_NULL, tmp_data, 1) < 0) {
		E("%s: bus access fail!\n", __func__);
		return;
	}
}

int hx85200a_register_write(uint32_t addr, uint8_t *val, uint32_t len)
{

	mutex_lock(&hx_s_ts->reg_lock);


	if (addr == ADDR_FLASH_SPI200_DATA)
		hx85200a_burst_enable(0);
	else if (len > DATA_LEN_4)
		hx85200a_burst_enable(1);
	else
		hx85200a_burst_enable(0);

	if (himax_bus_write(ADDR_AHB_ADDRESS_BYTE_0, addr, val,
		len + DATA_LEN_4) < 0) {
		E("%s: xfer fail!\n", __func__);
		mutex_unlock(&hx_s_ts->reg_lock);
		return BUS_FAIL;
	}

	mutex_unlock(&hx_s_ts->reg_lock);

	return NO_ERR;
}

static int hx85200a_register_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
	uint8_t tmp_addr[DATA_LEN_4];
	uint8_t tmp_data[DATA_LEN_4];


	mutex_lock(&hx_s_ts->reg_lock);

	if (addr == ADDR_FLASH_SPI200_DATA)
		hx85200a_burst_enable(0);
	else if (len > DATA_LEN_4)
		hx85200a_burst_enable(1);
	else
		hx85200a_burst_enable(0);

	// hx_parse_assign_cmd(ADDR_AHB_ADDRESS_BYTE_0, tmp_addr, DATA_LEN_1);
	if (himax_bus_write(ADDR_AHB_ADDRESS_BYTE_0, addr, NULL, 4) < 0) {
		E("%s: bus access fail!\n", __func__);
		mutex_unlock(&hx_s_ts->reg_lock);
		return BUS_FAIL;
	}

	hx_parse_assign_cmd(ADDR_AHB_ACCESS_DIRECTION, tmp_addr, DATA_LEN_1);
	hx_parse_assign_cmd(PARA_AHB_ACCESS_DIRECTION_READ,
		tmp_data, DATA_LEN_1);
	if (himax_bus_write(ADDR_AHB_ACCESS_DIRECTION, NUM_NULL,
		&tmp_data[0], 1) < 0) {
		E("%s: bus access fail!\n", __func__);
		mutex_unlock(&hx_s_ts->reg_lock);
		return BUS_FAIL;
	}

	if (himax_bus_read(ADDR_AHB_RDATA_BYTE_0, buf, len) < 0) {
		E("%s: bus access fail!\n", __func__);
		mutex_unlock(&hx_s_ts->reg_lock);
		return BUS_FAIL;
	}

	mutex_unlock(&hx_s_ts->reg_lock);

	return NO_ERR;
}


void hx85200a_system_reset(void)
{
	uint8_t tmp_data[DATA_LEN_4];

#if defined(HX_TP_TRIGGER_LCM_RST)
	hx_parse_assign_cmd(DATA_SYSTEM_RESET, tmp_data, DATA_LEN_4);
	hx85200a_register_write(ADDR_SYSTEM_RESET,
		tmp_data, DATA_LEN_4);
#else
	/* cmd reset */
	int retry = 0;

	himax_mcu_interface_on();
	hx_parse_assign_cmd(DATA_CLEAR, tmp_data, DATA_LEN_4);
	hx85200a_register_write(ADDR_CTRL_FW,
		tmp_data, DATA_LEN_4);
	do {
		/* reset code*/
		/**
		 * I2C_password[7:0] set Enter safe mode : 0x31 ==> 0x27
		 */
		/**
		 * I2C_password[15:8] set Enter safe mode :0x32 ==> 0x95
		 */
		tmp_data[0] = PARA_SENSE_OFF_0;
		tmp_data[1] = PARA_SENSE_OFF_1;
		if (himax_bus_write(ADDR_SENSE_ON_OFF_0, NUM_NULL, tmp_data,
			DATA_LEN_2) < 0)
			E("%s: bus access fail!\n", __func__);

		usleep_range(20000, 21000);

		/**
		 * I2C_password[7:0] set Enter safe mode : 0x31 ==> 0x00
		 */
		tmp_data[0] = 0x00;
		if (himax_bus_write(ADDR_SENSE_ON_OFF_0, NUM_NULL, tmp_data,
			1) < 0)
			E("%s: bus access fail!\n", __func__);

		usleep_range(10000, 11000);
		hx85200a_register_read(ADDR_FLAG_RESET_EVENT,
			tmp_data, DATA_LEN_4);
		I("%s:Read status from IC = %X,%X\n", __func__,
				tmp_data[0], tmp_data[1]);
	} while ((tmp_data[1] != 0x02 || tmp_data[0] != 0x00) && retry++ < 5);
#endif
}

static bool hx85200a_sense_off(bool check_en)
{
	uint8_t cnt = 0;
	uint8_t tmp_data[DATA_LEN_4];
	uint8_t tmp_addr[DATA_LEN_4];

	do {
		if (cnt == 0
		|| (tmp_data[0] != 0xA5
		&& tmp_data[0] != 0x00
		&& tmp_data[0] != 0x87)) {
			hx_parse_assign_cmd(DATA_FW_STOP, tmp_data, DATA_LEN_4);
			hx85200a_register_write(ADDR_CTRL_FW,
				tmp_data, DATA_LEN_4);
		}

		/*msleep(20);*/
		usleep_range(10000, 10001);
		/* check fw status */
		hx85200a_register_read(ADDR_CS_CENTRAL_STATE,
			tmp_data, DATA_LEN_4);

		if (tmp_data[0] != 0x05) {
			I("%s: Do not need wait FW, Status = 0x%02X!\n",
					__func__, tmp_data[0]);
			break;
		}

		hx85200a_register_read(ADDR_CTRL_FW, tmp_data, 4);
		I("%s: cnt = %d, data[0] = 0x%02X!\n", __func__,
			cnt, tmp_data[0]);
	} while (tmp_data[0] != 0x87 && (++cnt < 10) && check_en == true);

	cnt = 0;

	do {
		/**
		 * I2C_password[7:0] set Enter safe mode : 0x31 ==> 0x27
		 * I2C_password[15:8] set Enter safe mode :0x32 ==> 0x95
		 */
		tmp_data[0] = PARA_SENSE_OFF_0;
		tmp_data[1] = PARA_SENSE_OFF_1;
		hx_parse_assign_cmd(ADDR_SENSE_ON_OFF_0, tmp_addr, DATA_LEN_1);
		if (himax_bus_write(tmp_addr[0], NUM_NULL, tmp_data,
			2) < 0) {
			E("%s: bus access fail!\n", __func__);
			return false;
		}

		/**
		 * Check enter_save_mode
		 */
		hx85200a_register_read(ADDR_CS_CENTRAL_STATE,
			tmp_data, DATA_LEN_4);
		I("%s: Check enter_save_mode data[0]=%X\n", __func__,
			tmp_data[0]);

		if (tmp_data[0] == 0x0C)
			goto SUCCEED;

		usleep_range(10000, 10001);
#if defined(HX_RST_PIN_FUNC)
		himax_mcu_pin_reset(3);
#else
		hx85200a_system_reset();
#endif

	} while (cnt++ < 5);

	return false;
SUCCEED:
	return true;
}

static void hx85200a_sense_on(uint8_t FlashMode)
{
	uint8_t tmp_data[DATA_LEN_4];
	uint8_t tmp_addr[DATA_LEN_4];

	I("Enter %s\n", __func__);
	hx_s_core_fp._interface_on();
	hx_parse_assign_cmd(DATA_CLEAR, tmp_data, DATA_LEN_4);
	hx_s_core_fp._register_write(ADDR_CTRL_FW, tmp_data, DATA_LEN_4);
	/*msleep(20);*/
	usleep_range(10000, 10001);
	if (!FlashMode) {
		hx_s_core_fp._ic_reset(0);

	} else {
		I("%s:OK and Read status from IC = %X,%X\n", __func__,
			tmp_data[0], tmp_data[1]);
		/* reset code*/
		tmp_data[0] = 0x00;
		hx_parse_assign_cmd(ADDR_SENSE_ON_OFF_0, tmp_addr, DATA_LEN_4);
		if (himax_bus_write(tmp_addr[0], NUM_NULL,
			tmp_data, 1) < 0) {
			E("%s: cmd=%x bus access fail!\n",
			__func__,
			tmp_addr[0]);
		}
	}
}


bool hx85200a_read_event_stack(uint8_t *buf, uint32_t length)
{
	struct time_var t_start, t_end, t_delta;
	int len = length;

	if (hx_s_ts->debug_log_level & BIT(2))
		time_func(&t_start);

	himax_bus_read(hx_s_ic_setup._addr_event_stack, buf, length);

	if (hx_s_ts->debug_log_level & BIT(2)) {
		time_func(&t_end);
		t_delta.tv_nsec = (t_end.tv_sec * 1000000000 + t_end.tv_nsec)
			- (t_start.tv_sec * 1000000000 + t_start.tv_nsec);

		hx_s_ts->bus_speed = (len * 9 * 1000000
			/ (int)t_delta.tv_nsec) * 13 / 10;
	}

	return 1;
}

bool hx85200a_bin_desc_get(unsigned char *fw, uint32_t max_sz)
{
	uint32_t addr_t = 0;
	unsigned char *fw_buf = NULL;
	bool keep_on_flag = false;
	bool g_bin_desc_flag = false;

	do {
		fw_buf = &fw[addr_t];

		/*Check bin is with description table or not*/
		if (!g_bin_desc_flag) {
			if (fw_buf[0x00] == 0x00 && fw_buf[0x01] == 0x00
			&& fw_buf[0x02] == 0x00 && fw_buf[0x03] == 0x00
			&& fw_buf[0x04] == 0x00 && fw_buf[0x05] == 0x00
			&& fw_buf[0x06] == 0x00 && fw_buf[0x07] == 0x00
			&& fw_buf[0x0E] == 0x56)
				g_bin_desc_flag = true;

		}
		if (!g_bin_desc_flag) {
			I("%s: fw_buf[0x00] = %2X, fw_buf[0x0E] = %2X\n",
			__func__, fw_buf[0x00], fw_buf[0x0E]);
			I("%s: No description table\n",	__func__);
			break;
		}

		/*Get related data*/
		keep_on_flag = hx_s_core_fp._bin_desc_data_get(addr_t, fw_buf);

		addr_t = addr_t + FW_PAGE_SZ;
	} while (max_sz > addr_t && keep_on_flag);

	return g_bin_desc_flag;
}

uint32_t hx85200a_calc_crc_by_hw(uint8_t *start_addr, int reload_length)
{
	uint32_t result = 0;
	uint8_t tmp_data[DATA_LEN_4];
	int cnt = 0, ret = 0;
	int length = reload_length / DATA_LEN_4;

	ret = hx_s_core_fp._register_write(
		hx_s_ic_setup._addr_reload_addr_from,
		start_addr, DATA_LEN_4);
	if (ret < NO_ERR) {
		E("%s: bus access fail!\n", __func__);
		return HW_CRC_FAIL;
	}

	tmp_data[3] = 0x00;
	tmp_data[2] = 0x99;
	tmp_data[1] = (length >> 8);
	tmp_data[0] = length;
	ret = hx_s_core_fp._register_write(
		hx_s_ic_setup._addr_reload_addr_cmd_beat,
		tmp_data, DATA_LEN_4);
	if (ret < NO_ERR) {
		E("%s: bus access fail!\n", __func__);
		return HW_CRC_FAIL;
	}
	cnt = 0;

	do {
		ret = hx_s_core_fp._register_read(
			hx_s_ic_setup._addr_reload_status,
			tmp_data, DATA_LEN_4);
		if (ret < NO_ERR) {
			E("%s: bus access fail!\n", __func__);
			return HW_CRC_FAIL;
		}

		if ((tmp_data[0] & 0x01) != 0x01) {
			ret = hx_s_core_fp._register_read(
				hx_s_ic_setup._addr_reload_crc32_result,
				tmp_data, DATA_LEN_4);
			if (ret < NO_ERR) {
				E("%s: bus access fail!\n", __func__);
				return HW_CRC_FAIL;
			}
			I("%s:data[3]=%X,data[2]=%X,data[1]=%X,data[0]=%X\n",
				__func__,
				tmp_data[3],
				tmp_data[2],
				tmp_data[1],
				tmp_data[0]);
			result = ((tmp_data[3] << 24)
					+ (tmp_data[2] << 16)
					+ (tmp_data[1] << 8)
					+ tmp_data[0]);
			goto END;
		} else if (tmp_data[1] != 0x99) {
			I("Fail CRC4 = 0x99\n");
			result = HW_CRC_FAIL;
			goto END;
		} else {
			I("Waiting for HW ready!\n");
			usleep_range(1000, 1100);
			if (cnt >= 100)
				hx_s_core_fp._read_FW_status();
		}

	} while (cnt++ < 100);
END:
	return result;
}
#if defined(CONFIG_TOUCHSCREEN_HIMAX_INSPECT)
void hx85200a_get_noise_base(bool is_lpwup, int *rslt)/*Normal Threshold*/
{
	uint8_t tmp_data[4];
	uint32_t addr32 = 0x00;
	uint16_t NOISEMAX;
	uint16_t g_recal_thx;

	addr32 = hx_s_ic_setup._addr_normal_noise_thx;

	/*normal : 0x130001C4*/
	hx_s_core_fp._register_read(addr32, tmp_data, 4);
	NOISEMAX = tmp_data[0];

	hx_s_core_fp._register_read(hx_s_ic_setup._addr_recal_thx, tmp_data, 4);
	g_recal_thx = tmp_data[0];/*0x130001C8*/
	I("%s: NOISEMAX = %d, g_recal_thx = %d\n", __func__,
		NOISEMAX, g_recal_thx);

	rslt[0] = NOISEMAX;
	rslt[1] = g_recal_thx;
}

uint16_t hx85200a_get_palm_num(void)/*Palm Number*/
{
	uint8_t tmp_data[4];
	uint16_t palm_num;

	hx_s_core_fp._register_read(hx_s_ic_setup._addr_palm_num, tmp_data, 4);
	palm_num = tmp_data[0];
	I("%s: palm_num = %d ", __func__, palm_num);

	return palm_num;
}

void hx85200a_neg_noise_sup(uint8_t *data)
{
	I("%s Not support negative noise\n", __func__);
}

int hx85200a_get_noise_weight_test(uint8_t checktype)
{
	uint8_t tmp_data[4];
	uint16_t weight = 0;
	uint16_t value = 0;
	uint32_t addr32 = 0x00;


	/*0X130001D4 weighting value*/
	hx_s_core_fp._register_read(
		hx_s_ic_setup._addr_weight_sup, tmp_data, 4);
	value = (tmp_data[1] << 8) | tmp_data[0];
	I("%s: value = %d, %d, %d ", __func__, value, tmp_data[2], tmp_data[3]);

	switch (checktype) {
	case HX_WT_NOISE:
	case HX_SELF_WT_NOISE:
		addr32 = hx_s_ic_setup._addr_normal_weight_a;
		break;
	default:
		I("%s Not support type\n", __func__);
		break;
	}

	/*Normal:0X130001D0, weighting threshold*/
	hx_s_core_fp._register_read(addr32, tmp_data, 4);
	weight = tmp_data[0];
	I("%s: weight = %d ", __func__, weight);

	if (value > weight)
		return ERR_TEST_FAIL;
	else
		return 0;
}

int hx85200a_switch_mode_inspection(int mode)
{
	uint8_t tmp_data[4] = {0};

	if (hx_s_ts->debug_log_level & BIT(4))
		I("%s: Entering\n", __func__);

	/*Stop Handshaking*/
	hx_s_core_fp._register_write(hx_s_ic_setup._addr_rawdata_buf,
		tmp_data, 4);

	/*Swtich Mode*/
	switch (mode) {
	case HX_SORTING:
		tmp_data[3] = 0x00; tmp_data[2] = 0x00;
		tmp_data[1] = PWD_SORTING_START;
		tmp_data[0] = PWD_SORTING_START;
		break;
	case HX_OPEN:
		tmp_data[3] = 0x00; tmp_data[2] = 0x00;
		tmp_data[1] = PWD_OPEN_START;
		tmp_data[0] = PWD_OPEN_START;
		break;
	case HX_MICRO_OPEN:
		tmp_data[3] = 0x00; tmp_data[2] = 0x00;
		tmp_data[1] = HX85200A_PWD_OPEN_START;
		tmp_data[0] = HX85200A_PWD_OPEN_START;
		break;
	case HX_SHORT:
		tmp_data[3] = 0x00; tmp_data[2] = 0x00;
		tmp_data[1] = PWD_SHORT_START;
		tmp_data[0] = PWD_SHORT_START;
		break;

	case HX_GAPTEST_RAW:
	case HX_RAWDATA:
	case HX_BPN_RAWDATA:
	case HX_SC:
		tmp_data[3] = 0x00; tmp_data[2] = 0x00;
		tmp_data[1] = PWD_RAWDATA_START;
		tmp_data[0] = PWD_RAWDATA_START;
		break;

	case HX_WT_NOISE:
	case HX_ABS_NOISE:
		tmp_data[3] = 0x00; tmp_data[2] = 0x00;
		tmp_data[1] = PWD_NOISE_START;
		tmp_data[0] = PWD_NOISE_START;
		break;

	case HX_ACT_IDLE_RAWDATA:
	case HX_ACT_IDLE_BPN_RAWDATA:
	case HX_ACT_IDLE_NOISE:
		tmp_data[3] = 0x00; tmp_data[2] = 0x00;
		tmp_data[1] = PWD_ACT_IDLE_START;
		tmp_data[0] = PWD_ACT_IDLE_START;
		break;

	case HX_LP_RAWDATA:
	case HX_LP_BPN_RAWDATA:
	case HX_LP_ABS_NOISE:
	case HX_LP_WT_NOISE:
		tmp_data[3] = 0x00; tmp_data[2] = 0x00;
		tmp_data[1] = PWD_LP_START;
		tmp_data[0] = PWD_LP_START;
		break;
	case HX_LP_IDLE_RAWDATA:
	case HX_LP_IDLE_BPN_RAWDATA:
	case HX_LP_IDLE_NOISE:
		tmp_data[3] = 0x00; tmp_data[2] = 0x00;
		tmp_data[1] = PWD_LP_IDLE_START;
		tmp_data[0] = PWD_LP_IDLE_START;
		break;
	case HX_PEN_MODE_DATA:
		tmp_data[3] = 0x00; tmp_data[2] = 0x00;
		tmp_data[1] = PEN_MODE_START;
		tmp_data[0] = PEN_MODE_START;
		break;

	default:
		I("%s,Nothing to be done!\n", __func__);
		break;
	}

	if (hx_s_core_fp._assign_sorting_mode != NULL)
		hx_s_core_fp._assign_sorting_mode(tmp_data);
	I("%s: End of setting!\n", __func__);

	return 0;

}

void hx85200a_switch_data_type(uint8_t checktype, char **mode_str)
{
	uint8_t datatype = 0x00;

	if (hx_s_ts->debug_log_level & BIT(4)) {
		I("%s,Expected type[%d]=%s"
			, __func__
			, checktype, mode_str[checktype]);
	}
	switch (checktype) {
	case HX_SORTING:
		datatype = DATA_SORTING;
		break;
	case HX_OPEN:
		datatype = HX85200A_DATA_OPEN;
		break;
	case HX_MICRO_OPEN:
		datatype = HX85200A_DATA_MICRO_OPEN;
		break;
	case HX_SHORT:
		datatype = DATA_SHORT;
		break;
	case HX_RAWDATA:
	case HX_BPN_RAWDATA:
	case HX_SC:
	case HX_GAPTEST_RAW:
		datatype = DATA_RAWDATA;
		break;

	case HX_WT_NOISE:
	case HX_ABS_NOISE:
		datatype = DATA_NOISE;
		break;
	case HX_BACK_NORMAL:
		datatype = DATA_BACK_NORMAL;
		break;
	case HX_ACT_IDLE_RAWDATA:
	case HX_ACT_IDLE_BPN_RAWDATA:
		datatype = DATA_ACT_IDLE_RAWDATA;
		break;
	case HX_ACT_IDLE_NOISE:
		datatype = DATA_ACT_IDLE_NOISE;
		break;

	case HX_LP_RAWDATA:
	case HX_LP_BPN_RAWDATA:
		datatype = DATA_LP_RAWDATA;
		break;
	case HX_LP_WT_NOISE:
	case HX_LP_ABS_NOISE:
		datatype = DATA_LP_NOISE;
		break;
	case HX_LP_IDLE_RAWDATA:
	case HX_LP_IDLE_BPN_RAWDATA:
		datatype = DATA_LP_IDLE_RAWDATA;
		break;
	case HX_LP_IDLE_NOISE:
		datatype = DATA_LP_IDLE_NOISE;
		break;
	case HX_PEN_MODE_DATA:
		datatype = DATA_PEN_MODE;
		break;

	default:
		E("Wrong type=%d\n", checktype);
		break;
	}
	hx_s_core_fp._diag_register_set(datatype, 0x00, false);
}


uint32_t hx85200a_check_mode(uint8_t checktype, char **mode_str)
{
	int ret = 0;
	uint8_t tmp_data[4] = {0};
	uint8_t wait_pwd[2] = {0};

	if (hx_s_ts->debug_log_level & BIT(4))
		I("%s: Entering\n", __func__);

	switch (checktype) {
	case HX_SORTING:
		wait_pwd[0] = PWD_SORTING_END;
		wait_pwd[1] = PWD_SORTING_END;
		break;
	case HX_OPEN:
		wait_pwd[0] = PWD_OPEN_END;
		wait_pwd[1] = PWD_OPEN_END;
		break;
	case HX_MICRO_OPEN:
		wait_pwd[0] = HX85200A_PWD_OPEN_END;
		wait_pwd[1] = HX85200A_PWD_OPEN_END;
		break;
	case HX_SHORT:
		wait_pwd[0] = PWD_SHORT_END;
		wait_pwd[1] = PWD_SHORT_END;
		break;
	case HX_RAWDATA:
	case HX_BPN_RAWDATA:
	case HX_SC:
	case HX_GAPTEST_RAW:
		wait_pwd[0] = PWD_RAWDATA_END;
		wait_pwd[1] = PWD_RAWDATA_END;
		break;

	case HX_WT_NOISE:
	case HX_ABS_NOISE:
		wait_pwd[0] = PWD_NOISE_END;
		wait_pwd[1] = PWD_NOISE_END;
		break;

	case HX_ACT_IDLE_RAWDATA:
	case HX_ACT_IDLE_BPN_RAWDATA:
	case HX_ACT_IDLE_NOISE:
		wait_pwd[0] = PWD_ACT_IDLE_END;
		wait_pwd[1] = PWD_ACT_IDLE_END;
		break;

	case HX_LP_RAWDATA:
	case HX_LP_BPN_RAWDATA:
	case HX_LP_ABS_NOISE:
	case HX_LP_WT_NOISE:
		wait_pwd[0] = PWD_LP_END;
		wait_pwd[1] = PWD_LP_END;
		break;
	case HX_LP_IDLE_RAWDATA:
	case HX_LP_IDLE_BPN_RAWDATA:
	case HX_LP_IDLE_NOISE:
		wait_pwd[0] = PWD_LP_IDLE_END;
		wait_pwd[1] = PWD_LP_IDLE_END;
		break;
	case HX_PEN_MODE_DATA:
		wait_pwd[0] = PEN_MODE_END;
		wait_pwd[1] = PEN_MODE_END;
		break;
	default:
		E("Wrong type=%d\n", checktype);
		break;
	}

	if (hx_s_core_fp._check_sorting_mode != NULL) {
		ret = hx_s_core_fp._check_sorting_mode(tmp_data);
		if (ret != NO_ERR)
			return ret;
	}

	if ((wait_pwd[0] == tmp_data[0]) && (wait_pwd[1] == tmp_data[1])) {
		I("%s,It had been changed to [%d]=%s\n",
			__func__,
			checktype, mode_str[checktype]);
		return NO_ERR;
	} else {
		return 1;
	}
}

uint32_t hx85200a_wait_sorting_mode(uint8_t checktype, char **mode_str)
{
	uint8_t tmp_data[4] = {0};
	uint8_t wait_pwd[2] = {0};
	int count = 0;
	int i = 0;
	int len = (size_t)(sizeof(hx_s_ic_data->dbg_reg_ary)/sizeof(uint32_t));

	if (hx_s_ts->debug_log_level & BIT(4))
		I("%s:start!\n", __func__);

	switch (checktype) {
	case HX_SORTING:
		wait_pwd[0] = PWD_SORTING_END;
		wait_pwd[1] = PWD_SORTING_END;
		break;
	case HX_OPEN:
		wait_pwd[0] = PWD_OPEN_END;
		wait_pwd[1] = PWD_OPEN_END;
		break;
	case HX_MICRO_OPEN:
		wait_pwd[0] = HX85200A_PWD_OPEN_END;
		wait_pwd[1] = HX85200A_PWD_OPEN_END;
		break;
	case HX_SHORT:
		wait_pwd[0] = PWD_SHORT_END;
		wait_pwd[1] = PWD_SHORT_END;
		break;
	case HX_RAWDATA:
	case HX_BPN_RAWDATA:
	case HX_SC:
	case HX_GAPTEST_RAW:
		wait_pwd[0] = PWD_RAWDATA_END;
		wait_pwd[1] = PWD_RAWDATA_END;
		break;
	case HX_WT_NOISE:
	case HX_ABS_NOISE:
		wait_pwd[0] = PWD_NOISE_END;
		wait_pwd[1] = PWD_NOISE_END;
		break;
	case HX_ACT_IDLE_RAWDATA:
	case HX_ACT_IDLE_BPN_RAWDATA:
	case HX_ACT_IDLE_NOISE:
		wait_pwd[0] = PWD_ACT_IDLE_END;
		wait_pwd[1] = PWD_ACT_IDLE_END;
		break;

	case HX_LP_RAWDATA:
	case HX_LP_BPN_RAWDATA:
	case HX_LP_ABS_NOISE:
	case HX_LP_WT_NOISE:
		wait_pwd[0] = PWD_LP_END;
		wait_pwd[1] = PWD_LP_END;
		break;
	case HX_LP_IDLE_RAWDATA:
	case HX_LP_IDLE_BPN_RAWDATA:
	case HX_LP_IDLE_NOISE:
		wait_pwd[0] = PWD_LP_IDLE_END;
		wait_pwd[1] = PWD_LP_IDLE_END;
		break;
	case HX_PEN_MODE_DATA:
		wait_pwd[0] = PEN_MODE_END;
		wait_pwd[1] = PEN_MODE_END;
		break;

	default:
		I("No Change Mode and now type=%d\n", checktype);
		break;
	}
	I("%s:NowType[%d] = %s, Expected=0x%02X%02X\n",
		__func__, checktype, mode_str[checktype],
		 wait_pwd[1], wait_pwd[0]);
	do {
		if (hx_s_ts->debug_log_level & BIT(4))
			I("%s:start check_sorting_mode!\n", __func__);
		if (hx_s_core_fp._check_sorting_mode != NULL)
			hx_s_core_fp._check_sorting_mode(tmp_data);
		if (hx_s_ts->debug_log_level & BIT(4))
			I("%s:end check_sorting_mode!\n", __func__);
		if ((wait_pwd[0] == tmp_data[0]) &&
			(wait_pwd[1] == tmp_data[1]))
			return HX_INSP_OK;
		if (hx_s_ts->debug_log_level & BIT(4)) {
			for (i = 0; i < len; i++) {
				hx_s_core_fp._register_read(
					hx_s_ic_data->dbg_reg_ary[i],
					tmp_data, DATA_LEN_4);
				I(HX_PRINT_STS_LOG,
				__func__,
				hx_s_ic_data->dbg_reg_ary[i],
				tmp_data[0], tmp_data[1],
				tmp_data[2], tmp_data[3]);
			}

			I("Now retry %d times!\n", count);
		}
		count++;
		msleep(50);
	} while (count < 50);

	if (hx_s_ts->debug_log_level & BIT(4))
		I("%s:end\n", __func__);
	return HX_INSP_ESWITCHMODE;
}
#endif

void hx85200a_read_FW_ver(void)
{
	uint8_t data[32] = {0};

	hx_s_core_fp._register_read(hx_s_ic_setup._addr_fw_ver,
		data, DATA_LEN_4);
	hx_s_ic_data->vendor_panel_ver =  data[0];
	hx_s_ic_data->vendor_fw_ver = data[1] << 8 | data[2];
	I("PANEL_VER : %X\n", hx_s_ic_data->vendor_panel_ver);
	I("FW_VER : %X\n", hx_s_ic_data->vendor_fw_ver);

	hx_s_core_fp._register_read(hx_s_ic_setup._addr_fw_cfg,
		data, DATA_LEN_4);
	hx_s_ic_data->vendor_touch_cfg_ver = data[0];
	I("MAJOR_VER = %X\n", hx_s_ic_data->vendor_touch_cfg_ver);

//	hx_s_core_fp._register_read(hx_s_ic_setup._addr_fw_cfg,
//		data, DATA_LEN_4);
	hx_s_ic_data->vendor_display_cfg_ver = data[1];
	I("MINOR_VER = %X\n", hx_s_ic_data->vendor_display_cfg_ver);

	hx_s_core_fp._register_read(hx_s_ic_setup._addr_fw_vendor, data,
		DATA_LEN_4);
	hx_s_ic_data->vendor_cid_maj_ver = data[2];
	hx_s_ic_data->vendor_cid_min_ver = data[3];
	I("CID_VER : %X\n", (hx_s_ic_data->vendor_cid_maj_ver << 8
			| hx_s_ic_data->vendor_cid_min_ver));

	memset(data, 0x00, 32);
	hx_s_core_fp._register_read(hx_s_ic_setup._addr_cus_info, data, 12);
	memcpy(hx_s_ic_data->vendor_cus_info, data, 12);
	I("Cusomer ID = %s\n", hx_s_ic_data->vendor_cus_info);

	memset(data, 0x00, 32);
	hx_s_core_fp._register_read(hx_s_ic_setup._addr_proj_info, data, 12);
	memcpy(hx_s_ic_data->vendor_proj_info, data, 12);
	I("Project ID = %s\n", hx_s_ic_data->vendor_proj_info);
}
void hx85200a_show_FW_ver(void)
{
	I("PANEL_VER : %X\n", hx_s_ic_data->vendor_panel_ver);
	I("FW_VER : %X\n", hx_s_ic_data->vendor_fw_ver);
	I("MAJOR_VER = %X\n", hx_s_ic_data->vendor_touch_cfg_ver);
	I("MINOR_VER = %X\n", hx_s_ic_data->vendor_display_cfg_ver);
	I("CID_VER : %X\n", (hx_s_ic_data->vendor_cid_maj_ver << 8
			| hx_s_ic_data->vendor_cid_min_ver));
	I("Cusomer ID = %s\n", hx_s_ic_data->vendor_cus_info);
	I("Project ID = %s\n", hx_s_ic_data->vendor_proj_info);
}
uint8_t hx85200a_tp_info_check(void)
{
	char addr[DATA_LEN_4] = {0};
	char data[DATA_LEN_4] = {0};
	uint8_t err_cnt = 0;

	hx_parse_assign_cmd(hx_s_ic_setup._addr_info_channel_num,
		addr, DATA_LEN_4);
	hx_s_core_fp._register_read(hx_s_ic_setup._addr_info_channel_num,
		data, DATA_LEN_4);
	hx_s_ic_data->fw_rx_num = data[0];
	hx_s_ic_data->fw_tx_num = data[1];

	hx_parse_assign_cmd(hx_s_ic_setup._addr_info_max_pt, addr, DATA_LEN_4);
	hx_s_core_fp._register_read(hx_s_ic_setup._addr_info_max_pt,
		data, DATA_LEN_4);
	hx_s_ic_data->fw_max_pt = data[2];

	hx_parse_assign_cmd(hx_s_ic_setup._addr_chk_irq_edge, addr, DATA_LEN_4);
	hx_s_core_fp._register_read(hx_s_ic_setup._addr_chk_irq_edge,
		data, DATA_LEN_4);
	if ((data[1] & 0x01) == 1)
		hx_s_ic_data->fw_int_is_edge = true;
	else
		hx_s_ic_data->fw_int_is_edge = false;

	/*1. Read number of MKey R100070E8H to determin data size*/
	hx_parse_assign_cmd(hx_s_ic_setup._addr_mkey, addr, DATA_LEN_4);
	hx_s_core_fp._register_read(hx_s_ic_setup._addr_mkey, data, DATA_LEN_4);

	hx_s_ic_data->fw_bt_num = data[2] & 0x03;


	hx_s_core_fp._register_read(hx_s_ic_setup._addr_info_def_stylus,
		data, DATA_LEN_4);
	hx_s_ic_data->fw_stylus_func = data[0];

	if (hx_s_ic_data->rx_num != hx_s_ic_data->fw_rx_num) {
		err_cnt++;
		W("%s: RX_NUM, Set = %d ; FW = %d", __func__,
			hx_s_ic_data->rx_num, hx_s_ic_data->fw_rx_num);
	}

	if (hx_s_ic_data->tx_num != hx_s_ic_data->fw_tx_num) {
		err_cnt++;
		W("%s: TX_NUM, Set = %d ; FW = %d", __func__,
			hx_s_ic_data->tx_num, hx_s_ic_data->fw_tx_num);
	}

	if (hx_s_ic_data->bt_num != hx_s_ic_data->fw_bt_num) {
		err_cnt++;
		W("%s: BT_NUM, Set = %d ; FW = %d", __func__,
			hx_s_ic_data->bt_num, hx_s_ic_data->fw_bt_num);
	}

	if (hx_s_ic_data->max_pt != hx_s_ic_data->fw_max_pt) {
		err_cnt++;
		W("%s: MAX_PT, Set = %d ; FW = %d", __func__,
			hx_s_ic_data->max_pt, hx_s_ic_data->fw_max_pt);
	}

	if (hx_s_ic_data->int_is_edge != hx_s_ic_data->fw_int_is_edge) {
		err_cnt++;
		W("%s: INT_IS_EDGE, Set = %d ; FW = %d", __func__,
			hx_s_ic_data->int_is_edge,
			hx_s_ic_data->fw_int_is_edge);
	}

	if (hx_s_ic_data->stylus_func != hx_s_ic_data->fw_stylus_func) {
		err_cnt++;
		W("%s: STYLUS_FUNC, Set = %d ; FW = %d", __func__,
			hx_s_ic_data->stylus_func,
			hx_s_ic_data->fw_stylus_func);
	}

	if (hx_s_ic_data->stylus_func) {
		hx_s_core_fp._register_read(
			hx_s_ic_setup._addr_info_stylus_ratio,
			data, DATA_LEN_4);
		/*0x100071FE 0=off 1=on*/
		hx_s_ic_data->fw_stylus_id_v2 = data[1];
		/*0x100071FF 0=ratio_1 10=ratio_10*/
		hx_s_ic_data->fw_stylus_ratio = data[2];
		if (hx_s_ic_data->stylus_id_v2 !=
			hx_s_ic_data->fw_stylus_id_v2) {
			err_cnt++;
			W("%s: STYLUS_ID_V2, Set = %d ; FW = %d", __func__,
				hx_s_ic_data->stylus_id_v2,
				hx_s_ic_data->fw_stylus_id_v2);
		}
		if (hx_s_ic_data->stylus_ratio !=
			hx_s_ic_data->fw_stylus_ratio) {
			err_cnt++;
			W("%s: STYLUS_RATIO, Set = %d ; FW = %d", __func__,
				hx_s_ic_data->stylus_ratio,
				hx_s_ic_data->fw_stylus_ratio);
		}
	}

	if (err_cnt > 0)
		W("FIX_TOUCH_INFO does NOT match to FW information\n");
	else
		I("FIX_TOUCH_INFO is OK\n");

	return err_cnt;
}
#define HXLOG_CASCAD0 "After x5c write now is checking, times=%d, val=0x%02X!\n"
#define HXLOG_CASCAD1 "Waiting for cascade init=0x%02X over 100 times, please check power\n"
void hx85200a_cascade_cmd(void)
{
	uint8_t tmp_data[DATA_LEN_4];
	int retry = 0;

	I("%s: Start to EN cascad!\n", __func__);

	tmp_data[3] = 0x00;
	tmp_data[2] = 0x00;
	tmp_data[1] = 0x99;
	tmp_data[0] = 0x99;
	hx_s_core_fp._register_write(ADDR_CTRL_FW, tmp_data, DATA_LEN_4);

	usleep_range(10000, 10001);
	retry = 0;
	do {
		hx_s_core_fp._register_read(
			ADDR_CTRL_FW,
			tmp_data, DATA_LEN_4);

		if (tmp_data[0] == 0x00 && tmp_data[1] == 0x00) {
			I("write x5c check OK, times=%d!\n", retry);
			break;
		}

		I(HXLOG_CASCAD0, retry, tmp_data[0]);
		retry++;
		usleep_range(10000, 10001);
	} while (retry < 100);

	usleep_range(10000, 10001);
	retry = 0;
	do {
		hx_s_core_fp._register_read(
			HX85200A_ADDR_CHK_CASCAD,
			tmp_data, DATA_LEN_4);

		if (tmp_data[0] == 0x00) {
			I("cascade init, times=%d, val=0x%02X!\n",
				retry,
				tmp_data[0]);
			retry++;
			usleep_range(10000, 10001);
		} else {
			I("Starting setting cascade=0x%02x!\n",
				tmp_data[0]);
			break;
		}
	} while (retry < 100);

	if (retry == 100) {
		E(HXLOG_CASCAD1, tmp_data[0]);
		goto END;
	}

	usleep_range(10000, 10001);
	retry = 0;
	do {
		hx_s_core_fp._register_read(
			HX85200A_ADDR_CHK_CASCAD,
			tmp_data, DATA_LEN_4);

		if (tmp_data[0] == 0x64) {
			I("Cascade check OK, times=%d!\n", retry);
			break;
		} else if (tmp_data[0] == 0xff
			&& tmp_data[1] == 0xff) {
			I("Cascade check Fail, times=%d!\n", retry);
			break;
		}

		I("Cascade checking, times=%d, val=0x%02X!\n",
			retry,
			tmp_data[0]);
		retry++;
		usleep_range(10000, 10001);
	} while (retry < 2000);
END:
	return;
}

void hx85200a_resend_cmd_func(bool suspended)
{
#if defined(HX_HIGH_SENSE) || defined(HX_HEADPHONE)
	struct himax_ts_data *ts = hx_s_ts;
#endif

#if defined(HX_HIGH_SENSE)
	hx_s_core_fp._set_HSEN_enable(ts->HSEN_enable, suspended);
#endif
#if defined(HX_HEADPHONE)
	hx_s_core_fp._set_headphone_en(ts->hp_en, suspended);
#endif
#if defined(HX_USB_DETECT_GLOBAL)
	himax_cable_detect_func(true);
#endif
}

void hx85200a_ic_power_saving(void)
{
	uint8_t tmp_data[] = {0x88, 0x88, 0x00, 0x00};

	I("%s:R%08XH<-0x%02X%02X%02X%02X\n", __func__, ADDR_CTRL_FW,
		tmp_data[3], tmp_data[2], tmp_data[1], tmp_data[0]);
	hx85200a_register_write(ADDR_CTRL_FW,
				tmp_data, DATA_LEN_4);

}
void hx85200a_suspend_proc(bool suspended)
{
	I("%s: Entering!\n", __func__);
#if defined(HX_SMART_WAKEUP)
	if (hx_s_ts->SMWP_enable)
		hx_s_core_fp._set_SMWP_enable(hx_s_ts->SMWP_enable, suspended);
	else
#endif
		hx85200a_ic_power_saving();

}

void hx85200a_resume_proc(bool suspended)
{
	I("%s: Entering!\n", __func__);
#if defined(HX_SMART_WAKEUP)
	hx_s_core_fp._set_SMWP_enable(0, suspended);
#endif

	/* trigger reset */
	hx_s_core_fp._sense_on(0);

	hx_s_core_fp._resend_cmd_func(suspended);

}

void hx85200a_init(void)
{
	hx_s_core_fp._interface_on = himax_mcu_interface_on;
	hx_s_core_fp._wait_wip = himax_mcu_wait_wip;
	hx_s_core_fp._init_psl = himax_mcu_init_psl;
	hx_s_core_fp._resume_ic_action = himax_mcu_resume_ic_action;
	hx_s_core_fp._suspend_ic_action = himax_mcu_suspend_ic_action;
	hx_s_core_fp._power_on_init = himax_mcu_power_on_init;

	hx_s_core_fp._calc_crc_by_ap = himax_mcu_calc_crc_by_ap;
	hx_s_core_fp._calc_crc_by_hw = hx85200a_calc_crc_by_hw;
	hx_s_core_fp._set_reload_cmd = himax_mcu_set_reload_cmd;
	hx_s_core_fp._program_reload = himax_mcu_program_reload;
#if defined(HX_ULTRA_LOW_POWER)
	hx_s_core_fp._ulpm_in = himax_mcu_ulpm_in;
	hx_s_core_fp._black_gest_ctrl = himax_mcu_black_gest_ctrl;
#endif
	hx_s_core_fp._set_SMWP_enable = himax_mcu_set_SMWP_enable;
	hx_s_core_fp._set_HSEN_enable = himax_mcu_set_HSEN_enable;
	hx_s_core_fp._set_headphone_en = himax_mcu_set_headphone_en;
	hx_s_core_fp._usb_detect_set = himax_mcu_usb_detect_set;
	hx_s_core_fp._diag_register_set = himax_mcu_diag_register_set;
	// hx_s_core_fp._chip_self_test = himax_mcu_chip_self_test;
	hx_s_core_fp._idle_mode = himax_mcu_idle_mode;
	hx_s_core_fp._reload_disable = himax_mcu_reload_disable;
	hx_s_core_fp._read_ic_trigger_type = himax_mcu_read_ic_trigger_type;
	hx_s_core_fp._read_i2c_status = himax_mcu_read_i2c_status;
	hx_s_core_fp._read_FW_ver = hx85200a_read_FW_ver;
	hx_s_core_fp._show_FW_ver = hx85200a_show_FW_ver;
	hx_s_core_fp._read_event_stack = hx85200a_read_event_stack;
	hx_s_core_fp._return_event_stack = himax_mcu_return_event_stack;
	hx_s_core_fp._calculateChecksum = himax_mcu_calculateChecksum;
	hx_s_core_fp._read_FW_status = himax_mcu_read_FW_status;
	hx_s_core_fp._irq_switch = himax_mcu_irq_switch;
	hx_s_core_fp._assign_sorting_mode = himax_mcu_assign_sorting_mode;
	hx_s_core_fp._ap_notify_fw_sus = NULL;

	hx_s_core_fp._chip_erase = himax_mcu_chip_erase;
	hx_s_core_fp._block_erase = himax_mcu_block_erase;
	hx_s_core_fp._sector_erase = himax_mcu_sector_erase;
	hx_s_core_fp._flash_programming = himax_mcu_flash_programming;
	hx_s_core_fp._flash_page_write = himax_mcu_flash_page_write;
	hx_s_core_fp._fts_ctpm_fw_upgrade_with_sys_fs =
			himax_mcu_fts_ctpm_fw_upgrade_with_sys_fs;
	hx_s_core_fp._flash_dump_func = himax_mcu_flash_dump_func;
	hx_s_core_fp._flash_lastdata_check = himax_mcu_flash_lastdata_check;
	hx_s_core_fp._bin_desc_data_get = hx_bin_desc_data_get;
	hx_s_core_fp._bin_desc_get = hx85200a_bin_desc_get;
	hx_s_core_fp._diff_overlay_flash = hx_mcu_diff_overlay_flash;
	hx_s_core_fp._write_read_reg = himax_mcu_write_read_reg;
	hx_s_core_fp._get_DSRAM_data = himax_mcu_get_DSRAM_data;

#if defined(HX_RST_PIN_FUNC)
	hx_s_core_fp._pin_reset = himax_mcu_pin_reset;
#endif
	hx_s_core_fp._system_reset = hx85200a_system_reset;
	hx_s_core_fp._leave_safe_mode = himax_mcu_leave_safe_mode;
	hx_s_core_fp._ic_reset = himax_mcu_ic_reset;
	hx_s_core_fp._tp_info_check = hx85200a_tp_info_check;
	hx_s_core_fp._touch_information = himax_mcu_touch_information;
	hx_s_core_fp._calc_touch_data_size = himax_mcu_calcTouchDataSize;
	hx_s_core_fp._get_touch_data_size = himax_mcu_get_touch_data_size;
	hx_s_core_fp._hand_shaking = himax_mcu_hand_shaking;
	hx_s_core_fp._cal_data_len = himax_mcu_cal_data_len;
	hx_s_core_fp._diag_check_sum = himax_mcu_diag_check_sum;
	hx_s_core_fp._diag_parse_raw_data = himax_mcu_diag_parse_raw_data;
#if defined(HX_EXCP_RECOVERY)
	hx_s_core_fp._excp_hw_reset = himax_mcu_excp_hw_reset;
	hx_s_core_fp._excp_event_chk = himax_mcu_excp_event_chk;
	hx_s_core_fp._ic_excp_recovery = himax_mcu_ic_excp_recovery;
	hx_s_core_fp._excp_ic_reset = himax_mcu_excp_ic_reset;
#endif

	hx_s_core_fp._resend_cmd_func = hx85200a_resend_cmd_func;

#if defined(HX_TP_PROC_GUEST_INFO)
	hx_s_core_fp.guest_info_get_status = himax_guest_info_get_status;
	hx_s_core_fp.read_guest_info = hx_read_guest_info;
#endif
#if defined(CONFIG_TOUCHSCREEN_HIMAX_INSPECT)
	hx_s_core_fp._check_sorting_mode = himax_mcu_check_sorting_mode;
	hx_s_core_fp._turn_on_mp_func = hx_turn_on_mp_func;
	hx_s_core_fp._get_noise_base = hx85200a_get_noise_base;
	hx_s_core_fp._neg_noise_sup = hx85200a_neg_noise_sup;
	hx_s_core_fp._get_noise_weight_test = hx85200a_get_noise_weight_test;
	hx_s_core_fp._get_palm_num = hx85200a_get_palm_num;
	hx_s_core_fp._switch_mode = hx85200a_switch_mode_inspection;
	hx_s_core_fp._switch_data_type = hx85200a_switch_data_type;
	hx_s_core_fp._check_mode = hx85200a_check_mode;
	hx_s_core_fp._wait_sorting_mode = hx85200a_wait_sorting_mode;
#endif
	hx_s_core_fp._suspend_proc = hx85200a_suspend_proc;
	hx_s_core_fp._resume_proc = hx85200a_resume_proc;

	hx_s_core_fp._burst_enable = hx85200a_burst_enable;
	hx_s_core_fp._register_read = hx85200a_register_read;
	hx_s_core_fp._register_write = hx85200a_register_write;

	hx_s_core_fp._sense_off = hx85200a_sense_off;
	hx_s_core_fp._sense_on = hx85200a_sense_on;
	hx_s_core_fp._chip_init = hx85200a_chip_init;
#if defined(HX_ZERO_FLASH)
	hx_s_core_fp._en_hw_crc = NULL;
	hx_s_core_fp._opt_crc_clear = NULL;
#endif
#ifdef HX_CASCADE
	hx_s_core_fp._cascade_cmd = hx85200a_cascade_cmd;
#endif
}

/* init end*/
/* CORE_INIT */


static bool hx85200a_chip_detect(void)
{
	uint8_t tmp_data[DATA_LEN_4];
	bool ret = false;
	int i = 0;
	int j = 0;

	if (himax_bus_read(ADDR_AHB_CONTINOUS, tmp_data, 1) < 0) {
		E("%s: bus access fail!\n", __func__);
		return false;
	}

	if (hx85200a_sense_off(false) == false) {
		ret = false;
		E("%s:_sense_off Fail:\n", __func__);
		return ret;
	}
	for (i = 0; i < 5; i++) {
		if (hx85200a_register_read(HX85200A_REG_ICID,
			tmp_data, DATA_LEN_4) != 0) {
			ret = false;
			E("%s:_register_read Fail:\n", __func__);
			return ret;
		}
		I("%s:Read driver IC ID = %X, %X, %X\n", __func__,
				tmp_data[3], tmp_data[2], tmp_data[1]);

		if ((tmp_data[3] == 0x85)
		&& (tmp_data[2] == 0x20)) {
			strlcpy(hx_s_ts->chip_name,
				HX_85200A_SERIES_PWON, 30);
			(hx_s_ic_data)->ic_adc_num =
				HX85200A_DATA_ADC_NUM;

			hx_s_ic_data->isram_num =
				HX85200A_ISRAM_NUM;

			hx_s_ic_data->isram_sz =
				kzalloc(sizeof(uint32_t) *
				HX85200A_ISRAM_NUM,
				GFP_KERNEL);
			hx_s_ic_data->isram_addr =
				kzalloc(sizeof(uint32_t) *
				HX85200A_ISRAM_NUM,
				GFP_KERNEL);

			hx_s_ic_data->isram_sz[0] =
				HX85200A_ISRAM_SZ1;
			hx_s_ic_data->isram_sz[1] =
				HX85200A_ISRAM_SZ2;
			hx_s_ic_data->isram_sz[2] =
				HX85200A_ISRAM_SZ3;
			hx_s_ic_data->isram_addr[0] =
				HX85200A_ISRAM_ADDR1;
			hx_s_ic_data->isram_addr[1] =
				HX85200A_ISRAM_ADDR2;
			hx_s_ic_data->isram_addr[2] =
				HX85200A_ISRAM_ADDR3;

			hx_s_ic_data->dsram_sz =
				kzalloc(sizeof(uint32_t) *
				HX85200A_DSRAM_NUM,
				GFP_KERNEL);
			hx_s_ic_data->dsram_addr =
				kzalloc(sizeof(uint32_t) *
				HX85200A_DSRAM_NUM,
				GFP_KERNEL);

			hx_s_ic_data->dsram_sz[0] =
				HX85200A_DSRAM_SZ1;
			hx_s_ic_data->dsram_sz[1] =
				HX85200A_DSRAM_SZ2;
			hx_s_ic_data->dsram_sz[2] =
				HX85200A_DSRAM_SZ3;
			hx_s_ic_data->dsram_sz[3] =
				HX85200A_DSRAM_SZ4;
			hx_s_ic_data->dsram_addr[0] =
				HX85200A_DSRAM_ADDR1;
			hx_s_ic_data->dsram_addr[1] =
				HX85200A_DSRAM_ADDR2;
			hx_s_ic_data->dsram_addr[2] =
				HX85200A_DSRAM_ADDR3;
			hx_s_ic_data->dsram_addr[3] =
				HX85200A_DSRAM_ADDR4;


			hx_s_ic_data->flash_size =
				HX85200A_FLASH_SZ;

			hx_s_ic_data->dbg_reg_ary[0] = HX85200A_ADDR_FW_DBG_MSG;
			hx_s_ic_data->dbg_reg_ary[1] = ADDR_CHK_FW_STATUS;
			hx_s_ic_data->dbg_reg_ary[2] = ADDR_CHK_DD_STATUS;
			hx_s_ic_data->dbg_reg_ary[3] = ADDR_FLAG_RESET_EVENT;

			hx_s_ic_data->zero_event = 0;
			hx_s_ic_data->eb_event = 0;
			hx_s_ic_data->ec_event = 0;
			hx_s_ic_data->ed_event = 0;
			hx_s_ic_data->ee_event = 0;

#if defined(HX_ZERO_FLASH)
			g_zf_opt_crc.en_crc_clear = false;
			g_zf_opt_crc.fw_addr = 0;
			g_zf_opt_crc.en_opt_hw_crc = NO_OPT_CRC;
#endif
			hx_s_ic_setup = g_hx85200a_setup;
			hx85200a_init();

			I("%s:IC name = %s\n", __func__,
				hx_s_ts->chip_name);

			for (j = 0; j < HX85200A_ISRAM_NUM; j++) {
				I("isram[%d]addr=0x%08X,sz=0x%08x\n", j,
					hx_s_ic_data->isram_addr[j],
					hx_s_ic_data->isram_sz[j]);
			}

			I("Himax IC package %x%x%x in\n", tmp_data[3],
					tmp_data[2], tmp_data[1]);
			ret = true;
			goto FINAL;
		} else {
			ret = false;
			E("%s:Read driver ID register Fail:\n", __func__);
			E("Could NOT find Himax Chipset\n");
			E("Please check 1.VCCD,VCCA,VSP,VSN\n");
			E("2. LCM_RST,TP_RST\n");
			E("3. Power On Sequence\n");
		}
	}
FINAL:

	return ret;
}

bool _hx85200a_init(void)
{
	bool ret = false;

	I("%s\n", __func__);
	ret = hx85200a_chip_detect();
	return ret;
}


