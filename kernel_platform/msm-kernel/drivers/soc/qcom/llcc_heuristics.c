// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/soc/qcom/llcc-qcom.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/scmi_protocol.h>
#include <linux/qcom_scmi_vendor.h>
#include <soc/qcom/llcc_heuristics.h>

#define DELIM_CHAR		" "
#define SCMI_GET_PARAM_LEN	32

static struct scmi_protocol_handle *ph;
static const struct qcom_scmi_vendor_ops *ops;

static int qcom_llcc_set_params(struct scid_heuristics_params *param, u32 param_id)
{
	int ret = -EPERM;

	if (ops)
		ret = ops->set_param(ph, param, SCID_HEURISTICS_SCMI_STR, param_id,
				sizeof(struct scid_heuristics_params));

	return ret;
}

static int qcom_llcc_get_param(void *param, u32 param_id, int size)
{
	int ret = -EPERM;
	uint8_t buf[SCMI_GET_PARAM_LEN];

	if (size >= SCMI_GET_PARAM_LEN)
		return -EINVAL;

	memset(buf, 0, SCMI_GET_PARAM_LEN);
	if (ops) {
		ret = ops->get_param(ph, buf, SCID_HEURISTICS_SCMI_STR, param_id, 0,
				size);
		if (!ret)
			memcpy(param, buf, size);
	}

	return ret;
}

static ssize_t llcc_cpucp_control_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	char *token, *delim = DELIM_CHAR;
	struct scid_heuristics_data *heuristics_data;
	struct scid_heuristics_params *params;

	heuristics_data = (struct scid_heuristics_data *)dev_get_drvdata(dev);
	params = &heuristics_data->params;
	token = strsep((char **)&buf, delim);
	if (token == NULL)
		return count;

	if (sysfs_streq(token, "ENABLE"))
		params->scid_heuristics_enabled = 1;
	else
		params->scid_heuristics_enabled = 0;

	qcom_llcc_set_params(params, SCID_ACTIVATION_CONTROL);

	return count;
}

static ssize_t cpucp_llcc_stats_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	ssize_t cnt = 0, ret = 0;
	struct heuristics_scid_triger_stats trigger_stats;
	struct heuristics_scid_stats scid_stats;

	memset((void *)&trigger_stats, 0, sizeof(struct heuristics_scid_triger_stats));
	memset((void *)&scid_stats, 0, sizeof(struct heuristics_scid_stats));

	ret = qcom_llcc_get_param((void *)&scid_stats, HEURISTICS_SCID_STATS,
			sizeof(struct heuristics_scid_stats));
	if (ret) {
		pr_emerg("HEURISTICS_SCID_STATS Length Not matching %zd\n", ret);
		return cnt;
	}

	ret = qcom_llcc_get_param((void *)&trigger_stats, HEURISTICS_TRIGGER_STATUS,
			sizeof(struct heuristics_scid_triger_stats));
	if (ret) {
		pr_err("HEURISTICS_TRIGGER_STATUS Length Not matching %zd\n", ret);
		return cnt;
	}

	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "scid_act_count: %d\n",
			scid_stats.heuristics_scid_act_counter);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "scid_deact_count: %d\n",
			scid_stats.heuristics_scid_deact_counter);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "scid_act_residency: %lld\n",
			scid_stats.heuristics_scid_act_residency);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "ss_lpm_entry_count: %d\n",
			trigger_stats.ss_lpm_entry_counter);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "clk_dom0_trigger: %u\n",
			trigger_stats.clkdom0_act_trigger);
	cnt += scnprintf(buf + cnt, PAGE_SIZE - cnt, "clk_dom1_trigger: %u\n",
			trigger_stats.clkdom1_act_trigger);
	return cnt;
}

static DEVICE_ATTR_WO(llcc_cpucp_control);
static DEVICE_ATTR_RO(cpucp_llcc_stats);

static struct attribute *llcc_cpucp_attrs[] = {
	&dev_attr_llcc_cpucp_control.attr,
	&dev_attr_cpucp_llcc_stats.attr,
	NULL,
};

