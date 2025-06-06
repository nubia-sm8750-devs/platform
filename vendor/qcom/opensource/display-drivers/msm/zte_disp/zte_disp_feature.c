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
#include "zte_disp_pm.h"

void feed_panel_cmds(struct dsi_panel *panel, u32 feature, u32 mode)
{
    u32 power_mode = panel->power_mode;
    struct dsi_display_mode_priv_info *priv_info;
    struct dsi_cmd_desc *cmds = NULL;
    u8 *tx_buf = NULL;
    u32 count = 0;
    u32 cmd_base = 0;
    int rc = 0;
    int cmd_idx;

    if (feature != ZTE_LCD_HBM_CTRL && feature != ZTE_LCD_AOD_BL)
        return;

    if (panel->cur_mode)
        priv_info = panel->cur_mode->priv_info;
    else
        priv_info = NULL;

    if (feature == ZTE_LCD_AOD_BL) {
        if (power_mode == SDE_MODE_DPMS_ON)
            return;

        if (panel->disp_feature[ZTE_LCD_HBM_CTRL].mode)
            return;

        if ((panel->layer_flag & ZTE_LAYER_DIM) > 0)
            return;

        rc = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_LOW + mode);
    } else {
        cmd_base = (power_mode == SDE_MODE_DPMS_LP1 || power_mode == SDE_MODE_DPMS_LP2) ? DSI_CMD_SET_ZTE_HBM_OFF_AOD_ON : DSI_CMD_SET_ZTE_HBM_OFF;
        if (!mode) {
            if (priv_info) {
                count = priv_info->cmd_sets[cmd_base].count;
                cmds = priv_info->cmd_sets[cmd_base].cmds;
                if (cmds && count >= 1) {
                    tx_buf = (u8 *)cmds[count-1].msg.tx_buf;
                    if (tx_buf && tx_buf[0] == 0x51) {
                        if (cmd_base == DSI_CMD_SET_ZTE_HBM_OFF) {
                            tx_buf[1] = panel->cur_bl >> 8;
                            tx_buf[2] = panel->cur_bl & 0xff;
                        } else {
                            tx_buf[1] = 0;
                            tx_buf[2] = 0;
                        }
                        pr_info("[MSM_LCD] HBM OFF 0x%02X = 0x%02X 0x%02X\n",tx_buf[0], tx_buf[1], tx_buf[2]);
                    }
                }
            }
        }
        cmd_idx = cmd_base + mode;
        rc = zte_dsi_panel_tx_cmd_set(panel, cmd_idx);
        if (cmd_idx == DSI_CMD_SET_ZTE_HBM_OFF_AOD_ON)
            rc = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_LOW + panel->disp_feature[ZTE_LCD_AOD_BL].mode);

        if (cmd_idx == DSI_CMD_SET_ZTE_AOD_OFF_HBM_ON) {
            handle_panel_truly_status(panel, SDE_MODE_DPMS_ON, true);
        } else if(cmd_idx == DSI_CMD_SET_ZTE_HBM_OFF_AOD_ON) {
            handle_panel_truly_status(panel, SDE_MODE_DPMS_LP1, true);
        }
        if (!mode) {
            panel->hbm_off_timestamp = ktime_get();
        }
    }
}

int zte_set_disp_parameter(struct dsi_panel *panel, u32 feature, u32 feature_mode, bool from_node)
{
    int rc = 0;
    struct dsi_display_mode_priv_info *priv_info;
    struct dsi_cmd_desc *cmds = NULL;
    u8 *tx_buf;
    u32 count;
    static bool hbm_status = false;

    if (!panel) {
        pr_info("[MSM_LCD] No panel device\n");
        return -EINVAL;
    }

    if (panel->cur_mode)
        priv_info = panel->cur_mode->priv_info;
    else
        priv_info = NULL;

    if (!panel->disp_feature[feature].writeable) {
        pr_info("[MSM_LCD] %s is not writeable!\n", panel->disp_feature[feature].fname);
        return rc;
    }

    if (!panel->disp_feature[feature].panel_must_init)
        panel->disp_feature[feature].mode = feature_mode;

    if (panel->disp_feature[feature].logable)
        pr_info("[MSM_LCD] set [%s, %d]!\n", panel->disp_feature[feature].fname, feature_mode);

    mutex_lock(&panel->panel_lock);

    if (!panel->panel_initialized){
        pr_err("[MSM_LCD] skip when panel is not init!");
        goto exit;
    }

    switch(feature) {
        case ZTE_LCD_HBM_CTRL:
            if (hbm_status != feature_mode) {
                hbm_status = feature_mode;
                feed_panel_cmds(panel, ZTE_LCD_HBM_CTRL, feature_mode);
                if (from_node) {
                    panel->disp_feature[feature].mode = feature_mode;
                }
                panel->hbm_trigger = feature_mode;
                queue_delayed_work(panel->hbm_workq, &panel->hbm_delayed_work, 0);
            }
            break;
        case ZTE_LCD_ACL_CTRL:
            rc = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_ACL_OFF + feature_mode);
            break;
        case ZTE_LCD_AOD_BL:
            if (feature_mode <= 3)
                feed_panel_cmds(panel, ZTE_LCD_AOD_BL, feature_mode);
            break;
        case ZTE_LCD_BL_LIMIT:
            break;
        case ZTE_LCD_DIM_CTRL:
            if (feature_mode != panel->disp_feature[feature].mode) {
                rc = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_DIM_OFF + feature_mode);
                panel->disp_feature[feature].mode = feature_mode;
            }
            break;
        case ZTE_LCD_SET_BL:
        case ZTE_LCD_SET_SYNC_BL:
            if (panel->hbm_trigger)
                break;
            if (priv_info) {
                count = priv_info->cmd_sets[DSI_CMD_SET_ZTE_BL].count;
                cmds = priv_info->cmd_sets[DSI_CMD_SET_ZTE_BL].cmds;
                if (cmds && count > 1) {
                    tx_buf = (u8 *)cmds[count-1].msg.tx_buf;
                    if (tx_buf && tx_buf[0] == 0x51) {
                        tx_buf[1] = feature_mode >> 8;
                        tx_buf[2] = feature_mode & 0xff;
                        if (feature == ZTE_LCD_SET_BL)
                            pr_info("[MSM_LCD] DSI_CMD_SET_ZTE_BL 0x%02X = 0x%02X 0x%02X\n",tx_buf[0], tx_buf[1], tx_buf[2]);
                    }
                }
            }
            priv_info->cmd_sets[DSI_CMD_SET_ZTE_BL].logable = feature == ZTE_LCD_SET_BL ? true : false;
            rc = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_BL);
            break;
        default:
            break;
    }

exit:
    mutex_unlock(&panel->panel_lock);
    return rc;
}