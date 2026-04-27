/* Started by AICoder, pid:geb41kcca0s2d4814c42098cc250586ff582fe7e */
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Written by ZTE boot team, 20240122.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
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

/* Structure to hold platform data for hd6180 device */
/* add for hd6180 gpios control */
struct hd6180_platform_data {
	struct platform_device *pdev;
	int bd_msg_1p8_en;
	int bd_msg_reset_n;
	int bd_msg_download;
	int bd_msg_int;
	int bd_gps_sw;
	int bd_msg_boost_en;
	int bd_msg_wifi_sw;
	int bd_msg_sw;
	int gnss_elna_en0;
	int gnss_elna_en1;
	int bd_msg_lna_en;
	int bd_rf_sw_state;
};
static struct hd6180_platform_data *hd6180_pdata;

/* Debug flag to control power-on during boot */
/* for debug as soon as possible, set it to 0 while driver debug finished */
static int power_on_during_boot = 0;

/* Function to control power on/off of hd6180 device */
void hd6180_power_on(int on_or_off) 
{
	int on = on_or_off != 0 ? 1 : 0;
	if (on) {
		pr_info("%s: poweron the hd6180 chip\n", __func__);
		gpio_direction_output(hd6180_pdata->bd_msg_1p8_en, 1);
		msleep(200);
		enable_irq_wake(gpio_to_irq(hd6180_pdata->bd_msg_int));
	} else {
		pr_info("%s: poweroff the hd6180 chip\n", __func__);
		disable_irq_wake(gpio_to_irq(hd6180_pdata->bd_msg_int));
		gpio_direction_output(hd6180_pdata->bd_msg_1p8_en, 0);
	}
}

void hd6180_switch_rf_state(int state)
{
	switch(state) {
		case (1):
			pr_info("%s: rf switch to hd msg senging state\n", __func__);
			gpio_direction_output(hd6180_pdata->bd_msg_boost_en, 1);
			gpio_direction_output(hd6180_pdata->bd_msg_wifi_sw, 0);
			gpio_direction_output(hd6180_pdata->bd_msg_sw, 0);
			gpio_direction_output(hd6180_pdata->bd_msg_lna_en, 0);
			break;
		case (2):
			pr_info("%s: rf switch to hd msg recieving state\n", __func__);
			gpio_direction_output(hd6180_pdata->bd_msg_boost_en, 0);
			gpio_direction_output(hd6180_pdata->bd_msg_wifi_sw, 1);
			gpio_direction_output(hd6180_pdata->bd_msg_sw, 1);
			gpio_direction_output(hd6180_pdata->bd_msg_lna_en, 1);
			break;
		default:
			pr_info("%s: rf switch to qcom default state\n", __func__);
			gpio_direction_output(hd6180_pdata->bd_msg_boost_en, 0);
			gpio_direction_output(hd6180_pdata->bd_msg_wifi_sw, 0);
			gpio_direction_output(hd6180_pdata->bd_msg_sw, 1);
			gpio_direction_output(hd6180_pdata->bd_msg_lna_en, 0);
			break;
	}
	//pr_info("%s: bd_msg_boost_en = %d, bd_msg_wifi_sw = %d\n", __func__,
		//gpio_get_value(hd6180_pdata->bd_msg_boost_en), gpio_get_value(hd6180_pdata->bd_msg_wifi_sw));
	//pr_info("%s: bd_msg_sw = %d, bd_msg_lna_en = %d\n", __func__,
		//gpio_get_value(hd6180_pdata->bd_msg_sw), gpio_get_value(hd6180_pdata->bd_msg_lna_en));

	hd6180_pdata->bd_rf_sw_state = state;
}


static irqreturn_t handle_bd_msg_irq_rfswitch(int irq, void* data)
{
	int status;

	status = gpio_get_value(hd6180_pdata->bd_msg_int);
	pr_info("%s: hd6180, bd msg irq triggered..., status = %d\n", __func__, status);

	if (status) {
		/* switch rf state to sending state */
		hd6180_switch_rf_state(1);
	} else {
		/* switch rf state to receiving state */
		hd6180_switch_rf_state(2);
	}

	return IRQ_HANDLED;
}


