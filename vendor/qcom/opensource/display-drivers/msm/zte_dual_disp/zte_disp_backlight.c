/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_backlight.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2023/08
** Author : Display
******************************************************************/
#include <drm/drm_mipi_dsi.h>
#include "../dsi/dsi_display.h"
#include "zte_disp_backlight.h"

int zte_dsi_dual_panel_update_backlight(struct dsi_panel *panel,
	u32 bl_lvl)
{
	int rc = 0;
	unsigned long mode_flags = 0;
	struct mipi_dsi_device *dsi = NULL;
	static u32 last_brightness[2] = {0, 0};
	static u32 last_fps[2] = {0, 0};
	u32 disp_id = 0;

	if (!panel || (bl_lvl > 0xffff)) {
		DSI_ERR("invalid params\n");
		return -EINVAL;
	}

	disp_id = !strcmp(panel->type, "primary") ? 0 : 1;

    pr_info("MSM_LCD %s panel backlight = %d\n", panel->type, bl_lvl);

	dsi = &panel->mipi_device;

    if (bl_lvl > 0) {
		if (panel->power_mode == SDE_MODE_DPMS_LP1 || panel->power_mode == SDE_MODE_DPMS_LP2) {
			pr_info("MSM_LCD %s panel backlight donot set when pm %d\n", panel->type, panel->power_mode);
			goto exit;
		}

		if (panel->disp_feature[ZTE_LCD_FPS_CTRL].mode != last_fps[disp_id]) {
            if (last_brightness[disp_id] == bl_lvl) {
                pr_info("MSM_LCD %s panel backlight donot set when panel fps change\n", panel->type);
			    goto exit;
            }
		}
	}

	if (unlikely(panel->bl_config.lp_mode)) {
		mode_flags = dsi->mode_flags;
		dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	}
    
	if (!disp_id && panel->bl_config.bl_inverted_dbv)
		bl_lvl = (((bl_lvl & 0xff) << 8) | (bl_lvl >> 8));

	rc = mipi_dsi_dcs_set_display_brightness(dsi, bl_lvl);
	if (rc < 0)
		DSI_ERR("failed to update dcs backlight:%d\n", bl_lvl);

	if (unlikely(panel->bl_config.lp_mode))
		dsi->mode_flags = mode_flags;

    if (!disp_id && panel->bl_config.bl_inverted_dbv)
		bl_lvl = (((bl_lvl & 0xff) << 8) | (bl_lvl >> 8));

	dsi_panel_dim_handle(panel, bl_lvl > 0, false);

exit:
    last_fps[disp_id] = panel->disp_feature[ZTE_LCD_FPS_CTRL].mode;
	last_brightness[disp_id] = bl_lvl;
    panel->cur_bl = bl_lvl;
	return rc;
}

void dsi_panel_dim_handle(struct dsi_panel *panel, bool en, bool skip_cmds) {
	if (!panel
		|| en == panel->last_dimen
		|| panel->power_mode == SDE_MODE_DPMS_LP1
		|| panel->power_mode == SDE_MODE_DPMS_LP2)
		return;

    panel->set_dim = en ? 1 : 0;
    if (!skip_cmds)
		queue_delayed_work(panel->dim_workq, &panel->dim_work, msecs_to_jiffies(20));
    panel->last_dimen = en;
}