/*
 *  vi530x_module.c - Linux kernel modules for VI530X FlightSense TOF
 *						 sensor
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/of_gpio.h>
#include <linux/kobject.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/timekeeping.h>
#include <linux/time64.h>

#include "vi530x.h"
#include "vi530x_platform.h"
#include "vi530x_firmware.h"
#include "vi530x_api.h"
#include <linux/pm_runtime.h>

#define VI530X_IOCTL_PERIOD _IOW('p', 0x01, uint32_t)
#define VI530X_IOCTL_XTALK_CALIB _IOR('p', 0x02, struct VI530X_XTALK_Calib_Data)
#define VI530X_IOCTL_XTALK_CONFIG _IOW('p', 0x03, struct VI530X_XTALK_Config_Data)
#define VI530X_IOCTL_OFFSET_CALIB _IOWR('p', 0x04, struct VI530X_OFFSET_Calib_Data)
#define VI530X_IOCTL_OFFSET_CONFIG _IOW('p', 0x05, int16_t)
#define VI530X_IOCTL_POWER_ON _IO('p', 0x06)
#define VI530X_IOCTL_CHIP_INIT _IO('p', 0x07)
#define VI530X_IOCTL_START _IO('p', 0x08)
#define VI530X_IOCTL_STOP _IO('p', 0x09)
#define VI530X_IOCTL_MZ_DATA _IOR('p', 0x0a, struct VI530X_Measurement_Data)
#define VI530X_IOCTL_POWER_OFF _IO('p', 0x0b)

#define VI530X_DRV_NAME "vi530x"
static int xtalk_mark;
static int offset_mark;

static int32_t vi530x_io_init(struct  vi530x_data *data)
{
    int rc = 0;

    if (!data) {
        vi530x_errmsg("Invalid Args");
        return -EINVAL;
    }

        if ((data->client != NULL) &&
            (data->client->adapter != NULL) &&
            (data->pm_ctrl_client_enable)) {
            vi530x_dbgmsg("%s:%d: Calling get_sync",
            __func__, __LINE__);
            rc = pm_runtime_get_sync(data->client->adapter->dev.parent);
            if (rc < 0) {
            vi530x_errmsg("Failed to get sync rc: %d", rc);
            return -EINVAL;
            }
        }

    return rc;
}

static int32_t vi530x_io_release(struct  vi530x_data *data)
{
    if (!data) {
        vi530x_errmsg("Invalid Args");
        return -EINVAL;
    }

    if ((data->client != NULL) &&
        (data->client->adapter != NULL) &&
        (data->pm_ctrl_client_enable)) {
            vi530x_dbgmsg("%s:%d: Calling put_sync",
                __func__, __LINE__);
                pm_runtime_put_sync(data->client->adapter->dev.parent);
        }

        return 0;
}

static void vi530x_sensor_utils_parse_pm_ctrl_flag(struct device_node *of_node,
    struct  vi530x_data *data)
{
    struct device_node *of_parent = of_get_next_parent(of_node);

    if (of_parent != NULL) {
        data->pm_ctrl_client_enable =
        of_property_read_bool(of_parent, "qcom,pm-ctrl-client");
    }
}

static inline void getnstimeofday(struct timespec *tv)
{
	struct timespec64 now;

	ktime_get_real_ts64(&now);
	tv->tv_sec = now.tv_sec;
	tv->tv_nsec = now.tv_nsec/1000;
}
struct vi530x_api_fn_t {
	int32_t (*Power_ON)(VI530X_DEV dev);
	int32_t (*Power_OFF)(VI530X_DEV dev);
	void (*Set_Period)(VI530X_DEV dev, uint32_t period);
	int32_t (*Single_Measure)(VI530X_DEV dev);
	int32_t (*Start_Continuous_Measure)(VI530X_DEV dev);
	int32_t (*Stop_Continuous_Measure)(VI530X_DEV dev);
	int32_t (*Get_Measure_Data)(VI530X_DEV dev);
	int32_t (*Get_Interrupt_State)(VI530X_DEV dev);
	int32_t (*Chip_Init)(VI530X_DEV dev);
	int32_t (*Start_XTalk_Calibration)(VI530X_DEV dev);
	int32_t (*Start_Offset_Calibration)(VI530X_DEV dev);
	int32_t (*Get_XTalk_Parameter)(VI530X_DEV dev);
	int32_t (*Config_XTalk_Parameter)(VI530X_DEV dev);
};
static struct vi530x_api_fn_t vi530x_api_func_tbl = {
	.Power_ON = VI530X_Chip_PowerON,
	.Power_OFF = VI530X_Chip_PowerOFF,
	.Set_Period = VI530X_Set_Period,
	.Single_Measure = VI530X_Single_Measure,
	.Start_Continuous_Measure = VI530X_Start_Continuous_Measure,
	.Stop_Continuous_Measure = VI530X_Stop_Continuous_Measure,
	.Get_Measure_Data = VI530X_Get_Measure_Data,
	.Get_Interrupt_State = VI530X_Get_Interrupt_State,
	.Chip_Init = VI530X_Chip_Init,
	.Start_XTalk_Calibration = VI530X_Start_XTalk_Calibration,
	.Start_Offset_Calibration = VI530X_Start_Offset_Calibration,
	.Get_XTalk_Parameter = VI530X_Get_XTalk_Parameter,
	.Config_XTalk_Parameter = VI530X_Config_XTalk_Parameter,
};
struct vi530x_api_fn_t *vi530x_func_tbl;

static void vi530x_setupAPIFunctions(void)
{
	vi530x_func_tbl->Power_ON = VI530X_Chip_PowerON;
	vi530x_func_tbl->Power_OFF = VI530X_Chip_PowerOFF;
	vi530x_func_tbl->Set_Period = VI530X_Set_Period;
	vi530x_func_tbl->Single_Measure = VI530X_Single_Measure;
	vi530x_func_tbl->Start_Continuous_Measure = VI530X_Start_Continuous_Measure;
	vi530x_func_tbl->Stop_Continuous_Measure = VI530X_Stop_Continuous_Measure;
	vi530x_func_tbl->Get_Measure_Data = VI530X_Get_Measure_Data;
	vi530x_func_tbl->Get_Interrupt_State = VI530X_Get_Interrupt_State;
	vi530x_func_tbl->Chip_Init = VI530X_Chip_Init;
	vi530x_func_tbl->Start_XTalk_Calibration = VI530X_Start_XTalk_Calibration;
	vi530x_func_tbl->Start_Offset_Calibration = VI530X_Start_Offset_Calibration;
	vi530x_func_tbl->Get_XTalk_Parameter = VI530X_Get_XTalk_Parameter;
	vi530x_func_tbl->Config_XTalk_Parameter = VI530X_Config_XTalk_Parameter;
}

static void vi530x_enable_irq(struct  vi530x_data *data)
{
	if(!data)
		return;

	if(data->intr_state == VI530X_INTR_DISABLED)
	{
		data->intr_state = VI530X_INTR_ENABLED;
		enable_irq(data->irq);
	}
}

static void vi530x_disable_irq(struct  vi530x_data *data)
{
	if(!data)
		return;

	if(data->intr_state == VI530X_INTR_ENABLED)
	{
		data->intr_state = VI530X_INTR_DISABLED;
		disable_irq(data->irq);
	}
}

static ssize_t vi530x_chip_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct vi530x_data *data = dev_get_drvdata(dev);

	if(NULL != data)
		return scnprintf(buf, PAGE_SIZE, "%u\n", data->chip_enable);

	return -EINVAL;
}

static ssize_t vi530x_chip_enable_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct vi530x_data *data = dev_get_drvdata(dev);
	VI530X_Error Status = VI530X_ERROR_NONE;
	unsigned int val = 0;

	if(NULL != data)
	{
		mutex_lock(&data->work_mutex);
		if(sscanf(buf, "%u\n", &val) != 1)
		{
			mutex_unlock(&data->work_mutex);
			return -1;
		}
		if(val !=0 && val !=1)
		{
			vi530x_errmsg("enable store unvalid value=%u\n", val);
			mutex_unlock(&data->work_mutex);
			return count;
		}
		if(val == 1)
		{
			if(data->chip_enable == 0)
			{
				data->chip_enable = 1;
				vi530x_enable_irq(data);
				Status = vi530x_func_tbl->Power_ON(data);
				getnstimeofday(&data->start_ts);
			} else {
				vi530x_errmsg("already enabled!!\n");
			}
		} else {
			if(data->chip_enable == 1)
			{
				data->chip_enable = 0;
				vi530x_disable_irq(data);
				data->fwdl_status = 0;
				Status = vi530x_func_tbl->Power_OFF(data);
			}
			else {
				vi530x_errmsg("already disabled!!\n");
			}
		}
		mutex_unlock(&data->work_mutex);
		return Status ? -1 : count;
	}

	return -EPERM; 
}

static DEVICE_ATTR(chip_enable, 0664, vi530x_chip_enable_show, vi530x_chip_enable_store);

/* for debug */
static ssize_t vi530x_enable_debug_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct vi530x_data *data = dev_get_drvdata(dev);

	if(NULL != data)
		return snprintf(buf, PAGE_SIZE, "%u\n", data->enable_debug);

	return -EINVAL;
}