/* Store function for bd_msg_1p8_en sysfs entry */
static ssize_t bd_msg_1p8_en_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int on = 0;
	sscanf(buff, "%u", &on);
	hd6180_power_on(on);
	return strnlen(buff, size);
}

/* Show function for bd_msg_1p8_en sysfs entry */
static ssize_t bd_msg_1p8_en_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	int value;
	value = gpio_get_value(hd6180_pdata->bd_msg_1p8_en);
	return sprintf(buff, "%d\n", value);
}

/* Device attribute for bd_msg_1p8_en */
static DEVICE_ATTR(bd_msg_1p8_en, S_IRUGO | S_IWUSR, bd_msg_1p8_en_show, bd_msg_1p8_en_store);

/* Store function for bd_msg_reset_n sysfs entry */
static ssize_t bd_msg_reset_n_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int value = 0;
	sscanf(buff, "%u", &value);
	gpio_direction_output(hd6180_pdata->bd_msg_reset_n, value);
	return strnlen(buff, size);
}

/* Show function for bd_msg_reset_n sysfs entry */
static ssize_t bd_msg_reset_n_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	int value;
	value = gpio_get_value(hd6180_pdata->bd_msg_reset_n);
	return sprintf(buff, "%d\n", value);
}

/* Device attribute for bd_msg_reset_n */
static DEVICE_ATTR(bd_msg_reset_n, S_IRUGO | S_IWUSR, bd_msg_reset_n_show, bd_msg_reset_n_store);

/* Store function for bd_msg_download sysfs entry */
static ssize_t bd_msg_download_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int value = 0;
	sscanf(buff, "%u", &value);
	gpio_direction_output(hd6180_pdata->bd_msg_download, value);
	return strnlen(buff, size);
}

/* Show function for bd_msg_download sysfs entry */
static ssize_t bd_msg_download_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	int value;
	value = gpio_get_value(hd6180_pdata->bd_msg_download);
	return sprintf(buff, "%d\n", value);
}

/* Device attribute for bd_msg_download */
static DEVICE_ATTR(bd_msg_download, S_IRUGO | S_IWUSR, bd_msg_download_show, bd_msg_download_store);


/* Store function for rf_switch_state sysfs entry */
static ssize_t rf_switch_state_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t size)
{
	unsigned int value = 0;
	sscanf(buff, "%u", &value);
	hd6180_switch_rf_state(value);
	return strnlen(buff, size);
}

/* Show function for rf_switch_state sysfs entry */
static ssize_t rf_switch_state_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	int value;
	value = hd6180_pdata->bd_rf_sw_state;
	return sprintf(buff, "%d\n", value);
}

/* Device attribute for rf_switch_state */
static DEVICE_ATTR(rf_switch_state, S_IRUGO | S_IWUSR, rf_switch_state_show, rf_switch_state_store);


/* Array of attributes for hd6180 device */
static struct attribute  *hd6180_attrs[] = {
	&dev_attr_bd_msg_1p8_en.attr,
	&dev_attr_bd_msg_reset_n.attr,
	&dev_attr_bd_msg_download.attr,
	&dev_attr_rf_switch_state.attr,
	NULL,
};

/* Attribute group for hd6180 device */
static struct attribute_group hd6180_attr_grp = {
	.name = "zte-hd6180",
	.attrs = hd6180_attrs
};

