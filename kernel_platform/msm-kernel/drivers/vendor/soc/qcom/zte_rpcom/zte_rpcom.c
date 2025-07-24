// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * writen by ZTE boot team, 20240124.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include "zte_rpcom.h"

/* add for rpcom gpios conctrl*/
struct rpcom_platform_data {
	struct platform_device *pdev;
	struct regulator *uim_power;
	u32 vol_1p8_uv;
	u32 vol_3p0_uv;
	int tt_poweron;
	int tt_power_state;
	int tt_reset;
	int tt_download;
	int tt_download_state;
	int uim0_qc_on;
	int uim0_sw;
	int uim0_sw_state;
	int uim1_sw;
	int uim1_qc_on;
	int uim1_sw_state;
	int tt_rf_sw;
	int tt_rf_sw_state;
	int uim_power_level_state;
	int uim_hotplug;
	int tt_lna_3v3_en;
	int tt_trx_1p8_en;
};

static struct rpcom_platform_data *rpcom_pdata;

/*for debug as soon as possibile, set it to 0 while driver debug finished*/
static int power_on_during_boot = 0;

void rpcom_power_on(int on_or_off) 
{
	int on = on_or_off != 0 ? 1 : 0;

	if (on) {
		/* poweron for rpcom chip*/
		gpio_direction_output(rpcom_pdata->tt_reset, 0);
		msleep(20);
		gpio_direction_output(rpcom_pdata->tt_poweron, 1);
		gpio_set_value_cansleep(rpcom_pdata->tt_poweron, 1);
		msleep(220);
		gpio_direction_output(rpcom_pdata->tt_reset, 1);
		gpio_set_value_cansleep(rpcom_pdata->tt_reset, 1);
		msleep(20);
		pr_info("%s: power on or off ? on\n", __func__);
	} else {
		/* directly poweroff is not recommended*/
		/* rpcom suggest shutdown by AT command*/
		/* poweroff for rpcom chip*/
		gpio_direction_output(rpcom_pdata->tt_reset, 0);
		gpio_direction_output(rpcom_pdata->tt_poweron, 0);
		pr_info("%s: power on or off ? off\n", __func__);
	}
	rpcom_pdata->tt_power_state = gpio_get_value(rpcom_pdata->tt_poweron);
	pr_info("%s: status, tt_poweron = %d, tt_reset = %d\n",
		__func__, gpio_get_value(rpcom_pdata->tt_poweron), gpio_get_value(rpcom_pdata->tt_reset));
}

void rpcom_rf_sw(int on_or_off) {
	int on = on_or_off != 0 ? 1 : 0;

	if (on) {
		/* poweron for rpcom rf*/
		/* rf switch to rpcom tiantong*/
		gpio_direction_output(rpcom_pdata->tt_rf_sw, 1);
		gpio_set_value_cansleep(rpcom_pdata->tt_rf_sw, 1);
		gpio_direction_output(rpcom_pdata->tt_lna_3v3_en, 1);
		gpio_set_value_cansleep(rpcom_pdata->tt_lna_3v3_en, 1);
		gpio_direction_output(rpcom_pdata->tt_trx_1p8_en, 1);
		gpio_set_value_cansleep(rpcom_pdata->tt_trx_1p8_en, 1);
		pr_info("%s: tt_rf_sw to qcom or rpcom? rpcom\n", __func__);
	} else {
		/* poweroff for rpcom rf*/
		/* switch the power supply to qcom modem*/
		gpio_direction_output(rpcom_pdata->tt_rf_sw, 0);
		gpio_set_value_cansleep(rpcom_pdata->tt_rf_sw, 0);
		gpio_direction_output(rpcom_pdata->tt_lna_3v3_en, 0);
		gpio_set_value_cansleep(rpcom_pdata->tt_lna_3v3_en, 0);
		gpio_direction_output(rpcom_pdata->tt_trx_1p8_en, 0);
		gpio_set_value_cansleep(rpcom_pdata->tt_trx_1p8_en, 0);
		pr_info("%s: tt_rf_sw to qcom or rpcom? qcom\n", __func__);
	}
	rpcom_pdata->tt_rf_sw_state = gpio_get_value(rpcom_pdata->tt_rf_sw);
	pr_info("%s: status, tt_rf_sw_state = %d\n", __func__, rpcom_pdata->tt_rf_sw_state);
}

