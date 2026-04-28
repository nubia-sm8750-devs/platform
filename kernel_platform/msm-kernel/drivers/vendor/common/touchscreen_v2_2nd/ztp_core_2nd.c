/***********************
 * file : ztp_core_2nd.c
 **********************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/power_supply.h>
#include <linux/pm_wakeup.h>
#include "ztp_core_2nd.h"
#include "ztp_common_2nd.h"
//#include <vendor/soc/mediatek/board_state.h> /*add for meta mode*/

struct ztp_device_2nd *tpd_cdev_2nd = NULL;
struct proc_dir_entry *tpd_proc_dir_2nd = NULL;
char lcd_name_2nd[MAX_LCD_NAME_LEN] = { 0 };
char tp_error_info_2nd[MAX_LCD_NAME_LEN] = { 0 };

struct tp_ic_vendor_info_2nd tp_ic_vendor_info_l_2nd[] = {
	{TS_CHIP_SYNAPTICS, "synaptics"},
	{TS_CHIP_FOCAL, "focal"},
	{TS_CHIP_GOODIX, "goodix"	},
	{TS_CHIP_HIMAX, "himax"},
	{TS_CHIP_NOVATEK, "novatek"},
	{TS_CHIP_ILITEK, "ilitek"},
	{TS_CHIP_TLSC, "tlsc"},
	{TS_CHIP_CHIPONE, "chipone"},
	{TS_CHIP_GCORE, "galaxycore"},
	{TS_CHIP_OMNIVISION, "omnivision"},
	{TS_CHIP_BTL, "btltp"},
	{TS_CHIP_SEMI, "semi"},
	{TS_CHIP_SITRONIX,"sitronix"},
	{TS_CHIP_ASX,"aixiesheng"},
	{TS_CHIP_MAX, "Unknown"},
};

struct ztp_algo_info_2nd ztp_algo_info_l_2nd[] = {
	{zte_algo_enable_2nd, "algo_open_2nd"},
	{tp_jitter_check_pixel_2nd, "jitter_pixel_2nd"},
	{tp_jitter_timer_2nd_2nd, "jitter_timer_2nd"},
	{tp_edge_click_suppression_pixel_2nd, "click_pixel_2nd"},
	{tp_long_press_enable_2nd, "long_press_open_2nd"},
	{tp_long_press_timer_2nd, "long_press_timer_2nd"},
	{tp_long_press_pixel_2nd, "long_press_pixel_2nd"},
};

struct ztp_error_info_2nd tp_error_info_l_2nd[] = {
	{TP_I2C_R_ERROR_NO, "i2c_read_error"},
	{TP_I2C_W_ERROR_NO, "i2c_write_error"},
	{TP_SPI_R_ERROR_NO, "spi_read_error"},
	{TP_SPI_W_ERROR_NO, "spi_write_error"},
	{TP_CRC_ERROR_NO, "tp_crc_error"},
	{TP_FW_UPGRADE_ERROR_NO, "fw_upgrade_error"},
	{TP_ESD_CHECK_ERROR_NO, "tp_esd_error"},
	{TP_PROBE_ERROR_NO, "tp_probe_error"},
	{TP_SUSPEND_GESTURE_OPEN_NO, "suspend_open_gesture"},
	{TP_REQUEST_FIRMWARE_ERROR_NO, "request_fw_error"},
	{TP_GHOST_ERROR_NO, "ghost_check_error"},
	{TP_SELF_TEST_ERROR_NO, "tp_self_test_error"},
	{TP_GET_NOISE_ERROR_NO, "tp_get_noise_error"},
	{TP_ERROR_NO_MAX, "Unknown"},
};

#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
struct zlog_mod_info zlog_tp_dev_2nd = {
	.module_no = ZLOG_MODULE_TP,
	.name = "touchscreen_2nd",
	.device_name = "Unknown",
	.ic_name = "Unknown",
	.module_name = "TP_2nd",
	.fops = NULL,
};
#endif

int get_tp_algo_item_id_2nd(char *buf)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(ztp_algo_info_l_2nd); i++) {
		if (strnstr(buf, ztp_algo_info_l_2nd[i].ztp_algo_item_name_2nd, strlen(buf))) {
			TPD_DMESG("ztp_algo_item_id_2nd:%d.\n", ztp_algo_info_l_2nd[i].ztp_algo_item_id_2nd);
			return ztp_algo_info_l_2nd[i].ztp_algo_item_id_2nd;
		}
	}
	return -EIO;
}

int get_tp_chip_id_2nd(void)
{
	int i = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter:\n");
	cdev->tp_chip_id_2nd = TS_CHIP_MAX;
	TPD_DMESG("lcd_name_2nd %s.\n", lcd_name_2nd);
	for (i = 0; i < ARRAY_SIZE(tp_ic_vendor_info_l_2nd); i++) {
		if (strnstr(lcd_name_2nd, tp_ic_vendor_info_l_2nd[i].tp_ic_vendor_name_2nd, strlen(lcd_name_2nd))) {
			cdev->tp_chip_id_2nd = tp_ic_vendor_info_l_2nd[i].tp_chip_id_2nd;
			TPD_DMESG("tp_chip_id_2nd is 0x%02x.\n", cdev->tp_chip_id_2nd);
			return 0;
		}
	}
	return -EIO;
}

int get_tp_error_info_2nd(tp_error_no_2nd error_no_2nd)
{
	int i = 0;

	TPD_DMESG("tp 2nd error no: %d.\n", error_no_2nd);
	for (i = 0; i < ARRAY_SIZE(tp_error_info_l_2nd); i++) {
		if (tp_error_info_l_2nd[i].error_no_2nd == error_no_2nd) {
			snprintf(tp_error_info_2nd, sizeof(tp_error_info_2nd), "%s", tp_error_info_l_2nd[i].tp_error_info_2nd);
			return 0;
		}
	}
	snprintf(tp_error_info_2nd, sizeof(tp_error_info_2nd), "%s", "tp_2nd_error_unknown");
	return -EIO;
}

/* Started by AICoder, pid:q4642da288q6f2414b490bf76093413ae490782b */
int get_lcd_panel_name_2nd(void)
{
    struct device_node *cmdline_node_2nd;
    const char *cmd_line_2nd, *lcd_name_p_2nd;
    int ret = 0;

    memset(lcd_name_2nd, 0, sizeof(lcd_name_2nd));
    cmdline_node_2nd = of_find_node_by_path("/chosen");
    if (cmdline_node_2nd == NULL) {
        TPD_DMESG("Can't find /chosen node\n");
        return -ENODEV;
    }

    ret = of_property_read_string(cmdline_node_2nd, "bootargs", &cmd_line_2nd);
    if (ret < 0) {
        TPD_DMESG("Can't read bootargs property\n");
        return ret;
    }

    lcd_name_p_2nd = strstr(cmd_line_2nd, "lcd_name_2nd=");
    if (lcd_name_p_2nd) {
        sscanf(lcd_name_p_2nd, "lcd_name_2nd=%s", lcd_name_2nd);
        TPD_DMESG("lcd 2nd name: %s\n", lcd_name_2nd);
    } else {
        snprintf(lcd_name_2nd, sizeof(lcd_name_2nd), "Unknown_lcd_2nd");
        TPD_DMESG("lcd_name_2nd not found in bootargs\n");
    }

    return ret;
}
/* Ended by AICoder, pid:q4642da288q6f2414b490bf76093413ae490782b */

void tp_free_tp_firmware_data_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->tp_firmware != NULL) {
		if (cdev->tp_firmware->data != NULL) {
			vfree(cdev->tp_firmware->data);
			cdev->tp_firmware->data = NULL;
			cdev->tp_firmware->size = 0;
		}
		kfree(cdev->tp_firmware);
		cdev->tp_firmware = NULL;
	}
	cdev->fw_data_pos = 0;
}


int tp_alloc_tp_firmware_data_2nd(int buf_size)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	tp_free_tp_firmware_data_2nd();
	cdev->tp_firmware = kzalloc(sizeof(struct firmware), GFP_KERNEL);
	if (cdev->tp_firmware == NULL) {
		TPD_DMESG("alloc struct firmware failed");
		return -ENOMEM;
	}
	cdev->tp_firmware->data = vmalloc(sizeof(*cdev->tp_firmware) + buf_size);
	if (!cdev->tp_firmware->data) {
		TPD_DMESG("alloc tp_firmware->data failed");
		kfree(cdev->tp_firmware);
		return -ENOMEM;
	}
	cdev->tp_firmware->size = buf_size;
	memset((char *)cdev->tp_firmware->data, 0x00, sizeof(*cdev->tp_firmware) + buf_size);
	return 0;
}

int  tpd_copy_to_tp_firmware_data_2nd(char *buf)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int len = 0;

	if (!cdev->tp_firmware || !cdev->tp_firmware->data) {
		TPD_DMESG("Need set fw image size first");
		return -ENOMEM;
	}

	if (cdev->tp_firmware->size == 0) {
		TPD_DMESG("Invalid firmware size");
		return -EINVAL;
	}

	if (cdev->fw_data_pos >= cdev->tp_firmware->size)
		return 0;
	len = strlen(buf);
	if (cdev->fw_data_pos + len > cdev->tp_firmware->size)
		len = cdev->tp_firmware->size - cdev->fw_data_pos;
	memcpy((u8 *)&cdev->tp_firmware->data[cdev->fw_data_pos], buf, len);
	cdev->fw_data_pos += len;
	return len;
}

void  tpd_reset_fw_data_pos_and_size_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	cdev->tp_firmware->size = cdev->fw_data_pos;
	cdev->fw_data_pos = 0;
}
static ssize_t tp_module_info_read_2nd(struct file *file,
	char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t buffer_tpd[200];
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	if (cdev->get_tpinfo_2nd) {
		cdev->get_tpinfo_2nd(cdev);
	}

	len += snprintf(buffer_tpd + len, sizeof(buffer_tpd) - len, "TP module: %s(0x%x)\n",
			cdev->ic_tpinfo.vendor_name, cdev->ic_tpinfo.module_id);
	len += snprintf(buffer_tpd + len, sizeof(buffer_tpd) - len, "IC type : %s\n",
			cdev->ic_tpinfo.tp_name);
	if (cdev->ic_tpinfo.i2c_addr)
		len += snprintf(buffer_tpd + len, sizeof(buffer_tpd) - len, "I2C address: 0x%x\n",
			cdev->ic_tpinfo.i2c_addr);
	if (cdev->ic_tpinfo.spi_num)
		len += snprintf(buffer_tpd + len, sizeof(buffer_tpd) - len, "Spi num: %d\n",
			cdev->ic_tpinfo.spi_num);
	if (cdev->tp_chip_id_2nd == TS_CHIP_OMNIVISION) {
		len += snprintf(buffer_tpd + len, sizeof(buffer_tpd) - len, "Firmware version : %d\n",
				cdev->ic_tpinfo.firmware_ver);
	} else {
		len += snprintf(buffer_tpd + len, sizeof(buffer_tpd) - len, "Firmware version : 0x%x\n",
				cdev->ic_tpinfo.firmware_ver);
	}
	if (cdev->ic_tpinfo.config_ver)
		len += snprintf(buffer_tpd + len, sizeof(buffer_tpd) - len, "Config version:0x%x\n",
			cdev->ic_tpinfo.config_ver);
	if (cdev->ic_tpinfo.display_ver)
		len += snprintf(buffer_tpd + len, sizeof(buffer_tpd) - len, "Display version:0x%x\n",
			cdev->ic_tpinfo.display_ver);
	if (cdev->ic_tpinfo.chip_batch[0])
		len += snprintf(buffer_tpd + len, sizeof(buffer_tpd) - len, "Chip hard version:%s\n",
			cdev->ic_tpinfo.chip_batch);
	return simple_read_from_buffer(buffer, count, offset, buffer_tpd, len);
}