static ssize_t vi530x_enable_debug_store(struct device *dev,
					struct device_attribute *attr, const
					char *buf, size_t count)
{
	struct vi530x_data *data = dev_get_drvdata(dev);
	unsigned int on = 0;

	if(NULL != data)
	{
		mutex_lock(&data->work_mutex);
		if(sscanf(buf, "%u\n", &on) != 1)
		{
			mutex_unlock(&data->work_mutex);
			return -1;
		}
		if ((on != 0) &&  (on != 1)) {
			vi530x_errmsg("set debug=%d\n", on);
			mutex_unlock(&data->work_mutex);
			return count;
		}
		mutex_unlock(&data->work_mutex);
		data->enable_debug = on;
		return count;
	}

	return -EINVAL;
}

static DEVICE_ATTR(enable_debug, 0664, vi530x_enable_debug_show, vi530x_enable_debug_store);

static ssize_t vi530x_chip_init_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct vi530x_data *data = dev_get_drvdata(dev);
	VI530X_Error Status = VI530X_ERROR_NONE;
	unsigned int val = 0;

	if(NULL != data)
	{
		mutex_lock(&data->work_mutex);
		if(sscanf(buf, "%u\n", &val) != 1)
		{
			mutex_unlock(&data->work_mutex);
			return -1;
		}
		
		if(val)
		{
			vi530x_io_init(data);
			Status = vi530x_func_tbl->Chip_Init(data);
			vi530x_io_release(data);
			data->fwdl_status = 1;
		} else {
			mutex_unlock(&data->work_mutex);
			return -EINVAL;
		}
		mutex_unlock(&data->work_mutex);
		return Status ? -1 : count;
	}

	return -EPERM; 
}

