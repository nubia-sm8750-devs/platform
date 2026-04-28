
/***********************
 * file : ztp_state_change_2nd.c
 ***********************/

#include "ztp_common_2nd.h"
#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
#include "ztp_ufp_2nd.h"
#endif

static char *lcdstate_to_str_2nd[] = {
	"screen_on_2nd",
	"screen_off_2nd",
	"screen_in_doze",
};

static char *lcdchange_to_str_2nd[] = {
	"lcd_exit_lp",
	"lcd_enter_lp",
	"lcd_on",
	"lcd_off",
};
struct notifier_block tpd_nb;
atomic_t current_lcd_state_2nd = ATOMIC_INIT(SCREEN_ON);
atomic_t current_psensor_state_2nd = ATOMIC_INIT(DISABLE_PSENSOR);

#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
extern struct ufp_ops_2nd ufp_tp_ops_2nd;
#endif
DEFINE_MUTEX(ufp_mac_mutex);

static inline void lcd_on_thing_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int tp_time = 0;
	int ret = 0;

	tp_time = get_tp_consum_time_2nd(cdev->ztp_time.lcd_power_on_time);
	TPD_DMESG("tp_time lcd power on -> tp resume start:%d.", tp_time);
	if (cdev->ztp_time.tp_double_tap_time) {
		tp_time = get_tp_consum_time_2nd(cdev->ztp_time.tp_double_tap_time);
		TPD_DMESG("tp_time double tap -> tp resume:%d.", tp_time);
		cdev->ztp_time.tp_double_tap_time = 0;
	}
	cdev->tp_suspend_write_gesture = false;
	if (cdev->ztp_pm_suspend) {
		ret = wait_for_completion_timeout(&cdev->ztp_pm_completion, msecs_to_jiffies(700));
		if (!ret) {
			TPD_DMESG("Warning:still in pm_suspend(deep) and has timeout 700ms");
		}
	}
	queue_work(cdev->tpd_wq, &(cdev->resume_work));
}

static inline void lcd_off_thing_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
	reinit_completion(&ufp_tp_ops_2nd.ufp_completion);
#endif
	queue_work(cdev->tpd_wq, &(cdev->suspend_work));
}

static inline void lcd_doze_thing_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	queue_work(cdev->tpd_wq, 	&(cdev->suspend_work));
}

static void screen_on_2nd(lcdchange lcd_change)
{
	switch (lcd_change) {
	case ENTER_LP:
		atomic_set(&current_lcd_state_2nd, DOZE);
		lcd_doze_thing_2nd();
		break;
	case LCD_OFF:
		atomic_set(&current_lcd_state_2nd, SCREEN_OFF);
		lcd_off_thing_2nd();
		break;
	default:
		UFP_ERR("ignore err lcd change");
	}
}

static void screen_off_2nd(lcdchange lcd_change)
{
	switch (lcd_change) {
	case LCD_ON:
		atomic_set(&current_lcd_state_2nd, SCREEN_ON);
		lcd_on_thing_2nd();
		break;
	case ENTER_LP:
		atomic_set(&current_lcd_state_2nd, DOZE);
		break;
	default:
		UFP_ERR("ignore err lcd change");
	}
}

static void doze_2nd(lcdchange lcd_change)
{
	switch (lcd_change) {
	case EXIT_LP:
		atomic_set(&current_lcd_state_2nd, SCREEN_ON);
		lcd_on_thing_2nd();
		break;
	case LCD_OFF:
		atomic_set(&current_lcd_state_2nd, SCREEN_OFF);
		break;
	case LCD_ON:
		atomic_set(&current_lcd_state_2nd, SCREEN_ON);
		lcd_on_thing_2nd();
		break;
	default:
		UFP_ERR("ignore err lcd change");
	}
}

void change_tp_state_2nd(lcdchange lcd_change)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	mutex_lock(&cdev->tp_resume_mutex);

	UFP_INFO("current_lcd_state_2nd:%s, lcd change:%s",
			lcdstate_to_str_2nd[atomic_read(&current_lcd_state_2nd)],
							lcdchange_to_str_2nd[lcd_change]);
	switch (atomic_read(&current_lcd_state_2nd)) {
	case SCREEN_ON:
		screen_on_2nd(lcd_change);
		break;
	case SCREEN_OFF:
		screen_off_2nd(lcd_change);
		break;
	case DOZE:
		doze_2nd(lcd_change);
		break;
	default:
		atomic_set(&current_lcd_state_2nd, SCREEN_ON);
		lcd_on_thing_2nd();
		UFP_ERR("err lcd light change");
	}

	mutex_unlock(&cdev->tp_resume_mutex);
}