static ssize_t tp_wake_gesture_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}

	if (!cdev->TP_have_registered) {
		TPD_DMESG("cdev->TP_have_registered is null, return");
		return 0;
	}

	if (cdev->get_gesture_2nd) {
		cdev->get_gesture_2nd(cdev);
	}
	TPD_DMESG("val:%d.\n", cdev->b_gesture_enable);

	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->b_gesture_enable);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_wake_gesture_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (!cdev->TP_have_registered) {
		TPD_DMESG("cdev->TP_have_registered is null, return");
		return -EINVAL;
	}

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;
	input = input > 0 ? 1 : 0;
	TPD_DMESG("val %d.\n", input);

	if (cdev->wake_gesture_2nd) {
		cdev->wake_gesture_2nd(cdev, input);
	}
	return len;
}
static ssize_t tp_smart_cover_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	if (cdev->get_smart_cover_2nd) {
		cdev->get_smart_cover_2nd(cdev);
	}
	TPD_DMESG("val:%d.\n", cdev->b_smart_cover_enable);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->b_smart_cover_enable);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_smart_cover_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;
	input = input > 0 ? 1 : 0;
	TPD_DMESG("val %d.\n", input);
	if (cdev->set_smart_cover_2nd) {
		cdev->set_smart_cover_2nd(cdev, input);
	}
	return len;
}
static ssize_t tp_glove_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	if (cdev->get_glove_mode_2nd) {
		cdev->get_glove_mode_2nd(cdev);
	}
	TPD_DMESG("val:%d.\n", cdev->b_glove_enable);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->b_glove_enable);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_glove_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;
	input = input > 0 ? 1 : 0;
	TPD_DMESG("val %d.\n", input);
	if (cdev->set_glove_mode_2nd) {
		cdev->set_glove_mode_2nd(cdev, input);
	}
	return len;
}
static ssize_t tpfwupgrade_store_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int fw_size = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &fw_size);
	if (ret)
		return -EINVAL;
	TPD_DMESG("val %d.\n", fw_size);
	mutex_lock(&cdev->cmd_mutex);
	if (fw_size > 10) {
		if (cdev->tp_firmware != NULL) {
			if (cdev->tp_firmware->data != NULL)
				vfree(cdev->tp_firmware->data);
			kfree(cdev->tp_firmware);
		}
		cdev->fw_data_pos = 0;
		cdev->tp_firmware = kzalloc(sizeof(struct firmware), GFP_KERNEL);
		if (cdev->tp_firmware == NULL) {
			TPD_DMESG("alloc struct firmware failed");
			mutex_unlock(&cdev->cmd_mutex);
			return -ENOMEM;
		}
		cdev->tp_firmware->data = vmalloc(sizeof(*cdev->tp_firmware) + fw_size);
		if (!cdev->tp_firmware->data) {
			TPD_DMESG("alloc tp_firmware->data failed");
			kfree(cdev->tp_firmware);
			mutex_unlock(&cdev->cmd_mutex);
			return -ENOMEM;
		}
		cdev->tp_firmware->size = fw_size;
		memset((char *)cdev->tp_firmware->data, 0x00, sizeof(*cdev->tp_firmware) + fw_size);
	} else if (cdev->tp_firmware != NULL) {
		if (cdev->tp_fw_upgrade_2nd) {
			cdev->tp_fw_upgrade_2nd(cdev, NULL, 0);
		}
		if (cdev->tp_firmware->data != NULL) {
			vfree(cdev->tp_firmware->data);
			cdev->tp_firmware->data = NULL;
		}
		kfree(cdev->tp_firmware);
		cdev->tp_firmware = NULL;
		cdev->fw_data_pos = 0;
	}
	mutex_unlock(&cdev->cmd_mutex);
	return len;
}

static ssize_t suspend_show_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[30] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	if (cdev->tp_suspend_show_2nd) {
		cdev->tp_suspend_show_2nd(cdev);
	}
	TPD_DMESG("val:%d.\n", cdev->tp_suspend);
	len = snprintf(data_buf, sizeof(data_buf), "tp suspend is: %u\n", cdev->tp_suspend);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t suspend_store_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;
	input = input > 0 ? 1 : 0;
	TPD_DMESG("val %d.\n", input);

	mutex_lock(&cdev->cmd_mutex);
	if (cdev->sys_set_tp_suspend_flag == input) {
		TPD_DMESG("tp state don't need change.\n");
		mutex_unlock(&cdev->cmd_mutex);
		return len;
	}
	cdev->sys_set_tp_suspend_flag = input;
	if (cdev->set_tp_suspend_2nd) {
		cdev->set_tp_suspend_2nd(cdev, PROC_SUSPEND_NODE, input);
	}
	mutex_unlock(&cdev->cmd_mutex);
	return len;
}

static ssize_t tp_single_tap_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0)
		return 0;

	if (cdev->get_singletap_2nd)
		cdev->get_singletap_2nd(cdev);

	TPD_DMESG("val: %d.\n", cdev->b_single_tap_enable);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->b_single_tap_enable);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_single_tap_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	input = input > 0 ? 5 : 0;
	TPD_DMESG("val = %d\n", input);

	if (cdev->set_singletap_2nd)
		cdev->set_singletap_2nd(cdev, input);

	return len;
}

static ssize_t tp_single_aod_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0)
		return 0;

	if (cdev->get_singleaod_2nd)
		cdev->get_singleaod_2nd(cdev);

	TPD_DMESG("val: %d.\n", cdev->b_single_aod_enable);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->b_single_aod_enable);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_single_aod_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	input = input > 0 ? 5 : 0;
	TPD_DMESG("val = %d\n", input);

	if (cdev->set_singleaod_2nd)
		cdev->set_singleaod_2nd(cdev, input);

	return len;
}

static ssize_t tp_edge_report_limit_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	char *data_buf = NULL;
	int i = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0)
		return 0;
	data_buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (data_buf == NULL) {
		TPD_DMESG("alloc data_buf failed");
		return -ENOMEM;
	}

	len += snprintf(data_buf + len, PAGE_SIZE - len, "#######################################");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "#######################################\n\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "algo_open_2nd, echo algo_open_2nd:1 > edge_report_limit_2nd\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "jitter_pixel_2nd, echo jitter_pixel_2nd:10 > edge_report_limit_2nd\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "jitter_timer_2nd, echo jitter_timer_2nd:100 > edge_report_limit_2nd\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "click_pixel_2nd, echo click_pixel_2nd:10 > edge_report_limit_2nd\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "long_press_open_2nd, echo long_press_open_2nd:1 > edge_report_limit_2nd\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "long_press_timer_2nd, echo long_press_timer_2nd:500 > edge_report_limit_2nd\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "pixel limit level,user setting. echo 5 > edge_report_limit_2nd\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "long_press_pixel_2nd, echo long_press_pixel_2nd:10,10,20,20 > edge_report_limit_2nd\n\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len,  "#######################################");
	len += snprintf(data_buf + len, PAGE_SIZE - len,  "#######################################\n\n");

	len += snprintf(data_buf + len, PAGE_SIZE - len, "algo_open_2nd:%5u\n", cdev->zte_tp_algo_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len, "jitter_pixel_2nd:%5u\n", cdev->tp_jitter_check_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len, "jitter_timer_2nd:%5u\n", cdev->tp_jitter_timer_2nd_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len, "click_pixel_2nd:%5u\n", cdev->edge_click_sup_p_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len, "long_press_open_2nd:%5u\n", cdev->edge_long_press_check_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len, "long_press_timer_2nd:%5u\n", cdev->edge_long_press_timer_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len, "pixel limit level:%5u\n", cdev->edge_limit_pixel_level);

	len += snprintf(data_buf + len, PAGE_SIZE - len, "click_pixel_2nd width:");
	for (i = 0; i < MAX_LIMIT_NUM; i++) {
		if (len >= (PAGE_SIZE - 5))
			break;
		len += snprintf(data_buf + len, PAGE_SIZE - len, "%5u", cdev->edge_report_limit_2nd[i]);
	}
	len += snprintf(data_buf + len, PAGE_SIZE - len, "\n long_press_pixel_2nd:");
	for (i = 0; i < MAX_LIMIT_NUM; i++) {
		if (len >= (PAGE_SIZE - 5))
			break;
		len += snprintf(data_buf + len, PAGE_SIZE - len, "%5u", cdev->long_pess_suppression_2nd[i]);
	}
	len += snprintf(data_buf + len, PAGE_SIZE - len, "\n");
	simple_read_from_buffer(buffer, count, offset, data_buf, len);
	kfree(data_buf);
	return len;
}

static ssize_t tp_edge_report_limit_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0, i = 0;
	u8 *token = NULL;
	char *cur = NULL;
	char buff[100] = { 0 };
	u16 count = 0;
	unsigned int  s_to_u8 = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int algo_item = 0;

	len = (len < sizeof(buff)) ? len : sizeof(buff);
	if (buffer != NULL) {
		if (copy_from_user(buff, buffer, len)) {
			TPD_DMESG("Failed to copy data from user space\n");
			len = -EINVAL;
			goto out;
		}
	}
	algo_item = get_tp_algo_item_id_2nd(buff);
	if (algo_item < 0) {
		ret = kstrtouint_from_user(buffer, len, 10, &input);
		if (ret || input > 10)
			return -EINVAL;
		cdev->edge_limit_pixel_level = input;
		/* user set level  0-5: x_max x 1% increase
		     user set level  6-10:  x_max x 0.5% increase
		*/
#ifdef CONFIG_EDGE_MISTOUCH_PREVENTION_NARROW_2nd
		if (cdev->edge_limit_pixel_level <= 5)
			cdev->user_edge_limit[0] = cdev->max_x * cdev->edge_limit_pixel_level * 5 / 1000;
		else
			cdev->user_edge_limit[0] = cdev->max_x * 25  / 1000
				+ (cdev->max_x  * 3 / 1000) * (cdev->edge_limit_pixel_level - 5);
#else
		if (cdev->edge_limit_pixel_level <= 5)
			cdev->user_edge_limit[0] = cdev->max_x * cdev->edge_limit_pixel_level * 7 / 1000;
		else
			cdev->user_edge_limit[0] = cdev->max_x * 35  / 1000
				+ (cdev->max_x  * 4 / 1000) * (cdev->edge_limit_pixel_level - 5);
#endif
		cdev->user_edge_limit[1] = 0;
		TPD_DMESG("edge_limit_pixel_level = %d, limit[0,1] = [%d,%d]\n",
			cdev->edge_limit_pixel_level, cdev->user_edge_limit[0], cdev->user_edge_limit[1]);
	} else {
		cur = strchr(buff, ':');
		cur++;
		TPD_DMESG("cur = %s\n", cur);
		switch (algo_item) {
		case zte_algo_enable_2nd:
			ret = kstrtouint(cur, 10, &s_to_u8);
			if (ret == 0) {
				s_to_u8 = s_to_u8 > 0 ? 1 : 0;
				cdev->zte_tp_algo_2nd = s_to_u8;
				TPD_DMESG("zte_tp_algo_2nd = %d\n", cdev->zte_tp_algo_2nd);
			}
			break;
		case tp_jitter_check_pixel_2nd:
			ret = kstrtouint(cur, 10, &s_to_u8);
			if (ret == 0) {
				cdev->tp_jitter_check_2nd = s_to_u8;
				TPD_DMESG("tp_jitter_check_2nd = %d\n", cdev->tp_jitter_check_2nd);
			}
			break;
		case tp_jitter_timer_2nd_2nd:
			ret = kstrtouint(cur, 10, &s_to_u8);
			if (ret == 0) {
				cdev->tp_jitter_timer_2nd_2nd = s_to_u8;
				TPD_DMESG("tp_jitter_timer_2nd_2nd = %d\n", cdev->tp_jitter_timer_2nd_2nd);
			}
			break;
		case tp_edge_click_suppression_pixel_2nd:
			ret = kstrtouint(cur, 10, &s_to_u8);
			if (ret == 0) {
				cdev->edge_click_sup_p_2nd = s_to_u8;
				TPD_DMESG("tp_edge_click_suppression_pixel_2nd = %d\n", cdev->edge_click_sup_p_2nd);
				for (i = 0; i < 4; i++)
					cdev->edge_report_limit_2nd[i] = cdev->edge_click_sup_p_2nd;
			}
			break;
		case tp_long_press_enable_2nd:
			ret = kstrtouint(cur, 10, &s_to_u8);
			if (ret == 0) {
				s_to_u8 = s_to_u8 > 0 ? 1 : 0;
				cdev->edge_long_press_check_2nd = s_to_u8;
				TPD_DMESG("edge_long_press_check_2nd = %d\n", cdev->edge_long_press_check_2nd);
			}
			break;
		case tp_long_press_timer_2nd:
			ret = kstrtouint(cur, 10, &s_to_u8);
			if (ret == 0) {
				cdev->edge_long_press_timer_2nd = s_to_u8;
				TPD_DMESG("zte_tp_algo_2nd = %d\n", cdev->edge_long_press_timer_2nd);
			}
			break;
		case tp_long_press_pixel_2nd:
			while ((token = strsep(&cur, ",")) != NULL) {
			ret = kstrtouint(token, 10, &s_to_u8);
			if (ret == 0) {
				cdev->long_pess_suppression_2nd[count] = s_to_u8;
				TPD_DMESG("long_pess_suppression_2nd[%d] = %d\n", count, cdev->long_pess_suppression_2nd[count]);
				count++;
			}
			if (count >= MAX_LIMIT_NUM)
				break;
			}
			break;
		default:
			TPD_DMESG("ignore algo item");
			break;
		}
	}

out:
	return len;
}

