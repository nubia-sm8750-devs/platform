/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_backlight.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2023/08
** Author : Display
******************************************************************/
#include "zte_disp_panel_info.h"
#include <linux/printk.h>

int lcd_info_proc_show(struct seq_file *m, void *v)
{
    struct dsi_panel *panel = (struct dsi_panel *)m->private;
	if (!panel) {
		pr_err("MSM_LCD No panel device\n");\
		return -ENODEV;
	}
    seq_printf(m, "%s\n", panel->name);
	return 0;
}

int lcd_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, lcd_info_proc_show, pde_data(inode));
}

const struct proc_ops lcd_info_proc_fops= {
  	.proc_open	= lcd_info_proc_open,
  	.proc_read    = seq_read,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release,
};

void load_panel_info(void *panel) {
    struct dsi_panel *p = (struct dsi_panel *)panel;
    pr_info("MSM_LCD create %s lcd_id node\n", p->type);
	proc_create_data(!strcmp(p->type, "primary") ? "driver/lcd_id" : "driver/sec_lcd_id", 0664, NULL, &lcd_info_proc_fops, panel);
}