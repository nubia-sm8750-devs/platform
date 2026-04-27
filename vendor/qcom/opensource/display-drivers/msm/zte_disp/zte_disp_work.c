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
char *msg[12] = {
	/* hbm msg */
	"HBM_STATUS=OFF",
	"HBM_STATUS=ON",
	"HBM_STATUS=FG_RELEASE",
	"HBM_STATUS=FG_PRESS",
	"HBM_SET_RESULT=SUCCESSFUL",
	"HBM_SET_RESULT=FAILED",
	/* fps msg */
	"LCD_FPS=60",
	"LCD_FPS=90",
	"LCD_FPS=120",
	"LCD_FPS=144",
	"LCD_FPS=165",
};

int get_index(int mode) {
	int id = 0;

	if (mode == 90) {
		id = 1;
	} else if (mode == 120) {
		id = 2;
	} else if (mode == 144) {
		id = 3;
	} else if (mode == 165) {
		id = 4;
	}

	return id + MSG_FPS;
}

void zte_panel_send_uevent(int type, int mode, int ret)
{
	char *envp[3];
	struct device *dev= get_disp_dev();

	if (!dev)
		return;

	switch(type) {
		case MSG_HBM:
		case MSG_FOD:
			envp[0] = msg[type + mode];
			envp[1] = msg[ret > 0 ? 4 : 5];
			break;
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
	struct dsi_panel *panel = container_of(work,
				struct dsi_panel, dim_work);

	if (!panel) {
		pr_err("MSM_LCD no primary panel device\n");
		goto exit;
	}
    zte_set_disp_parameter(panel, ZTE_LCD_DIM_CTRL, panel->set_dim, false);

exit:
    return;
}

void panel_hbm_work_handler(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct dsi_panel *panel = container_of(dwork,
				struct dsi_panel, hbm_delayed_work);
	bool hbm = false;

	if (!panel) {
		pr_err("MSM_LCD no primary panel device\n");
		goto exit;
	}

	hbm = panel->disp_feature[ZTE_LCD_HBM_CTRL].mode || (panel->layer_flag & ZTE_LAYER_DIM);

	zte_panel_send_uevent(MSG_HBM, hbm ? 1 : 0, 1);
exit:
    return;
}

/* Started by AICoder, pid:f08fc3cb37p413d144e10b05604bff328285a747 */
// This function is responsible for handling the delayed work related to the panel icon.
// It takes a pointer to a work_struct as an argument and retrieves the associated dsi_panel structure.
void panel_icon_work_handler(struct work_struct *work) {
    struct delayed_work *dwork = to_delayed_work(work);
    struct dsi_panel *panel = container_of(dwork, struct dsi_panel, icon_delayed_work);
    s64 ms_per_frame = 0;
    u32 delay = 0;

    if (!panel) {
        pr_err("MSM_LCD no primary panel device\n");
        return;
    }

    ms_per_frame = 1000 / (panel->cur_mode->timing.refresh_rate);
    delay = 3 * ms_per_frame;
    panel->irq_rec_enable = true;
    init_completion(&panel->tx_done_gate);

    /* Wait for 3 frames for pp_done */
    if (!wait_for_completion_timeout(&panel->tx_done_gate, delay)) {
        pr_err("MSM_LCD is unable to accurately report the trigger identification timing!\n");
    }
    panel->irq_rec_enable = false;
    zte_panel_send_uevent(MSG_FOD, panel->layer_flag & ZTE_LAYER_ICON ? 1 : 0, 1);
}
/* Ended by AICoder, pid:f08fc3cb37p413d144e10b05604bff328285a747 */

void setting_bl_work_handler(struct work_struct *work)
{
	struct dsi_panel *panel = container_of(work,
				struct dsi_panel, rebl_work);

	if (!panel) {
		pr_err("MSM_LCD no primary panel device\n");
		goto exit;
	}

    pr_info("[MSM_LCD] set bl %d\n", panel->cur_bl);

	if (panel->cur_bl) {
		zte_set_disp_parameter(panel, ZTE_LCD_SET_BL, panel->cur_bl, false);
	}

exit:
    return;
}

void setting_aodbl_work_handler(struct work_struct *work)
{
	struct dsi_panel *panel = container_of(work,
				struct dsi_panel, aodbl_work);

	if (!panel) {
		pr_err("MSM_LCD no primary panel device\n");
		goto exit;
	}

    pr_info("[MSM_LCD] set aodbl %d\n", panel->disp_feature[ZTE_LCD_AOD_BL].mode);

	zte_set_disp_parameter(panel, ZTE_LCD_AOD_BL, panel->disp_feature[ZTE_LCD_AOD_BL].mode, false);

exit:
    return;
}

void zte_schedule_rebl_work(struct dsi_panel *panel,
		bool en)
{
	if (en == panel->pending_rebl)
		return;

	if (en) {
		queue_work(panel->rebl_workq, &panel->rebl_work);
		panel->pending_rebl = true;
	} else {
		panel->pending_rebl = false;
	}
}

void zte_schedule_aodbl_work(struct dsi_panel *panel,
		bool en)
{
	if (en == panel->pending_aodbl)
		return;

	if (en) {
		queue_work(panel->aodbl_workq, &panel->aodbl_work);
		panel->pending_aodbl = true;
	} else {
		panel->pending_aodbl = false;
	}
}

void report_aod_status(struct dsi_panel *panel, u32 msg)
{
	static bool faod = false;
	static bool paod = false;
    
	pr_info("[MSM_LCD] aod msg[%d], faod[%d], paod[%d]\n", msg, faod, paod);
	switch(msg) {
		case FRAME_AOD_ACTIVE:
			faod = true;
			zte_schedule_rebl_work(panel, false);
			if (paod)
				zte_schedule_aodbl_work(panel, true);
			break;
		case FRAME_AOD_MISS:
			faod = false;
			zte_schedule_aodbl_work(panel, false);
			if (!paod) 
				zte_schedule_rebl_work(panel, true);
			break;
		case PM_AOD_ON:
			paod = true;
			zte_schedule_rebl_work(panel, false);
			if (faod)
				zte_schedule_aodbl_work(panel, true);
			break;
		case PM_AOD_OFF:
			paod = false;
			zte_schedule_aodbl_work(panel, false);
			if (!faod)
				zte_schedule_rebl_work(panel, true);
			break;
		default:
			break;
	}
}

/* Started by AICoder, pid:a6ba671ce5fbacb149660b2f7014ed454337a839 */
// This function is responsible for triggering interrupts related to the SDE encoder.
// It takes two parameters: a pointer to a sde_encoder_phys structure and an unsigned integer irq_type representing the type of interrupt.
void sde_irq_triger(void *sde_encoder_phys, unsigned int irq_type) {
    struct sde_encoder_phys *phys_enc = sde_encoder_phys;
    struct sde_connector *c_conn = NULL;
    struct dsi_panel *panel;
    struct dsi_display *display;

    if (!phys_enc || !phys_enc->connector) {
        pr_err("Invalid phys_enc params\n");
        return;
    }

    c_conn = to_sde_connector(phys_enc->connector);
    if (!c_conn) {
        pr_err("Invalid c_conn params\n");
        return;
    }

    if (c_conn->connector_type != DRM_MODE_CONNECTOR_DSI) {
        //pr_err("not in dsi mode\n");
        return;
    }

    display = (struct dsi_display *) c_conn->display;
    if (!display || !display->panel) {
        pr_err("msm_lcd Invalid display panel\n");
        return;
    }

    panel = display->panel;

    if (panel->irq_rec_enable) {
        if (irq_type == PP_DONE) {
            complete_all(&panel->tx_done_gate);
        }
    }
}
/* Ended by AICoder, pid:a6ba671ce5fbacb149660b2f7014ed454337a839 */