static ssize_t get_one_key_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	if (cdev->get_one_key_2nd) {
		cdev->get_one_key_2nd(cdev);
	}
	TPD_DMESG("val:%d.\n", cdev->one_key_enable);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->one_key_enable);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t set_one_key_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	input = input > 0 ? 1 : 0;

	TPD_DMESG("val = %d\n", input);

	if (cdev->set_one_key_2nd) {
		cdev->set_one_key_2nd(cdev, input);
	}
	cdev->one_key_enable = input;
	return len;
}

static ssize_t get_play_game_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	if (cdev->get_play_game_2nd) {
		cdev->get_play_game_2nd(cdev);
	}
	TPD_DMESG("val:%d.\n", cdev->play_game_enable);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->play_game_enable);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t set_play_game_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	/*input = input > 0 ? 1 : 0;*/

	TPD_DMESG("val = %d\n", input);

	if (cdev->set_play_game_2nd) {
		cdev->set_play_game_2nd(cdev, input);
	}

	return len;
}

static ssize_t get_tp_report_rate_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	if (cdev->get_tp_report_rate_2nd) {
		cdev->get_tp_report_rate_2nd(cdev);
	}
	TPD_DMESG("val:%d.\n", cdev->tp_report_rate);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->tp_report_rate);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t set_tp_report_rate_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	/*input = input > 0 ? 1 : 0;*/

	TPD_DMESG("val = %d\n", input);

	if (cdev->set_tp_report_rate_2nd) {
		cdev->set_tp_report_rate_2nd(cdev, input);
	}

	return len;
}

static ssize_t get_tp_noise_show_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	int retval = -1;
	uint8_t data_buf[30] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}

	mutex_lock(&cdev->cmd_mutex);
	if (cdev->get_noise_2nd) {
		TPD_DMESG("get tp noise start need tp resume.");
		cdev->need_tp_resume = true;
		reinit_completion(&cdev->tp_event_completion);
		retval = cdev->get_noise_2nd(cdev);
		cdev->need_tp_resume = false;
		complete(&cdev->tp_event_completion);
		TPD_DMESG("get tp noise end.");
	}
	if (cdev->tp_firmware != NULL) {
		len = snprintf(data_buf, sizeof(data_buf), "%zu\n", cdev->tp_firmware->size);
		TPD_DMESG("get tp noise size:%zu.\n", cdev->tp_firmware->size);
	}

	mutex_unlock(&cdev->cmd_mutex);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t get_tp_noise_store_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	mutex_lock(&cdev->cmd_mutex);
	if (cdev->tp_firmware != NULL) {
		if (cdev->tp_firmware->data != NULL) {
			vfree(cdev->tp_firmware->data);
			cdev->tp_firmware->data = NULL;
		}
		kfree(cdev->tp_firmware);
		cdev->tp_firmware = NULL;
	}
	cdev->fw_data_pos = 0;
	mutex_unlock(&cdev->cmd_mutex);
	return len;
}

static ssize_t headset_state_show_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[30] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}

	if (!cdev->TP_have_registered) {
		TPD_DMESG("cdev->TP_have_registered is null, return");
		return 0;
	}

	if (cdev->headset_state_show_2nd) {
		cdev->headset_state_show_2nd(cdev);
	}
	TPD_DMESG("val:%d.\n", cdev->headset_state);
	len = snprintf(data_buf, sizeof(data_buf), "headset state: %u\n", cdev->headset_state);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t headset_state_store_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (!cdev->TP_have_registered) {
		TPD_DMESG("cdev->TP_have_registered is null, return");
		return -EINVAL;
	}

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;
	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;
	input = input > 0 ? 1 : 0;
	TPD_DMESG("headset_state: val %d.\n", input);

	if (cdev->set_headset_state_2nd) {
		cdev->set_headset_state_2nd(cdev, input);
	}
	return len;
}

static ssize_t get_rotation_limit_level_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	TPD_DMESG("val:%d.\n", cdev->rotation_limit_level);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->rotation_limit_level);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t set_rotation_limit_level_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	/*input = input > 0 ? 1 : 0;*/

	TPD_DMESG("val = %d\n", input);
	if (input > 3)
		input = 3;
	cdev->rotation_limit_level = input;

	return len;
}

static ssize_t display_rotation_show_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[30] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}

	TPD_DMESG("val:%d.\n", cdev->display_rotation);
	len = snprintf(data_buf, sizeof(data_buf), "display rotation: %d\n", cdev->display_rotation);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t set_display_rotation_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (!cdev->TP_have_registered) {
		TPD_DMESG("cdev->TP_have_registered is null, return");
		return -EINVAL;
	}

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;
	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;
	cdev->display_rotation = input;
	TPD_DMESG("display rotation: val %d.\n", cdev->display_rotation);

	if (cdev->set_display_rotation_2nd) {
		cdev->set_display_rotation_2nd(cdev, input);
	}
	return len;
}

static ssize_t tp_sensibility_level_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	if (cdev->get_sensibility_2nd) {
		cdev->get_sensibility_2nd(cdev);
	}
	TPD_DMESG("ensibility level:val %d.\n", cdev->sensibility_level);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->sensibility_level);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_sensibility_level_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;

	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;

	cdev->sensibility_level = input;
	TPD_DMESG("ensibility level:val %d.\n", cdev->sensibility_level);
	if (cdev->set_sensibility_2nd) {
		cdev->set_sensibility_2nd(cdev, input);
	}

	return len;
}

static ssize_t tp_pen_only_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	if (cdev->get_pen_only_mode_2nd) {
		cdev->get_pen_only_mode_2nd(cdev);
	}
	TPD_DMESG(":pen only model: %d.\n", cdev->pen_only_mode);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->pen_only_mode);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_pen_only_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;

	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;
       input = input > 0 ? 1 : 0;
	cdev->pen_only_mode = input;
	TPD_DMESG("pen only mode:%d.\n", cdev->pen_only_mode);
	if (cdev->set_pen_only_mode_2nd) {
		cdev->set_pen_only_mode_2nd(cdev, input);
	}

	return len;
}

static ssize_t tp_self_test_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	int len = 0;
	char *data_buf = NULL;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0)
		return 0;
	data_buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (data_buf == NULL) {
		TPD_DMESG("alloc data_buf failed");
		return -ENOMEM;
	}

	if (*offset != 0) {
		return 0;
	}
	if (cdev->get_tp_self_test_result_2nd) {
		len = cdev->get_tp_self_test_result_2nd(cdev, data_buf);
	}
	simple_read_from_buffer(buffer, count, offset, data_buf, len);
	kfree(data_buf);
	tp_free_tp_firmware_data_2nd();
	return len;
}

static ssize_t tp_self_test_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if(tp_alloc_tp_firmware_data_2nd(TP_TEST_FILE_SIZE)) {
		TPD_DMESG(" alloc tp firmware data fail");
		return -ENOMEM;
	}

	if (cdev->tp_self_test_2nd) {
		cdev->tp_self_test_2nd(cdev);
	}
	tpd_reset_fw_data_pos_and_size_2nd();
	return len;
}

static ssize_t get_finger_lock_flag_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	TPD_DMESG("val:%d.\n", cdev->finger_lock_flag);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->finger_lock_flag);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t set_finger_lock_flag_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	input = input > 0 ? 1 : 0;

	TPD_DMESG("val = %d\n", input);
	cdev->finger_lock_flag = input;
#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
	if (cdev->finger_lock_flag) {
		if (ufp_tp_ops_2nd.wait_completion) {
			complete(&ufp_tp_ops_2nd.ufp_completion);
			ufp_tp_ops_2nd.wait_completion = false;
		}
	}
#endif
	return len;
}

static ssize_t tp_zlog_debug_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	char *data_buf = NULL;
	int i = 0;	
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	bool tp_error_find = false;

	if (*offset != 0) {
		return 0;
	}
	data_buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (data_buf == NULL) {
		TPD_DMESG("alloc data_buf failed");
		return -ENOMEM;
	}
	for (i = 0; i < TP_ERROR_NO_MAX; i++) {
		get_tp_error_info_2nd(i);
		if (cdev->zlog_item.count[i]) {
			tp_error_find = true;
			len += snprintf(data_buf + len, PAGE_SIZE -len, "%s=true\n", tp_error_info_2nd);
			len += snprintf(data_buf + len, PAGE_SIZE -len, "%s count:%lu.\n", tp_error_info_2nd, cdev->zlog_item.count[i]);
		} else {
			len += snprintf(data_buf + len, PAGE_SIZE -len, "%s=false\n", tp_error_info_2nd);
		}
	}
	if (tp_error_find) {
		len += snprintf(data_buf + len, PAGE_SIZE -len, "tp_error_find=true\n");
		len += snprintf(data_buf + len, PAGE_SIZE -len, "%s\n", cdev->tp_error_last_log_buffer);
	} else {
		len += snprintf(data_buf + len, PAGE_SIZE -len, "tp_error_find=false\n");
	}
	simple_read_from_buffer(buffer, count, offset, data_buf, len);
	kfree(data_buf);
	return len;
}

static ssize_t tp_zlog_debug_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	char data_buf[10] = {0};

	len = len >= sizeof(data_buf) ? sizeof(data_buf) - 1 : len;
	ret = copy_from_user(data_buf, buffer, len);
	if (ret)
		return -EINVAL;

	ret = kstrtouint(data_buf, 0, &input);
	if (ret)
		return -EINVAL;
#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
	tpd_print_zlog_2nd("zlog adb debug");
#endif
  	switch (input) {
	case TP_I2C_R_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_I2C_R_ERROR_NO);
		break;
	case TP_I2C_W_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_I2C_W_ERROR_NO);
		break;
	case TP_SPI_R_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_SPI_R_ERROR_NO);
		break;
	case TP_SPI_W_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_SPI_W_ERROR_NO);
		break;
	case TP_CRC_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_CRC_ERROR_NO);
		break;
	case TP_FW_UPGRADE_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_FW_UPGRADE_ERROR_NO);
		break;
	case TP_REQUEST_FIRMWARE_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_REQUEST_FIRMWARE_ERROR_NO);
		break;
	case TP_ESD_CHECK_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_ESD_CHECK_ERROR_NO);
#ifdef CONFIG_TOUCHSCREEN_LCD_NOTIFY_2nd
		tpd_notifier_call_chain_2nd(TP_ESD_CHECK_ERROR);
#endif
		break;
	case TP_PROBE_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_PROBE_ERROR_NO);
		break;
	case TP_SUSPEND_GESTURE_OPEN_NO:
		tpd_zlog_record_notify_2nd(TP_SUSPEND_GESTURE_OPEN_NO);
		break;
	case TP_SELF_TEST_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_SELF_TEST_ERROR_NO);
		break;
	case TP_GET_NOISE_ERROR_NO:
		tpd_zlog_record_notify_2nd(TP_GET_NOISE_ERROR_NO);
		break;
	default:
		break;
	}
	return len;
}

static ssize_t tp_debug_log_enable_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0)
		return 0;

	TPD_DMESG("val: %d.\n", cdev->debug_log_enable);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->debug_log_enable);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_debug_log_enable_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	input = input > 0 ? 1 : 0;
	TPD_DMESG("val = %d\n", input);

	 cdev->debug_log_enable = input;
	return len;
}

static ssize_t tp_palm_mode_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0)
		return 0;

	if (cdev->tp_palm_mode_read_2nd)
		cdev->tp_palm_mode_read_2nd(cdev);

	TPD_DMESG("tpd: val: %d.\n", cdev->palm_mode_en);
	len = snprintf(data_buf, sizeof(data_buf), "%u\n", cdev->palm_mode_en);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_palm_mode_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	input = input > 0 ? 1 : 0;
	TPD_DMESG("tpd: val = %d\n", input);

	if (cdev->tp_palm_mode_write_2nd)
		cdev->tp_palm_mode_write_2nd(cdev, input);

	return len;
}