static DEVICE_ATTR(chip_init, 0220, NULL, vi530x_chip_init_store);

static ssize_t vi530x_period_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct vi530x_data *data = dev_get_drvdata(dev);

	if(data != NULL)
		return scnprintf(buf, PAGE_SIZE, "%u\n", data->period);

	return -EINVAL;
}

static ssize_t vi530x_period_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t val = 0;
	struct vi530x_data *data = dev_get_drvdata(dev);

	if(data != NULL)
	{
		mutex_lock(&data->work_mutex);
		if(sscanf(buf, "%u\n", &val) != 1)
		{
			mutex_unlock(&data->work_mutex);
			return -EINVAL;
		}
		vi530x_io_init(data);
		data->period = val;
		vi530x_func_tbl->Set_Period(data, data->period);
		vi530x_io_release(data);
		mutex_unlock(&data->work_mutex);
		return count;
	}

	return -EPERM;
}

static DEVICE_ATTR(period, 0664, vi530x_period_show, vi530x_period_store);
static  int tof_flag = 0;

static ssize_t vi530x_capture_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct vi530x_data *data = dev_get_drvdata(dev);
	VI530X_Error Status = VI530X_ERROR_NONE;
	unsigned int val = 0;

	if(NULL != data)
	{
		mutex_lock(&data->work_mutex);

		if(sscanf(buf, "%u\n", &val) != 1)
		{
			mutex_unlock(&data->work_mutex);
			return -1;
		}
		if(val !=0 && val !=1)
		{
			vi530x_errmsg("capture store unvalid value=%u\n", val);
			mutex_unlock(&data->work_mutex);
			return count;
		}

		if(val == 1)
		{
            if(!tof_flag) {
                vi530x_io_init(data);
                tof_flag = 1;
        }
			Status = vi530x_func_tbl->Start_Continuous_Measure(data);
		} else {
			Status = vi530x_func_tbl->Stop_Continuous_Measure(data);
            vi530x_io_release(data);
            tof_flag = 0;
		}
		mutex_unlock(&data->work_mutex);
		return Status ? -1 : count;
	}

	return -EPERM; 
}

static DEVICE_ATTR(capture, 0220, NULL, vi530x_capture_store);

static ssize_t vi530x_xtalk_calib_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct vi530x_data *data = dev_get_drvdata(dev);

	if(data != NULL)
		return scnprintf(buf, PAGE_SIZE, "%d.%u\n", data->XtalkData.xtalk_cal, data->XtalkData.xtalk_peak);

	return -EPERM;
}

static ssize_t vi530x_xtalk_calib_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct vi530x_data *data = dev_get_drvdata(dev);
	VI530X_Error Status = VI530X_ERROR_NONE;
	unsigned int val = 0;

	if(NULL != data)
	{
		mutex_lock(&data->work_mutex);

		if(sscanf(buf, "%u\n", &val) != 1)
		{
			mutex_unlock(&data->work_mutex);
			return -1;
		}
		if(val !=1)
		{
			vi530x_errmsg("xtalk calibration store unvalid value=%u\n", val);
			mutex_unlock(&data->work_mutex);
			return count;
		}
		vi530x_io_init(data);
		xtalk_mark = 1;
		Status = vi530x_func_tbl->Start_XTalk_Calibration(data);
		mdelay(600);
		xtalk_mark = 0;
		vi530x_io_release(data);
		mutex_unlock(&data->work_mutex);
		return Status ? -1 : count;
	}

	return -EPERM;
}

static DEVICE_ATTR(xtalk_calib, 0664, vi530x_xtalk_calib_show, vi530x_xtalk_calib_store);

static ssize_t vi530x_offset_calib_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct vi530x_data *data = dev_get_drvdata(dev);

	if(data != NULL)
		return scnprintf(buf, PAGE_SIZE, "%d\n", data->OffsetData.offset_cal);

	return -EPERM;
}

static ssize_t vi530x_offset_calib_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct vi530x_data *data = dev_get_drvdata(dev);
	VI530X_Error Status = VI530X_ERROR_NONE;
	int val = 0;

	if(NULL != data)
	{
		mutex_lock(&data->work_mutex);
		if(sscanf(buf, "%d\n", &val) != 1)
		{
			mutex_unlock(&data->work_mutex);
			return -1;
		}
		if(val <= 0)
		{
			vi530x_errmsg("offset calibration store unvalid value=%u\n", val);
			mutex_unlock(&data->work_mutex);
			return count;
		}
		vi530x_io_init(data);
		data->OffsetData.offset_mili = val;
		offset_mark = 1;
		Status = vi530x_func_tbl->Start_Offset_Calibration(data);
		offset_mark = 0;
		vi530x_io_release(data);
		mutex_unlock(&data->work_mutex);
		return Status ? -1 : count;
	}

	return -EPERM;
}

