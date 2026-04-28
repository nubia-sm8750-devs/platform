#include "../sde/sde_connector.h"
#include "../dsi/dsi_display.h"
#include "zte_disp_backlight.h"
#include "zte_disp_work.h"
#include "zte_disp_layer.h"

int zte_spec_layer_report(void *sde_connector, void *sde_connector_state, uint64_t flag)
{
    struct sde_connector *c_conn = sde_connector;
	struct sde_connector_state *c_state = sde_connector_state;
    struct dsi_display *display;
    struct dsi_panel *panel;

    if (!c_conn || !c_state)
       return -EINVAL;

    if (c_conn->connector_type != DRM_MODE_CONNECTOR_DSI)
       return 0;

    if (!c_conn->display)
       return 0;

    display = c_conn->display;

    if (!display->panel)
        return 0;

    panel = display->panel;

    if (panel->layer_flag != flag) {
        pr_info("MSM_LCD layer status [dim,icon,aod,ext_hdr,hdr,sensor] = [%llu,%llu,%llu,%llu,%llu,%llu]",
                                                                    flag & ZTE_LAYER_DIM,
                                                                    flag & ZTE_LAYER_ICON,
                                                                    flag & ZTE_LAYER_AOD,
                                                                    flag & ZTE_LAYER_EXTENDHDR,
                                                                    flag & ZTE_LAYER_HDRVIDEO,
                                                                    flag & ZTE_LAYER_SENSOR);
        panel->layer_flag = flag;
    }
    return 0;
}

/* Started by AICoder, pid:v09b4paf03v2353148ed0a65b0d2806ecba15a30 */
bool panel_layer_contains_exhdr(u64 flag)
{
	if (flag & ZTE_LAYER_EXTENDHDR)
		return true;

	return false;
}

bool panel_layer_contains_hdrvideo(u64 flag)
{
	if (flag & ZTE_LAYER_HDRVIDEO)
		return true;

	return false;
}
/* Ended by AICoder, pid:v09b4paf03v2353148ed0a65b0d2806ecba15a30 */