void change_psensor_state_2nd(psensor_state psensor_state)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev == NULL)
	{
		UFP_ERR("err: cdev is NULL");
		return;
	}

	mutex_lock(&cdev->tp_resume_mutex);
	if (atomic_read(&current_psensor_state_2nd) == psensor_state)
	{
		UFP_ERR("err: current_psensor_state_2nd equal change_state");
		mutex_unlock(&cdev->tp_resume_mutex);
		return;
	}
	switch (psensor_state) {
	case DISABLE_PSENSOR:
		cdev->is_psensor_enable = false;
		atomic_set(&current_psensor_state_2nd, DISABLE_PSENSOR);
		break;
	case ENABLE_PSENSOR:
		cdev->is_psensor_enable = true;
		atomic_set(&current_psensor_state_2nd, ENABLE_PSENSOR);
		break;
	case PSENSOR_BEGIN_SUSPEND:
#ifdef CONFIG_TOUCHSCREEN_LCD_NOTIFY_2nd
		tpd_notifier_call_chain_2nd(PSENSOR_NOTIFY_LCD_SUSPEND);
#endif
		atomic_set(&current_psensor_state_2nd, PSENSOR_BEGIN_SUSPEND);
		break;
	case PSENSOR_BEGIN_RESUME:
#ifdef CONFIG_TOUCHSCREEN_LCD_NOTIFY_2nd
		tpd_notifier_call_chain_2nd(PSENSOR_NOTIFY_LCD_RESUME);
#endif
		atomic_set(&current_psensor_state_2nd, PSENSOR_BEGIN_RESUME);
		break;
	default:
		atomic_set(&current_psensor_state_2nd, DISABLE_PSENSOR);
		cdev->is_psensor_enable = false;
		UFP_ERR("err psensor_state change");
	}

	mutex_unlock(&cdev->tp_resume_mutex);
}

static void tpd_resume_work_2nd(struct work_struct *work)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int tp_time = 0;

	if (cdev->tp_resume_func_2nd) {
		cdev->ztp_time.tp_reume_start_time = jiffies;
		cdev->tp_resume_func_2nd(cdev->tp_data);
		tp_time = get_tp_consum_time_2nd(cdev->ztp_time.tp_reume_start_time);
		TPD_DMESG("tp_time tp resume start -> tp resume end:%d.", tp_time);
		cdev->ghost_rst_num = 0;
	}
}

static void tpd_suspend_work_2nd(struct work_struct *work)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->tp_suspend_func_2nd)
		cdev->tp_suspend_func_2nd(cdev->tp_data);
}

int suspend_tp_need_awake_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->tpd_suspend_need_awake_2nd) {
		return cdev->tpd_suspend_need_awake_2nd(cdev);
	}
	return 0;
}

bool tp_esd_check_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->tpd_esd_check_2nd) {
		return cdev->tpd_esd_check_2nd(cdev);
	}
	return 0;
}

void set_lcd_reset_processing_2nd(bool enable)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (enable) {
		cdev->ignore_tp_irq = true;
	} else {
		cdev->ignore_tp_irq = false;
	}
	TPD_DMESG("cdev->ignore_tp_irq is %d.\n", cdev->ignore_tp_irq);
}

void enable_tpd_irq_2nd(bool value)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->tpd_enable_irq_2nd) {
		TPD_DMESG("tpd_enable_irq_2nd %d\n", value);
		cdev->tpd_enable_irq_2nd(value);
	}
}

int set_gpio_mode_2nd(u8 mode)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->set_gpio_mode_2nd) {
		return cdev->set_gpio_mode_2nd(cdev, mode);
	}
	return -EIO;
}

void tpd_reset_gpio_output_2nd(bool value)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->tp_reset_gpio_output_2nd) {
		cdev->tp_reset_gpio_output_2nd(value);
	}
}

#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
extern void ufp_report_lcd_state_2nd(void);

static void ufp_report_lcd_state_work_2nd(struct work_struct *work)
{
	ufp_report_lcd_state_2nd();
}

void ufp_report_lcd_state_delayed_work_2nd(u32 ms)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	mod_delayed_work(cdev->tpd_wq, &cdev->tpd_report_lcd_state_work, msecs_to_jiffies(ms));

}

void cancel_report_lcd_state_delayed_work_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	cancel_delayed_work_sync(&cdev->tpd_report_lcd_state_work);

}
#endif

void tpd_resume_work_init_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	INIT_WORK(&cdev->resume_work, tpd_resume_work_2nd);
	INIT_WORK(&cdev->suspend_work,tpd_suspend_work_2nd);
#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
	INIT_DELAYED_WORK(&cdev->tpd_report_lcd_state_work, ufp_report_lcd_state_work_2nd);
#endif

}

void tpd_resume_work_deinit_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	cancel_work_sync(&cdev->resume_work);
	cancel_work_sync(&cdev->suspend_work);
#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
	cancel_delayed_work_sync(&cdev->tpd_report_lcd_state_work);
#endif

}