static DEVICE_ATTR(offset_calib, 0664, vi530x_offset_calib_show, vi530x_offset_calib_store);

static ssize_t vi530x_xtalk_data_read(struct file *filp,
	struct kobject *kobj, struct bin_attribute *attr,
	char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct vi530x_data *data = dev_get_drvdata(dev);
	void *src = (void *) &(data->XtalkData);

	mutex_lock(&data->work_mutex);
	if (!data->chip_enable) {
		vi530x_errmsg("can't set calib data while disable sensor\n");
		mutex_unlock(&data->work_mutex);
		return -EBUSY;
	}

	if (count > sizeof(struct VI530X_XTALK_Calib_Data))
		count = sizeof(struct VI530X_XTALK_Calib_Data);

	memcpy(buf, src, count);
	mutex_unlock(&data->work_mutex);

	return count;
}

static ssize_t vi530x_xtalk_data_write(struct file *filp,
	struct kobject *kobj, struct bin_attribute *attr,
	char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct vi530x_data *data = dev_get_drvdata(dev);
	struct VI530X_XTALK_Config_Data *xtalk_data =
				(struct VI530X_XTALK_Config_Data *)buf;
	int rc = 0;

	mutex_lock(&data->work_mutex);

	if (!data->chip_enable) {
		rc = -EBUSY;
		vi530x_errmsg("can't set calib data while disable sensor\n");
		goto error;
	}

	if (count != sizeof(struct VI530X_XTALK_Config_Data))
		goto invalid;

	if(data->enable_debug)
	{
		vi530x_errmsg("xtalk config: %d\n", xtalk_data->xtalk_config);
		vi530x_errmsg("xtalk maxratio: %d\n", xtalk_data->maxratio);
	}

	vi530x_io_init(data);
	data->XtalkConfig.xtalk_config = xtalk_data->xtalk_config;
	data->XtalkConfig.maxratio = xtalk_data->maxratio;
	rc = vi530x_func_tbl->Config_XTalk_Parameter(data);
	if (rc) {
		vi530x_errmsg("config xtalk calibration data fail %d", rc);
		goto error;
	}
	vi530x_io_release(data);
	mutex_unlock(&data->work_mutex);

	return count;

invalid:
	vi530x_errmsg("invalid syntax");
	rc = -EINVAL;
	goto error;

error:
	mutex_unlock(&data->work_mutex);

	return rc;
}

static ssize_t vi530x_offset_data_read(struct file *filp,
	struct kobject *kobj, struct bin_attribute *attr,
	char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct vi530x_data *data = dev_get_drvdata(dev);
	void *src = (void *) &(data->OffsetData);

	mutex_lock(&data->work_mutex);
	if (!data->chip_enable) {
		vi530x_errmsg("can't set calib data while disable sensor\n");
		mutex_unlock(&data->work_mutex);
		return -EBUSY;
	}

	if (count > sizeof(struct VI530X_OFFSET_Calib_Data))
		count = sizeof(struct VI530X_OFFSET_Calib_Data);

	memcpy(buf, src, count);
	data->offset_config = data->OffsetData.offset_cal;
	mutex_unlock(&data->work_mutex);

	return count;
}

static ssize_t vi530x_offset_data_write(struct file *filp,
	struct kobject *kobj, struct bin_attribute *attr,
	char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct vi530x_data *data = dev_get_drvdata(dev);
	struct VI530X_OFFSET_Calib_Data *offset_data =
				(struct VI530X_OFFSET_Calib_Data *)buf;
	int rc = 0;

	mutex_lock(&data->work_mutex);
	if (!data->chip_enable) {
		rc = -EBUSY;
		vi530x_errmsg("can't set calib data while disable sensor\n");
		goto error;
	}

	if (count != sizeof(struct VI530X_OFFSET_Calib_Data))
		goto invalid;

	if(data->enable_debug)
		vi530x_errmsg("offset config: %d\n", offset_data->offset_cal);

	data->offset_config = offset_data->offset_cal;
	mutex_unlock(&data->work_mutex);
	return count;

invalid:
	vi530x_errmsg("invalid syntax");
	rc = -EINVAL;
	goto error;

error:
	mutex_unlock(&data->work_mutex);
	return rc;
}

static struct attribute *vi530x_attributes[] = {
	&dev_attr_chip_enable.attr,
	&dev_attr_enable_debug.attr,
	&dev_attr_chip_init.attr,
	&dev_attr_period.attr,
	&dev_attr_capture.attr,
	&dev_attr_xtalk_calib.attr,
	&dev_attr_offset_calib.attr,
	NULL,
};

static const struct attribute_group vi530x_attr_group = {
	.name = NULL,
	.attrs = vi530x_attributes,
};

