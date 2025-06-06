// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
 //ADJ_APERTURE
//adj_aperture
#include "cam_adj_aperture_dev.h"
#include "cam_req_mgr_dev.h"
#include "cam_adj_aperture_soc.h"
#include "cam_adj_aperture_core.h"
#include "cam_trace.h"
#include "camera_main.h"
#include "cam_compat.h"
#include "cam_mem_mgr_api.h"


#include <linux/regulator/driver.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#define COMPSIZE   64
struct cam_adj_aperture_ctrl_t  *pa_ctrl = NULL;


typedef enum  ADJSTATE
{
    ADJSTATEMAX = 1,//open max
    ADJSTATEMID,//open mid
    ADJSTATEMIN,//open min
    ADJSTATECLOSE = 0x1000,//close
    ADJSTATEOPEN = 0x1001,//open
}ADJSTATE;


static int  find_compatible_for_platform_device(struct platform_device *pdev, char *pcompatible) {
    struct device_node *np = pdev->dev.of_node;
    const char *tempcompatible = NULL;

    if (!np) {
        CAM_ERR(CAM_ACTUATOR, "aperture Device has no device tree node");
        return -ENODEV;
    }

    tempcompatible = of_get_property(np, "compatible", NULL);
    if (!tempcompatible) {
        CAM_ERR(CAM_ACTUATOR, "aperture Device is missing compatible property");
        return -EINVAL;
    }
    memcpy(pcompatible, tempcompatible, COMPSIZE);

    CAM_DBG(CAM_ACTUATOR, "aperture Compatible name: %s", tempcompatible);

    return 0;
}

static ssize_t camera_open_adj_fnum(void){
    int ret = 0;

    gpio_direction_output(54+512, 1); //gpio+offset
    msleep(2);
    gpio_direction_output(56+512, 1);
    msleep(2);
    gpio_direction_output(153+512, 1);
    msleep(200);
    ret = camera_io_init(&pa_ctrl->io_master_info);
    if (ret < 0) {
        CAM_ERR(CAM_ACTUATOR, "aperture  cci init failed: rc: %d", ret);
    }

    return  0;
}

static ssize_t camera_close_adj_fnum(void){

    msleep(200);
    gpio_direction_output(54+512, 0);
    msleep(2);
    gpio_direction_output(56+512, 0);
    msleep(2);
    gpio_direction_output(153+512, 0);
    camera_io_release(&pa_ctrl->io_master_info);

    return  0;
}

static ssize_t camera_adj_aperture_stream_on(void){
    int ret = 0;

    struct cam_sensor_i2c_reg_array adj_setting = {
        .reg_addr = 0x00,
        .reg_data = 0x01,
        .delay = 0x00U,
        .data_mask = 0x00U,
    };

    struct cam_sensor_i2c_reg_setting adj_write_setting = {
    .size = 1,
    .reg_setting = &adj_setting,
    .addr_type = CAMERA_SENSOR_I2C_TYPE_WORD,
    .data_type = CAMERA_SENSOR_I2C_TYPE_DWORD,
    .delay = 0,
    };

    ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);
    msleep(10);

    return  ret;
}

static ssize_t camera_adj_aperture_stream_off(void){
    int ret = 0;

    struct cam_sensor_i2c_reg_array adj_setting = {
        .reg_addr = 0x00,
        .reg_data = 0x00,
        .delay = 0x00U,
        .data_mask = 0x00U,
    };

    struct cam_sensor_i2c_reg_setting adj_write_setting = {
    .size = 1,
    .reg_setting = &adj_setting,
    .addr_type = CAMERA_SENSOR_I2C_TYPE_WORD,
    .data_type = CAMERA_SENSOR_I2C_TYPE_DWORD,
    .delay = 0,
    };
    msleep(200);
    ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);

    return  0;
}

