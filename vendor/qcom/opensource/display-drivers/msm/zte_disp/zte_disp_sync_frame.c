#include "zte_disp_feature.h"
#include "../sde/sde_connector.h"
#include "../sde/sde_encoder.h"
#include "../sde/sde_encoder_phys.h"
#include "../sde/sde_trace.h"
#include "zte_disp_work.h"
#include "zte_disp_layer.h"
#include <drm/drm_vblank.h>

#define to_sde_encoder_phys_cmd(x) \
    	container_of(x, struct sde_encoder_phys_cmd, base)

void sync_te(struct sde_encoder_virt *sde_enc) {
    struct sde_connector *c_conn = NULL;
    struct dsi_display *display = NULL;
    struct dsi_panel * p = NULL;
    struct sde_encoder_phys *phys_encoder = NULL;
    struct sde_encoder_phys_cmd *cmd_enc = NULL;
    struct sde_encoder_phys_cmd_te_timestamp *te_timestamp;
    ktime_t te_ktime;
    s64 diff, delay;
    s64 us_per_frame;
    s64 vsync_area = 9000;
    u32 fps;

    if (!sde_enc || !sde_enc->cur_master || !sde_enc->cur_master->connector)
	    return;

    c_conn = to_sde_connector(sde_enc->cur_master->connector);
	if (!c_conn)
		return;

    phys_encoder = sde_enc->phys_encs[0];
    if (!phys_encoder)
        return;

    cmd_enc = to_sde_encoder_phys_cmd(phys_encoder);
    if (!cmd_enc)
        return;

    if (c_conn->connector_type != DRM_MODE_CONNECTOR_DSI)
        return;

    display = c_conn->display;

	if (!display || !display->panel)
		return;

    p = display->panel;

    fps = p->cur_mode->timing.refresh_rate;
    if (fps >= 120) {
        vsync_area = 200;
    }

    us_per_frame = 1000000 / fps;

    if (c_conn->encoder)
        sde_encoder_wait_for_event(c_conn->encoder, MSM_ENC_VBLANK);

    te_timestamp = list_last_entry(&cmd_enc->te_timestamp_list, struct sde_encoder_phys_cmd_te_timestamp, list);
    te_ktime = te_timestamp->timestamp;
    diff = ktime_to_us(ktime_sub(ktime_get(), te_ktime));
    /* away te signal "diff" us */
    diff = diff % us_per_frame;
    if (diff < vsync_area) {
        delay = vsync_area - diff;
    } else {
        delay = us_per_frame - diff + vsync_area;
    }
    usleep_range(delay, delay + 10);
}

void sde_feeds_hbm(struct sde_encoder_virt *sde_enc, bool en)
{
    static bool last_en;
    struct sde_connector *c_conn = NULL;
    struct dsi_display *display = NULL;
    struct dsi_panel * p = NULL;
    static u32 enter_fps = 60;

    if(last_en == en)
        return;

    last_en = en;

    if (!sde_enc || !sde_enc->cur_master || !sde_enc->cur_master->connector)
	    return;

    c_conn = to_sde_connector(sde_enc->cur_master->connector);
	if (!c_conn)
		return;

    if (c_conn->connector_type != DRM_MODE_CONNECTOR_DSI)
        return;

    display = c_conn->display;

	if (!display || !display->panel)
		return;

    p = display->panel;
    if (en) {
        enter_fps = p->cur_mode->timing.refresh_rate;
    } else {
        if (enter_fps != p->cur_mode->timing.refresh_rate) {
            /* need flush one frame to stay stable */
            if (c_conn->encoder)
                sde_encoder_wait_for_event(c_conn->encoder, MSM_ENC_VBLANK);
        }
    }
    SDE_ATRACE_BEGIN("sde_hbm");
    sync_te(sde_enc);
    zte_set_disp_parameter(p, ZTE_LCD_HBM_CTRL, en, false);
    SDE_ATRACE_END("sde_hbm");
}

void sde_make_fod_trigger(struct dsi_panel *panel, bool fod)
{
    static bool last_fod = false;

    if (fod == last_fod)
        return;

    if (fod) {
        if (!panel->hbm_trigger)
            return;
    }

    queue_delayed_work(panel->icon_workq, &panel->icon_delayed_work, 0);

    last_fod = fod;
}

