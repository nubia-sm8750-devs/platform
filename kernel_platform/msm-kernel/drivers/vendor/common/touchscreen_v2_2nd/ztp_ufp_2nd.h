#ifndef _ZTP_UFP_2ND_H_
#define _ZTP_UFP_2ND_H_

#define ZTE_ONE_KEY

/* zte: add for fp uevent report */
#define SINGLE_TAP_GESTURE "single_tap2=true"
#define DOUBLE_TAP_GESTURE "double_tap2=true"
#define AOD_AREAMEET_DOWN "aod_areameet_down=true"
#define AREAMEET_DOWN "areameet_down=true"
#define AREAMEET_UP "areameet_up=true"

#define UFP_FP_DOWN 1
#define UFP_FP_UP 0

struct ufp_ops_2nd {
	struct platform_device *uevent_pdev;
	struct completion ufp_completion;
	bool aod_fp_down;
	bool wait_completion;
};

/* Log define */
#define UFP_INFO(fmt, arg...)	pr_info("[ZTE_LDD_TP_2nd][TPD_UFP][INFO]:(%s, %d): "fmt"\n", __func__, __LINE__, ##arg)
#define UFP_ERR(fmt, arg...)	pr_err("[ZTE_LDD_TP_2nd][TPD_UFP][ERROR]:(%s, %d): "fmt"\n", __func__, __LINE__, ##arg)

extern struct ufp_ops_2nd ufp_tp_ops_2nd;

#ifdef CONFIG_TOUCHSCREEN_POINT_SIMULAT_UF_2nd
void uf_touch_report_2nd(int is_down, int x, int y, int finger_id);
#endif
#ifdef ZTE_ONE_KEY
void one_key_report_2nd(int is_down, int x, int y, int finger_id);
#endif

int ufp_get_lcdstate_2nd(void);
void ufp_report_gesture_uevent_2nd(char *str);
void report_ufp_uevent_2nd(int enable);
void ufp_report_lcd_state_delayed_work_2nd(u32 ms);

#endif /* _ZTP_UFP_2ND_H_ */