static ssize_t camera_adj_aperture_switch_state(unsigned dload){
    int ret = 0;

    struct cam_sensor_i2c_reg_array adj_setting = {
        .reg_addr = 0x10,
        .reg_data = 0x00,
        .delay = 0x00U,
        .data_mask = 0x00U,
    };

    struct cam_sensor_i2c_reg_setting adj_write_setting = {
    .size = 1,
    .reg_setting = &adj_setting,
    .addr_type = CAMERA_SENSOR_I2C_TYPE_WORD,
    .data_type = CAMERA_SENSOR_I2C_TYPE_DWORD,
    .delay = 0,
    };

    switch(dload)
    {
        case ADJSTATEMAX:
        adj_setting.reg_addr = 0x10; //open max
        adj_setting.reg_data = 0xFFF;
        ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);
        break;
        case ADJSTATEMID:
        adj_setting.reg_addr = 0x10; //open mid
        adj_setting.reg_data = 0x800;
        ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);
        break;
        break;
        case  ADJSTATEMIN:
        adj_setting.reg_addr = 0x10; //open min
        adj_setting.reg_data = 0x00;
        ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);
        break;
    }

    return  ret;
}

static int  camera_state = ADJSTATEMAX;
static ssize_t camera_switch_adj_fnum_state( unsigned dload){
    int ret = 0;

    pa_ctrl->io_master_info.qup_client->i2c_client->addr = 0x6c;
    camera_open_adj_fnum();

    CAM_DBG(CAM_ACTUATOR, "aperture dload%d", dload);
    camera_adj_aperture_stream_on();
    ret = camera_adj_aperture_switch_state(dload);
    camera_adj_aperture_stream_off();

    camera_close_adj_fnum();

    return ret;
}

static ssize_t camera_switch_gpio_rgltr_disable(void){
    struct device_node *np = NULL;
    struct platform_device *pdev = NULL;
    struct regulator *regu= NULL;
    int ret;

    return  0;
    np = of_find_node_by_name(NULL, "qcom,cam-sensor0");
    if (np) {
        pdev = of_find_device_by_node(np);
        of_node_put(np);
        if (pdev) {
            regu = regulator_get(&pdev->dev, "cam_vio");
            if (regu) {
                ret = regulator_disable(regu);
            }

            regu = regulator_get(&pdev->dev, "cam_v_custom1");
            if (regu) {
                ret = regulator_disable(regu);
            }
        }else{
            CAM_ERR(CAM_SENSOR, "zte add pdev of Node cam-sensor not found");
            return -EPROBE_DEFER;
        }
    }else{
        CAM_ERR(CAM_SENSOR, "zte add cam-sensor not found");
        return -EPROBE_DEFER;
    }

    return 0;
}

static ssize_t camera_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", camera_state);
}

static ssize_t camera_enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t size)
{
    unsigned int dload = 0;

    if (sscanf(buf, "%u", &dload) !=1)
        return -EINVAL;

    if (dload){
        if(!camera_switch_adj_fnum_state(dload))
        {
            camera_state = dload;
        }else
        {
             camera_state = -1;
             CAM_ERR(CAM_ACTUATOR, "aperture  write failed");
        }

    } else {
        camera_switch_gpio_rgltr_disable();
        //camera_state = 0;
    }
    return strnlen(buf, size);
}

static struct kobj_attribute camera_regulator_attr    = __ATTR(camera_adj,   0664, camera_enable_show,   camera_enable_store);

static struct attribute *peripheral_sw_attrs[] = {
    &camera_regulator_attr.attr,
    NULL,
};

static struct attribute_group peripheral_sw_attr_group = {
    .attrs = peripheral_sw_attrs,
};

struct kobject *peripheral_sw_kobj;

int   camera_switch_init(void)
{
    int rc = 0;

    peripheral_sw_kobj = kobject_create_and_add("adj_sw", NULL);
    if (!peripheral_sw_kobj)
    {
        CAM_ERR(CAM_SENSOR, "zte   adj_sw NULL");
        return -ENOMEM;
    }
    rc = sysfs_create_group(peripheral_sw_kobj,&peripheral_sw_attr_group);
    if(rc)
    {
        CAM_ERR(CAM_SENSOR, "zte   rc fail");
    }

    return rc;
}