static ssize_t ghost_debug_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	char *data_buf = NULL;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	data_buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (data_buf == NULL) {
		TPD_DMESG("alloc data_buf failed");
		return -ENOMEM;
	}
	TPD_DMESG("ghost_check_single_time_2nd is %d", cdev->ghost_check_single_time_2nd);
	TPD_DMESG("ghost_check_multi_time_2nd is %d", cdev->ghost_check_multi_time_2nd);
	TPD_DMESG("ghost_check_single_count_2nd is %d", cdev->ghost_check_single_count_2nd);
	TPD_DMESG("ghost_check_multi_count_2nd is %d", cdev->ghost_check_multi_count_2nd);
	TPD_DMESG("ghost_check_start_time_2nd is %d", cdev->ghost_check_start_time_2nd);
	TPD_DMESG("ghost_check_ignore_id_2nd is %d", cdev->ghost_check_ignore_id_2nd);
	TPD_DMESG("ghost_check_ignore_edge_area_2nd is %d", cdev->ghost_check_ignore_edge_area_2nd);
	TPD_DMESG("ghost_check_ignore_corner_x_2nd is %d", cdev->ghost_check_ignore_corner_x_2nd);
	TPD_DMESG("ghost_check_ignore_corner_y_2nd is %d", cdev->ghost_check_ignore_corner_y_2nd);

	len += snprintf(data_buf + len, PAGE_SIZE - len, "#######################################\n\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "single_time,multi_time,single_count,multi_count,start_time,ignore_id,ignore_edge_area,ignore_corner_x,ignore_corner_y \n");
	len += snprintf(data_buf + len, PAGE_SIZE - len, "echo 25,20,5,8,35,9,30,40,50 > ghost_debug \n\n");
	len += snprintf(data_buf + len, PAGE_SIZE - len,  "#######################################\n\n");
	len += snprintf(data_buf + len, PAGE_SIZE,
		"ghost_check_single_time_2nd is %d\n", cdev->ghost_check_single_time_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len,
		"ghost_check_multi_time_2nd is %d\n", cdev->ghost_check_multi_time_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len,
		"ghost_check_single_count_2nd is %d\n", cdev->ghost_check_single_count_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len,
		"ghost_check_multi_count_2nd is %d\n", cdev->ghost_check_multi_count_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len,
		"ghost_check_start_time_2nd is %d\n", cdev->ghost_check_start_time_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len,
		"ghost_check_ignore_id_2nd is %d\n", cdev->ghost_check_ignore_id_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len,
		"ghost_check_ignore_edge_area_2nd is %d\n", cdev->ghost_check_ignore_edge_area_2nd);
	len += snprintf(data_buf + len, PAGE_SIZE - len,
		"ghost_check_ignore_corner_x_2nd is %d\n", cdev->ghost_check_ignore_corner_x_2nd);
		len += snprintf(data_buf + len, PAGE_SIZE - len,
		"ghost_check_ignore_corner_y_2nd is %d\n", cdev->ghost_check_ignore_corner_y_2nd);
	simple_read_from_buffer(buffer, count, offset, data_buf, len);
	kfree(data_buf);
	return len;
}

static ssize_t ghost_debug_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
 	int ret = 0 ;
	u8 *token = NULL;
	char *cur = NULL;
	char buff[100] = { 0 };
	unsigned int  s_to_u8 = 0;
	u8 data[20] = { 0 };
	u16 count = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	len = (len < sizeof(buff)) ? len : sizeof(buff);
	if (buffer != NULL) {
		if (copy_from_user(buff, buffer, len)) {
			TPD_DMESG("Failed to copy data from user space\n");
			len = -EINVAL;
			goto out;
		}
	}
	cur = &buff[0];
	while (((token = strsep(&cur, ",")) != NULL) && (count < 10)) {
		ret = kstrtouint(token, 10, &s_to_u8);
		if (ret == 0) {
			data[count] = s_to_u8;
			count++;
		}
	}
	cdev->ghost_check_single_time_2nd = data[0];
	cdev->ghost_check_multi_time_2nd = data[1];
	cdev->ghost_check_single_count_2nd = data[2];
	cdev->ghost_check_multi_count_2nd = data[3];
	cdev->ghost_check_start_time_2nd = data[4];
	cdev->ghost_check_ignore_id_2nd = data[5];
	cdev->ghost_check_ignore_edge_area_2nd =  data[6];
	cdev->ghost_check_ignore_corner_x_2nd =  data[7];
	cdev->ghost_check_ignore_corner_y_2nd =  data[8];
out:
	return len;
}

static ssize_t tp_rst_debug_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}

	if (cdev->tp_rst_level_get_2nd) {
		cdev->tp_rst_level_get_2nd(cdev);
	}

	TPD_DMESG("tp_rst now level:%d\n", cdev->tp_rst_level);
	len = snprintf(data_buf, sizeof(data_buf), "%d\n", cdev->tp_rst_level);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_rst_debug_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret) {
		return -EINVAL;
	}

	input = input > 0 ? 1 : 0;
	TPD_DMESG("tp_rst new level:%d\n", input);

	if (cdev->tp_rst_level_set_2nd) {
		cdev->tp_rst_level_set_2nd(cdev, input);
	}

	return len;
}

#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
void tpd_get_hex_diffdata_2nd(struct ztp_device_2nd *cdev)
{
	int index = 0, i = 0;
	s16 hex_diffdata[49] = {0};
	int x = 0, y = 0;
	s16 max_diffdata = 0;

	for (i = 2, index = 0; index < 49; i += 2, index++) {
		hex_diffdata[index] = (cdev->roi_diffdata[i] << 8) + cdev->roi_diffdata[i + 1];
	}
	max_diffdata = hex_diffdata[24];
	TPD_DBG("max_diffdata:%d,\n", max_diffdata);
	if (tpd_cdev_2nd->debug_log_enable) {
		for (y = 0; y < 7; y++) {
			pr_cont("TPD_DIFFDATA[%2d]", (y + 1));
			for (x = 0; x < 7; x++) {
				pr_cont("%5d,", hex_diffdata[y + x * 7]);
			}
			pr_cont("\n");
		}
	}
}

void tpd_clean_diffdata_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->collect_diffdata_enable) {
		if (cdev->wait_collect_completion) {
			TPD_DMESG("collect diffdata complete\n");
			cdev->wait_collect_completion = false;
			//complete(&cdev->diffdata_collect_completion);
		}
	}
	cdev->get_diffdata_count = 0;
}

static ssize_t tp_roi_diffdata_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	unsigned char *diff_data = NULL;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0)
		return 0;

	if (cdev->collect_diffdata_enable) {
		cdev->wait_collect_completion = true;
		if (!wait_for_completion_timeout(&cdev->diffdata_collect_completion, msecs_to_jiffies(80))) {
			TPD_DMESG("wait diffdata_collect_completion timeout!");
		}
	}
	if (cdev->roi_diffdata_switch && cdev->get_roi_diffdata_2nd)
		diff_data = cdev->get_roi_diffdata_2nd(cdev);

	if (diff_data == NULL) {
		TPD_DMESG("diff_data is NULL\n");
		return -ENOMEM;
	}
if (cdev->collect_diffdata_enable)
	return simple_read_from_buffer(buffer, count, offset, diff_data, ROI_DIFFDATA_LENGTH  * COLLECT_MAX_COUNT);
else
	return simple_read_from_buffer(buffer, count, offset, diff_data, ROI_DIFFDATA_LENGTH);
}

static ssize_t tp_roi_diffdata_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
 	int ret = 0;
	unsigned int input = 0;

	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	input = input > 0 ? 1 : 0;
	TPD_DMESG("tpd: %s val = %d\n", __func__, input);
	cdev->collect_diffdata_enable = input;
	TPD_DMESG("tpd: collect_diffdata_enable = %d\n", cdev->collect_diffdata_enable);
	return len;
}

static ssize_t tp_roi_enable_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	int ret = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (*offset != 0) {
		return 0;
	}
	if (cdev->tp_send_roi_cmd_2nd) {
		ret = cdev->tp_send_roi_cmd_2nd(cdev, TS_CMD_READ);
		if (ret) {
			TPD_DMESG("ts read roi cmd failed\n");
			len = snprintf(data_buf, sizeof(data_buf), "ts read roi cmd failed\n");
			goto out;
		}
	}
	TPD_DMESG("%s roi switch val:%d.\n", __func__, cdev->roi_diffdata_switch);
	TPD_DMESG("tpd: collect_diffdata_enable = %d\n", cdev->collect_diffdata_enable);

	len = snprintf(data_buf, sizeof(data_buf), "%d\n",cdev->roi_diffdata_switch);
out:
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_roi_enable_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
 	int ret = 0;
	unsigned int input = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	ret = kstrtouint_from_user(buffer, len, 10, &input);
	if (ret)
		return -EINVAL;

	input = input > 0 ? 1 : 0;
	TPD_DMESG("tpd: %s val = %d\n", __func__, input);
	if (cdev->roi_diffdata_switch == input) {
		TPD_DMESG("no need to send same cmd \n");
		return len;
	}
	cdev->roi_diffdata_switch = input;
	tpd_clean_diffdata_2nd();
	if (cdev->tp_send_roi_cmd_2nd) {
		ret = cdev->tp_send_roi_cmd_2nd(cdev, TS_CMD_WRITE);
		if (ret) {
			TPD_DMESG("ts send roi cmd failed\n");
			return -EIO;
		}
	}
	return len;
}
#endif

static ssize_t tp_BBAT_test_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int ret = 0;

	if (*offset != 0)
		return 0;

	if (cdev->tp_bbat_test_2nd) {
		ret = cdev->tp_bbat_test_2nd(cdev);
		if (ret) {
			TPD_DMESG("tp bbat test failed\n");
		}
	} else if (tpd_cdev_2nd->TP_have_registered == false) {
		ret = TP_RST_BBAT_TEST_FAIL;
	}
	len = snprintf(data_buf, sizeof(data_buf), "%d\n", ret);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_BBAT_test_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	TPD_DMESG("reserved no use");
	return len;
}

static ssize_t tp_test_read_2nd(struct file *file,
					 char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	uint8_t data_buf[10] = {0};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int ret = 0;

	if (*offset != 0)
		return ret;

	if (cdev->ztp_probe_fail_chip_id_2nd != TS_CHIP_MAX) {
		ret = PROBE_TEST_ERR;
		TPD_DMESG("TP probe failed, chip_id = 0x%02x", cdev->ztp_probe_fail_chip_id_2nd);
		goto exit;
	} else {
		TPD_DMESG("TP probe success, chip_id = 0x%02x", cdev->tp_chip_id_2nd);
	}

	if (!cdev->fw_ready) {
		ret = FW_TEST_ERR;
		TPD_DMESG("TP fw upgrade failed");
		goto exit;
	} else {
		TPD_DMESG("TP fw upgrade success");
	}

exit:
	len = snprintf(data_buf, sizeof(data_buf), "%d\n", ret);
	return simple_read_from_buffer(buffer, count, offset, data_buf, len);
}

static ssize_t tp_test_write_2nd(struct file *file,
				const char __user *buffer, size_t len, loff_t *off)
{
	TPD_DMESG("reserved no use");
	return len;
}

static const struct proc_ops proc_ops_tp_module_Info_2nd = {
	.proc_read = tp_module_info_read_2nd,
};
static const struct proc_ops proc_ops_wake_gesture_2nd = {
	.proc_read = tp_wake_gesture_read_2nd,
	.proc_write = tp_wake_gesture_write_2nd,
};
static const struct proc_ops proc_ops_smart_cover_2nd = {
	.proc_read = tp_smart_cover_read_2nd,
	.proc_write = tp_smart_cover_write_2nd,
};

static const struct proc_ops proc_ops_glove_2nd = {
	.proc_read = tp_glove_read_2nd,
	.proc_write = tp_glove_write_2nd,
};

static const struct proc_ops proc_ops_tpfwupgrade_2nd = {
	.proc_write = tpfwupgrade_store_2nd,
};

static const struct proc_ops proc_ops_suspend_2nd = {
	.proc_read = suspend_show_2nd,
	.proc_write = suspend_store_2nd,
};

static const struct proc_ops proc_ops_headset_state_2nd = {
	.proc_read = headset_state_show_2nd,
	.proc_write = headset_state_store_2nd,
};

static const struct proc_ops proc_ops_mrotation_2nd = {
	.proc_read = display_rotation_show_2nd,
	.proc_write = set_display_rotation_2nd,
};

static const struct proc_ops proc_ops_single_tap_2nd = {
	.proc_read = tp_single_tap_read_2nd,
	.proc_write = tp_single_tap_write_2nd,
};

static const struct proc_ops proc_ops_single_aod_2nd = {
	.proc_read = tp_single_aod_read_2nd,
	.proc_write = tp_single_aod_write_2nd,
};

static const struct proc_ops proc_ops_get_noise_2nd = {
	.proc_read = get_tp_noise_show_2nd,
	.proc_write = get_tp_noise_store_2nd,
};

static const struct proc_ops proc_ops_edge_report_limit_2nd = {
	.proc_read = tp_edge_report_limit_read_2nd,
	.proc_write = tp_edge_report_limit_write_2nd,
};

static const struct proc_ops proc_ops_onekey_2nd = {
	.proc_read = get_one_key_2nd,
	.proc_write = set_one_key_2nd,
};

static const struct proc_ops proc_ops_playgame_2nd = {
	.proc_read = get_play_game_2nd,
	.proc_write = set_play_game_2nd,
};

static const struct proc_ops proc_ops_tp_report_rate_2nd = {
	.proc_read = get_tp_report_rate_2nd,
	.proc_write = set_tp_report_rate_2nd,
};

