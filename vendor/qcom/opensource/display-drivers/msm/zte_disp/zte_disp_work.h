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
    FRAME_AOD_ACTIVE = 0,
    FRAME_AOD_MISS,
    PM_AOD_ON,
    PM_AOD_OFF,
};

enum {
	MSG_HBM = 0,
	MSG_FOD = 2,
	MSG_FPS = 6,
};

enum rq_type {
  	RD_PTR = 0,
  	PP_DONE = 1,
};

void dimming_work_handler(struct work_struct *work);

void report_aod_status(struct dsi_panel *panel, u32 msg);

void setting_bl_work_handler(struct work_struct *work);

void setting_aodbl_work_handler(struct work_struct *work);

void panel_hbm_work_handler(struct work_struct *work);

void panel_icon_work_handler(struct work_struct *work);

void zte_panel_send_uevent(int type, int mode, int ret);

void sde_irq_triger(void *sde_encoder_phys, unsigned int irq_type);
