/***************************************************************
** Copyright (C), 2023, ZTE Mobile Comm Corp., Ltd
**
** File : zte_panel_backlight.c
** Description : ZTE display panel
** Version : 1.0
** Date : 2023/08
** Author : Display
******************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/kobject.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include "zte_disp_feature.h"

char *node_name[2][ZTE_LCD_MAX_CTRL] = {
	{
		"driver/lcd_hbm",
		"driver/lcd_color_gamut",
		"driver/lcd_fps",
		"driver/lcd_acl",
		"driver/lcd_aod_bl",
		"driver/lcd_state",
		"driver/lcd_bl_limit",
		"skip",
		"skip",
		"skip"
	},
	{
		"driver/sec_lcd_hbm",
		"driver/sec_lcd_color_gamut",
		"driver/sec_lcd_fps",
		"driver/sec_lcd_acl",
		"driver/sec_lcd_aod_bl",
		"driver/sec_lcd_state",
		"driver/sec_lcd_bl_limit",
		"skip",
		"skip",
		"skip"
	}
};

char *feature_name[ZTE_LCD_MAX_CTRL] = {
	"lcd_hbm",
	"lcd_color_gamut",
	"lcd_fps",
	"lcd_acl",
	"lcd_aod_bl",
    "lcd_state",
	"lcd_bl_limit",
    "lcd_dim",
	"lcd_bl",
	"sync_bl",
};

#define LCD_PROC_FILE_DEFINE(name, nodeid) \
static ssize_t name##_proc_write(struct file *file, \
		const char __user *buffer, size_t count, loff_t *ppos) \
{ \
	char *tmp = kzalloc((count+1), GFP_KERNEL); \
	u32 mode; \
	struct dsi_panel *panel =  pde_data(file_inode(file));\
	if (!tmp) \
		return -ENOMEM; \
	if (copy_from_user(tmp, buffer, count)) { \
		kfree(tmp); \
		return -EFAULT; \
	} \
    if (nodeid == ZTE_LCD_BL_LIMIT) { \
		sscanf(tmp, "%d", &mode); \
	} else { \
		mode = *tmp - '0'; \
	} \
	if (!panel) {\
		DSI_ERR("MSM_LCD no primary display device\n");\
		return -ENODEV;\
	}\
	zte_set_disp_parameter(panel, nodeid, mode, true); \
	kfree(tmp); \
	return count; \
} \
static int name##_show(struct seq_file *m, void *v) \
{ \
    struct dsi_panel *panel = (struct dsi_panel *)m->private;\
	if (!panel) {\
		pr_err("No panel device\n");\
		return -ENODEV;\
	}\
    seq_printf(m, "%d\n", panel->disp_feature[nodeid].mode); \
	return 0; \
} \
static int name##_proc_open(struct inode *inode, struct file *file) \
{ \
	return single_open(file, name##_show, pde_data(inode)); \
} \
static const struct proc_ops name##_proc_fops = { \
  	.proc_open	= name##_proc_open, \
  	.proc_read	= seq_read, \
  	.proc_lseek	= seq_lseek, \
 	.proc_release	= single_release, \
	.proc_write	= name##_proc_write, \
}; \
static void name##_init(void *panel) \
{ \
    struct dsi_panel *p = (struct dsi_panel *)panel;\
	pr_info("MSM_LCD %s create node %s\n", p->type, node_name[!strcmp(p->type, "primary") ? 0 : 1][nodeid]);\
	proc_create_data(node_name[!strcmp(p->type, "primary") ? 0 : 1][nodeid], 0664, NULL, & name##_proc_fops, panel); \
	return; \
}