static const struct proc_ops proc_ops_sensibility_level_2nd = {
	.proc_read = tp_sensibility_level_read_2nd,
	.proc_write = tp_sensibility_level_write_2nd,
};

static const struct proc_ops proc_ops_pen_only_2nd = {
	.proc_read = tp_pen_only_read_2nd,
	.proc_write = tp_pen_only_write_2nd,
};

static const struct proc_ops proc_ops_tp_self_test_2nd = {
	.proc_read = tp_self_test_read_2nd,
	.proc_write = tp_self_test_write_2nd,
};

static const struct proc_ops proc_ops_finger_lock_flag_2nd = {
	.proc_read = get_finger_lock_flag_2nd,
	.proc_write = set_finger_lock_flag_2nd,
};

static const struct proc_ops proc_ops_zlog_debug_2nd = {
	.proc_read = tp_zlog_debug_read_2nd,
	.proc_write = tp_zlog_debug_write_2nd,
};

static const struct proc_ops proc_ops_debug_log_enable_2nd = {
	.proc_read = tp_debug_log_enable_read_2nd,
	.proc_write = tp_debug_log_enable_write_2nd,
};

static const struct proc_ops proc_ops_palm_mode_2nd = {
	.proc_read = tp_palm_mode_read_2nd,
	.proc_write = tp_palm_mode_write_2nd,
};

static const struct proc_ops proc_ops_rotation_limit_level_2nd = {
	.proc_read = get_rotation_limit_level_2nd,
	.proc_write = set_rotation_limit_level_2nd,
};

static const struct proc_ops proc_ops_ghost_debug_2nd = {
	.proc_read = ghost_debug_read_2nd,
	.proc_write = ghost_debug_write_2nd,
};

static const struct proc_ops proc_ops_tp_rst_debug_2nd = {
	.proc_read = tp_rst_debug_read_2nd,
	.proc_write = tp_rst_debug_write_2nd,
};

#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
static const struct proc_ops proc_ops_tp_roi_enable_2nd = {
	.proc_read = tp_roi_enable_read_2nd,
	.proc_write = tp_roi_enable_write_2nd,
};

static const struct proc_ops proc_ops_tp_roi_diffdata_2nd = {
	.proc_read = tp_roi_diffdata_read_2nd,
	.proc_write = tp_roi_diffdata_write_2nd,
};
#endif

static const struct proc_ops proc_ops_BBAT_test_2nd = {
	.proc_read = tp_BBAT_test_read_2nd,
	.proc_write = tp_BBAT_test_write_2nd,
};

static const struct proc_ops proc_ops_tp_test_2nd = {
	.proc_read = tp_test_read_2nd,
	.proc_write = tp_test_write_2nd,
};

static void create_tpd_proc_entry_2nd(void)
{
	struct proc_dir_entry *tpd_proc_entry_2nd = NULL;

	TPD_DMESG("enter\n");
	tpd_proc_dir_2nd = proc_mkdir(PROC_TOUCH_DIR_2nd, NULL);
	if (tpd_proc_dir_2nd == NULL) {
		TPD_DMESG("mkdir touchscreen_2nd failed!\n");
		return;
	}
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_INFO_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_tp_module_Info_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create ts_information_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_WAKE_GESTURE_2nd, 0664,  tpd_proc_dir_2nd, &proc_ops_wake_gesture_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create wake_gesture_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_SMART_COVER_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_smart_cover_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create smart_cover_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_GLOVE_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_glove_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create glove_mode_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_FW_UPGRADE_2nd, 0664,  tpd_proc_dir_2nd, &proc_ops_tpfwupgrade_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create FW_upgrade_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_SUSPEND_2nd, 0664,  tpd_proc_dir_2nd, &proc_ops_suspend_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create suspend_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_HEADSET_STATE_2nd, 0664,  tpd_proc_dir_2nd, &proc_ops_headset_state_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create headset_state_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_ROTATION_LIMIT_LEVEL_2nd, 0664,  tpd_proc_dir_2nd, &proc_ops_rotation_limit_level_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create rotation_limit_level_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_MROTATION_2nd, 0664,  tpd_proc_dir_2nd, &proc_ops_mrotation_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create mRotation_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_TP_SINGLETAP_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_single_tap_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create single_tap_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_TP_SINGLEAOD_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_single_aod_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create single_aod_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_GET_NOISE_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_get_noise_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create get_noise_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_EDGE_REPORT_LIMIT_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_edge_report_limit_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create edge_report_limit_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_ONEKEY_2nd, 0664,  tpd_proc_dir_2nd, &proc_ops_onekey_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create one_key_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_PLAY_GAME_2nd, 0664,  tpd_proc_dir_2nd, &proc_ops_playgame_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create play_game_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_TP_REPORT_RATE_2nd, 0664,  tpd_proc_dir_2nd, &proc_ops_tp_report_rate_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create tp_report_rate_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_SENSIBILITY_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_sensibility_level_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create sensilibity_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_PEN_ONLY_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_pen_only_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create pen_only_2nd failed!\n");
		tpd_proc_entry_2nd = proc_create(PROC_TOUCH_TP_SELF_TEST_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_tp_self_test_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create tp_self_test_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_FINGER_LOCK_FLAG_2nd, 0664,  tpd_proc_dir_2nd, &proc_ops_finger_lock_flag_2nd);
	if (tpd_proc_entry_2nd == NULL)
		TPD_DMESG("proc_create finger_lock_flag_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_ZLOG_DEBUG_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_zlog_debug_2nd);
	if (tpd_proc_entry_2nd == NULL)
		pr_err("proc_create zlog_debug_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_DEBUG_LOG_ENABLE_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_debug_log_enable_2nd);
	if (tpd_proc_entry_2nd == NULL)
		pr_err("proc_create debug_log_enable_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_GHOST_DEBUG_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_ghost_debug_2nd);
	if (tpd_proc_entry_2nd == NULL)
		pr_err("proc_create ghost_debug_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_TP_RST_DEBUG_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_tp_rst_debug_2nd);
	if (tpd_proc_entry_2nd == NULL)
		pr_err("proc_create tp_rst_debug_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TOUCH_TP_PALM_MODE_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_palm_mode_2nd);
	if (tpd_proc_entry_2nd == NULL)
		pr_err("proc_create tp_palm_mode_2nd failed!\n");
#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
	tpd_proc_entry_2nd = proc_create(PROC_ROI_ENABLE_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_tp_roi_enable_2nd);
	if (tpd_proc_entry_2nd == NULL)
		pr_err("proc_create tp_roi_enable_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_ROI_DIFFDATA_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_tp_roi_diffdata_2nd);
	if (tpd_proc_entry_2nd == NULL)
		pr_err("proc_create tp_roi_diffdata_2nd failed!\n");
#endif
	tpd_proc_entry_2nd = proc_create(PROC_BBAT_TEST_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_BBAT_test_2nd);
	if (tpd_proc_entry_2nd == NULL)
		pr_err("proc_create BBAT_test_2nd failed!\n");
	tpd_proc_entry_2nd = proc_create(PROC_TP_TEST_2nd, 0664, tpd_proc_dir_2nd, &proc_ops_tp_test_2nd);
	if (tpd_proc_entry_2nd == NULL)
		pr_err("proc_create tp_test_2nd failed!\n");
}

void tpd_proc_deinit_2nd(void)
{
	if (tpd_proc_dir_2nd == NULL) {
		TPD_DMESG("proc/touchscreen_2nd is NULL!\n");
		return;
	}
	remove_proc_entry(PROC_TOUCH_INFO_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_WAKE_GESTURE_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_SMART_COVER_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_GLOVE_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_FW_UPGRADE_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_SUSPEND_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_HEADSET_STATE_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_ROTATION_LIMIT_LEVEL_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_MROTATION_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_TP_SINGLETAP_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_TP_SINGLEAOD_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_GET_NOISE_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_EDGE_REPORT_LIMIT_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_ONEKEY_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_PLAY_GAME_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_TP_REPORT_RATE_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_SENSIBILITY_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_PEN_ONLY_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_TP_SELF_TEST_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_FINGER_LOCK_FLAG_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_ZLOG_DEBUG_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_DEBUG_LOG_ENABLE_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_TP_PALM_MODE_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_GHOST_DEBUG_2nd, tpd_proc_dir_2nd);
#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
	remove_proc_entry(PROC_ROI_ENABLE_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_ROI_DIFFDATA_2nd, tpd_proc_dir_2nd);
#endif
	remove_proc_entry(PROC_BBAT_TEST_2nd, tpd_proc_dir_2nd);
	remove_proc_entry(PROC_TOUCH_DIR_2nd, NULL);
}

static ssize_t tpd_sysfs_fwimage_store_2nd(struct file *file,
		struct kobject *kobj, struct bin_attribute *attr,
		char *buf, loff_t pos, size_t count)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	mutex_lock(&cdev->cmd_mutex);
	if (!cdev->tp_firmware || !cdev->tp_firmware->data) {
		TPD_DMESG("Need set fw image size first");
		return -ENOMEM;
	}

	if (cdev->tp_firmware->size == 0) {
		TPD_DMESG("Invalid firmware size");
		return -EINVAL;
	}
	if (cdev->fw_data_pos >= cdev->tp_firmware->size) {
		cdev->fw_data_pos = 0;
		return -EINVAL;
	}
	if (cdev->fw_data_pos + count > cdev->tp_firmware->size)
		count = cdev->tp_firmware->size - cdev->fw_data_pos;
	TPD_DMESG("cdev->fw_data_pos: %d, count:%zu\n", cdev->fw_data_pos, count);

	memcpy((u8 *)&cdev->tp_firmware->data[cdev->fw_data_pos], buf, count);
	cdev->fw_data_pos += count;
	mutex_unlock(&cdev->cmd_mutex);
	return count;
}

static ssize_t tpd_sysfs_fwimage_show_2nd(struct file *file,
		struct kobject *kobj, struct bin_attribute *attr,
		char *buf, loff_t pos, size_t count)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	mutex_lock(&cdev->cmd_mutex);
	if (!cdev->tp_firmware || !cdev->tp_firmware->data) {
		TPD_DMESG("Need set fw image size first");
		return -ENOMEM;
	}

	if (cdev->tp_firmware->size == 0) {
		TPD_DMESG("Invalid firmware size");
		return -EINVAL;
	}

	if (cdev->fw_data_pos >= cdev->tp_firmware->size) {
		cdev->fw_data_pos = 0;
		vfree(cdev->tp_firmware->data);
		cdev->tp_firmware->data = NULL;
		kfree(cdev->tp_firmware);
		cdev->tp_firmware = NULL;
		TPD_DMESG("tpd, tp_firmware free.\n");
		mutex_unlock(&cdev->cmd_mutex);
		return 0;
	}
	if (cdev->fw_data_pos + count > cdev->tp_firmware->size)
		count = cdev->tp_firmware->size - cdev->fw_data_pos;
	TPD_DMESG("cdev->fw_data_pos: %d, count:%zu\n", cdev->fw_data_pos, count);
	memcpy(buf, (u8 *)&cdev->tp_firmware->data[cdev->fw_data_pos], count);
	cdev->fw_data_pos += count;
	mutex_unlock(&cdev->cmd_mutex);
	return count;
}

static const struct bin_attribute fwimage_attr_2nd = {
	.attr = {
		.name = "fwimage_2nd",
		.mode = 0666,
	},
	.size = 0,
	.write = tpd_sysfs_fwimage_store_2nd,
	.read = tpd_sysfs_fwimage_show_2nd,
};

static int tpd_fw_sysfs_init_2nd(void)
{
	int ret = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter\n");
	if (!cdev->zte_touch_pdev_2nd){
		TPD_DMESG("zte_touch_pdev_2nd is NULL.");
		return -EINVAL;
	}
	cdev->zte_touch_kobj = kobject_create_and_add("fwupdate_2nd",
					&cdev->zte_touch_pdev_2nd->dev.kobj);
	if (!cdev->zte_touch_kobj) {
		TPD_DMESG("failed create sub dir for fwupdate_2nd");
		return -EINVAL;
	}
	ret = sysfs_create_bin_file(cdev->zte_touch_kobj, &fwimage_attr_2nd);
	if (ret) {
		TPD_DMESG("failed create fwimage_2nd bin node, %d", ret);
		kobject_put(cdev->zte_touch_kobj);
	}

	return ret;
}

static void tpd_fw_sysfs_remove_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (!cdev->zte_touch_kobj)
		return;
	sysfs_remove_bin_file(cdev->zte_touch_kobj, &fwimage_attr_2nd);
	kobject_put(cdev->zte_touch_kobj);
}

int lcd_fps_notify_2nd(u8 lcd_fps)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->lcd_fps_notify_2nd) {
		return cdev->lcd_fps_notify_2nd(cdev,lcd_fps);
	}
	return 0;
}

static void tpd_report_uevent_2nd(u8 gesture_key)
{
	char *envp[2] = {NULL};
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	pm_wakeup_event(&cdev->pdev->dev, 2000);
	TPD_DMESG("pm_wakeup_event 2000 success");

	switch (gesture_key) {
	case single_tap2:
		cdev->ztp_time.tp_single_tap_time = jiffies;
		TPD_DMESG("single tap2 gesture");
		envp[0] = "single_tap2=true";
		break;
	case double_tap2:
		cdev->ztp_time.tp_double_tap_time = jiffies;
		TPD_DMESG("double tap2 gesture");
		envp[0] = "double_tap2=true";
		break;
	case pen_low_batt:
		TPD_DMESG("pen low batt");
		envp[0] = "pen_capacity_low=true";
		break;
	default:
		TPD_DMESG("no such gesture key(%d)", gesture_key);
		return;
	}

	kobject_uevent_env(&(cdev->zte_touch_pdev_2nd->dev.kobj), KOBJ_CHANGE, envp);
}

int zte_touch_pdev_register_2nd(void)
{
	int ret = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	cdev->zte_touch_pdev_2nd = platform_device_alloc("zte_touch_2nd", -1);
	if (!cdev->zte_touch_pdev_2nd) {
		TPD_DMESG("failed to allocate platform device");
		ret = -ENOMEM;
		goto alloc_failed;
	}

	ret = platform_device_add(cdev->zte_touch_pdev_2nd);
	if (ret < 0) {
		TPD_DMESG("failed to add platform device ret=%d", ret);
		goto register_failed;
	}

	cdev->tpd_report_uevent_2nd = tpd_report_uevent_2nd;
	return 0;

register_failed:
	platform_device_put(cdev->zte_touch_pdev_2nd);
alloc_failed:
	cdev->tpd_report_uevent_2nd = NULL;

	return ret;
}

void zte_touch_pdev_unregister_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->zte_touch_pdev_2nd) {
		TPD_DMESG("device put");
		platform_device_unregister(cdev->zte_touch_pdev_2nd);
	}
}

