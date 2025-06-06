// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define pr_fmt(fmt) "mpam_msc: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/string.h>
#include <linux/qcom_scmi_vendor.h>
#include <linux/scmi_protocol.h>
#include <soc/qcom/mpam_msc.h>
#include <soc/qcom/mpam_slc.h>

static LIST_HEAD(qcom_mpam_list);
#define DELIM_CHAR		" "

struct qcom_mpam_msc *qcom_msc_lookup(uint32_t msc_id)
{
	struct qcom_mpam_msc *mpam_msc;

	list_for_each_entry(mpam_msc, &qcom_mpam_list, node) {
		if (mpam_msc->msc_id == msc_id)
			return mpam_msc;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(qcom_msc_lookup);

int msc_system_get_partition(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->get_cache_partition)
			ret = qcom_msc->ops->get_cache_partition(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_get_partition);

int msc_system_get_device_capability(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->get_cache_partition_capability)
			ret = qcom_msc->ops->get_cache_partition_capability(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_get_device_capability);

int msc_system_set_partition(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->set_cache_partition)
			ret = qcom_msc->ops->set_cache_partition(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_set_partition);

int msc_system_mon_config(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->mon_config)
			ret = qcom_msc->ops->mon_config(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_mon_config);

int msc_system_reset_partition(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->set_cache_partition)
			ret = qcom_msc->ops->reset_cache_partition(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_reset_partition);

int attach_dev(struct device *dev, struct qcom_mpam_msc *qcom_msc,
		uint32_t msc_type)
{
	int ret = -EINVAL;

	switch (msc_type) {
	case SLC:
		break;
	default:
		return ret;
	}


	list_add_tail(&qcom_msc->node, &qcom_mpam_list);
	return 0;
}
EXPORT_SYMBOL_GPL(attach_dev);

void detach_dev(struct device *dev, struct qcom_mpam_msc *qcom_msc, uint32_t msc_type)
{
	switch (msc_type) {
	case SLC:
		break;
	default:
		return;
	}

	list_del(&qcom_msc->node);
}
EXPORT_SYMBOL_GPL(detach_dev);

int msc_system_mon_alloc_info(uint32_t msc_id, void *arg1, void *arg2)
{
	int i;
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;
	struct qcom_msc_slc_mon_val val;
	struct qcom_slc_mon_data_info *info;
	struct msc_query *query;
	union mon_values *mon_data;


	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	query = (struct msc_query *) arg1;
	if (query == NULL)
		return -EINVAL;

	info = (struct qcom_slc_mon_data_info *) arg2;
	if (info == NULL)
		return -EINVAL;

	if (query->qcom_msc_id.qcom_msc_type != SLC)
		return -EINVAL;

	mon_data = (union mon_values *) arg2;
	if (mon_data == NULL)
		return -EINVAL;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->mon_stats_read)
			ret = qcom_msc->ops->mon_stats_read(qcom_msc->dev, arg1, &val);
		break;
	default:
		break;
	}

	if (ret == 0) {
		for (i = 0; i < SLC_NUM_PARTIDS; i++) {
			if ((query->client_id == val.data[i].part_info.client_id) &&
					(query->part_id == val.data[i].part_info.part_id)) {
				mon_data->capacity.num_cache_lines = val.data[i].num_cache_lines;
				break;
			}
		}

		if (i == SLC_NUM_PARTIDS)
			return -EINVAL;
	}

	mon_data->capacity.last_capture_time = val.last_capture_time;

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_mon_alloc_info);