static struct bin_attribute vi530x_xtalk_data_attr = {
	.attr = {
		.name = "xtalk_calib_data",
		.mode = 0664/*S_IWUGO | S_IRUGO*/,
	},
	.size = sizeof(struct VI530X_XTALK_Calib_Data),
	.read = vi530x_xtalk_data_read,
	.write = vi530x_xtalk_data_write,
};

static struct bin_attribute vi530x_offset_data_attr = {
	.attr = {
		.name = "offset_calib_data",
		.mode = 0664/*S_IWUGO | S_IRUGO*/,
	},
	.size = sizeof(struct VI530X_OFFSET_Calib_Data),
	.read = vi530x_offset_data_read,
	.write = vi530x_offset_data_write,
};

static irqreturn_t vi530x_irq_handler(int vec, void *info)
{
	struct vi530x_data *data = (struct vi530x_data *)info;
	VI530X_Error Status = VI530X_ERROR_NONE;

	if(!data || !data->fwdl_status)
		return IRQ_HANDLED;

	if (data->irq == vec)
	{
		if(xtalk_mark)
		{
			Status = vi530x_func_tbl->Get_XTalk_Parameter(data);
			if(Status != VI530X_ERROR_NONE)
				vi530x_errmsg("%d : Status = %d\n" , __LINE__, Status);
		}

		if(!xtalk_mark && !offset_mark)
		{
			Status = vi530x_func_tbl->Get_Measure_Data(data);
			if(Status != VI530X_ERROR_NONE)
			{
				vi530x_errmsg("%d : Status = %d\n" , __LINE__, Status);
				return IRQ_HANDLED;
			}
			input_report_abs(data->input_dev, INPUT_TOF, data->Rangedata.tof);
			input_report_abs(data->input_dev, INPUT_CONFIDENCE, data->Rangedata.confidence);
			input_report_abs(data->input_dev, INPUT_PEAK, data->Rangedata.peak);
			input_report_abs(data->input_dev, INPUT_NOISE, data->Rangedata.noise);
			input_report_abs(data->input_dev, INPUT_INTEGRAL_TIMES, data->Rangedata.integral_times);
			input_sync(data->input_dev);
		}
	}
	return IRQ_HANDLED;
}

static int vi530x_open(struct inode *inode, struct file *file)
{
	struct vi530x_data *data = container_of(file->private_data,
		struct vi530x_data, miscdev);

	vi530x_io_init(data);
	vi530x_errmsg("open!\n");
	return 0;
}