static void   camera_switch_exit(void)
{
    sysfs_remove_group(peripheral_sw_kobj,&peripheral_sw_attr_group);
    kobject_put(peripheral_sw_kobj);
}

#if  0
int cam_adj_aperture_driver_init(void)
{

    camera_switch_init();

    return 0;
}

void cam_adj_aperture_driver_exit(void)
{
    camera_switch_exit();
}
#endif
static struct cam_i3c_actuator_data {
	struct cam_adj_aperture_ctrl_t                  *a_ctrl;
	struct completion                            probe_complete;
} g_i3c_actuator_data[MAX_CAMERAS];

struct completion *cam_adj_aperture_get_i3c_completion(uint32_t index)
{
	return &g_i3c_actuator_data[index].probe_complete;
}

static int cam_actuator_subdev_close_internal(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	struct cam_adj_aperture_ctrl_t *a_ctrl =
		v4l2_get_subdevdata(sd);

	if (!a_ctrl) {
		CAM_ERR(CAM_ACTUATOR, "a_ctrl ptr is NULL");
		return -EINVAL;
	}

	mutex_lock(&(a_ctrl->actuator_mutex));
	cam_adj_aperture_shutdown(a_ctrl);
	mutex_unlock(&(a_ctrl->actuator_mutex));

	return 0;
}

static int cam_actuator_subdev_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	bool crm_active = cam_req_mgr_is_open();

	if (crm_active) {
		CAM_DBG(CAM_ACTUATOR,
			"CRM is ACTIVE, close should be from CRM");
		return 0;
	}

	return cam_actuator_subdev_close_internal(sd, fh);
}

static long cam_actuator_subdev_ioctl(struct v4l2_subdev *sd,
	unsigned int cmd, void *arg)
{
	int rc = 0;
	struct cam_adj_aperture_ctrl_t *a_ctrl =
		v4l2_get_subdevdata(sd);

	switch (cmd) {
	case VIDIOC_CAM_CONTROL:
		rc = cam_adj_aperture_driver_cmd(a_ctrl, arg);
		if (rc) {
			if (rc == -EBADR)
				CAM_INFO(CAM_ACTUATOR,
					"Failed for driver_cmd: %d, it has been flushed",
					rc);
			else
				CAM_ERR(CAM_ACTUATOR,
					"Failed for driver_cmd: %d", rc);
		}
		break;
	case CAM_SD_SHUTDOWN:
		if (!cam_req_mgr_is_shutdown()) {
			CAM_ERR(CAM_CORE, "SD shouldn't come from user space");
			return 0;
		}

		rc = cam_actuator_subdev_close_internal(sd, NULL);
		break;
	default:
		CAM_ERR(CAM_ACTUATOR, "Invalid ioctl cmd: %u", cmd);
		rc = -ENOIOCTLCMD;
		break;
	}
	return rc;
}

#ifdef CONFIG_COMPAT
static long cam_actuator_init_subdev_do_ioctl(struct v4l2_subdev *sd,
	unsigned int cmd, unsigned long arg)
{
	struct cam_control cmd_data;
	int32_t rc = 0;

	if (copy_from_user(&cmd_data, (void __user *)arg,
		sizeof(cmd_data))) {
		CAM_ERR(CAM_ACTUATOR,
			"Failed to copy from user_ptr=%pK size=%zu",
			(void __user *)arg, sizeof(cmd_data));
		return -EFAULT;
	}

	switch (cmd) {
	case VIDIOC_CAM_CONTROL:
		cmd = VIDIOC_CAM_CONTROL;
		rc = cam_actuator_subdev_ioctl(sd, cmd, &cmd_data);
		if (rc) {
			CAM_ERR(CAM_ACTUATOR,
				"Failed in actuator subdev handling rc: %d",
				rc);
			return rc;
		}
		break;
	default:
		CAM_ERR(CAM_ACTUATOR, "Invalid compat ioctl: %d", cmd);
		rc = -ENOIOCTLCMD;
		break;
	}

	if (!rc) {
		if (copy_to_user((void __user *)arg, &cmd_data,
			sizeof(cmd_data))) {
			CAM_ERR(CAM_ACTUATOR,
				"Failed to copy to user_ptr=%pK size=%zu",
				(void __user *)arg, sizeof(cmd_data));
			rc = -EFAULT;
		}
	}
	return rc;
}
#endif