int msc_system_mon_read_miss_info(uint32_t msc_id, void *arg1, void *arg2)
{
	int i;
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;
	struct qcom_msc_slc_mon_val val;
	struct qcom_slc_mon_data_info *info;
	struct msc_query *query;
	union mon_values *mon_data;


	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	query = (struct msc_query *) arg1;
	if (query == NULL)
		return -EINVAL;

	info = (struct qcom_slc_mon_data_info *) arg2;
	if (info == NULL)
		return -EINVAL;

	if (query->qcom_msc_id.qcom_msc_type != SLC)
		return -EINVAL;

	mon_data = (union mon_values *) arg2;
	if (mon_data == NULL)
		return -EINVAL;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->mon_stats_read)
			ret = qcom_msc->ops->mon_stats_read(qcom_msc->dev, arg1, &val);
		break;
	default:
		break;
	}

	if (ret == 0) {
		for (i = 0; i < SLC_NUM_PARTIDS; i++) {
			if ((query->client_id == val.data[i].part_info.client_id) &&
					(query->part_id == val.data[i].part_info.part_id)) {
				mon_data->misses.num_rd_misses = val.data[i].rd_misses;
				break;
			}
		}

		if (i == SLC_NUM_PARTIDS)
			return -EINVAL;
	}

	mon_data->misses.last_capture_time = val.last_capture_time;
	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_mon_read_miss_info);

static int msc_system_mon_read(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->mon_stats_read)
			ret = qcom_msc->ops->mon_stats_read(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}

static ssize_t slc_mon_read_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	ssize_t count = 0;
	struct qcom_mpam_msc *qcom_mpam_msc;
	struct qcom_msc_slc_mon_val mon_buf;
	struct msc_query query;
	int i = 0;

	qcom_mpam_msc = qcom_msc_lookup(2);
	if (qcom_mpam_msc == NULL)
		return count;

	query.qcom_msc_id.qcom_msc_type = qcom_mpam_msc->qcom_msc_id.qcom_msc_type;
	query.qcom_msc_id.qcom_msc_class = qcom_mpam_msc->qcom_msc_id.qcom_msc_class;
	query.qcom_msc_id.idx = qcom_mpam_msc->qcom_msc_id.idx;
	msc_system_mon_read(2, &query, &mon_buf);


	count += scnprintf(buf + count, PAGE_SIZE - count,
			"Capacity dump for PART ID\n");
	for (i = 0; i < SLC_NUM_PARTIDS; i++) {
		count += scnprintf(buf + count, PAGE_SIZE - count,
			"\tClient_id:%d, Part id:%d, Cache line used:<%d>\n",
			mon_buf.data[i].part_info.client_id,
			mon_buf.data[i].part_info.part_id,
			mon_buf.data[i].num_cache_lines);
	}
	count += scnprintf(buf + count, PAGE_SIZE - count,
			"Read MISS for PART ID\n");
	for (i = 0; i < SLC_NUM_PARTIDS; i++) {
		count += scnprintf(buf + count, PAGE_SIZE - count,
			"\tClient_id:%d, Part id:%d, Read MISSes:<%lld>\n",
			mon_buf.data[i].part_info.client_id,
			mon_buf.data[i].part_info.part_id,
			mon_buf.data[i].rd_misses);
	}
	return count;
}

