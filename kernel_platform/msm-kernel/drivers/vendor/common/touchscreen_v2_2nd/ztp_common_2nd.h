/*
 * FILE: ztp_common_2nd.h
 *
 */
#ifndef __ZTP_COMMON_2ND_H_
#define __ZTP_COMMON_2ND_H_

#include <linux/device.h>
#include <linux/rwsem.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/input.h>
#include <linux/proc_fs.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
//#include <vendor/common/zte_tpd.h>
#include <linux/firmware.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/version.h>
#include "ztp_ufp_2nd.h"
#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
#include <vendor/comdef/zlog_common_base.h>
#endif

#ifdef TPD_DMESG
#undef TPD_DMESG
#endif
#define TPD_DMESG(fmt, arg...) pr_info("[ZTE_LDD_TP_2nd][TPD_COMMON]:(%s, %d):" fmt, __func__, __LINE__, ##arg)
#define TPD_ZLOG(fmt, arg...) pr_info("[ZTE_LDD_TP_2nd][TPD_ZLOG]: (%s, %d):" fmt, __func__, __LINE__, ##arg)
#define TPD_DBG(fmt, arg...)						\
do {									\
	if (tpd_cdev_2nd->debug_log_enable)						\
		pr_info("[ZTE_LDD_TP_2nd][TPD_COMMON][DBG]:(%s, %d): " fmt, __func__, __LINE__, ##arg);	\
} while (0)

#define CONFIG_CREATE_TPD_SYS_INTERFACE

#define PROC_TOUCH_DIR_2nd					"touchscreen_2nd"
#define PROC_TOUCH_INFO_2nd					"ts_information_2nd"
#define PROC_TOUCH_FW_UPGRADE_2nd			"FW_upgrade_2nd"
#define PROC_TOUCH_SMART_COVER_2nd		"smart_cover_2nd"
#define PROC_TOUCH_GLOVE_2nd				"glove_mode_2nd"
#define PROC_TOUCH_WAKE_GESTURE_2nd		"wake_gesture_2nd"
#define PROC_TOUCH_SUSPEND_2nd		"suspend_2nd"
#define PROC_TOUCH_HEADSET_STATE_2nd		"headset_state_2nd"
#define PROC_TOUCH_ROTATION_LIMIT_LEVEL_2nd   "rotation_limit_level_2nd"
#define PROC_TOUCH_MROTATION_2nd		"mRotation_2nd"
#define PROC_TOUCH_TP_SINGLETAP_2nd		"single_tap_2nd"
#define PROC_TOUCH_TP_SINGLEAOD_2nd		"single_aod_2nd"
#define PROC_TOUCH_GET_NOISE_2nd		"get_noise_2nd"
#define PROC_TOUCH_EDGE_REPORT_LIMIT_2nd		"edge_report_limit_2nd"
#define PROC_TOUCH_ONEKEY_2nd			"one_key_2nd"
#define PROC_TOUCH_PLAY_GAME_2nd		"play_game_2nd"
#define PROC_TOUCH_SENSIBILITY_2nd		"sensibility_2nd"
#define PROC_TOUCH_TP_REPORT_RATE_2nd		"tp_report_rate_2nd"
#define PROC_TOUCH_PEN_ONLY_2nd		"pen_only_2nd"
#define PROC_TOUCH_TP_SELF_TEST_2nd		"tp_self_test_2nd"
#define PROC_TOUCH_FINGER_LOCK_FLAG_2nd     "finger_lock_flag_2nd"
#define PROC_TOUCH_TP_PALM_MODE_2nd		"tp_palm_mode_2nd"
#define PROC_TOUCH_GHOST_DEBUG_2nd		"ghost_debug_2nd"
#define PROC_TOUCH_TP_RST_DEBUG_2nd		"tp_rst_debug_2nd"
#define PROC_ZLOG_DEBUG_2nd	"zlog_debug_2nd"
#define PROC_DEBUG_LOG_ENABLE_2nd	"debug_log_enable_2nd"
#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
#define PROC_ROI_ENABLE_2nd	"tp_roi_enable_2nd"
#define PROC_ROI_DIFFDATA_2nd	"tp_roi_diffdata_2nd"
#endif
#define PROC_BBAT_TEST_2nd	"BBAT_test_2nd"
#define PROC_TP_TEST_2nd	"tp_test_2nd"
#define  MAX_VENDOR_NAME_LEN 40
#define  MAX_LCD_NAME_LEN 128
#define  MAX_LIMIT_NUM 4
#define VENDOR_END 0xff

#define TP_RAWDATA_TEST_FAIL      0x0001
#define TP_MODULE_TYPE_ERR        0x0080
#define TP_OPEN_TEST_FAIL         0x0200
#define TP_SHORT_TEST_FAIL        0x0400
#define TP_UNIFORMITY_TEST_FAIL   0x0800
#define TP_NOISE_TEST_FAIL        0x4000
#define TP_CB_TEST_FAIL           0x8000
#define TP_PANEL_DIFF_TEST_FAIL	  0x10000
#define TP_COMP_CAP_TEST_FAIL     0x20000
#define TP_OTHER_TEST_FAIL        0x40000

#define TP_INT_BBAT_TEST_FAIL     0x01
#define TP_RST_BBAT_TEST_FAIL     0x02

#define BLANK		1
#define UNBLANK		0
#define KB			(1024)
#define MB			(KB * KB)
#define RT_DATA_LEN	(10 * KB)
#define TP_TEST_FILE_SIZE		(1 * MB)
#define ZLOG_INFO_LEN (2 * KB)
#define ZTP_TIMEOUT_COMERR_PM               700
#define LAST_LOG_BUFF_SIZE	16
#define LAST_LOG_BUFF_LEN	200
#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
#define ROI_DIFFDATA_LENGTH 100
#define COLLECT_MAX_COUNT	5
#endif
typedef enum tp_error_no_2nd {
	TP_I2C_R_ERROR_NO = 1,
	TP_I2C_W_ERROR_NO,
	TP_SPI_R_ERROR_NO,
	TP_SPI_W_ERROR_NO,
	TP_CRC_ERROR_NO,
	TP_FW_UPGRADE_ERROR_NO,
	TP_ESD_CHECK_ERROR_NO,
	TP_PROBE_ERROR_NO,
	TP_SUSPEND_GESTURE_OPEN_NO,
	TP_REQUEST_FIRMWARE_ERROR_NO,
	TP_GHOST_ERROR_NO,
	TP_SELF_TEST_ERROR_NO,
	TP_GET_NOISE_ERROR_NO,
	TP_ERROR_NO_MAX,
} tp_error_no_2nd;

struct tp_error_item_2nd {
	unsigned long  count[TP_ERROR_NO_MAX];
	unsigned long  timer[TP_ERROR_NO_MAX];
};

#define tpd_save_last_log(fmt, args...) do {\
	mutex_lock(&tpd_cdev_2nd->zlog_mutex); \
	memset(tpd_cdev_2nd->last_log_buffer[tpd_cdev_2nd->pos], 0, LAST_LOG_BUFF_LEN);\
	snprintf(tpd_cdev_2nd->last_log_buffer[tpd_cdev_2nd->pos], LAST_LOG_BUFF_LEN, fmt, ##args);\
	tpd_cdev_2nd->pos++; \
	tpd_cdev_2nd->pos &= LAST_LOG_BUFF_SIZE - 1; \
	mutex_unlock(&tpd_cdev_2nd->zlog_mutex); \
} while (0)

#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
extern struct zlog_mod_info zlog_tp_dev_2nd;
#define tpd_print_zlog_2nd(fmt, args...) do {\
	if (tpd_cdev_2nd->ztp_zlog_buffer != NULL) { \
		snprintf(tpd_cdev_2nd->ztp_zlog_buffer, ZLOG_INFO_LEN , fmt, ##args);\
	} \
} while (0)
#endif

enum ts_chip_2nd {
	TS_CHIP_INDETER		= 0x00,
	TS_CHIP_SYNAPTICS	= 0x01,
	TS_CHIP_ATMEL		= 0x02,
	TS_CHIP_CYTTSP		= 0x03,
	TS_CHIP_FOCAL		= 0x04,
	TS_CHIP_GOODIX		= 0x05,
	TS_CHIP_MELFAS		= 0x06,
	TS_CHIP_MSTAR		= 0x07,
	TS_CHIP_HIMAX		= 0x08,
	TS_CHIP_NOVATEK		= 0x09,
	TS_CHIP_ILITEK		= 0x0A,
	TS_CHIP_TLSC		= 0x0B,
	TS_CHIP_CHIPONE		= 0x0C,
	TS_CHIP_HYNITRON	= 0x0D,
	TS_CHIP_GTX8		= 0x0E,
	TS_CHIP_GT9897		= 0x0E,
	TS_CHIP_GCORE		= 0x0F,
	TS_CHIP_SITRONIX	= 0x10,
	TS_CHIP_OMNIVISION	= 0x11,
	TS_CHIP_BTL			= 0x12,
	TS_CHIP_SEMI		= 0x13,
	TS_CHIP_ASX			= 0x14,
	TS_CHIP_MAX			= 0xFF,
};

enum tp_test_err_flag_2nd {
	PROBE_TEST_ERR	= 1,
	FW_TEST_ERR	= 2,
};

enum tp_test_type_2nd {
	RAWDATA_TEST = 0,
	DELTA_TEST = 1,
	BASE_TEST = 2,
};

#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
enum ts_cmd_2nd {
	TS_CMD_READ = 0,
	TS_CMD_WRITE,
	TS_CMD_UNDEF,
};
#endif

enum {
	PROC_SUSPEND_NODE = 0,
	SYS_SUSPEND_NODE = 1,
};

enum {
	mRotatin_0 = 0,
	mRotatin_90 = 1,
	mRotatin_180 = 2,
	mRotatin_270 = 3,
};

enum {
	tp_freq_120Hz = 0,
	tp_freq_240Hz = 1,
	tp_freq_360Hz = 2,
	tp_freq_480Hz = 3,
};

enum {
	single_tap2,
	double_tap2,
	pen_low_batt,
};

enum {
	gpio_mode = 0,
	spi_mode =1,
};

typedef enum lcdstate {
	SCREEN_ON = 0,
	SCREEN_OFF,
	DOZE,
} lcdstate;

typedef enum lcdchange {
	EXIT_LP = 0,
	ENTER_LP,
	LCD_ON,
	LCD_OFF,
} lcdchange;

typedef enum psensor_state {
	DISABLE_PSENSOR = 0,
	ENABLE_PSENSOR,
	PSENSOR_BEGIN_SUSPEND,
	PSENSOR_BEGIN_RESUME,
} psensor_state;

struct tpvendor_t {
	int vendor_id;
	char *vendor_name;
};

struct tp_point_log_2nd {
	unsigned int x;
	unsigned int y;
};

struct tp_ic_vendor_info_2nd {
	u8 tp_chip_id_2nd;
	char *tp_ic_vendor_name_2nd;
};

struct ztp_error_info_2nd {
	tp_error_no_2nd error_no_2nd;
	char *tp_error_info_2nd;
};

enum ztp_algo_item_2nd {
	zte_algo_enable_2nd = 0,
	tp_jitter_check_pixel_2nd,
	tp_jitter_timer_2nd_2nd,
	tp_edge_click_suppression_pixel_2nd,
	tp_long_press_enable_2nd,
	tp_long_press_timer_2nd,
	tp_long_press_pixel_2nd,
};

struct ztp_algo_info_2nd {
	u8 ztp_algo_item_id_2nd;
	char *ztp_algo_item_name_2nd;
};

struct tpd_tpinfo_t_2nd {
	unsigned int chip_model_id;
	unsigned int chip_part_id;
	unsigned int chip_ver;
	unsigned int module_id;
	unsigned int firmware_ver;
	unsigned int config_ver;
	unsigned int display_ver;
	unsigned int i2c_addr;
	unsigned int i2c_type;
	unsigned int spi_num;
	char tp_name[MAX_VENDOR_NAME_LEN];
	char vendor_name[MAX_VENDOR_NAME_LEN];
	char chip_batch[MAX_VENDOR_NAME_LEN];
};

struct ts_firmware_2nd {
	u8 *data;
	int size;
};

struct zte_tp_ctl_2nd {
	int is_single_tap;
	int is_single_aod;
	int is_single_fp;
	int is_wakeup_gesture;
	int is_one_key;
	int is_play_game;
	int is_palm_mode;
	int tp_report_rate;
};

struct zte_tp_time_2nd {
	unsigned long lcd_power_on_time;
	unsigned long tp_reume_start_time;
	unsigned long tp_single_tap_time;
	unsigned long tp_double_tap_time;
	unsigned long tp_fw_upgrade_start_time;
	unsigned long tp_probe_start_time;
};

struct ztp_device_2nd {
	int b_gesture_enable;
	int b_smart_cover_enable;
	int b_glove_enable;
	int rotation_limit_level;
	int display_rotation;
	bool TP_have_registered;
	bool tp_suspend;
	bool sys_set_tp_suspend_flag;
	bool headset_state;
	bool ignore_tp_irq;
	bool tp_suspend_write_gesture;
	bool is_psensor_enable;
	bool tp_resume_before_lcd_cmd;
	bool tp_suspend_after_lcd_cmd_off_end;
	bool zte_tp_algo_2nd;
	bool ufp_enable_2nd;
	bool charger_mode;
	bool fw_ready;
	bool start_ghost_check_timer;
	bool start_temp_ctrl_timer;
	bool notify_temp_ctrl_zone_down;
	bool debug_log_enable;
	bool tpd_proximity_suspended; /* true:suspend false:resume */
	bool is_tp_in_aod_mode;
	u16 ufp_circle_center_x_2nd;
	u16 ufp_circle_center_y_2nd;
	u16 ufp_circle_radius_2nd;
	u8 tp_jitter_check_2nd;
	bool edge_long_press_check_2nd;
	u8 edge_click_sup_p_2nd;
	u16 edge_report_limit_2nd[MAX_LIMIT_NUM];
	u16 user_edge_limit[MAX_LIMIT_NUM];
	u16 long_pess_suppression_2nd[MAX_LIMIT_NUM];
	u8 edge_limit_pixel_level;
	u16 long_press_max_count;
	u16 edge_long_press_timer_2nd;
	u16 tp_jitter_timer_2nd_2nd;
	u8 sensibility_level;
	u8 pen_only_mode;

#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
	bool touch_press;
	bool collect_diffdata_enable;
	bool wait_collect_completion;
	u8 roi_diffdata[ROI_DIFFDATA_LENGTH];
	u8 collect_diffdata[ROI_DIFFDATA_LENGTH * COLLECT_MAX_COUNT];
	u8 roi_diffdata_switch;
	u8 get_diffdata_count;
	struct completion diffdata_collect_completion;
#endif
	u16 max_x;
	u16 max_y;
	u8 tp_chip_id_2nd;
	u32 fw_data_pos;
	int b_single_tap_enable;
	int b_single_aod_enable;
	int one_key_enable;
	int play_game_enable;
	int tp_report_rate;
	int sensibility_enable;
	int finger_lock_flag;
	int palm_mode_en;
	int tp_rst_level;
	int point_down_num;
	bool ghost_check_config_2nd;
	u8 ghost_check_single_time_2nd;
	u8 ghost_check_multi_time_2nd;
	u8 ghost_check_single_count_2nd;
	u8 ghost_check_multi_count_2nd;
	u8 ghost_check_start_time_2nd;
	int ghost_check_ignore_id_2nd;
	int ghost_check_ignore_edge_area_2nd;
	int ghost_check_ignore_corner_x_2nd;
	int ghost_check_ignore_corner_y_2nd;
	int ghost_rst_num;
	int differ_max;
	int tpd_proximity_enable; /* 1:enable 0:disable */
	int tpd_proximity_state; /* 1:far 0:near */

	struct input_dev *input;
	struct workqueue_struct *tpd_wq;
	struct workqueue_struct *tpd_report_wq;
	struct delayed_work tpd_report_work0;
	struct delayed_work tpd_report_work1;
	struct delayed_work tpd_report_work2;
	struct delayed_work tpd_report_work3;
	struct delayed_work tpd_report_work4;
	struct delayed_work tpd_report_work5;
	struct delayed_work tpd_report_work6;
	struct delayed_work tpd_report_work7;
	struct delayed_work tpd_report_work8;
	struct delayed_work tpd_report_work9;
	struct delayed_work tpd_probe_work;
	struct delayed_work charger_work;
	struct delayed_work send_cmd_work;
	struct delayed_work ghost_check_work;
	struct delayed_work temp_ctr_zone_work;
	struct work_struct suspend_work;
	struct work_struct resume_work;
	struct notifier_block charger_notifier;
	struct zte_tp_ctl_2nd ztp_ctl;
	struct zte_tp_time_2nd ztp_time;
#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
	struct delayed_work tpd_report_lcd_state_work;
#endif
#ifdef CONFIG_TOUCHSCREEN_POINT_REPORT_CHECK_2nd
	struct delayed_work point_report_check_work;
#endif
#ifdef CONFIG_TOUCHSCREEN_PSENSOR_REPORT_CHECK_2nd
	struct delayed_work psensor_report_check_work;
#endif
#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
	struct zlog_client *zlog_client;
	struct delayed_work zlog_register_work_2nd;
	char *ztp_zlog_buffer;
	bool zlog_regisered;
	unsigned long tp_reset_timer;
#endif
	u8 ztp_probe_fail_chip_id_2nd;
	struct tp_error_item_2nd zlog_item;
	struct mutex zlog_mutex;
	u16 head;
	u16 tail;
	u16 pos;
	char last_log_buffer[LAST_LOG_BUFF_SIZE][LAST_LOG_BUFF_LEN];
	char *tp_error_last_log_buffer;
	struct tm now_time;
	struct tm probe_time;

	struct bin_attribute attr_fwimage;
	struct kobject *zte_touch_kobj;
	struct firmware *tp_firmware;
	struct mutex cmd_mutex;
	struct mutex report_mutex;
	struct mutex report_down_mutex;
	struct mutex tp_resume_mutex;
	struct tpd_tpinfo_t_2nd ic_tpinfo;
	void *private;
	struct device  *dev;
	struct platform_device *pdev;
	struct platform_device *zte_touch_pdev_2nd;
	void *tp_data;
	struct completion ztp_pm_completion;
	bool ztp_pm_suspend;
	struct completion bbat_test_completion;
	bool need_tp_resume;
	struct completion tp_event_completion;
	u8 bbat_test_result;
	bool bbat_test_enter;
	bool bbat_int_test;
	int zte_tp_selftest_result_2nd;

	int (*tp_resume_func_2nd)(void *data);
	int (*tp_suspend_func_2nd)(void *data);
	int (*tp_fw_upgrade_2nd)(struct ztp_device_2nd *cdev, char *fwname, int fwname_len);
	int (*get_tpinfo_2nd)(struct ztp_device_2nd *cdev);
	int (*get_gesture_2nd)(struct ztp_device_2nd *cdev);
	int (*wake_gesture_2nd)(struct ztp_device_2nd *cdev, int enable);
	int (*get_smart_cover_2nd)(struct ztp_device_2nd *cdev);
	int (*set_smart_cover_2nd)(struct ztp_device_2nd *cdev, int enable);
	int (*get_glove_mode_2nd)(struct ztp_device_2nd *cdev);
	int (*set_glove_mode_2nd)(struct ztp_device_2nd *cdev, int enable);
	int (*tp_suspend_show_2nd)(struct ztp_device_2nd *cdev);
	int (*set_tp_suspend_2nd)(struct ztp_device_2nd *cdev, u8 suspend_node, int enable);
	bool (*tpd_suspend_need_awake_2nd)(struct ztp_device_2nd *cdev);
	int (*set_headset_state_2nd)(struct ztp_device_2nd *cdev, int enable);
	int (*headset_state_show_2nd)(struct ztp_device_2nd *cdev);
	int (*set_display_rotation_2nd)(struct ztp_device_2nd *cdev, int mrotation);
	bool (*tpd_esd_check_2nd)(struct ztp_device_2nd *cdev);
	void (*tpd_enable_irq_2nd)(bool value);
	void (*tpd_report_uevent_2nd)(u8 gesture_key);
	int (*tp_hw_reset_2nd)(void);
	void (*tp_reset_gpio_output_2nd)(bool value);
	int (*get_singletap_2nd)(struct ztp_device_2nd *cdev);
	int (*set_singletap_2nd)(struct ztp_device_2nd *cdev, int enable);
	int (*get_singleaod_2nd)(struct ztp_device_2nd *cdev);
	int (*set_singleaod_2nd)(struct ztp_device_2nd *cdev, int enable);
	int (*get_noise_2nd)(struct ztp_device_2nd *cdev);
	int (*get_one_key_2nd)(struct ztp_device_2nd *cdev);
	int (*set_one_key_2nd)(struct ztp_device_2nd *cdev, int enable);
	int (*get_play_game_2nd)(struct ztp_device_2nd *cdev);
	int (*set_play_game_2nd)(struct ztp_device_2nd *cdev, int enable);
	int (*set_tp_report_rate_2nd)(struct ztp_device_2nd *cdev, int enable);
	int (*get_tp_report_rate_2nd)(struct ztp_device_2nd *cdev);
	int (*set_sensibility_leve_2nd)(struct ztp_device_2nd *cdev, u8 level);
	int (*set_gpio_mode_2nd)(struct ztp_device_2nd *cdev, u8 mode);
	int (*get_sensibility_2nd)(struct ztp_device_2nd *cdev);
	int (*set_sensibility_2nd)(struct ztp_device_2nd *cdev, u8 enable);
	int (*get_pen_only_mode_2nd)(struct ztp_device_2nd *cdev);
	int (*set_pen_only_mode_2nd)(struct ztp_device_2nd *cdev, u8 enable);
	int (*tpd_shutdown_2nd)(struct ztp_device_2nd *cdev);
	int (*get_tp_self_test_result_2nd)(struct ztp_device_2nd *cdev, char *buf);
	int (*tp_self_test_2nd)(struct ztp_device_2nd *cdev);
	int (*lcd_fps_notify_2nd)(struct ztp_device_2nd *cdev, u8 lcd_fps);
	int (*charger_state_notify_2nd)(struct ztp_device_2nd *cdev);
	int (*tpd_send_cmd_2nd)(struct ztp_device_2nd *cdev);
	int (*tp_palm_mode_read_2nd)(struct ztp_device_2nd *cdev);
	int (*tp_palm_mode_write_2nd)(struct ztp_device_2nd *cdev, int enable);
	int (*tp_psensor_report_check_2nd)(struct ztp_device_2nd *cdev);
	int (*tp_rst_level_get_2nd)(struct ztp_device_2nd *cdev);
	int (*tp_rst_level_set_2nd)(struct ztp_device_2nd *cdev, int rst_level);
	int (*ghost_check_reset_2nd)(struct ztp_device_2nd *cdev);
#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
	int (*tp_send_roi_cmd_2nd)(struct ztp_device_2nd *cdev, enum ts_cmd_2nd cmd);
	unsigned char *(*get_roi_diffdata_2nd)(struct ztp_device_2nd *cdev);
#endif
	int (*tp_bbat_test_2nd)(struct ztp_device_2nd *cdev);
};

extern char lcd_name_2nd[];

extern struct ztp_device_2nd *tpd_cdev_2nd;
extern void set_lcd_reset_processing_2nd(bool enable);
extern void enable_tpd_irq_2nd(bool value);
void tpd_zlog_record_notify_2nd(tp_error_no_2nd error_no_2nd);
#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
void tpd_clean_diffdata_2nd(void);
void tpd_get_hex_diffdata_2nd(struct ztp_device_2nd *cdev);
#endif
int get_tp_chip_id_2nd(void);
void tpd_touch_press_2nd(struct input_dev *input, u16 x, u16 y, u16 id, u8 touch_major, u8  pressure);
void tpd_touch_release_2nd(struct input_dev *input, u16 id);
void tpd_clean_all_event_2nd(void);
int set_gpio_mode_2nd(u8 mode);
void tp_free_tp_firmware_data_2nd(void);
int tp_alloc_tp_firmware_data_2nd(int buf_size);
int  tpd_copy_to_tp_firmware_data_2nd(char *buf);
void  tpd_reset_fw_data_pos_and_size_2nd(void);
void change_tp_state_2nd(lcdchange lcd_change);
void change_psensor_state_2nd(psensor_state psensor_state);
int get_tp_consum_time_2nd(unsigned long jiffies_time);
#endif
