/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_backlight.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2024/06
** Author : Display
******************************************************************/
int zte_dsi_panel_update_backlight(struct dsi_panel *panel, u32 bl_lvl);
void dsi_panel_dim_handle(struct dsi_panel *panel, bool en, bool skip_cmds);
#ifdef CONFIG_DRM_ZTE_DISP_DUAL_PANEL
int zte_dsi_dual_panel_update_backlight(struct dsi_panel *panel,
	u32 bl_lvl);
#endif