void rpcom_fw_download(int on_or_off) {
	int on = on_or_off != 0 ? 1 : 0;

	if (on) {
		gpio_direction_output(rpcom_pdata->tt_download, 1);
		gpio_set_value_cansleep(rpcom_pdata->tt_download, 1);
		pr_info("%s: enter rpcom_fw_download mode...", __func__);
	} else {
		gpio_direction_output(rpcom_pdata->tt_download, 0);
		pr_info("%s: exit rpcom_fw_download mode...", __func__);
	}
	rpcom_pdata->tt_download_state = gpio_get_value(rpcom_pdata->tt_download);
	pr_info("%s: status, tt_download = %d\n",
		__func__, rpcom_pdata->tt_download_state);
}

void rpcom_uim0_poweron(int on_or_off)
{
	int on = on_or_off != 0 ? 1 : 0;
	pr_info("%s: uim0 power on or off ? %d\n", __func__, on);

	if (on) {
		/* uim0 switch to rpcom */
		pr_info("uim0 state is on!\n");
		gpio_direction_output(rpcom_pdata->uim0_qc_on, 0);
		gpio_direction_output(rpcom_pdata->uim0_sw, 1);
		gpio_set_value_cansleep(rpcom_pdata->uim0_sw, 1);
		rpcom_pdata->uim0_sw_state = 1;
		pr_info("%s: uim0 switch to rpcom, uim0_sw_state = %d\n", __func__, rpcom_pdata->uim0_sw_state);
	} else {
		/* uim0 switch to qcom modem */
		gpio_direction_output(rpcom_pdata->uim0_qc_on, 1);
		gpio_set_value_cansleep(rpcom_pdata->uim0_qc_on, 1);
		gpio_direction_output(rpcom_pdata->uim0_sw, 0);
		rpcom_pdata->uim0_sw_state = 0;
		pr_info("%s: uim0 switch to qcom modem, uim0_sw_state = %d\n", __func__, rpcom_pdata->uim0_sw_state);
	}
}

void rpcom_uim1_poweron(int on_or_off)
{
	int on = on_or_off != 0 ? 1 : 0;
	pr_info("%s: uim1 power on or off ? %d\n", __func__, on);

	if (on) {
		/* uim1 switch to rpcom */
		pr_info("uim1 state is on!\n");
		gpio_direction_output(rpcom_pdata->uim1_qc_on, 0);
		gpio_direction_output(rpcom_pdata->uim1_sw, 1);
		gpio_set_value_cansleep(rpcom_pdata->uim1_sw, 1);
		rpcom_pdata->uim1_sw_state = 1;
		pr_info("%s: uim1 switch to rpcom, uim1_sw_state = %d\n", __func__, rpcom_pdata->uim1_sw_state);
	} else {
		/* uim1 switch to qcom modem */
		gpio_direction_output(rpcom_pdata->uim1_qc_on, 1);
		gpio_set_value_cansleep(rpcom_pdata->uim1_qc_on, 1);
		gpio_direction_output(rpcom_pdata->uim1_sw, 0);
		rpcom_pdata->uim1_sw_state = 0;
		pr_info("%s: uim1 switch to qcom modem, uim1_sw_state = %d\n", __func__, rpcom_pdata->uim1_sw_state);
	}
}


