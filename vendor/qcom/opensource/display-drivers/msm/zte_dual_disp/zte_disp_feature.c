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

int zte_set_disp_parameter(struct dsi_panel *panel, u32 feature, u32 feature_mode, bool from_node)
{
    int rc = 0;
    struct dsi_display_mode_priv_info *priv_info;
    struct dsi_cmd_desc *cmds = NULL;
    u8 *tx_buf;
    u32 count;

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
    pr_info("[MSM_LCD] set feature %d!\n", feature);
    switch(feature) {
        case ZTE_LCD_ACL_CTRL:
            rc = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_ACL_OFF + feature_mode);
            break;
        case ZTE_LCD_AOD_BL:
            if (panel->power_mode == SDE_MODE_DPMS_LP1 || panel->power_mode == SDE_MODE_DPMS_LP2) {
                rc = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_LOW + feature_mode);
            } else {
                pr_info("[MSM_LCD] skip aod bl in non-aod mode!");
            }
            break;
        case ZTE_LCD_DIM_CTRL:
            if (feature_mode != panel->disp_feature[feature].mode) {
                rc = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_DIM_OFF + feature_mode);
                panel->disp_feature[feature].mode = feature_mode;
            }
            break;
        case ZTE_LCD_SET_BL:
            if (priv_info) {
                count = priv_info->cmd_sets[DSI_CMD_SET_ZTE_BL].count;
                cmds = priv_info->cmd_sets[DSI_CMD_SET_ZTE_BL].cmds;
                if (cmds && count >= 1) {
                    tx_buf = (u8 *)cmds[count-1].msg.tx_buf;
                    if (tx_buf && tx_buf[0] == 0x51) {
                        if (!strcmp(panel->type, "primary")) {
                            tx_buf[1] = feature_mode >> 8;
                            tx_buf[2] = feature_mode & 0xff;
                            pr_info("[MSM_LCD] DSI_CMD_SET_ZTE_BL 0x%02X = 0x%02X 0x%02X\n",tx_buf[0], tx_buf[1], tx_buf[2]);
                        } else {
                            tx_buf[1] = feature_mode & 0xff;
                            pr_info("[MSM_LCD] DSI_CMD_SET_ZTE_BL 0x%02X = 0x%02X\n",tx_buf[0], tx_buf[1]);
                        } 
                    }
                }  
            }
            rc = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_BL);
            break;
        default:
            break;
    }

exit:
    mutex_unlock(&panel->panel_lock);
    return rc;
}