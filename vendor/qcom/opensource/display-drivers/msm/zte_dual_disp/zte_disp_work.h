/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_backlight.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2023/08
** Author : Display
******************************************************************/
enum {
	MSG_FPS = 0,
	MSG_MAX,
};

void dimming_work_handler(struct work_struct *work);

void report_aod_status(struct dsi_panel *panel, u32 msg);

void setting_bl_work_handler(struct work_struct *work);

void setting_aodbl_work_handler(struct work_struct *work);

void zte_panel_send_uevent(int type, int mode, int ret);