static int rpcom_config_regulator_uimpower(struct platform_device *pdev)
{
	int rc = 0;

	rpcom_pdata->uim_power = devm_regulator_get(&pdev->dev, "uim_power");
	if (IS_ERR(rpcom_pdata->uim_power)) {
		rc = PTR_ERR(rpcom_pdata->uim_power);
		pr_err("%s:Regulator uim_power get failed rc=%d\n",
			__func__, rc);
		rpcom_pdata->uim_power = NULL;
		return rc;
	}
	return rc;
}


static int rpcom_set_regulator_uimpower(struct platform_device *dev, u8 level)
{
	int rc = 0;

	switch (level) {
		case (1):
			if (regulator_count_voltages(rpcom_pdata->uim_power) > 0) {
				rc = regulator_set_voltage(rpcom_pdata->uim_power,
					rpcom_pdata->vol_1p8_uv, rpcom_pdata->vol_1p8_uv);
				if (rc) {
					pr_err("%s:failed to set voltage uim_power to %u!\n",
							__func__, rpcom_pdata->vol_1p8_uv);
					goto deinit_vregs;
				}
			}
			break;
		case (2):
			if (regulator_count_voltages(rpcom_pdata->uim_power) > 0) {
				rc = regulator_set_voltage(rpcom_pdata->uim_power,
					rpcom_pdata->vol_3p0_uv, rpcom_pdata->vol_3p0_uv);
				if (rc) {
					pr_err("%s:failed to set voltage uim_power to %u!\n",
							__func__, rpcom_pdata->vol_3p0_uv);
					goto deinit_vregs;
				}
			}
			break;
		default:
			break;
	}

	if (level) {
		if (!IS_ERR_OR_NULL(rpcom_pdata->uim_power)) {
			rc = regulator_enable(rpcom_pdata->uim_power);
			if (rc) {
				pr_err("%s:Enable regulator uim_power failed rc=%d\n", __func__, rc);
				goto deinit_vregs;
			}
		}
		return rc;
	}

	if (!IS_ERR_OR_NULL(rpcom_pdata->uim_power)) {
		rc = regulator_disable(rpcom_pdata->uim_power);
		if (rc) {
			pr_err("%s:Disable regulator uim_power failed rc=%d\n", __func__, rc);
		}
	}
	return 0;

deinit_vregs:
	regulator_set_voltage(rpcom_pdata->uim_power, 0, 0);
	if (!IS_ERR_OR_NULL(rpcom_pdata->uim_power))
		regulator_disable(rpcom_pdata->uim_power);
	return rc;
}



static ssize_t tt_download_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int dload = 0;

	if (sscanf(buff, "%u", &dload) !=1)
		return -EINVAL;
	rpcom_fw_download(dload); 
	return strnlen(buff, size);
}

static ssize_t tt_download_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "%d\n", rpcom_pdata->tt_download_state);
}


static ssize_t tt_poweron_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int on = 0;

	sscanf(buff, "%u", &on);
	rpcom_power_on(on);

	return strnlen(buff, size);
}

static ssize_t tt_poweron_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "%d\n", rpcom_pdata->tt_power_state);
}


static ssize_t reset_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int reset = 0;
	sscanf(buff, "%u", &reset);
	if (reset) {
		pr_info("rpcom reset ...!\n");
		gpio_set_value_cansleep(rpcom_pdata->tt_reset, 0);
		msleep(220);
		gpio_set_value_cansleep(rpcom_pdata->tt_reset, 1);
	} else {
		pr_info("nothing to be done!\n");
	}
	pr_info("%s: status, tt_reset=%d\n", __func__, gpio_get_value(rpcom_pdata->tt_reset));
	return strnlen(buff, size);
}


static ssize_t tt_uim0_sw_on_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int on = 0;

	if (sscanf(buff, "%u", &on) !=1)
		return -EINVAL;

	rpcom_uim0_poweron(on);
	rpcom_pdata->uim0_sw_state = on !=0 ? 1 : 0;
	return strnlen(buff, size);
}