static int ztp_parse_dt_2nd(struct device_node *node, struct ztp_device_2nd *cdev)
{
	int ret = 0;
	int i = 0;
	u32 value = 0;

	if (!cdev) {
		TPD_DMESG("invalid tp_dev");
		return -EINVAL;
	}

	cdev->zte_tp_algo_2nd = of_property_read_bool(node, "zte,tp_algo_2nd");
	if (cdev->zte_tp_algo_2nd)
		TPD_DMESG("zte_tp_algo_2nd enabled");
	cdev->edge_long_press_check_2nd = of_property_read_bool(node, "zte,tp_long_press_2nd");
	if (cdev->edge_long_press_check_2nd) {
		TPD_DMESG("edge_long_press_check_2nd enabled");
		ret = of_property_read_u32(node, "zte,tp_long_press_timer_2nd", &value);
		if (!ret) {
			cdev->edge_long_press_timer_2nd = value;
			TPD_DMESG("tp_long_press_timer_2nd is %d", cdev->edge_long_press_timer_2nd);
		}
		ret = of_property_read_u32(node, "zte,tp_long_press_left_v_2nd", &value);
		if (!ret) {
			cdev->long_pess_suppression_2nd[0] = value;
			TPD_DMESG("tp_long_press_left_v_2nd is %d", cdev->long_pess_suppression_2nd[0]);
		}
		ret = of_property_read_u32(node, "zte,tp_long_press_right_v_2nd", &value);
		if (!ret) {
			cdev->long_pess_suppression_2nd[1] = value;
			TPD_DMESG("tp_long_press_right_v_2nd is %d", cdev->long_pess_suppression_2nd[1]);
		}
		ret = of_property_read_u32(node, "zte,tp_long_press_left_h_2nd", &value);
		if (!ret) {
			cdev->long_pess_suppression_2nd[2] = value;
			TPD_DMESG("tp_long_press_left_h_2nd is %d", cdev->long_pess_suppression_2nd[2]);
		}
		ret = of_property_read_u32(node, "zte,tp_long_press_right_h_2nd", &value);
		if (!ret) {
			cdev->long_pess_suppression_2nd[3] = value;
			TPD_DMESG("tp_long_press_right_h_2nd is %d", cdev->long_pess_suppression_2nd[3]);
		}
	}

	cdev->ghost_check_config_2nd = of_property_read_bool(node, "zte,ghost_check_config_2nd");
	if (cdev->ghost_check_config_2nd) {
		TPD_DMESG("ghost_check_config_2nd enabled");
		ret = of_property_read_u32(node, "zte,ghost_check_single_time_2nd", &value);
		if (!ret) {
			cdev->ghost_check_single_time_2nd = value;
		} else {
			cdev->ghost_check_single_time_2nd = 25;
		}
		ret = of_property_read_u32(node, "zte,ghost_check_multi_time_2nd", &value);
		if (!ret) {
			cdev->ghost_check_multi_time_2nd = value;
		} else {
			cdev->ghost_check_multi_time_2nd = 20;
		}
		ret = of_property_read_u32(node, "zte,ghost_check_single_count_2nd", &value);
		if (!ret) {
			cdev->ghost_check_single_count_2nd = value;
		} else {
			cdev->ghost_check_single_count_2nd = 5;
		}
		ret = of_property_read_u32(node, "zte,ghost_check_multi_count_2nd", &value);
		if (!ret) {
			cdev->ghost_check_multi_count_2nd = value;
		} else {
			cdev->ghost_check_multi_count_2nd = 8;
		}
		ret = of_property_read_u32(node, "zte,ghost_check_start_time_2nd", &value);
		if (!ret) {
			cdev->ghost_check_start_time_2nd = value;
		} else {
			cdev->ghost_check_start_time_2nd = 35;
		}
		ret = of_property_read_u32(node, "zte,ghost_check_ignore_id_2nd", &value);
		if (!ret) {
			cdev->ghost_check_ignore_id_2nd = value;
		} else {
			cdev->ghost_check_ignore_id_2nd = -1;
		}
		ret = of_property_read_u32(node, "zte,ghost_check_ignore_edge_area_2nd", &value);
		if (!ret) {
			cdev->ghost_check_ignore_edge_area_2nd = value;
		} else {
			cdev->ghost_check_ignore_edge_area_2nd = 41;
		}
		ret = of_property_read_u32(node, "zte,ghost_check_ignore_corner_x_2nd", &value);
		if (!ret) {
			cdev->ghost_check_ignore_corner_x_2nd = value;
		} else {
			cdev->ghost_check_ignore_corner_x_2nd = 81;
		}
		ret = of_property_read_u32(node, "zte,ghost_check_ignore_corner_y_2nd", &value);
		if (!ret) {
			cdev->ghost_check_ignore_corner_y_2nd = value;
		} else {
			cdev->ghost_check_ignore_corner_y_2nd = 81;
		}
	} else {
		cdev->ghost_check_single_time_2nd = 25;
		cdev->ghost_check_multi_time_2nd = 20;
		cdev->ghost_check_single_count_2nd = 5;
		cdev->ghost_check_multi_count_2nd = 8;
		cdev->ghost_check_start_time_2nd = 35;
		cdev->ghost_check_ignore_id_2nd = -1;
		cdev->ghost_check_ignore_edge_area_2nd = 41;
		cdev->ghost_check_ignore_corner_x_2nd = 81;
		cdev->ghost_check_ignore_corner_y_2nd = 81;
	}
	TPD_DMESG("ghost_check_single_time_2nd is %d", cdev->ghost_check_single_time_2nd);
	TPD_DMESG("ghost_check_multi_time_2nd is %d", cdev->ghost_check_multi_time_2nd);
	TPD_DMESG("ghost_check_single_count_2nd is %d", cdev->ghost_check_single_count_2nd);
	TPD_DMESG("ghost_check_multi_count_2nd is %d", cdev->ghost_check_multi_count_2nd);
	TPD_DMESG("ghost_check_start_time_2nd is %d", cdev->ghost_check_start_time_2nd);
	TPD_DMESG("ghost_check_ignore_id_2nd is %d", cdev->ghost_check_ignore_id_2nd);
	TPD_DMESG("ghost_check_ignore_edge_area_2nd is %d", cdev->ghost_check_ignore_edge_area_2nd);
	TPD_DMESG("ghost_check_ignore_corner_x_2nd is %d", cdev->ghost_check_ignore_corner_x_2nd);
	TPD_DMESG("ghost_check_ignore_corner_y_2nd is %d", cdev->ghost_check_ignore_corner_y_2nd);

	ret = of_property_read_u32(node, "zte,tp_jitter_check_2nd", &value);
	if (!ret) {
		cdev->tp_jitter_check_2nd = value;
		TPD_DMESG("tp_jitter_check_2nd is %d", cdev->tp_jitter_check_2nd);
		if (cdev->tp_jitter_check_2nd) {
			ret = of_property_read_u32(node, "zte,tp_jitter_timer_2nd_2nd", &value);
			if (!ret) {
				cdev->tp_jitter_timer_2nd_2nd = value;
				TPD_DMESG("tp_jitter_timer_2nd_2nd is %d", cdev->tp_jitter_timer_2nd_2nd);
			}
		}
	}
	ret = of_property_read_u32(node, "zte,tp_edge_click_suppression_pixel_2nd", &value);
	if (!ret) {
		cdev->edge_click_sup_p_2nd = value;
		TPD_DMESG("tp_edge_click_suppression_pixel_2nd is %d", cdev->edge_click_sup_p_2nd);
		for (i = 0; i < 4; i++)
			cdev->edge_report_limit_2nd[i] = cdev->edge_click_sup_p_2nd;
	}
#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
	cdev->ufp_enable_2nd = of_property_read_bool(node, "zte,ufp_enable_2nd");
	if (cdev->ufp_enable_2nd) {
		TPD_DMESG("ufp_enable_2nd enabled");
		ret = of_property_read_u32(node, "zte,ufp_circle_center_x_2nd", &value);
		if (!ret) {
			cdev->ufp_circle_center_x_2nd = value;
			TPD_DMESG("ufp_circle_center_x_2nd is %d", cdev->ufp_circle_center_x_2nd);
		}
		ret = of_property_read_u32(node, "zte,ufp_circle_center_y_2nd", &value);
		if (!ret) {
			cdev->ufp_circle_center_y_2nd = value;
			TPD_DMESG("ufp_circle_center_y_2nd is %d", cdev->ufp_circle_center_y_2nd);
		}
		ret = of_property_read_u32(node, "zte,ufp_circle_radius_2nd", &value);
		if (!ret) {
			cdev->ufp_circle_radius_2nd = value;
			TPD_DMESG("ufp_circle_radius_2nd is %d", cdev->ufp_circle_radius_2nd);
		}
	}
#endif
	return 0;
}

static void ztp_probe_work_2nd(struct work_struct *work)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int tp_time = 0;

	cdev->ztp_time.tp_probe_start_time = jiffies;

#ifdef CONFIG_TOUCHSCREEN_FTS_3383
	fts_ts_init_2nd();
#endif

	tp_time = get_tp_consum_time_2nd(cdev->ztp_time.tp_probe_start_time);
	TPD_DMESG("tp_time tp probe start -> tp probe end:%d.", tp_time);
}

void tpd_probe_work_init_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	INIT_DELAYED_WORK(&cdev->tpd_probe_work, ztp_probe_work_2nd);

}

void tpd_probe_work_deinit_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	cancel_delayed_work_sync(&cdev->tpd_probe_work);

}

void tpd_get_last_log_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int len = 0;
	u8 count = 0;

	if (!cdev->tp_error_last_log_buffer) {
		mutex_unlock(&cdev->zlog_mutex);
		TPD_ZLOG("tp_error_last_log_buffer is NULL");
		return;
	}
	memset(cdev->tp_error_last_log_buffer, 0, ZLOG_INFO_LEN);
	time64_to_tm(ktime_get_real_seconds(), 0, &cdev->now_time);
	len += snprintf(cdev->tp_error_last_log_buffer + len, ZLOG_INFO_LEN - len,
		"now time:%04d%02d%02d-%02d:%02d:%02d, TP probe time:%04d%02d%02d-%02d:%02d:%02d.\n",
		(int)(cdev->now_time.tm_year + 1900), cdev->now_time.tm_mon + 1,
		cdev->now_time.tm_mday, cdev->now_time.tm_hour, cdev->now_time.tm_min,
		cdev->now_time.tm_sec, (int)(cdev->probe_time.tm_year + 1900), cdev->probe_time.tm_mon + 1,
		cdev->probe_time.tm_mday, cdev->probe_time.tm_hour, cdev->probe_time.tm_min, cdev->probe_time.tm_sec);

