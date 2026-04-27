/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_backlight.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2023/08
** Author : Display
******************************************************************/
#include "zte_disp_feature.h"
#include "zte_disp_work.h"
#include "sde_encoder_phys.h"
char *msg[4] = {
	"LCD_FPS=60",
	"LCD_FPS=90",
	"LCD_FPS=120",
	"LCD_FPS=144",
};

int get_index(int mode) {
	int id = 0;

	if (mode == 90) {
		id = 1;
	} else if (mode == 120) {
		id = 2;
	} else if (mode == 144) {
		id = 3;
	}

	return id;
}

void zte_panel_send_uevent(int type, int mode, int ret)
{
	char *envp[3];
	struct device *dev= get_disp_dev();

	if (!dev)
		return;

	switch(type) {
		case MSG_FPS:
			envp[0] = msg[get_index(mode)];
			envp[1] = NULL;
			break;
		default:
			break;
	}
	envp[2] = NULL;
	kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
	if (type == MSG_FPS) {
		pr_info("[MSM_LCD] uevent = %s\n", envp[0]);
	} else {
		pr_info("[MSM_LCD] uevent = %s , %s\n", envp[0], envp[1]);
	}
}

void dimming_work_handler(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct dsi_panel *panel = container_of(dwork,
				struct dsi_panel, dim_work);

	if (!panel) {
		pr_err("MSM_LCD no primary panel device\n");
		goto exit;
	}
    zte_set_disp_parameter(panel, ZTE_LCD_DIM_CTRL, panel->set_dim, false);

exit:
    return;
}