static ssize_t tt_uim0_sw_on_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "%d\n", rpcom_pdata->uim0_sw_state);
}


static ssize_t tt_uim1_sw_on_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int on = 0;

	if (sscanf(buff, "%u", &on) !=1)
		return -EINVAL;

	rpcom_uim1_poweron(on);
	rpcom_pdata->uim1_sw_state = on !=0 ? 1 : 0;
	return strnlen(buff, size);
}

static ssize_t tt_uim1_sw_on_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "%d\n", rpcom_pdata->uim1_sw_state);
}


static ssize_t tt_rf_sw_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int sw = 0;

	if (sscanf(buff, "%u", &sw) !=1)
		return -EINVAL;

	rpcom_rf_sw(sw);

	return strnlen(buff, size);
}

static ssize_t tt_rf_sw_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "%d\n", rpcom_pdata->tt_rf_sw_state);
}


static ssize_t uim_power_level_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int level = 0;
	int ret = 0;

	if (sscanf(buff, "%u", &level) !=1)
		return -EINVAL;

	ret = rpcom_set_regulator_uimpower(rpcom_pdata->pdev, level);
	if (ret) {
		pr_err("rpcom: failed to enable regulator uimpower!!\n");
		rpcom_pdata->uim_power_level_state = 0;
		return -EINVAL;
	}
	pr_err("rpcom: succeed to enable regulator uimpower!!\n");
	rpcom_pdata->uim_power_level_state = level;

	return strnlen(buff, size);
}

static ssize_t uim_power_level_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "%d\n", rpcom_pdata->uim_power_level_state);
}


static ssize_t uim_hotplug_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int uim_hotplug_sts = 0;
	sscanf(buff, "%u", &uim_hotplug_sts);
	if (uim_hotplug_sts) {
		pr_info("rpcom uim_hotplug, prepare to switch to qcom modem...!\n");
		gpio_direction_output(rpcom_pdata->uim_hotplug, 1);
		gpio_set_value_cansleep(rpcom_pdata->uim_hotplug, 1);
		msleep(80);
		gpio_set_value_cansleep(rpcom_pdata->uim_hotplug, 0);
	} else {
		pr_info("nothing to be done!\n");
	}
	return strnlen(buff, size);
}