static long vi530x_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long rc=0;
	void __user *argp = (void __user *)arg;
	struct vi530x_data *data = container_of(file->private_data,
		struct vi530x_data, miscdev);

	if(!data)
		return -EFAULT;

	switch (cmd) {
		case VI530X_IOCTL_POWER_ON:
			mutex_lock(&data->work_mutex);
			vi530x_enable_irq(data);
			rc = vi530x_func_tbl->Power_ON(data);
			if(rc != VI530X_ERROR_NONE)
			{
				vi530x_errmsg("%d, CHIP POWER ON FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EIO;
			}
			getnstimeofday(&data->start_ts);
			mutex_unlock(&data->work_mutex);
			break;
		case VI530X_IOCTL_CHIP_INIT:
			mutex_lock(&data->work_mutex);
			rc = vi530x_func_tbl->Chip_Init(data);
			if(rc != VI530X_ERROR_NONE)
			{
				vi530x_errmsg("%d, CHIP INIT FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EIO;
			}
			data->fwdl_status = 1;
			mutex_unlock(&data->work_mutex);
			break;
		case VI530X_IOCTL_PERIOD:
			mutex_lock(&data->work_mutex);
			if (copy_from_user(&(data->period), (uint32_t *)argp, sizeof(uint32_t)))
			{
				vi530x_errmsg("%d, GET PERIOD DATA FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EFAULT;
			}
			if(data->enable_debug)
				vi530x_errmsg("period setting: %d\n", data->period);
			vi530x_func_tbl->Set_Period(data, data->period);
			mutex_unlock(&data->work_mutex);
			break;
		case VI530X_IOCTL_XTALK_CALIB:
			mutex_lock(&data->work_mutex);
			xtalk_mark = 1;
			rc = vi530x_func_tbl->Start_XTalk_Calibration(data);
			if(rc != VI530X_ERROR_NONE)
			{
				vi530x_errmsg("%d, PERFORM XTALK CALIB FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EINVAL;
			}
			msleep(800);
			if (copy_to_user(( struct VI530X_XTALK_Calib_Data *)argp, &(data->XtalkData),
				sizeof( struct VI530X_XTALK_Calib_Data)))
			{
				vi530x_errmsg("%d, COPY XTALK CALIB DATA FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EFAULT;
			}
			xtalk_mark = 0;
			mutex_unlock(&data->work_mutex);
			break;
		case VI530X_IOCTL_XTALK_CONFIG:
			mutex_lock(&data->work_mutex);
			if (copy_from_user(&(data->XtalkConfig), (struct VI530X_XTALK_Config_Data *)argp,
				sizeof(struct VI530X_XTALK_Config_Data)))
			{
				vi530x_errmsg("%d, GET XTALK CALIB DATA FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EFAULT;
			}

			if(data->enable_debug)
			{
				vi530x_errmsg("xtalk config: %d\n", data->XtalkConfig.xtalk_config);
				vi530x_errmsg("xtalk maxratio: %d\n", data->XtalkConfig.maxratio);
			}

			rc = vi530x_func_tbl->Config_XTalk_Parameter(data);
			if(rc != VI530X_ERROR_NONE)
			{
				vi530x_errmsg("%d, CONFIG XTALK PARAMETER FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EINVAL;
			}
			mutex_unlock(&data->work_mutex);
			break;
		case VI530X_IOCTL_OFFSET_CALIB:
			mutex_lock(&data->work_mutex);
			offset_mark = 1;
			if (copy_from_user(&(data->OffsetData.offset_mili), &(((struct VI530X_OFFSET_Calib_Data*)argp)->offset_mili),
				sizeof(int16_t)))
			{
				vi530x_errmsg("%d, GOPY OFFSET CALIB DATA FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EFAULT;
			}
			rc = vi530x_func_tbl->Start_Offset_Calibration(data);
			if (rc != VI530X_ERROR_NONE)
			{
				vi530x_errmsg("%d, PERFORM OFFSET CALIB FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EINVAL;
			}
			if (copy_to_user(&(((struct VI530X_OFFSET_Calib_Data *)argp)->offset_cal), &(data->OffsetData.offset_cal),
				sizeof(int16_t)))
			{
				vi530x_errmsg("%d, COPY OFFSET CALIB DATA FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EFAULT;
			}
			offset_mark = 0;
			data->offset_config = data->OffsetData.offset_cal;
			mutex_unlock(&data->work_mutex);
			break;
		case VI530X_IOCTL_OFFSET_CONFIG:
			mutex_lock(&data->work_mutex);
			if (copy_from_user(&(data->offset_config), (int16_t *)argp, sizeof(int16_t)))
			{
				vi530x_errmsg("%d, GET OFFSET CALIB DATA FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EFAULT;
			}

			if(data->enable_debug)
				vi530x_errmsg("offset config: %d\n", data->offset_config);

			mutex_unlock(&data->work_mutex);
			break;
		case VI530X_IOCTL_START:
			mutex_lock(&data->work_mutex);
			rc = vi530x_func_tbl->Start_Continuous_Measure(data);
			if(rc != VI530X_ERROR_NONE)
			{
				vi530x_errmsg("%d, CHIP START RANGE FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EIO;
			}
			mutex_unlock(&data->work_mutex);
			break;
		case VI530X_IOCTL_MZ_DATA:
			mutex_lock(&data->work_mutex);
			if (copy_to_user((struct VI530X_Measurement_Data *)argp, &(data->Rangedata),
				sizeof(struct VI530X_Measurement_Data)))
			{
				vi530x_errmsg("%d, COPY CONTINUOUS DATA FAIL\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EFAULT;
			}
			mutex_unlock(&data->work_mutex);
			break;
		case VI530X_IOCTL_STOP:
			mutex_lock(&data->work_mutex);
			rc = vi530x_func_tbl->Stop_Continuous_Measure(data);
			if(rc != VI530X_ERROR_NONE)
			{
				vi530x_errmsg("%d, CHIP STOP RANGE FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EIO;
			}
			mutex_unlock(&data->work_mutex);
			break;
		case VI530X_IOCTL_POWER_OFF:
			mutex_lock(&data->work_mutex);
			vi530x_disable_irq(data);
			data->fwdl_status = 0;
			rc = vi530x_func_tbl->Power_OFF(data);
			if(rc != VI530X_ERROR_NONE)
			{
				vi530x_errmsg("%d, CHIP POWER OFF FAILED\n", __LINE__);
				mutex_unlock(&data->work_mutex);
				return -EIO;
			}
			mutex_unlock(&data->work_mutex);
			break;
		default:
			rc = -EFAULT;
	}

	return rc;
}

static int vi530x_release(struct inode *inode, struct file *file)
{
	vi530x_errmsg("release!\n");
	struct vi530x_data *data = container_of(file->private_data,
		struct vi530x_data, miscdev);

	vi530x_io_release(data);

	return 0;
}

static const struct file_operations vi530x_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = vi530x_ioctl,
	.open = vi530x_open,
	.release = vi530x_release,
};

static int vi530x_parse_dt(struct device_node *np, struct  vi530x_data *data)
{

	if (!data || !np)
		return -EINVAL;

   vi530x_sensor_utils_parse_pm_ctrl_flag(np, data);

	data->pwren_gpio = of_get_named_gpio(np, "vi530x,pwren-gpio", 0);
	if(data->pwren_gpio < 0)
	{
		vi530x_errmsg("get pwren gpio: %d error\n", data->pwren_gpio);
		return -ENODEV;
	}
	vi530x_infomsg("INT GPIO: %d\n", data->pwren_gpio);

	data->irq_gpio = of_get_named_gpio(np, "vi530x,irq-gpio", 0);
	if(data->irq_gpio < 0)
	{
		vi530x_errmsg("get irq gpio: %d error\n", data->irq_gpio);
		return -ENODEV;
	}
	vi530x_infomsg("INT GPIO: %d\n", data->irq_gpio);

	data->xshut_gpio = of_get_named_gpio(np, "vi530x,xshut-gpio", 0);
	if(data->xshut_gpio < 0)
	{
		vi530x_errmsg("get xshut gpio: %d error\n",data->xshut_gpio);
		return -ENODEV;
	}
	vi530x_infomsg("XSHUT GPIO: %d\n", data->xshut_gpio);

	return  0;
}

static int vi530x_setup(struct  vi530x_data *data)
{
	int rc=0;
	int irq = 0;
	uint8_t buf = 0;
	uint8_t chipid[3] = {0};
	uint32_t ChipVersion = 0;

	if (!data)
		return -EINVAL;

	if (!gpio_is_valid(data->irq_gpio) || !gpio_is_valid(data->xshut_gpio) || !gpio_is_valid(data->pwren_gpio))
		return -ENODEV;

	gpio_request(data->pwren_gpio, "vi530x pwren gpio");
	gpio_request(data->xshut_gpio, "vi530x xshut gpio");
	gpio_request(data->irq_gpio, "vi530x irq gpio");
	gpio_direction_input(data->irq_gpio);
	irq = gpio_to_irq(data->irq_gpio);
	if(irq<0)
	{
		vi530x_errmsg("fail to map GPIO: %d to INT: %d\n", data->irq_gpio, irq);
		rc = -EINVAL;
		goto exit_free_gpio;
	} else {
		vi530x_dbgmsg("request irq: %d\n", irq);
		rc = request_threaded_irq(irq,  NULL, vi530x_irq_handler,
			IRQF_TRIGGER_FALLING |IRQF_ONESHOT, "vi530x_interrupt", (void *)data);
		if (rc) {
			vi530x_errmsg("%s(%d), Could not allocate VI530X_INT ! result:%d\n",__FUNCTION__, __LINE__, rc);
			goto exit_free_irq;
		}
	}
	data->irq = irq;
	data->intr_state = VI530X_INTR_DISABLED;
	disable_irq(data->irq);
	data->fwdl_status = 0;
	vi530x_func_tbl = &vi530x_api_func_tbl;
	vi530x_setupAPIFunctions();
	vi530x_func_tbl->Power_ON(data);
	vi530x_read_multibytes(data, VI530X_REG_CHIPID_BASE, chipid, 3);
	ChipVersion = (chipid[1] << 16) + (chipid[0] << 8) + chipid[2];
	vi530x_read_byte(data, VI530X_REG_DEV_ADDR, &buf);
	vi530x_func_tbl->Power_OFF(data);
	if(buf != VI530X_CHIP_ADDR)
	{
		vi530x_errmsg("VI530X I2C Transfer Failed, ChipVersion=0x%x, ChipAddr = 0x%x\n", ChipVersion, buf);
		rc = -EFAULT;
		goto exit_free_irq;
	}
	vi530x_infomsg("VI530X I2C Transfer Successfully, ChipVersion=0x%x, ChipAddr = 0x%x\n", ChipVersion, buf);

	data->input_dev = input_allocate_device();
	if (data->input_dev == NULL) {
		vi530x_errmsg("Error allocating input_dev.\n");
		goto input_dev_alloc_err;
	}
	data->input_dev->name = "vi530x";
	data->input_dev->id.bustype = BUS_I2C;
	input_set_drvdata(data->input_dev, data);
	set_bit(EV_ABS, data->input_dev->evbit);
	input_set_abs_params(data->input_dev, INPUT_TOF, 0, 0xffffffff, 0, 0);
	input_set_abs_params(data->input_dev, INPUT_CONFIDENCE, 0, 0xff, 0, 0);
	input_set_abs_params(data->input_dev, INPUT_PEAK, 0, 0xffffffff, 0, 0);
	input_set_abs_params(data->input_dev, INPUT_NOISE, 0, 0xffffffff, 0, 0);
	input_set_abs_params(data->input_dev, INPUT_INTEGRAL_TIMES, 0, 0xffffffff, 0, 0);
	rc = input_register_device(data->input_dev);
	if(rc) {
		vi530x_errmsg("Error registering input_dev.\n");
		goto input_reg_err;
	}
	rc = sysfs_create_group(&data->input_dev->dev.kobj, &vi530x_attr_group);
	if (rc) {
		vi530x_errmsg("Error creating sysfs attribute group.\n");
		goto sysfs_create_group_err;
	}
	rc = sysfs_create_bin_file(&data->input_dev->dev.kobj, &vi530x_xtalk_data_attr);
	if (rc) {
		rc = -ENOMEM;
		vi530x_errmsg("%d error:%d\n", __LINE__, rc);
		goto sysfs_create_bin_err1;
	}
	rc = sysfs_create_bin_file(&data->input_dev->dev.kobj, &vi530x_offset_data_attr);
	if (rc) {
		rc = -ENOMEM;
		vi530x_errmsg("%d error:%d\n", __LINE__, rc);
		goto sysfs_create_bin_err2;
	}

	data->miscdev.minor = MISC_DYNAMIC_MINOR;
	data->miscdev.name = "vi530x";
	data->miscdev.fops = &vi530x_fops;
	if (misc_register(&data->miscdev) != 0)
	{
		vi530x_errmsg("Could not register misc. dev for VI530X Sensor\n");
		rc = -ENOMEM;
		goto misc_register_err;
	}
	data->period = 30;
	data->xtalk_config = 0;
	data->offset_config = 0;
	data->enable_debug = 0;

	return 0;

misc_register_err:
	sysfs_remove_bin_file(&data->input_dev->dev.kobj,
		&vi530x_offset_data_attr);
sysfs_create_bin_err2:
	sysfs_remove_bin_file(&data->input_dev->dev.kobj,
		&vi530x_xtalk_data_attr);
sysfs_create_bin_err1:
	sysfs_remove_group(&data->input_dev->dev.kobj,
		&vi530x_attr_group);
sysfs_create_group_err:
	input_unregister_device(data->input_dev);
input_reg_err:
	input_free_device(data->input_dev);
input_dev_alloc_err:
exit_free_irq:
	free_irq(irq, data);
exit_free_gpio:
	gpio_free(data->xshut_gpio);
	gpio_free(data->irq_gpio);
	return rc;
}

static int vi530x_probe(struct i2c_client *client)
{
	struct vi530x_data *vi530x_data = NULL;
	struct device *dev = &client->dev;
	struct device_node *node;
	int ret  = 0;

	vi530x_infomsg("Enter!!\n");
	vi530x_data = kzalloc(sizeof(struct vi530x_data), GFP_KERNEL);
	if(!vi530x_data)
	{
		vi530x_errmsg("devm_kzalloc error\n");
		return -ENOMEM;
	}
	if (!dev->of_node)
	{
		kfree(vi530x_data);
		return -EINVAL;
	}
	/* setup device data */
	vi530x_data->dev_name = dev_name(&client->dev);
	vi530x_data->client = client;
	vi530x_data->dev = dev;
	node = dev->of_node;
	i2c_set_clientdata(client, vi530x_data);
	mutex_init(&vi530x_data->work_mutex);
	ret = vi530x_parse_dt(node, vi530x_data);
	vi530x_io_init(vi530x_data);

	if(ret) {
		vi530x_errmsg("VI530X Parse DT Failed\n");
		goto exit_error;
	}
	ret = vi530x_setup(vi530x_data);
	if(ret) {
		vi530x_errmsg("VI530X Setup Failed\n");
		goto exit_error;
	}
	vi530x_io_release(vi530x_data);

	return 0;

exit_error:
	vi530x_io_release(vi530x_data);
	mutex_destroy(&vi530x_data->work_mutex);
	i2c_set_clientdata(client, NULL);
	kfree(vi530x_data);
	return ret;

}

static void vi530x_remove(struct i2c_client *client)
{
	struct vi530x_data *data = i2c_get_clientdata(client);

	if(data->input_dev)
	{
		vi530x_dbgmsg("to unregister sysfs dev\n");
		sysfs_remove_group(&data->input_dev->dev.kobj,
			&vi530x_attr_group);
		sysfs_remove_bin_file(&data->input_dev->dev.kobj,
			&vi530x_xtalk_data_attr);
		sysfs_remove_bin_file(&data->input_dev->dev.kobj,
			&vi530x_offset_data_attr);
		vi530x_dbgmsg("to unregister input dev\n");
		input_unregister_device(data->input_dev);
	}
	if (!IS_ERR(data->miscdev.this_device) &&
			data->miscdev.this_device != NULL) {
		vi530x_dbgmsg("to unregister misc dev\n");
		misc_deregister(&data->miscdev);
	}
	if(data->xshut_gpio)
	{
		gpio_direction_output(data->xshut_gpio, 0);
		gpio_free(data->xshut_gpio);
	}
	if(data->irq_gpio)
	{
		free_irq(data->irq, data);
		gpio_free(data->irq_gpio);
	}
	if(data->pwren_gpio)
	{
		gpio_direction_output(data->pwren_gpio, 0);
		gpio_free(data->pwren_gpio);
	}

	i2c_set_clientdata(client, NULL);
	mutex_destroy(&data->work_mutex);
	kfree(data);
}
static const struct i2c_device_id vi530x_id[] = {
	{ VI530X_DRV_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, vi530x_id);

static const struct of_device_id vi530x_dt_match[] = {
	{.compatible = "evisionics,vi530x",},
	{},
};
MODULE_DEVICE_TABLE(of, vi530x_dt_match);

struct i2c_driver vi530x_driver = {
	.driver  = {
		.name = VI530X_DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = vi530x_dt_match,
	},
	.probe = vi530x_probe,
	.remove = vi530x_remove,
	.id_table = vi530x_id,
};

int vi530x_init(void)
{
	return i2c_add_driver(&vi530x_driver);
}
void vi530x_exit(void)
{
	i2c_del_driver(&vi530x_driver);
}

//module_init(vi530x_init);
//module_exit(vi530x_exit);

MODULE_AUTHOR("William.li<william.li@vidar.ai>");
MODULE_DESCRIPTION("VI530X FlightSense TOF  sensor Driver");
MODULE_LICENSE("GPL");