static struct attribute_group llcc_cpucp_group = {
	.attrs	= llcc_cpucp_attrs,
};

static int cpucp_llcc_sysfs_init(struct platform_device *pdev)
{
	if (sysfs_create_group(&pdev->dev.kobj, &llcc_cpucp_group)) {
		pr_err("Unable to create sysfs group\n");
		return -EINVAL;
	}

	return 0;
}

static int cpucp_llcc_init(struct platform_device *pdev)
{
	struct scmi_device *scmi_dev;
	int result;

	scmi_dev = get_qcom_scmi_device();
	if (IS_ERR(scmi_dev)) {
		pr_err("Error getting scmi_dev\n");
		result = PTR_ERR(scmi_dev);
		if (result == -EPROBE_DEFER)
			return result;
	}
	if (!scmi_dev || !scmi_dev->handle)
		return -EINVAL;

	ops = scmi_dev->handle->devm_protocol_get(scmi_dev, QCOM_SCMI_VENDOR_PROTOCOL, &ph);
	cpucp_llcc_sysfs_init(pdev);

	return 0;
}

static int heuristics_scid_probe(struct platform_device *pdev)
{
	int ret, dom_idx;
	struct device_node *node;
	struct scid_heuristics_data *heuristics_data;
	int number_of_domains;
	struct scid_heuristics_params *heuristics_param;
	bool flag;

	heuristics_data = devm_kzalloc(&pdev->dev, sizeof(struct scid_heuristics_data), GFP_KERNEL);
	if (heuristics_data == NULL)
		return -ENOMEM;

	heuristics_param = &heuristics_data->params;
	node = pdev->dev.of_node;
	ret = of_property_read_u32(node, "qcom,heuristics_scid",
			&heuristics_param->heuristics_scid);
	if (ret)
		return ret;

	number_of_domains = of_property_count_u32_elems(node, "freq,threshold_idx");
	if (number_of_domains > ARRAY_SIZE(heuristics_param->freq_idx))
		return -EINVAL;

	flag = of_property_read_bool(node, "qcom,scid_heuristics_enabled");
	heuristics_param->scid_heuristics_enabled = flag ? 1 : 0;
	ret = of_property_read_u32(node, "heuristics_scid_thread_interval",
			&heuristics_param->thread_interval);
	if (ret)
		return ret;

	for (dom_idx = 0; dom_idx < number_of_domains; dom_idx++) {
		ret = of_property_read_u32_index(node, "freq,threshold_idx", dom_idx,
							&(heuristics_param->freq_idx[dom_idx]));
		if (ret)
			return ret;

		ret = of_property_read_u32_index(node, "freq,threshold_residency", dom_idx,
				&(heuristics_param->freq_idx_residency[dom_idx]));
		if (ret)
			return ret;

		if (heuristics_param->thread_interval <
				heuristics_param->freq_idx_residency[dom_idx]) {
			pr_err("Thread interval configured less than residency value!\n");
			return -EINVAL;
		}
	}

	ret = cpucp_llcc_init(pdev);
	if (ret)
		return ret;

	if (heuristics_param->scid_heuristics_enabled)
		qcom_llcc_set_params(heuristics_param, HEURISTICS_INIT);

	platform_set_drvdata(pdev, heuristics_data);
	return ret;
}

int heuristics_scid_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &llcc_cpucp_group);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id heuristics_scid_table[] = {
	{ .compatible = "qcom,scid-heuristics" },
	{}
};

MODULE_DEVICE_TABLE(of, heuristics_scid_table);

static struct platform_driver heuristics_scid_driver = {
	.driver = {
		.name = "scid-heuristics",
		.of_match_table = heuristics_scid_table,
	},
	.probe = heuristics_scid_probe,
	.remove = heuristics_scid_remove,
};

module_platform_driver(heuristics_scid_driver);

MODULE_SOFTDEP("pre: llcc_qcom");
MODULE_DESCRIPTION("QCOM HEURISTICS SCID driver");
MODULE_LICENSE("GPL");