void sde_check_layer_flush(struct sde_encoder_virt *sde_enc, u64 flag)
{
	struct sde_connector *c_conn = NULL;
    struct dsi_display *display = NULL;
    struct dsi_panel *p = NULL;
    static bool last_faod = false;
    bool faod;

    if (!sde_enc || !sde_enc->cur_master || !sde_enc->cur_master->connector)
	    return;

    c_conn = to_sde_connector(sde_enc->cur_master->connector);
	if (!c_conn)
		return;

    if (c_conn->connector_type != DRM_MODE_CONNECTOR_DSI)
        return;

    display = c_conn->display;

    if (!display || !display->panel)
        return;

    p = display->panel;

    faod = flag & ZTE_LAYER_AOD ? true : false;

    if (last_faod != faod) {
        report_aod_status(p, faod ? FRAME_AOD_ACTIVE : FRAME_AOD_MISS);
        last_faod = faod;
    }

    sde_make_fod_trigger(p, (flag & ZTE_LAYER_ICON) > 0);
}

void sde_feeds_syncbl(struct sde_encoder_virt *sde_enc, u64 flag)
{
	struct sde_connector *c_conn = NULL;
    struct dsi_display *display = NULL;
    struct dsi_panel * p = NULL;
    u32 bl = 0;
    u64 dark = 0;
    static u32 last_bl = 0;
    static u64 last_dark = 0;

    if (!sde_enc || !sde_enc->cur_master || !sde_enc->cur_master->connector)
	    return;

    c_conn = to_sde_connector(sde_enc->cur_master->connector);
	if (!c_conn)
		return;

    if (c_conn->connector_type != DRM_MODE_CONNECTOR_DSI)
        return;

    display = c_conn->display;

    if (!display || !display->panel)
        return;

    p = display->panel;

    bl = mult_frac(sde_connector_get_property(c_conn->base.state, CONNECTOR_PROP_BRIGHTNESS),
                    p->bl_config.bl_max_level,
			        p->bl_config.brightness_max_level);
    dark = sde_connector_get_property(c_conn->base.state, CONNECTOR_PROP_ZTE_HDR_RATIO);

    if (dark != last_dark) {
        last_dark = dark;
        //pr_err("MSM_LCD dark = %llu\n", dark);
        if (!dark) {
            sync_te(sde_enc);
            zte_set_disp_parameter(p, ZTE_LCD_SET_SYNC_BL, bl, false);
            return;
        }
    }

    if (panel_layer_contains_exhdr(flag) || panel_layer_contains_hdrvideo(flag)) {
        if (bl != last_bl) {
            last_bl = bl;
            if (bl > 0) {
                //pr_err("MSM_LCD drm_bl = %d\n", bl);
                SDE_ATRACE_BEGIN("sde_sync_bl");
                sync_te(sde_enc);
                zte_set_disp_parameter(p, ZTE_LCD_SET_SYNC_BL, bl, false);
                SDE_ATRACE_END("sde_sync_bl");
            }
        }
    }
}

int sde_connector_feed_cmds_sync_frame(void *sde_encoder_virt)
{
    struct sde_encoder_virt *sde_enc = sde_encoder_virt;
	struct sde_connector *c_conn = NULL;
    uint64_t flags = 0;

    if (!sde_enc || !sde_enc->cur_master || !sde_enc->cur_master->connector) {
		pr_err("Invalid sde_enc params\n");
		return -EINVAL;
	}

    c_conn = to_sde_connector(sde_enc->cur_master->connector);
	if (!c_conn) {
		pr_err("Invalid c_conn params\n");
		return -EINVAL;
	}

    if (c_conn->connector_type != DRM_MODE_CONNECTOR_DSI) {
		pr_err("not in dsi mode\n");
		return 0;
	}

    flags = sde_connector_get_property(c_conn->base.state, CONNECTOR_PROP_ZTE_LAYER);

    sde_feeds_hbm(sde_enc, flags & ZTE_LAYER_DIM ? true : false);

    sde_feeds_syncbl(sde_enc, flags);

    sde_check_layer_flush(sde_enc, flags);

    return 0;
}