static int rpcom_populate_dt_pinfo(struct platform_device *pdev)
{
	int rc = 0;
	u32 tempval;

	pr_info("%s\n", __func__);
	if (!rpcom_pdata) {
		pr_info("%s rpcom_pdata is null \n", __func__);
		return -ENOMEM;
	}
	if (!pdev->dev.of_node) {
		pr_err("%s: dev.of_node is null\n",__func__);
		return -EINVAL;
	}

	rpcom_pdata->tt_poweron =
		of_get_named_gpio(pdev->dev.of_node,
			"tt_poweron", 0);
	pr_info("%s: tt_poweron = %d\n", __func__, rpcom_pdata->tt_poweron);
	if (gpio_is_valid(rpcom_pdata->tt_poweron)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->tt_poweron, "tt_poweron")) {
			pr_err("failed to request tt_poweron gpio!\n");
			return -EINVAL;
		}
	}

	rpcom_pdata->tt_reset =
		of_get_named_gpio(pdev->dev.of_node,
			"tt_reset", 0);
	pr_info("%s: tt_reset = %d\n", __func__, rpcom_pdata->tt_reset);
	if (gpio_is_valid(rpcom_pdata->tt_reset)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->tt_reset, "tt_reset")) {
			pr_err("failed to request tt_reset gpio!\n");
			return -EINVAL;
		}
	}

	rpcom_pdata->tt_download =
		of_get_named_gpio(pdev->dev.of_node,
			"tt_download", 0);
	pr_info("%s: tt_download = %d\n", __func__, rpcom_pdata->tt_download);
	if (gpio_is_valid(rpcom_pdata->tt_download)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->tt_download, "tt_download")) {
			pr_err("failed to request tt_download gpio!\n");
			return -EINVAL;
		}
	}

	rpcom_pdata->uim0_qc_on =
		of_get_named_gpio(pdev->dev.of_node,
			"uim0_qc_on", 0);
	pr_info("%s: uim0_qc_on = %d\n", __func__, rpcom_pdata->uim0_qc_on);
	if (gpio_is_valid(rpcom_pdata->uim0_qc_on)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->uim0_qc_on, "uim0_qc_on")) {
			pr_err("failed to request uim0_qc_on gpio!\n");
			return -EINVAL;
		}
	}

	rpcom_pdata->uim1_qc_on =
		of_get_named_gpio(pdev->dev.of_node,
			"uim1_qc_on", 0);
	pr_info("%s: uim1_qc_on = %d\n", __func__, rpcom_pdata->uim1_qc_on);
	if (gpio_is_valid(rpcom_pdata->uim1_qc_on)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->uim1_qc_on, "uim1_qc_on")) {
			pr_err("failed to request uim1_qc_on gpio!\n");
			return -EINVAL;
		}
	}

	rpcom_pdata->uim0_sw =
		of_get_named_gpio(pdev->dev.of_node,
			"uim0_sw", 0);
	pr_info("%s: uim0_sw = %d\n", __func__, rpcom_pdata->uim0_sw);
	if (gpio_is_valid(rpcom_pdata->uim0_sw)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->uim0_sw, "uim0_sw")) {
			pr_err("failed to request uim0_sw gpio!\n");
			return -EINVAL;
		}
	}

	rpcom_pdata->uim1_sw =
		of_get_named_gpio(pdev->dev.of_node,
			"uim1_sw", 0);
	pr_info("%s: uim1_sw = %d\n", __func__, rpcom_pdata->uim1_sw);
	if (gpio_is_valid(rpcom_pdata->uim1_sw)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->uim1_sw, "uim1_sw")) {
			pr_err("failed to request uim1_sw gpio!\n");
			return -EINVAL;
		}
	}

	rpcom_pdata->tt_rf_sw =
		of_get_named_gpio(pdev->dev.of_node,
			"tt_rf_sw", 0);
	pr_info("%s: tt_rf_sw = %d\n", __func__, rpcom_pdata->tt_rf_sw);
	if (gpio_is_valid(rpcom_pdata->tt_rf_sw)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->tt_rf_sw, "tt_rf_sw")) {
			pr_err("failed to request tt_rf_sw gpio!\n");
			return -EINVAL;
		}
	}

	rpcom_pdata->uim_hotplug =
		of_get_named_gpio(pdev->dev.of_node,
			"uim_hotplug", 0);
	pr_info("%s: uim_hotplug = %d\n", __func__, rpcom_pdata->uim_hotplug);
	if (gpio_is_valid(rpcom_pdata->uim_hotplug)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->uim_hotplug, "uim_hotplug")) {
			pr_err("failed to request uim_hotplug gpio!\n");
			return -EINVAL;
		}
	}

	rpcom_pdata->tt_lna_3v3_en =
		of_get_named_gpio(pdev->dev.of_node,
			"tt_lna_3v3_en", 0);
	pr_info("%s: tt_lna_3v3_en = %d\n", __func__, rpcom_pdata->tt_lna_3v3_en);
	if (gpio_is_valid(rpcom_pdata->tt_lna_3v3_en)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->tt_lna_3v3_en, "tt_lna_3v3_en")) {
			pr_err("failed to request tt_lna_3v3_en gpio!\n");
			return -EINVAL;
		}
	}

	rpcom_pdata->tt_trx_1p8_en =
		of_get_named_gpio(pdev->dev.of_node,
			"tt_trx_1p8_en", 0);
	pr_info("%s: tt_trx_1p8_en = %d\n", __func__, rpcom_pdata->tt_trx_1p8_en);
	if (gpio_is_valid(rpcom_pdata->tt_trx_1p8_en)) {
		if (devm_gpio_request(&pdev->dev, rpcom_pdata->tt_trx_1p8_en, "tt_trx_1p8_en")) {
			pr_err("failed to request tt_trx_1p8_en gpio!\n");
			return -EINVAL;
		}
	}

	rc = of_property_read_u32(pdev->dev.of_node, "linux,3p0-uv", &tempval);
	if (rc) {
		pr_err("%s:failed to read linux,3p0-uv!\n", __func__);
	}
	rpcom_pdata->vol_3p0_uv = tempval;

	rc = of_property_read_u32(pdev->dev.of_node, "linux,1p8-uv", &tempval);
	if (rc) {
		pr_err("%s:failed to read linux,1p8-uv!\n", __func__);
	}
	rpcom_pdata->vol_1p8_uv = tempval;

	return 0;
}