static ssize_t slc_dev_capability_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	ssize_t count = 0;
	int client_idx = 0, partid_idx = 0, ret;
	struct qcom_slc_capability *slc_capability;
	struct  slc_client_capability *slc_client_cap;
	struct slc_partid_capability *slc_partid_cap;
	struct qcom_mpam_msc *qcom_mpam_msc;
	struct msc_query query;
	struct qcom_slc_gear_val gear_config;

	qcom_mpam_msc = qcom_msc_lookup(2);
	if (qcom_mpam_msc == NULL)
		return count;

	slc_capability =  (struct qcom_slc_capability *)qcom_mpam_msc->msc_capability;

	count += scnprintf(buf + count, PAGE_SIZE - count,
			"MSC_ID:<%02d>, MSC_NAME:<%13s>, qcom_msc_id: <%d>",
		qcom_mpam_msc->msc_id, qcom_mpam_msc->msc_name,
		qcom_mpam_msc->qcom_msc_id.idx);

	query.qcom_msc_id.qcom_msc_type = qcom_mpam_msc->qcom_msc_id.qcom_msc_type;
	query.qcom_msc_id.qcom_msc_class = qcom_mpam_msc->qcom_msc_id.qcom_msc_class;
	query.qcom_msc_id.idx = qcom_mpam_msc->qcom_msc_id.idx;

	count += scnprintf(buf + count, PAGE_SIZE - count, "\n");
	for (client_idx = 0; client_idx < slc_capability->num_clients; client_idx++) {
		slc_client_cap = &(slc_capability->slc_client_cap[client_idx]);
		count += scnprintf(buf + count, PAGE_SIZE - count,
				"\t  Client Name:<%13s>,   Client ID: <%d>, Number of Part ID's: <%d>,\tSupport: %s\n",
				slc_client_cap->client_name,
				slc_client_cap->client_info.client_id,
				slc_client_cap->client_info.num_part_id,
				slc_client_cap->enabled ? "ENABLED" : "DISABLED");

		if (slc_client_cap->enabled == false)
			continue;

		query.client_id = slc_client_cap->client_info.client_id;
		for (partid_idx = 0; partid_idx < slc_client_cap->client_info.num_part_id; partid_idx++) {
			slc_partid_cap = &(slc_client_cap->slc_partid_cap[partid_idx]);
			count += scnprintf(buf + count, PAGE_SIZE - count,
					"\t\t\t  Part id:<%d>,   num gears: <%d>,",
					slc_partid_cap->part_id,
					slc_partid_cap->num_gears);

			query.part_id = partid_idx;
			ret = msc_system_get_partition(qcom_mpam_msc->msc_id, &query, &gear_config);
			if (ret || (gear_config.gear_val >= GEAR_MAX)) {
				gear_config.gear_val = 0;
				count += scnprintf(buf + count, PAGE_SIZE - count,
						" Gear configured Improperly!\n");
				continue;
			}

			count += scnprintf(buf + count, PAGE_SIZE - count, " Gear val Configured: %s\n",
					gear_index[gear_config.gear_val]);
		}
	}

	return count;
}

static ssize_t slc_part_mon_config_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	char *token, *delim = DELIM_CHAR;
	struct msc_query query;
	struct slc_mon_config_val mon_cfg_val;
	u32 val;
	struct qcom_mpam_msc *qcom_mpam_msc;

	qcom_mpam_msc = qcom_msc_lookup(2);
	if (qcom_mpam_msc == NULL)
		return count;

	query.qcom_msc_id.qcom_msc_type = qcom_mpam_msc->qcom_msc_id.qcom_msc_type;
	query.qcom_msc_id.qcom_msc_class = qcom_mpam_msc->qcom_msc_id.qcom_msc_class;
	query.qcom_msc_id.idx = qcom_mpam_msc->qcom_msc_id.idx;

	token = strsep((char **)&buf, delim);
	if (token == NULL)
		return count;

	if (kstrtou32(token, 0, &val))
		return count;

	query.client_id = (uint16_t) val;

	token = strsep((char **)&buf, delim);
	if (token == NULL)
		return count;

	if (kstrtou32(token, 0, &val))
		return count;

	query.part_id = (uint16_t) val;

	token = strsep((char **)&buf, delim);
	if (token == NULL)
		return count;

	if (kstrtou32(token, 0, &val))
		return count;

	mon_cfg_val.slc_mon_function = (uint32_t) val;
	token = strsep((char **)&buf, delim);
	if (token == NULL)
		return count;

	if (kstrtou32(token, 0, &val))
		return count;

	mon_cfg_val.enable = (uint32_t) val;

	msc_system_mon_config(qcom_mpam_msc->msc_id, &query, &mon_cfg_val);
	return count;
}

static ssize_t slc_part_config_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	char *token, *delim = DELIM_CHAR;
	struct msc_query query;
	struct qcom_slc_gear_val gear_config;
	unsigned long val;
	struct qcom_mpam_msc *qcom_mpam_msc;

	qcom_mpam_msc = qcom_msc_lookup(2);
	if (qcom_mpam_msc == NULL)
		return count;

	query.qcom_msc_id.qcom_msc_type = qcom_mpam_msc->qcom_msc_id.qcom_msc_type;
	query.qcom_msc_id.qcom_msc_class = qcom_mpam_msc->qcom_msc_id.qcom_msc_class;
	query.qcom_msc_id.idx = qcom_mpam_msc->qcom_msc_id.idx;

	token = strsep((char **)&buf, delim);
	if (token == NULL)
		return count;

	if (kstrtoul(token, 0, &val))
		return count;

	query.client_id = (uint16_t) val;

	token = strsep((char **)&buf, delim);
	if (token == NULL)
		return count;

	if (kstrtoul(token, 0, &val))
		return count;

	query.part_id = (uint16_t) val;

	token = strsep((char **)&buf, delim);
	if (token == NULL)
		return count;

	if (kstrtoul(token, 0, &val))
		return count;

	gear_config.gear_val = val;
	msc_system_set_partition(qcom_mpam_msc->msc_id, &query, &gear_config);
	return count;
}

