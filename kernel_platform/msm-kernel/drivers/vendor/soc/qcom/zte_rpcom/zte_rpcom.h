// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * writen by ZTE boot team, 20240124.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */




void rpcom_power_on(int on_or_off); 

void rpcom_rf_sw(int on_or_off);

void rpcom_fw_download(int on_or_off);

void rpcom_uim0_poweron(int on_or_off);

void rpcom_uim1_poweron(int on_or_off);


static int rpcom_config_regulator_uimpower(struct platform_device *pdev);

static int rpcom_set_regulator_uimpower(struct platform_device *dev, u8 level);


static ssize_t tt_download_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size);
static ssize_t tt_download_show(struct device *dev, struct device_attribute *attr, char *buff);
static DEVICE_ATTR(tt_download, S_IRUGO | S_IWUSR, tt_download_show,  tt_download_store);


static ssize_t tt_poweron_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size);
static ssize_t tt_poweron_show(struct device *dev, struct device_attribute *attr, char *buff);
static DEVICE_ATTR(tt_poweron, S_IRUGO | S_IWUSR, tt_poweron_show, tt_poweron_store);


static ssize_t reset_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size);
static DEVICE_ATTR(tt_reset, S_IRUGO | S_IWUSR, NULL, reset_store);

static ssize_t tt_uim0_sw_on_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size);
static ssize_t tt_uim0_sw_on_show(struct device *dev, struct device_attribute *attr, char *buff);
static DEVICE_ATTR(tt_uim0_sw_on, S_IRUGO | S_IWUSR, tt_uim0_sw_on_show, tt_uim0_sw_on_store);


static ssize_t tt_uim1_sw_on_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size);
static ssize_t tt_uim1_sw_on_show(struct device *dev, struct device_attribute *attr, char *buff);
static DEVICE_ATTR(tt_uim1_sw_on, S_IRUGO | S_IWUSR, tt_uim1_sw_on_show, tt_uim1_sw_on_store);


static ssize_t tt_rf_sw_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size);
static ssize_t tt_rf_sw_show(struct device *dev, struct device_attribute *attr, char *buff);
static DEVICE_ATTR(tt_rf_sw, S_IRUGO | S_IWUSR, tt_rf_sw_show,  tt_rf_sw_store);


static ssize_t uim_power_level_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size);
static ssize_t uim_power_level_show(struct device *dev, struct device_attribute *attr, char *buff);
static DEVICE_ATTR(uim_power_level, S_IRUGO | S_IWUSR, uim_power_level_show,  uim_power_level_store);


static ssize_t uim_hotplug_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size);
static DEVICE_ATTR(uim_hotplug, S_IRUGO | S_IWUSR, NULL, uim_hotplug_store);


static struct attribute  *rpcom_attrs[] = {
	&dev_attr_tt_poweron.attr,
	&dev_attr_tt_reset.attr,
	&dev_attr_tt_download.attr,
	&dev_attr_tt_uim0_sw_on.attr,
	&dev_attr_tt_uim1_sw_on.attr,
	&dev_attr_tt_rf_sw.attr,
	&dev_attr_uim_power_level.attr,
	&dev_attr_uim_hotplug.attr,
	NULL,
};

static struct attribute_group rpmcom_attr_grp = {
	.name = "zte-rpcom",
	.attrs = rpcom_attrs
};


static int rpcom_populate_dt_pinfo(struct platform_device *pdev);



