/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_backlight.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2023/08
** Author : Display
******************************************************************/
#include "zte_disp_panel.h"
#include "zte_lcd_reg_debug.h"
#include "zte_disp_panel_info.h"
#include "zte_disp_work.h"

LCD_PROC_FILE_DEFINE(zte_lcd_hbm, ZTE_LCD_HBM_CTRL)
LCD_PROC_FILE_DEFINE(zte_lcd_aod_bl, ZTE_LCD_AOD_BL)
LCD_PROC_FILE_DEFINE(zte_lcd_color_gamut, ZTE_LCD_COLOR_GAMUT_CTRL)
LCD_PROC_FILE_DEFINE(zte_lcd_acl, ZTE_LCD_ACL_CTRL)
LCD_PROC_FILE_DEFINE(zte_lcd_cur_fps, ZTE_LCD_FPS_CTRL)
LCD_PROC_FILE_DEFINE(zte_panel_state, ZTE_LCD_STATE_CTRL)
LCD_PROC_FILE_DEFINE(zte_lcd_bl_limit, ZTE_LCD_BL_LIMIT)

static void dsi_panel_parse_feature_config(struct dsi_panel *panel)
{
	struct dsi_parser_utils *utils = &panel->utils;
    zte_panel_state_init(panel);

    if (utils->read_bool(utils->data, "zte,hbm_enabled"))
        zte_lcd_hbm_init(panel);

    if (utils->read_bool(utils->data, "zte,aod_enabled"))
        zte_lcd_aod_bl_init(panel);

    if (utils->read_bool(utils->data, "zte,fps_enabled"))
        zte_lcd_cur_fps_init(panel);

    if (utils->read_bool(utils->data, "zte,color_space_enabled"))
        zte_lcd_color_gamut_init(panel);

    if (utils->read_bool(utils->data, "zte,acl_enabled"))
        zte_lcd_acl_init(panel);

    if (utils->read_bool(utils->data, "zte,bl_limit_enabled"))
        zte_lcd_bl_limit_init(panel);
}

void zte_disp_common_func(struct dsi_panel *panel)
{
    int id;

    if (!panel) {
        pr_info("MSM_LCD No panel device\n");
  		return;
    }

    panel->disp_feature = kcalloc(ZTE_LCD_MAX_CTRL, sizeof(struct zte_disp_feature), GFP_KERNEL);
    if (!panel->disp_feature) {
  		pr_err("%s: %d MSM_LCD kzalloc memory failed\n", __func__, __LINE__);
  		return;
  	}

    pr_info("MSM_LCD %s panel init\n", panel->type);
    for (id = 0; id < ZTE_LCD_MAX_CTRL; id++) {
        panel->disp_feature[id].fname = feature_name[id];
        panel->disp_feature[id].mode = 0;
        panel->disp_feature[id].panel_must_init = true;
        panel->disp_feature[id].writeable = true;
        panel->disp_feature[id].logable = true;

        if (id == ZTE_LCD_COLOR_GAMUT_CTRL){
            panel->disp_feature[id].panel_must_init = false;
        } else if (id == ZTE_LCD_FPS_CTRL) {
            panel->disp_feature[id].mode = 60;
            panel->disp_feature[id].writeable = false;
        } else if (id == ZTE_LCD_ACL_CTRL) {
            panel->disp_feature[id].panel_must_init = false;
        } else if (id == ZTE_LCD_AOD_BL) {
            panel->disp_feature[id].mode = 1;
            panel->disp_feature[id].panel_must_init = false;
        } else if (id == ZTE_LCD_STATE_CTRL) {
            panel->disp_feature[id].writeable = false;
        } else if (id == ZTE_LCD_BL_LIMIT) {
            panel->disp_feature[id].mode = panel->bl_config.brightness_max_level;
            panel->disp_feature[id].panel_must_init = false;
        } else if (id == ZTE_LCD_SET_SYNC_BL) {
            panel->disp_feature[id].logable = false;
        }
    }

    load_panel_info(panel);

    dsi_panel_parse_feature_config(panel);

    zte_lcd_reg_debug_func(panel);

    INIT_DELAYED_WORK(&panel->dim_work, dimming_work_handler);
    panel->dim_workq = create_singlethread_workqueue("panel_dim_workq");
}