static ssize_t slc_dev_reset_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int j = 0, k = 0, ret;
	struct qcom_slc_capability *slc_capability;
	struct  slc_client_capability *slc_client_cap;
	struct slc_partid_capability *slc_partid_cap;
	struct qcom_mpam_msc *qcom_mpam_msc;
	struct msc_query query;
	struct qcom_slc_gear_val gear_config;

	qcom_mpam_msc = qcom_msc_lookup(2);
	if (qcom_mpam_msc == NULL)
		return count;

	slc_capability =  (struct qcom_slc_capability *)qcom_mpam_msc->msc_capability;

	query.qcom_msc_id.qcom_msc_type = qcom_mpam_msc->qcom_msc_id.qcom_msc_type;
	query.qcom_msc_id.qcom_msc_class = qcom_mpam_msc->qcom_msc_id.qcom_msc_class;
	query.qcom_msc_id.idx = qcom_mpam_msc->qcom_msc_id.idx;

	for (j = 0; j < slc_capability->num_clients; j++) {
		slc_client_cap = &(slc_capability->slc_client_cap[j]);

		if (slc_client_cap->enabled == false)
			continue;

		query.client_id = slc_client_cap->client_info.client_id;
		for (k = 0; k < slc_client_cap->client_info.num_part_id; k++) {
			slc_partid_cap = &(slc_client_cap->slc_partid_cap[k]);

			query.part_id = k;
			gear_config.gear_val = 0;
			ret = msc_system_reset_partition(qcom_mpam_msc->msc_id, &query, &gear_config);
			if (ret)
				continue;
		}
	}

	return count;
}

static DEVICE_ATTR_RO(slc_dev_capability);
static DEVICE_ATTR_RO(slc_mon_read);
static DEVICE_ATTR_WO(slc_part_config);
static DEVICE_ATTR_WO(slc_part_mon_config);
static DEVICE_ATTR_WO(slc_dev_reset);

static struct attribute *mpam_slc_sysfs_attrs[] = {
	&dev_attr_slc_dev_capability.attr,
	&dev_attr_slc_mon_read.attr,
	&dev_attr_slc_part_config.attr,
	&dev_attr_slc_part_mon_config.attr,
	&dev_attr_slc_dev_reset.attr,
	NULL,
};

static struct attribute_group mpam_slc_sysfs_group = {
	.attrs	= mpam_slc_sysfs_attrs,
};

static int mpam_msc_probe(struct platform_device *pdev)
{
	int ret;

	ret = sysfs_create_group(&pdev->dev.kobj, &mpam_slc_sysfs_group);
	if (ret) {
		pr_err("Unable to create sysfs group\n");
		return ret;
	}

	if (of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev) < 0)
		dev_err(&pdev->dev, "Mpam driver probe failed.!\n");

	return 0;
}

int mpam_msc_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &mpam_slc_sysfs_group);
	return 0;
}
static const struct of_device_id mpam_msc_table[] = {
	{ .compatible = "qcom,mpam-msc" },
	{}
};

MODULE_DEVICE_TABLE(of, mpam_msc_table);

static struct platform_driver mpam_msc_driver = {
	.driver = {
		.name = "mpam-msc",
		.of_match_table = mpam_msc_table,
	},
	.probe = mpam_msc_probe,
	.remove = mpam_msc_remove,
};

module_platform_driver(mpam_msc_driver);

MODULE_SOFTDEP("pre: mpam");
MODULE_DESCRIPTION("QCOM MPAM MSC driver");
MODULE_LICENSE("GPL");
