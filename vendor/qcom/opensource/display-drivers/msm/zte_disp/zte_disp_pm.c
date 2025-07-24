#include "zte_disp_feature.h"
#include "zte_disp_pm.h"
#include "zte_disp_work.h"

void handle_panel_truly_status(struct dsi_panel *panel, int power_mode, bool skip_cmds)
{
    static bool aod = false;
    int rc = 0;

    if (skip_cmds) {
        aod = (power_mode != SDE_MODE_DPMS_ON);
    } else {
        switch(power_mode) {
            case SDE_MODE_DPMS_LP1:
                if (!aod) {
                    rc = dsi_panel_set_lp1(panel);
                    aod = !aod;
                }
                break;
            case SDE_MODE_DPMS_LP2:
                rc = dsi_panel_set_lp2(panel);
                break;
            case SDE_MODE_DPMS_ON:
                if (aod) {
                    rc = dsi_panel_set_nolp(panel);
                    aod = !aod;
                }
                break;
            default:
                aod = false;
                break;
        }

    }
}

int zte_dsi_display_set_power(struct drm_connector *connector,
		int power_mode, void *disp)
{
	struct dsi_display *display = disp;
    struct dsi_panel *panel;
	int rc = 0;

	if (!display || !display->panel) {
		DSI_ERR("invalid display/panel\n");
		return -EINVAL;
	}

    panel = display->panel;

    panel->disp_feature[ZTE_LCD_STATE_CTRL].mode = power_mode;

    handle_panel_truly_status(panel, power_mode, false);

    if (power_mode <= SDE_MODE_DPMS_LP2) {
        panel->power_mode = power_mode;
    } else if (power_mode == SDE_MODE_DPMS_OFF) {
        panel->disp_feature[ZTE_LCD_ACL_CTRL].mode = 0;
        panel->disp_feature[ZTE_LCD_DIM_CTRL].mode = 0;
        panel->disp_feature[ZTE_LCD_HBM_CTRL].mode = 0;
        panel->hbm_trigger = 0;
    }

    report_aod_status(panel,
         (power_mode == SDE_MODE_DPMS_LP1 || power_mode == SDE_MODE_DPMS_LP2) ? PM_AOD_ON : PM_AOD_OFF);

	return rc;
}