#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
		zlog_client_record(cdev->zlog_client,
			 "now time:%04d%02d%02d-%02d:%02d:%02d, TP probe time:%04d%02d%02d-%02d:%02d:%02d",
			(int)(cdev->now_time.tm_year + 1900), cdev->now_time.tm_mon + 1,
			cdev->now_time.tm_mday, cdev->now_time.tm_hour, cdev->now_time.tm_min,
			cdev->now_time.tm_sec, (int)(cdev->probe_time.tm_year + 1900), cdev->probe_time.tm_mon + 1,
			cdev->probe_time.tm_mday, cdev->probe_time.tm_hour, cdev->probe_time.tm_min, cdev->probe_time.tm_sec);
#endif
	mutex_lock(&cdev->zlog_mutex);
	tpd_cdev_2nd->tail = tpd_cdev_2nd->pos - 1;
	tpd_cdev_2nd->head = tpd_cdev_2nd->pos - 1;
	tpd_cdev_2nd->tail &= LAST_LOG_BUFF_SIZE - 1;
	tpd_cdev_2nd->head &= LAST_LOG_BUFF_SIZE - 1;
	do {
#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
		zlog_client_record(cdev->zlog_client, "%s.", tpd_cdev_2nd->last_log_buffer[tpd_cdev_2nd->head]);
#endif
		len += snprintf(cdev->tp_error_last_log_buffer + len, ZLOG_INFO_LEN - len, "%s",
			tpd_cdev_2nd->last_log_buffer[tpd_cdev_2nd->head]);
		TPD_ZLOG("%s.", tpd_cdev_2nd->last_log_buffer[tpd_cdev_2nd->head]);
		tpd_cdev_2nd->head--;
		tpd_cdev_2nd->head &= LAST_LOG_BUFF_SIZE - 1;
		count++;
	} while ((tpd_cdev_2nd->tail != tpd_cdev_2nd->head) && (count <= LAST_LOG_BUFF_SIZE));
	mutex_unlock(&cdev->zlog_mutex);
}

void tpd_last_log_init_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	mutex_init(&cdev->zlog_mutex);
	cdev->tp_error_last_log_buffer = vmalloc(ZLOG_INFO_LEN);
	if (!cdev->tp_error_last_log_buffer) {
		TPD_ZLOG("tp_error_last_log_buffer malloc fail");
		return;
	}
	memset(cdev->tp_error_last_log_buffer, 0, ZLOG_INFO_LEN);
}

#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
void tpd_zlog_register_2nd(struct ztp_device_2nd *cdev)
{
	if (cdev->zlog_client) {
		TPD_ZLOG("ztp zlog already registered, no need register again!");
		return;
	}

	cdev->zlog_client = zlog_register_client(&zlog_tp_dev_2nd);
	if (!cdev->zlog_client) {
		TPD_ZLOG("zlog register client zlog_tp_dev_2nd fail\n");
	} else {
		cdev->ztp_zlog_buffer = vmalloc(ZLOG_INFO_LEN);
		if (!cdev->ztp_zlog_buffer) {
			TPD_ZLOG("ztp_zlog_buffer malloc fail");
			return;
		}
		memset(cdev->ztp_zlog_buffer, 0, ZLOG_INFO_LEN);
		if (cdev->ztp_probe_fail_chip_id_2nd != TS_CHIP_MAX) {
			tpd_print_zlog_2nd("tp probe fail, chip id:%d",cdev->ztp_probe_fail_chip_id_2nd);
			tpd_zlog_record_notify_2nd(TP_PROBE_ERROR_NO);
		}
	}
	cdev->zlog_regisered = true;
}

int tpd_zlog_check_2nd(tp_error_no_2nd error_no_2nd)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int ret = 0;

	if (error_no_2nd > TP_ERROR_NO_MAX) {
		TPD_ZLOG("error_no_2nd is to large.\n");
		return  -EIO;
	}

	if ((cdev->zlog_item.count[error_no_2nd] > 0)
		&& (jiffies_to_msecs(jiffies - cdev->zlog_item.timer[error_no_2nd])) < 60000) {
		TPD_ZLOG("zlog error repeated notify, timer:%d, no:%d",
			jiffies_to_msecs(jiffies - cdev->zlog_item.timer[error_no_2nd]) ,error_no_2nd);
		ret = -EIO;
	}
	cdev->zlog_item.count[error_no_2nd]++;
	return ret;
}