static struct v4l2_subdev_core_ops cam_actuator_subdev_core_ops = {
	.ioctl = cam_actuator_subdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = cam_actuator_init_subdev_do_ioctl,
#endif
};

static struct v4l2_subdev_ops cam_actuator_subdev_ops = {
	.core = &cam_actuator_subdev_core_ops,
};

static const struct v4l2_subdev_internal_ops cam_actuator_internal_ops = {
	.close = cam_actuator_subdev_close,
};


static int cam_actuator_init_subdev(struct cam_adj_aperture_ctrl_t *a_ctrl)
{
	int rc = 0;

	a_ctrl->v4l2_dev_str.internal_ops =
		&cam_actuator_internal_ops;
	a_ctrl->v4l2_dev_str.ops =
		&cam_actuator_subdev_ops;
	strlcpy(a_ctrl->device_name, CAMX_ADJ_APERTURE_DEV_NAME,
		sizeof(a_ctrl->device_name));
	a_ctrl->v4l2_dev_str.name =
		a_ctrl->device_name;
	a_ctrl->v4l2_dev_str.sd_flags =
		(V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS);
	a_ctrl->v4l2_dev_str.ent_function =
		CAM_ACTUATOR_DEVICE_TYPE;
	a_ctrl->v4l2_dev_str.token = a_ctrl;
	a_ctrl->v4l2_dev_str.close_seq_prior =
		 CAM_SD_CLOSE_MEDIUM_PRIORITY;

	rc = cam_register_subdev(&(a_ctrl->v4l2_dev_str));
	if (rc)
		CAM_ERR(CAM_ACTUATOR,
			"Fail with cam_register_subdev rc: %d", rc);

	return rc;
}

int  aperture_open_count = 0;
struct mutex adj_aperture_mutex;

static int adj_aperture_open(struct inode *inode, struct file *file)
{
    struct cam_adj_aperture_ctrl_t *a_ctrl = container_of(file->private_data,
    struct cam_adj_aperture_ctrl_t, miscdev);

    mutex_lock(&adj_aperture_mutex);
    if(aperture_open_count == 0)
    {
        camera_open_zte_adj_aperture(a_ctrl);
    }
    aperture_open_count++;
    mutex_unlock(&adj_aperture_mutex);

    return 0;
}

static int adj_aperture_release(struct inode *inode, struct file *file)
{
    struct cam_adj_aperture_ctrl_t *a_ctrl = container_of(file->private_data,
    struct cam_adj_aperture_ctrl_t, miscdev);

    mutex_lock(&adj_aperture_mutex);
    aperture_open_count--;
    if(aperture_open_count == 0)
    {
        CAM_INFO(CAM_ACTUATOR,"release");
        cam_zte_adj_aperture_thread_resources_close(a_ctrl);
        camera_close_zte_adj_aperture(a_ctrl);
    }
    mutex_unlock(&adj_aperture_mutex);

    return 0;
}

static long cam_adj_aperture_subdev_ioctl(struct cam_adj_aperture_ctrl_t *a_ctrl,
	unsigned int cmd, void *arg)
{
	int rc = 0;

	switch (cmd) {
	case VIDIOC_CAM_CONTROL:
		rc = cam_zte_adj_aperture_driver_cmd(a_ctrl, arg);
		if (rc) {
			if (rc == -EBADR)
				CAM_INFO(CAM_ACTUATOR,
					"Failed for driver_cmd: %d, it has been flushed",
					rc);
			else
				CAM_ERR(CAM_ACTUATOR,
					"Failed for driver_cmd: %d", rc);
		}
		break;
	default:
		CAM_ERR(CAM_ACTUATOR, "Invalid ioctl cmd: %u", cmd);
		rc = -ENOIOCTLCMD;
		break;
	}
	return rc;
}

