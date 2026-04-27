#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/pm_wakeup.h>
#include "ztp_common_2nd.h"

#define SINGLE_TAP_DELAY	600

#ifdef ZTE_ONE_KEY
#define MAX_POINTS_SUPPORT 10
#define FP_GESTURE_DOWN	"fp_gesture_down=true"
#define FP_GESTURE_UP	"fp_gesture_up=true"

static char *one_key_finger_id_2nd[] = {
	"finger_id=0",
	"finger_id=1",
	"finger_id=2",
	"finger_id=3",
	"finger_id=4",
	"finger_id=5",
	"finger_id=6",
	"finger_id=7",
	"finger_id=8",
	"finger_id=9",
};
#endif

static char *tppower_to_str_2nd[] = {
	"TP_POWER_STATUS=2",		/* TP_POWER_ON */
	"TP_POWER_STATUS=1",		/* TP_POWER_OFF */
	"TP_POWER_STATUS=3",		/* TP_POWER_AOD */
};

struct ufp_ops_2nd ufp_tp_ops_2nd;

extern atomic_t current_lcd_state_2nd;

int ufp_get_lcdstate_2nd(void)
{
	return atomic_read(&current_lcd_state_2nd);
}

void ufp_report_gesture_uevent_2nd(char *str)
{
	char *envp[2];
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	envp[0] = str;
	envp[1] = NULL;
	kobject_uevent_env(&(ufp_tp_ops_2nd.uevent_pdev->dev.kobj), KOBJ_CHANGE, envp);

	pm_wakeup_event(&cdev->pdev->dev, 2000);
	UFP_INFO("pm_wakeup_event 2000 success");

	UFP_INFO("%s", str);
	if (strcmp(str, SINGLE_TAP_GESTURE) == 0) {
		cdev->ztp_time.tp_single_tap_time = jiffies;
	} else if (strcmp(str, DOUBLE_TAP_GESTURE) == 0) {
		cdev->ztp_time.tp_double_tap_time = jiffies;
	}
}

static inline void __report_ufp_uevent_2nd(char *str)
{
	char *envp[3];

	if (!ufp_tp_ops_2nd.uevent_pdev) {
		UFP_ERR("uevent pdev is null!");
		return;
	}

	if (!strcmp(str, AOD_AREAMEET_DOWN))
		ufp_report_gesture_uevent_2nd(SINGLE_TAP_GESTURE);

	envp[0] = str;
	envp[1] = tppower_to_str_2nd[atomic_read(&current_lcd_state_2nd)];
	envp[2] = NULL;
	kobject_uevent_env(&(ufp_tp_ops_2nd.uevent_pdev->dev.kobj), KOBJ_CHANGE, envp);
	UFP_INFO("%s", str);
}

void report_ufp_uevent_2nd(int enable)
{
	static int area_meet_down = 0;

	if (enable && !area_meet_down) {
		area_meet_down = 1;
		if (atomic_read(&current_lcd_state_2nd) == SCREEN_ON) {/* fp func enable is guaranted by user*/
			__report_ufp_uevent_2nd(AREAMEET_DOWN);
		 } else {
			__report_ufp_uevent_2nd(AOD_AREAMEET_DOWN);
			ufp_tp_ops_2nd.aod_fp_down = true;
		}
	} else if (!enable && area_meet_down) {
			area_meet_down = 0;
			__report_ufp_uevent_2nd(AREAMEET_UP);
			if (ufp_tp_ops_2nd.aod_fp_down && ufp_tp_ops_2nd.wait_completion) {
				complete(&ufp_tp_ops_2nd.ufp_completion);
			}
			ufp_tp_ops_2nd.aod_fp_down = false;
	}
}

static inline int zte_in_zeon_2nd(int x, int y)
{
	int ret = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if ((cdev->ufp_circle_center_x_2nd - cdev->ufp_circle_radius_2nd < x) &&
		(cdev->ufp_circle_center_x_2nd  + cdev->ufp_circle_radius_2nd > x) &&
		(cdev->ufp_circle_center_y_2nd  - cdev->ufp_circle_radius_2nd < y) &&
		(cdev->ufp_circle_center_y_2nd + cdev->ufp_circle_radius_2nd > y)) {
		ret = 1;
	}

	return ret;
}

#ifdef ZTE_ONE_KEY
static inline void report_one_key_uevent_2nd(char *str, int i)
{
	char *envp[3];

	envp[0] = str;
	envp[1] = one_key_finger_id_2nd[i];
	envp[2] = NULL;
	kobject_uevent_env(&(ufp_tp_ops_2nd.uevent_pdev->dev.kobj), KOBJ_CHANGE, envp);
	UFP_INFO("%s", str);
}

/* We only track the first finger in zeon */
void one_key_report_2nd(int is_down, int x, int y, int finger_id)
{
	int retval;
	static char one_key_finger[MAX_POINTS_SUPPORT] = {0};
	static int one_key_down = 0;

	if (is_down) {
		retval = zte_in_zeon_2nd(x, y);
		if (retval && !one_key_finger[finger_id] && !one_key_down) {
			one_key_finger[finger_id] = 1;
			one_key_down = 1;
			report_one_key_uevent_2nd(FP_GESTURE_DOWN, finger_id);
		}
	} else if (one_key_finger[finger_id]) {
			one_key_finger[finger_id] = 0;
			one_key_down = 0;
			report_one_key_uevent_2nd(FP_GESTURE_UP, finger_id);
	}
}
#endif

#ifdef CONFIG_TOUCHSCREEN_POINT_SIMULAT_UF_2nd
/* We only track the first finger in zeon */
void uf_touch_report_2nd(int is_down, int x, int y, int finger_id)
{
	int retval;
	static int fp_finger[MAX_POINTS_SUPPORT] = { 0 };
	static int area_meet_down = 0;

	if (is_down) {
		retval = zte_in_zeon_2nd(x, y);
		if (retval && !fp_finger[finger_id] && !area_meet_down) {
			fp_finger[finger_id] = 1;
			area_meet_down = 1;
			__report_ufp_uevent_2nd(AREAMEET_DOWN);
		}
	} else if (fp_finger[finger_id]) {
			fp_finger[finger_id] = 0;
			area_meet_down = 0;
			__report_ufp_uevent_2nd(AREAMEET_UP);
	}
}
#endif

static inline void report_lcd_uevent_2nd(struct kobject *kobj, char **envp)
{
	int retval;

	envp[0] = "aod=true";
	envp[1] = NULL;
	retval = kobject_uevent_env(kobj, KOBJ_CHANGE, envp);
	if (retval != 0)
		UFP_ERR("lcd state uevent send failed!");
}

void ufp_report_lcd_state_2nd(void)
{
	char *envp[2];

	if (!ufp_tp_ops_2nd.uevent_pdev) {
		UFP_ERR("uevent pdev is null!");
		return;
	}

	report_lcd_uevent_2nd(&(ufp_tp_ops_2nd.uevent_pdev->dev.kobj), envp);
}
EXPORT_SYMBOL(ufp_report_lcd_state_2nd);

int ufp_mac_init_2nd(void)
{
	if (tpd_cdev_2nd->zte_touch_pdev_2nd)
		ufp_tp_ops_2nd.uevent_pdev = tpd_cdev_2nd->zte_touch_pdev_2nd;
	init_completion(&ufp_tp_ops_2nd.ufp_completion);
	ufp_tp_ops_2nd.aod_fp_down = false;
	ufp_tp_ops_2nd.wait_completion = false;
	return 0;
}

void  ufp_mac_exit_2nd(void)
{
	ufp_tp_ops_2nd.uevent_pdev = NULL;
}