void tpd_zlog_record_notify_2nd(tp_error_no_2nd error_no_2nd)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int len = 0;
	unsigned long after_reset_time = 0;

	if (error_no_2nd >= TP_ERROR_NO_MAX) {
		TPD_ZLOG("error_no_2nd is to large.\n");
		return;
	}
	if(!cdev->zlog_regisered)
		tpd_zlog_register_2nd(cdev);

	if ((cdev->zlog_client == NULL) || (cdev->ztp_zlog_buffer == NULL)) {
		TPD_ZLOG("zlog unregistered.\n");
		return;
	}
	after_reset_time = jiffies_to_msecs(jiffies - cdev->tp_reset_timer);
	len = strlen(cdev->ztp_zlog_buffer);
	if (cdev->tp_chip_id_2nd == TS_CHIP_OMNIVISION) {
		snprintf(cdev->ztp_zlog_buffer + len, ZLOG_INFO_LEN - len, " IC name: %s, module name:%s, Firmware version: %d",
			zlog_tp_dev_2nd.ic_name, zlog_tp_dev_2nd.device_name, cdev->ic_tpinfo.firmware_ver);
	} else {
		snprintf(cdev->ztp_zlog_buffer + len, ZLOG_INFO_LEN - len, " IC name: %s, module name:%s, Firmware version: 0x%x",
			zlog_tp_dev_2nd.ic_name, zlog_tp_dev_2nd.device_name, cdev->ic_tpinfo.firmware_ver);
	}
	switch (error_no_2nd) {
	case TP_I2C_R_ERROR_NO:
		if ((tpd_zlog_check_2nd(error_no_2nd) < 0) || (after_reset_time < 200))
			break;

		cdev->zlog_item.timer[error_no_2nd] = jiffies;
		TPD_ZLOG("tpd i2c read err,count:%d. %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd i2c read err,count:%d\n %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		if (cdev->zlog_item.count[error_no_2nd] % 10)
			zlog_client_notify(cdev->zlog_client,  ZLOG_TP_I2C_R_WARN_NO);
		else
			zlog_client_notify(cdev->zlog_client,  ZLOG_TP_I2C_R_ERROR_NO);
		break;
	case TP_I2C_W_ERROR_NO:
		if ((tpd_zlog_check_2nd(error_no_2nd) < 0) || (after_reset_time < 200))
			break;
		cdev->zlog_item.timer[error_no_2nd] = jiffies;
		TPD_ZLOG("tpd i2c write err,count:%d. %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd i2c write err,count:%d.\n %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		if (cdev->zlog_item.count[error_no_2nd] % 10)
			zlog_client_notify(cdev->zlog_client,  ZLOG_TP_I2C_W_WARN_NO);
		else
			zlog_client_notify(cdev->zlog_client,  ZLOG_TP_I2C_W_ERROR_NO);
		break;
	case TP_SPI_R_ERROR_NO:
		if ((tpd_zlog_check_2nd(error_no_2nd) < 0) || (after_reset_time < 200))
			break;
		cdev->zlog_item.timer[error_no_2nd] = jiffies;
		TPD_ZLOG("tpd SPI read err,count:%d.%s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd SPI read err,count:%d\n %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		if (cdev->zlog_item.count[error_no_2nd] % 10)
			zlog_client_notify(cdev->zlog_client,  ZLOG_TP_SPI_R_WARN_NO);
		else
			zlog_client_notify(cdev->zlog_client,  ZLOG_TP_SPI_R_ERROR_NO);
		break;
	case TP_SPI_W_ERROR_NO:
		if ((tpd_zlog_check_2nd(error_no_2nd) < 0) || (after_reset_time < 200))
			break;
		cdev->zlog_item.timer[error_no_2nd] = jiffies;
		TPD_ZLOG("tpd SPI write err,count:%d.%s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd SPI write err,count:%d\n %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		if (cdev->zlog_item.count[error_no_2nd] % 10)
			zlog_client_notify(cdev->zlog_client,  ZLOG_TP_SPI_W_WARN_NO);
		else
			zlog_client_notify(cdev->zlog_client,  ZLOG_TP_SPI_W_ERROR_NO);
		break;
	case TP_CRC_ERROR_NO:
		if ((tpd_zlog_check_2nd(error_no_2nd) < 0) || (after_reset_time < 200))
			break;
		cdev->zlog_item.timer[error_no_2nd] = jiffies;
		TPD_ZLOG("tpd crc check err,count:%d. %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd crc check err,count:%d.\n %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_notify(cdev->zlog_client,  ZLOG_TP_CRC_ERROR_NO);
		break;
	case TP_FW_UPGRADE_ERROR_NO:
		if (tpd_zlog_check_2nd(error_no_2nd) < 0)
			break;
		cdev->zlog_item.timer[error_no_2nd] = jiffies;
		TPD_ZLOG("tpd firmware upgrade err,count:%d. %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd firmware upgrade err,count:%d. \n %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_notify(cdev->zlog_client,  ZLOG_TP_FW_UPGRADE_ERROR_NO);
		break;
	case TP_REQUEST_FIRMWARE_ERROR_NO:
		if (tpd_zlog_check_2nd(error_no_2nd) < 0)
			break;
		cdev->zlog_item.timer[error_no_2nd] = jiffies;
		TPD_ZLOG("tpd request firmware upgrade err,count:%d. %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd request firmware upgrade err,count:%d.\n %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_notify(cdev->zlog_client,  ZLOG_TP_REQUEST_FIRMWARE_ERROR_NO);
		break;
	case TP_ESD_CHECK_ERROR_NO:
		if (tpd_zlog_check_2nd(error_no_2nd) < 0)
			break;
		cdev->zlog_item.timer[error_no_2nd] = jiffies;
		TPD_ZLOG("tpd esd check err,count:%d. %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd esd check err,count:%d.\n %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		if (cdev->zlog_item.count[error_no_2nd] % 10)
			zlog_client_notify(cdev->zlog_client,  ZLOG_TP_ESD_CHECK_WARN_NO);
		else
			zlog_client_notify(cdev->zlog_client,  ZLOG_TP_ESD_CHECK_ERROR_NO);
		break;
	case TP_PROBE_ERROR_NO:
		TPD_ZLOG("tpd probe err. %s\n",cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd probe err.\n %s\n",cdev->ztp_zlog_buffer);
		zlog_client_notify(cdev->zlog_client,  ZLOG_TP_ESD_CHECK_ERROR_NO);
		break;
	case TP_SUSPEND_GESTURE_OPEN_NO:
		TPD_ZLOG("tpd gesture open when suspend. %s\n",cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd tp gesture open when suspend.\n %s\n",cdev->ztp_zlog_buffer);
		zlog_client_notify(cdev->zlog_client,  ZLOG_TP_SUSPEND_GESTURE_OPEN_NO);
		break;
	case TP_GHOST_ERROR_NO:
		if (tpd_zlog_check_2nd(error_no_2nd) < 0)
			break;
		cdev->zlog_item.timer[error_no_2nd] = jiffies;
		TPD_ZLOG("tpd ghost err,count:%d. %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_record(cdev->zlog_client, "tpd ghost err,count:%d.\n %s\n",
			cdev->zlog_item.count[error_no_2nd], cdev->ztp_zlog_buffer);
		zlog_client_notify(cdev->zlog_client,  ZLOG_TP_GHOST_ERROR_NO);
		break;
	case TP_SELF_TEST_ERROR_NO:
		cdev->zlog_item.count[error_no_2nd]++;
		TPD_DMESG("tpd self test err,count:%d\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_GET_NOISE_ERROR_NO:
		cdev->zlog_item.count[error_no_2nd]++;
		TPD_DMESG("tpd get noise data err,count:%d\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	default:
		break;
	}
	tpd_get_last_log_2nd();
	memset(cdev->ztp_zlog_buffer, 0, ZLOG_INFO_LEN);
}

static void zlog_register_work_2nd(struct work_struct *work)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if(!cdev->zlog_regisered)
		tpd_zlog_register_2nd(cdev);
}

void zlog_register_work_init_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	INIT_DELAYED_WORK(&cdev->zlog_register_work_2nd, zlog_register_work_2nd);

}

void zlog_register_work_deinit_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	cancel_delayed_work_sync(&cdev->zlog_register_work_2nd);
	vfree(cdev->ztp_zlog_buffer);
	cdev->ztp_zlog_buffer = NULL;
}

void tpd_zlog_init_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int i = 0;

	cdev->ztp_zlog_buffer = NULL;
	cdev->zlog_regisered = false;
	cdev->tp_reset_timer = jiffies;
	for (i = 0; i < TP_ERROR_NO_MAX; i++) {
		cdev->zlog_item.timer[i] = jiffies;
	}
}
#else
void tpd_zlog_record_notify_2nd(tp_error_no_2nd error_no_2nd)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (error_no_2nd >= TP_ERROR_NO_MAX) {
		TPD_DMESG("error_no_2nd is to large.\n");
		return;
	}
	tpd_get_last_log_2nd();
	cdev->zlog_item.count[error_no_2nd]++;
	switch (error_no_2nd) {
	case TP_I2C_R_ERROR_NO:
		TPD_DMESG("tpd i2c read err,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_I2C_W_ERROR_NO:
		TPD_DMESG("tpd i2c write err,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_SPI_R_ERROR_NO:
		TPD_DMESG("spi read err,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_SPI_W_ERROR_NO:
		TPD_DMESG("spi write err,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_CRC_ERROR_NO:
		TPD_DMESG("tpd crc check error,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_FW_UPGRADE_ERROR_NO:
		TPD_DMESG("tpd firmware upgrade err,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_REQUEST_FIRMWARE_ERROR_NO:
		TPD_DMESG("tpd request firmware upgrade err,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_ESD_CHECK_ERROR_NO:
		TPD_DMESG("tpd esd check err err,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_PROBE_ERROR_NO:
		TPD_DMESG("tpd probe err.\n");
		break;
	case TP_SUSPEND_GESTURE_OPEN_NO:
		TPD_DMESG("tpd gesture open when suspend.\n");
		break;
	case TP_GHOST_ERROR_NO:
		TPD_DMESG("tpd ghost err,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_SELF_TEST_ERROR_NO:
		TPD_DMESG("tpd self test err,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	case TP_GET_NOISE_ERROR_NO:
		TPD_DMESG("tpd get noise data err,count:%lu\n", cdev->zlog_item.count[error_no_2nd]);
		break;
	default:
		break;
	}
}
#endif

static bool tpd_get_charger_ststus_2nd(void)
{
	static struct power_supply *batt_psy;
	union power_supply_propval val = { 0, };
	bool status = false;

	if (batt_psy == NULL)
		batt_psy = power_supply_get_by_name("battery");
	if (batt_psy) {
		batt_psy->desc->get_property(batt_psy, POWER_SUPPLY_PROP_STATUS, &val);
	}
	if ((val.intval == POWER_SUPPLY_STATUS_CHARGING) ||
		(val.intval == POWER_SUPPLY_STATUS_FULL)) {
		status = true;
	} else {
		status = false;
	}
	TPD_DMESG("charger status:%d", status);
	return status;
}

static void tpd_charger_detect_work_2nd(struct work_struct *work)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (!cdev->TP_have_registered) {
		TPD_DMESG("cdev->TP_have_registered is null, return");
		return;
	}

	cdev->charger_mode = tpd_get_charger_ststus_2nd();

	if(cdev->charger_state_notify_2nd)
		cdev->charger_state_notify_2nd(cdev);
}

static int tpd_charger_notify_call_2nd(struct notifier_block *nb, unsigned long event, void *data)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	struct power_supply *psy = data;

	if ((cdev== NULL) || (cdev->tpd_wq == NULL))
		return NOTIFY_DONE;
	if (event != PSY_EVENT_PROP_CHANGED) {
		return NOTIFY_DONE;
	}

	if ((strcmp(psy->desc->name, "usb") == 0)
	    || (strcmp(psy->desc->name, "ac") == 0)) {
		queue_delayed_work(cdev->tpd_wq, &cdev->charger_work, msecs_to_jiffies(500));
	}

	return NOTIFY_DONE;
}

static int tpd_init_charger_notifier_2nd(void)
{
	int ret = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("Init Charger notifier");

	cdev->charger_notifier.notifier_call = tpd_charger_notify_call_2nd;
	ret = power_supply_reg_notifier(&cdev->charger_notifier);
	return ret;
}

void tpd_charger_work_init_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	INIT_DELAYED_WORK(&cdev->charger_work, tpd_charger_detect_work_2nd);
	tpd_init_charger_notifier_2nd();
}

void tpd_charger_work_deinit_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	cancel_delayed_work_sync(&cdev->charger_work);
	power_supply_unreg_notifier(&cdev->charger_notifier);
}

static void send_cmd_work(struct work_struct *work)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->tpd_send_cmd_2nd)
		cdev->tpd_send_cmd_2nd(cdev);
}

static void tp_ghost_check_2nd_work(struct work_struct *work)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (tp_ghost_check_2nd()) {
		TPD_DMESG("may be ghost point");
		if (cdev->ghost_rst_num < 3) {
			if (cdev->ghost_check_reset_2nd) {
				cdev->ghost_check_reset_2nd(cdev);
				TPD_DMESG("ghost check reset, ghost_rst_num = %d", (cdev->ghost_rst_num + 1));
			}
			cdev->ghost_rst_num++ ;
		} else {
			TPD_DMESG("ghost_rst_num has already exceeded 3 times, skip");
		}
	}
	ghost_check_reset_2nd();
	cdev->start_ghost_check_timer = false;
}

int tpd_workqueue_init_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	cdev->tpd_wq = create_singlethread_workqueue("tpd_wq_2nd");

	if (!cdev->tpd_wq) {
		goto err_create_tpd_report_wq_failed;
	}
	if (tpd_report_work_init_2nd())
		goto err_tpd_report_work_init_2nd_failed;
	tpd_probe_work_init_2nd();
	tpd_resume_work_init_2nd();
	tpd_charger_work_init_2nd();
#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
	zlog_register_work_init_2nd();
#endif
	INIT_DELAYED_WORK(&cdev->send_cmd_work, send_cmd_work);
	INIT_DELAYED_WORK(&cdev->ghost_check_work, tp_ghost_check_2nd_work);

	return 0;
err_tpd_report_work_init_2nd_failed:
	if (!cdev->tpd_wq) {
		destroy_workqueue(cdev->tpd_wq);
	}
err_create_tpd_report_wq_failed:
	TPD_DMESG("create tpd workqueue failed\n");
	return -ENOMEM;
}

void tpd_workqueue_deinit_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	tpd_report_work_deinit_2nd();
	tpd_resume_work_deinit_2nd();
	tpd_probe_work_deinit_2nd();
	tpd_charger_work_deinit_2nd();
	cancel_delayed_work_sync(&cdev->send_cmd_work);
	cancel_delayed_work_sync(&cdev->ghost_check_work);
}

static void  zte_touch_deinit_2nd(void)
{
	static bool ztp_release = false;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev == NULL || ztp_release) {
		TPD_DMESG("zte touch deinit, return\n");
		return;
	}
#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
	ufp_mac_exit_2nd();
#endif
	tpd_proc_deinit_2nd();
	tpd_workqueue_deinit_2nd();
	if (!cdev->tpd_wq) {
		destroy_workqueue(cdev->tpd_wq);
	}
	tpd_fw_sysfs_remove_2nd();
	zte_touch_pdev_unregister_2nd();
#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
	zlog_register_work_deinit_2nd();
#endif

	device_init_wakeup(&cdev->pdev->dev, false); //remove wakeup node
	TPD_DMESG("device_init_wakeup false");

	ztp_release = true;
}

static int zte_touch_probe_2nd(struct platform_device *pdev)
{
	struct ztp_device_2nd *ztp_dev = NULL;

	TPD_DMESG("enter");

	ztp_dev = devm_kzalloc(&pdev->dev, sizeof(struct ztp_device_2nd), GFP_KERNEL);
	if (!ztp_dev) {
		TPD_DMESG("Failed to allocate memory for ztp dev");
		return -ENOMEM;
	}
	tpd_cdev_2nd = ztp_dev;
	ztp_dev->pdev =  pdev;
	platform_set_drvdata(pdev, ztp_dev);
	zte_touch_pdev_register_2nd();
	ztp_parse_dt_2nd(pdev->dev.of_node, ztp_dev);
	get_lcd_panel_name_2nd();
	mutex_init(&ztp_dev->cmd_mutex);
	mutex_init(&ztp_dev->report_mutex);
	mutex_init(&ztp_dev->report_down_mutex);
	mutex_init(&ztp_dev->tp_resume_mutex);
#ifdef CONFIG_TOUCHSCREEN_LCD_NOTIFY_2nd
	lcd_notify_register_2nd();
#endif
	create_tpd_proc_entry_2nd();
	tpd_fw_sysfs_init_2nd();
	tpd_clean_all_event_2nd();
	ghost_check_reset_2nd();
#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
	ufp_mac_init_2nd();
#endif
	if(tpd_workqueue_init_2nd())
		return -ENOMEM;
	queue_delayed_work(ztp_dev->tpd_wq, &ztp_dev->tpd_probe_work, msecs_to_jiffies(1000));
#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
	tpd_zlog_init_2nd();
	queue_delayed_work(ztp_dev->tpd_wq, &ztp_dev->zlog_register_work_2nd, msecs_to_jiffies(3000));
#endif
	tpd_last_log_init_2nd();

	device_init_wakeup(&ztp_dev->pdev->dev, true); //add wakeup node
	TPD_DMESG("device_init_wakeup true");

#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
	init_completion(&ztp_dev->diffdata_collect_completion);
#endif

	ztp_dev->ztp_probe_fail_chip_id_2nd = TS_CHIP_MAX;
	ztp_dev->fw_ready = false;
	ztp_dev->ghost_rst_num = 0;

	ztp_dev->is_tp_in_aod_mode = false;

	init_completion(&ztp_dev->ztp_pm_completion);
	ztp_dev->ztp_pm_suspend = false;

	init_completion(&ztp_dev->bbat_test_completion);
	ztp_dev->bbat_test_enter = false;
	init_completion(&ztp_dev->tp_event_completion);
	time64_to_tm(ktime_get_real_seconds(), 0, &ztp_dev->probe_time);
	TPD_DMESG("end\n");
	return 0;
}

static int  zte_touch_remove_2nd(struct platform_device *pdev)
{
	TPD_DMESG("enter\n");
	zte_touch_deinit_2nd();
	return 0;
}

static void zte_touch_shutdown_2nd(struct platform_device *pdev)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter\n");
	if (cdev->tpd_shutdown_2nd)
		cdev->tpd_shutdown_2nd(cdev);
	tpd_workqueue_deinit_2nd();
}

static const struct of_device_id zte_touch_of_match_2nd[] = {
	{ .compatible = "zte_tp_2nd", },
	{ },
};

static int zte_touch_pm_suspend_2nd(struct device *dev)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("system enters into pm_suspend");
	cdev->ztp_pm_suspend = true;
	reinit_completion(&cdev->ztp_pm_completion);
	return 0;
}

static int zte_touch_pm_resume_2nd(struct device *dev)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("system resumes from pm_suspend");
	cdev->ztp_pm_suspend = false;
	complete(&cdev->ztp_pm_completion);
	return 0;
}

static const struct dev_pm_ops zte_touch_pm_ops_2nd = {
	.suspend = zte_touch_pm_suspend_2nd,
	.resume = zte_touch_pm_resume_2nd,
};

static struct platform_driver zte_touch_device_driver_2nd = {
	.probe		= zte_touch_probe_2nd,
	.remove		= zte_touch_remove_2nd,
	.shutdown	= zte_touch_shutdown_2nd,
	.driver		= {
		.name	= "zte_tp_2nd",
		.owner	= THIS_MODULE,
		.of_match_table = zte_touch_of_match_2nd,
		.pm = &zte_touch_pm_ops_2nd,
	}
};

int __init zte_touch_init_2nd(void)
{
	TPD_DMESG("enter 2024-9-24");

	/*if (!strncmp(zte_get_bootmode(), "meta", 4)) {
		TPD_DMESG("in meta mode, return");
		return -EPERM;
	}*/

	return platform_driver_register(&zte_touch_device_driver_2nd);
}

static void __exit zte_touch_exit_2nd(void)
{
#ifdef CONFIG_TOUCHSCREEN_FTS_3383
	if (tpd_cdev_2nd->tp_chip_id_2nd == TS_CHIP_FOCAL) {
		fts_ts_exit_2nd();
	}
#endif

#ifdef CONFIG_TOUCHSCREEN_LCD_NOTIFY_2nd
	lcd_notify_unregister_2nd();
#endif

	zte_touch_deinit_2nd();
	platform_driver_unregister(&zte_touch_device_driver_2nd);
}

late_initcall(zte_touch_init_2nd);
module_exit(zte_touch_exit_2nd);

MODULE_AUTHOR("zte");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("zte tp");