static long adj_aperture_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long rc=0;
    struct cam_adj_aperture_ctrl_t *a_ctrl = container_of(file->private_data,
        struct cam_adj_aperture_ctrl_t, miscdev);
    struct cam_control cmd_data = {0};

    if(!a_ctrl)
        return -EFAULT;

    if (copy_from_user(&cmd_data, (void __user *)arg,
        sizeof(cmd_data))) {
            CAM_ERR(CAM_ACTUATOR,
            "Failed to copy from user_ptr=%pK size=%zu",
            (void __user *)arg, sizeof(cmd_data));
        return -EFAULT;
    }

    switch (cmd) {
    case VIDIOC_CAM_CONTROL:
        cmd = VIDIOC_CAM_CONTROL;
        rc = cam_adj_aperture_subdev_ioctl(a_ctrl, cmd, &cmd_data);
        if (rc) {
            CAM_ERR(CAM_ACTUATOR,
                "Failed in actuator subdev handling rc: %d",
                rc);
            return rc;
        }
        break;
    default:
        CAM_ERR(CAM_ACTUATOR, "Invalid compat ioctl: %d", cmd);
        rc = -ENOIOCTLCMD;
        break;
    }

    return rc;
}

static const struct file_operations adj_aperture_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = adj_aperture_ioctl,
    .open = adj_aperture_open,
    .release = adj_aperture_release,
};


static int cam_adj_aperture_i2c_component_bind(struct device *dev,
	struct device *master_dev, void *data)
{
	int32_t                          rc = 0;
	int32_t                          i = 0;
	struct i2c_client               *client;
	struct cam_adj_aperture_ctrl_t      *a_ctrl;
	struct cam_hw_soc_info          *soc_info = NULL;
	struct cam_adj_aperture_soc_private *soc_private = NULL;
    struct platform_device *pdev = to_platform_device(dev);
    char  comp_name[COMPSIZE] = {0};


	client = container_of(dev, struct i2c_client, dev);
	if (!client) {
		CAM_ERR(CAM_ACTUATOR,
			"Failed to get i2c client");
		return -EFAULT;
	}

	/* Create sensor control structure */
	a_ctrl = CAM_MEM_ZALLOC(sizeof(*a_ctrl), GFP_KERNEL);
	if (!a_ctrl)
		return -ENOMEM;

	a_ctrl->io_master_info.qup_client = CAM_MEM_ZALLOC(sizeof(
		struct cam_sensor_qup_client), GFP_KERNEL);
	if (!(a_ctrl->io_master_info.qup_client)) {
		rc = -ENOMEM;
		goto free_ctrl;
	}

	i2c_set_clientdata(client, a_ctrl);

	soc_private = CAM_MEM_ZALLOC(sizeof(struct cam_adj_aperture_soc_private),
		GFP_KERNEL);
	if (!soc_private) {
		rc = -ENOMEM;
		goto free_qup;
	}
	a_ctrl->soc_info.soc_private = soc_private;

	a_ctrl->io_master_info.qup_client->i2c_client = client;
	soc_info = &a_ctrl->soc_info;
	soc_info->dev = &client->dev;
	soc_info->dev_name = client->name;
	a_ctrl->io_master_info.master_type = I2C_MASTER;

	rc = cam_cam_adj_aperture_parse_dt(a_ctrl, &client->dev);
	if (rc < 0) {
		CAM_ERR(CAM_ACTUATOR, "failed: cam_sensor_parse_dt rc %d", rc);
		goto free_soc;
	}

	if (rc)
		goto free_soc;
#if  0
	if (soc_private->i2c_info.slave_addr != 0)
		a_ctrl->io_master_info.client->addr =
			soc_private->i2c_info.slave_addr;
#endif
	a_ctrl->i2c_data.per_frame =
		CAM_MEM_ZALLOC(sizeof(struct i2c_settings_array) *
		MAX_PER_FRAME_ARRAY, GFP_KERNEL);
	if (a_ctrl->i2c_data.per_frame == NULL) {
		rc = -ENOMEM;
		goto unreg_subdev;
	}