static const struct of_device_id zte_rpcom_of_match[] = {
	{ .compatible = "zte-rpcom", },
	{ },
};

MODULE_DEVICE_TABLE(of, zte_rpcom_of_match);

static int zte_rpcom_probe(struct platform_device *pdev)
{
	int ret = 0;
	pr_info("%s into!\n", __func__);

	rpcom_pdata = kzalloc(sizeof(*rpcom_pdata), GFP_KERNEL);
	if (!rpcom_pdata) {
		pr_info("%s rpcom_pdata is null \n", __func__);
		return -ENOMEM;
	}

	rpcom_pdata->pdev = pdev;

	ret = rpcom_populate_dt_pinfo(pdev);
	if (ret) {
		pr_err("%s:failed to populate device tree info: ret = %d\n", __func__, ret);
		goto out;
	}


	ret = rpcom_config_regulator_uimpower(pdev);
	if (ret) {
		pr_err("%s:failed to config regulator uimpower!! ret = %d\n", __func__, ret);
		goto out;
	}

	pdev->dev.platform_data = rpcom_pdata;

	ret = sysfs_create_group(&pdev->dev.kobj, &rpmcom_attr_grp);
	if (ret) {
		pr_err("%s:failed to create the rpcom attr group, %d\n", __func__, ret);
		goto out;
	}

	/* for debug */
	if (power_on_during_boot) {
		pr_info("%s rpcom tiantong default state = %d\n", __func__, power_on_during_boot);
		/* poweron for rpcom chip */
		rpcom_power_on(1);
		/* switch to rpcom rf */
		rpcom_rf_sw(1);
	} else {
		pr_info("%s rpcom tiantong default state = %d\n", __func__, power_on_during_boot);
		/* switch to qcom rf, default state */
		rpcom_rf_sw(0);
		/* poweroff for rpcom chip */
		rpcom_power_on(0);
	}
	
	rpcom_uim0_poweron(0);
	rpcom_uim1_poweron(0);

	pr_info("%s end\n", __func__);
	return 0;

out:
	kfree((void *)rpcom_pdata);
	return ret;
}

static int  zte_rpcom_remove(struct platform_device *pdev)
{
	return 0;
}


static struct platform_driver zte_rpcom_driver = {
	.probe			= zte_rpcom_probe,
	.remove			= zte_rpcom_remove,
	.driver			= {
		.name	= "zte-rpcom",
		.owner	= THIS_MODULE,
		.of_match_table = zte_rpcom_of_match,
	}
};



static int __init zte_rpcom_init(void)
{
	pr_info("%s: enter!!\n", __func__);
	return platform_driver_register(&zte_rpcom_driver);
}

static void __exit zte_rpcom_exit(void)
{
	pr_info("%s: enter!!\n", __func__);
	return platform_driver_unregister(&zte_rpcom_driver);
}

late_initcall(zte_rpcom_init);
module_exit(zte_rpcom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZTE light Inc.");