/* Populate GPIO information from device tree */
static int hd6180_populate_dt_pinfo(struct platform_device *pdev)
{
	int ret;

	pr_info("%s\n", __func__);
	if (!hd6180_pdata) {
		pr_info("%s hd6180_pdata is null \n", __func__);
		return -ENOMEM;
	}
	if (!pdev->dev.of_node) {
		pr_err("%s: dev.of_node is null\n",__func__);
		return -EINVAL;
	}

#define GET_GPIO(name, var) \
		do { \
			hd6180_pdata->var = of_get_named_gpio(pdev->dev.of_node, name, 0); \
			pr_info("%s: %s = %d\n", __func__, name, hd6180_pdata->var); \
			if (gpio_is_valid(hd6180_pdata->var)) { \
				if (devm_gpio_request(&pdev->dev, hd6180_pdata->var, name)) { \
					dev_err(&pdev->dev, "failed to request %s gpio!\n", name); \
					return -EINVAL; \
				} \
			} \
		} while (0)

		GET_GPIO("bd_msg_1p8_en", bd_msg_1p8_en);
		GET_GPIO("bd_msg_reset_n", bd_msg_reset_n);
		GET_GPIO("bd_msg_download", bd_msg_download);
		GET_GPIO("bd_msg_int", bd_msg_int);
		GET_GPIO("bd_gps_sw", bd_gps_sw);
		GET_GPIO("bd_msg_boost_en", bd_msg_boost_en);
		GET_GPIO("bd_msg_wifi_sw", bd_msg_wifi_sw);
		GET_GPIO("bd_msg_sw", bd_msg_sw);
		GET_GPIO("gnss_elna_en0", gnss_elna_en0);
		GET_GPIO("gnss_elna_en1", gnss_elna_en1);
		GET_GPIO("bd_msg_lna_en", bd_msg_lna_en);

#undef GET_GPIO

	ret = devm_request_irq(&pdev->dev,
			gpio_to_irq(hd6180_pdata->bd_msg_int),
			handle_bd_msg_irq_rfswitch,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"bd_msg_int", hd6180_pdata);
	if (ret) {
		dev_err(&pdev->dev, "failed to request bd_msg_int as irq,, ret = %d\n", ret);
	}

	return 0;
}

/* Device tree match table */
static const struct of_device_id zte_hd6180_of_match[] = {
	{ .compatible = "zte-hd6180", },
	{ },
};
MODULE_DEVICE_TABLE(of, zte_hd6180_of_match);

/* Probe function for hd6180 device */
static int zte_hd6180_probe(struct platform_device *pdev)
{
	int ret = 0;
	pr_info("%s into!\n", __func__);

	/* Allocate memory for platform data */
	hd6180_pdata = kzalloc(sizeof(*hd6180_pdata), GFP_KERNEL);
	if (!hd6180_pdata) {
		pr_info("%s hd6180_pdata is null \n", __func__);
		return -ENOMEM;
	}

	hd6180_pdata->pdev = pdev;

	/* Populate GPIO information from device tree */
	ret = hd6180_populate_dt_pinfo(pdev);
	if (ret) {
		pr_err("%s: failed to populate device tree info: ret = %d\n", __func__, ret);
		goto out;
	}

	pdev->dev.platform_data = hd6180_pdata;

	/* Create sysfs entries */
	ret = sysfs_create_group(&pdev->dev.kobj, &hd6180_attr_grp);
	if (ret) {
		pr_err("%s:failed to create the hd6180 attr group: ret = %d\n", __func__, ret);
		goto out;
	}

	/* Control power state based on debug flag */
	if (power_on_during_boot) {
		pr_info("%s power on hd6180 device\n", __func__);
		hd6180_power_on(1);
		hd6180_switch_rf_state(1);
	} else {
		pr_info("%s power off hd6180 device\n", __func__);
		hd6180_power_on(0);
	}

	pr_info("%s end\n", __func__);
	return 0;

out:
	kfree((void *)hd6180_pdata);
	return ret;
}

/* Remove function for hd6180 device */
static int  zte_hd6180_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &hd6180_attr_grp);
	kfree((void *)hd6180_pdata);

	return 0;
}

/* Platform driver structure for hd6180 device */
static struct platform_driver zte_hd6180_driver = {
	.probe			= zte_hd6180_probe,
	.remove			= zte_hd6180_remove,
	.driver			= {
		.name	= "zte-hd6180",
		.owner	= THIS_MODULE,
		.of_match_table = zte_hd6180_of_match,
	}
};

/* Initialization function for hd6180 driver */
static int __init zte_hd6180_init(void)
{
	pr_info("%s: enter!!\n", __func__);
	return platform_driver_register(&zte_hd6180_driver);
}

/* Exit function for hd6180 driver */
static void __exit zte_hd6180_exit(void)
{
	pr_info("%s: enter!!\n", __func__);
	return platform_driver_unregister(&zte_hd6180_driver);
}

late_initcall(zte_hd6180_init);
module_exit(zte_hd6180_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Drivers for hd6180 ctrl");
MODULE_AUTHOR("ZTE boot team");
/* Ended by AICoder, pid:geb41kcca0s2d4814c42098cc250586ff582fe7e */