	cam_sensor_module_add_i2c_device((void *) a_ctrl, CAM_SENSOR_ACTUATOR);

	INIT_LIST_HEAD(&(a_ctrl->i2c_data.init_settings.list_head));

	for (i = 0; i < MAX_PER_FRAME_ARRAY; i++)
		INIT_LIST_HEAD(&(a_ctrl->i2c_data.per_frame[i].list_head));

	a_ctrl->bridge_intf.device_hdl = -1;
	a_ctrl->bridge_intf.link_hdl = -1;
	a_ctrl->bridge_intf.ops.get_dev_info =
		cam_adj_aperture_publish_dev_info;
	a_ctrl->bridge_intf.ops.link_setup =
		cam_adj_aperture_establish_link;
	a_ctrl->bridge_intf.ops.apply_req =
		cam_adj_aperture_apply_request;
	a_ctrl->last_flush_req = 0;
	a_ctrl->cam_act_state = CAM_ADJ_APERTURE_INIT;

    find_compatible_for_platform_device(pdev, comp_name);
    CAM_ERR(CAM_ACTUATOR, "conglin5 comp_name%s strcmp %d", comp_name, strcmp(comp_name, "zte,cam-i2c-adj_fnum"));
    if(!strcmp(comp_name, "zte,cam-i2c-adj_fnum"))
    {
        pa_ctrl = a_ctrl;
        CAM_ERR(CAM_ACTUATOR, "conglin12 %p", pa_ctrl);
    }

    a_ctrl->miscdev.minor = MISC_DYNAMIC_MINOR;
    a_ctrl->miscdev.name = "v4l-subdev_aperture";
    a_ctrl->miscdev.fops = &adj_aperture_fops;
    if (misc_register(&a_ctrl->miscdev) != 0)
    {
        CAM_ERR(CAM_ACTUATOR, "misc_register  fail");
        goto unreg_subdev;
    }

    aperture_open_count = 0;
    mutex_init(&adj_aperture_mutex);

	return rc;

unreg_subdev:
	cam_unregister_subdev(&(a_ctrl->v4l2_dev_str));
free_soc:
	CAM_MEM_FREE(soc_private);
free_qup:
	CAM_MEM_FREE(a_ctrl->io_master_info.qup_client);
free_ctrl:
	CAM_MEM_FREE(a_ctrl);
	return rc;
	rc = cam_actuator_init_subdev(a_ctrl);
}

static void cam_adj_aperture_i2c_component_unbind(struct device *dev,
	struct device *master_dev, void *data)
{
	struct i2c_client               *client = NULL;
	struct cam_adj_aperture_ctrl_t      *a_ctrl = NULL;

	client = container_of(dev, struct i2c_client, dev);
	if (!client) {
		CAM_ERR(CAM_ACTUATOR,
			"Failed to get i2c client");
		return;
	}

	a_ctrl = i2c_get_clientdata(client);
	/* Handle I2C Devices */
	if (!a_ctrl) {
		CAM_ERR(CAM_ACTUATOR, "Actuator device is NULL");
		return;
	}

    if (!IS_ERR(a_ctrl->miscdev.this_device) &&
            a_ctrl->miscdev.this_device != NULL) {
        misc_deregister(&a_ctrl->miscdev);
    }

	CAM_INFO(CAM_ACTUATOR, "i2c remove invoked");
	mutex_lock(&(a_ctrl->actuator_mutex));
	cam_adj_aperture_shutdown(a_ctrl);
	mutex_unlock(&(a_ctrl->actuator_mutex));
	cam_unregister_subdev(&(a_ctrl->v4l2_dev_str));

	/*Free Allocated Mem */
	CAM_MEM_FREE(a_ctrl->i2c_data.per_frame);
	a_ctrl->i2c_data.per_frame = NULL;
	a_ctrl->soc_info.soc_private = NULL;
	v4l2_set_subdevdata(&a_ctrl->v4l2_dev_str.sd, NULL);
	CAM_MEM_FREE(a_ctrl);
}