int get_tp_consum_time_2nd(unsigned long jiffies_time)
{
	int tp_time = 0;

	tp_time = jiffies_to_msecs(jiffies - jiffies_time);
	return tp_time;
}

#ifdef CONFIG_TOUCHSCREEN_LCD_NOTIFY_2nd
/*for lcd low power mode*/
int ufp_notifier_cb_2nd(int in_lp)
{
	int retval = 0;

	TPD_DMESG("in lp %d!", in_lp);

	if (in_lp)
		change_tp_state_2nd(ENTER_LP);
	else
		change_tp_state_2nd(EXIT_LP);

	return retval;
}

static int tpd_lcd_notifier_callback_2nd(struct notifier_block *self,
	unsigned long event, void *data)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	int tp_time = 0;
	int ret = 0;

	if (tpd_cdev_2nd == NULL) {
		TPD_DMESG("zte touch deinit, return\n");
		return -ENOMEM;
	}
	switch (event) {
	case LCD_POWER_ON:
		cdev->ztp_time.lcd_power_on_time = jiffies;
		TPD_DMESG("lcd power on\n");
		set_lcd_reset_processing_2nd(true);
		tpd_reset_gpio_output_2nd(1);
		enable_tpd_irq_2nd(false);
		break;
	case LCD_RESET:
		tp_time = get_tp_consum_time_2nd(cdev->ztp_time.lcd_power_on_time);
		TPD_DMESG("lcd reset, tp_time tp power on -> lcd reset time:%d\n", tp_time);
		if (cdev->tp_resume_before_lcd_cmd)
			 change_tp_state_2nd(LCD_ON);
		break;
	case LCD_CMD_ON:
		TPD_DMESG("lcd cmd on\n");
		if (!cdev->tp_resume_before_lcd_cmd)
			change_tp_state_2nd(LCD_ON);
		set_lcd_reset_processing_2nd(false);
		enable_tpd_irq_2nd(true);
		break;
	case LCD_CMD_OFF:
		TPD_DMESG("lcd cmd off\n");
		if (cdev->need_tp_resume) {
			TPD_DMESG("tp event processing, need wait");
			ret = wait_for_completion_timeout(&cdev->tp_event_completion, msecs_to_jiffies(2000));
			if (!ret) {
				TPD_DMESG("Warning:wait tp_event_completion timeout 2000ms");
			}
		}
		if (suspend_tp_need_awake_2nd()) {
			tpd_notifier_call_chain_2nd(LCD_SUSPEND_POWER_ON);
		} else {
			tpd_notifier_call_chain_2nd(LCD_SUSPEND_POWER_OFF);
			set_lcd_reset_processing_2nd(true);
			enable_tpd_irq_2nd(false);
		}
		if (!cdev->tp_suspend_after_lcd_cmd_off_end)
			change_tp_state_2nd(LCD_OFF);
		break;
	case LCD_CMD_OFF_END:
		TPD_DMESG("lcd cmd off end\n");
		if (cdev->tp_suspend_after_lcd_cmd_off_end) {
			change_tp_state_2nd(LCD_OFF);
			usleep_range(5000, 6000);
		}
		break;
	case LCD_POWER_OFF:
		TPD_DMESG("lcd power off\n");
		break;
	case LCD_POWER_OFF_RESET_LOW:
		TPD_DMESG("lcd power off reset low\n");
		 tpd_reset_gpio_output_2nd(0);
		break;
	case LCD_ENTER_AOD:
		if (cdev->ztp_time.tp_single_tap_time) {
			tp_time = get_tp_consum_time_2nd(cdev->ztp_time.tp_single_tap_time);
			TPD_DMESG("tp_time single tap -> tp enter aod:%d.", tp_time);
			cdev->ztp_time.tp_single_tap_time = 0;
		}
		TPD_DMESG("lcd enter aod\n");
		cdev->is_tp_in_aod_mode = true;
		ufp_notifier_cb_2nd(true);
#ifdef CONFIG_TOUCHSCREEN_UFP_MAC_2nd
		ufp_report_lcd_state_delayed_work_2nd(50);
#endif
		break;
case LCD_EXIT_AOD:
		TPD_DMESG("lcd exit aod\n");
		cdev->is_tp_in_aod_mode = false;
		ufp_notifier_cb_2nd(false);
		break;
	default:
		TPD_DMESG("lcd state unknown\n");
		break;
	}
	return 0;
}

void lcd_notify_register_2nd(void)
{
	int ret = 0;

	tpd_nb.notifier_call = tpd_lcd_notifier_callback_2nd;
	ret = lcd_notifier_register_client_2nd(&tpd_nb);
	if (ret) {
		TPD_DMESG(" Unable to register fb_notifier: %d\n", ret);
	}
	TPD_DMESG(" register lcd notifier success\n");
}
void lcd_notify_unregister_2nd(void)
{
	lcd_notifier_unregister_client_2nd(&tpd_nb);
}
#endif