const static struct component_ops cam_adj_aperture_i2c_component_ops = {
	.bind = cam_adj_aperture_i2c_component_bind,
	.unbind = cam_adj_aperture_i2c_component_unbind,
};

#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
static int cam_adj_aperture_driver_i2c_probe(struct i2c_client *client)
{
	int rc = 0;

	if (client == NULL) {
		CAM_ERR(CAM_ACTUATOR, "Invalid Args client: %pK",
			client);
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		CAM_ERR(CAM_ACTUATOR, "%s :: i2c_check_functionality failed",
			 client->name);
		return -EFAULT;
	}

	CAM_ERR(CAM_ACTUATOR, "Adding sensor actuator component");
	//rc = component_add(&client->dev, &cam_adj_aperture_i2c_component_ops);
     cam_adj_aperture_i2c_component_bind(&client->dev, NULL, NULL);
	if (rc)
		CAM_ERR(CAM_ACTUATOR, "failed to add component rc: %d", rc);

	return rc;
}
#else
static int32_t cam_adj_aperture_driver_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;

	if (client == NULL || id == NULL) {
		CAM_ERR(CAM_ACTUATOR, "Invalid Args client: %pK id: %pK",
			client, id);
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		CAM_ERR(CAM_ACTUATOR, "%s :: i2c_check_functionality failed",
			 client->name);
		return -EFAULT;
	}

	CAM_ERR(CAM_ACTUATOR, "Adding sensor actuator component");
	//rc = component_add(&client->dev, &cam_adj_aperture_i2c_component_ops);
   cam_adj_aperture_i2c_component_bind(&client->dev, NULL, NULL);
	if (rc)
		CAM_ERR(CAM_ACTUATOR, "failed to add component rc: %d", rc);

	return rc;
}
#endif

#if KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE
void cam_adj_aperture_driver_i2c_remove(
	struct i2c_client *client)
{
    cam_adj_aperture_i2c_component_unbind(&client->dev, NULL, NULL);
	//component_del(&client->dev, &cam_adj_aperture_i2c_component_ops);
}
#else
static int32_t cam_adj_aperture_driver_i2c_remove(
	struct i2c_client *client)
{
    cam_adj_aperture_i2c_component_unbind(&client->dev, NULL, NULL);
	//component_del(&client->dev, &cam_adj_aperture_i2c_component_ops);
	return 0;
}
#endif


static const struct i2c_device_id i2c_id[] = {
	{ADJ_APERTURE_DRIVER_I2C, (kernel_ulong_t)NULL},
	{ }
};

static const struct of_device_id cam_adj_aperture_i2c_driver_dt_match[] = {
    {.compatible = "zte,cam-i2c-adj_fnum"},
	{}
};

MODULE_DEVICE_TABLE(of, cam_adj_aperture_i2c_driver_dt_match);

struct i2c_driver cam_adj_aperture_i2c_driver = {
	.id_table = i2c_id,
	.probe  = cam_adj_aperture_driver_i2c_probe,
	.remove = cam_adj_aperture_driver_i2c_remove,
	.driver = {
		.of_match_table = cam_adj_aperture_i2c_driver_dt_match,
		.owner = THIS_MODULE,
		.name = ADJ_APERTURE_DRIVER_I2C,
		.suppress_bind_attrs = true,
	},
};

int cam_adj_aperture_driver_init(void)
{
    int32_t rc = 0;

    camera_switch_init();

    rc = i2c_add_driver(&cam_adj_aperture_i2c_driver);
    if (rc) {
        CAM_ERR(CAM_ACTUATOR, "i2c_add_driver failed rc = %d", rc);
        camera_switch_exit();
    }

    camera_zte_aperture_init_kfifo();

    return rc;
}

void cam_adj_aperture_driver_exit(void)
{
    camera_switch_exit();
    i2c_del_driver(&cam_adj_aperture_i2c_driver);
}

MODULE_DESCRIPTION("cam_adj_aperture_driver");
MODULE_LICENSE("GPL v2");
