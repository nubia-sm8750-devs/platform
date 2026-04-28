// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "cam_var_aperture_dev.h"
#include "cam_req_mgr_dev.h"
#include "cam_var_aperture_soc.h"
#include "cam_var_aperture_core.h"
#include "cam_trace.h"
#include "camera_main.h"
#include "cam_compat.h"
#include "cam_mem_mgr_api.h"

#include <linux/regulator/driver.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#define COMPSIZE   64
static struct cam_var_aperture_ctrl_t  *pa_ctrl = NULL;

static unsigned long long aperture_adcvalue = 0xFFF;
static int aperture_test_flag = 0;

module_param(aperture_adcvalue, ullong, 0644);

typedef enum  ADJSTATE
{
    ADJSTATEMAX = 1,//open max
    ADJSTATEMID,//open mid
    ADJSTATEMIN,//open min
    ADJSTATEDEFAULT,// default
    ADJSTATECLOSE,//close
    ADJSTATESTREAMON,//enable
    ADJSTATEOPEN,//open
    ADJSTATEFIXMIN = 0x10,
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
    msleep(330);
    ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);

    return  0;
}

static ssize_t camera_close_adj_fnum(void){
    camera_adj_aperture_stream_off();
    gpio_direction_output(153+512, 0);
    msleep(2);
    gpio_direction_output(54+512, 0);
    msleep(2);


    uint32_t gpio_avdd = 0;
    of_property_read_u32(pa_ctrl->soc_info.dev->of_node, "gpio-avdd", &gpio_avdd);
    gpio_direction_output(gpio_avdd+512, 0);
    gpio_free(gpio_avdd+512);

    camera_io_release(&pa_ctrl->io_master_info);

    return  0;
}

static ssize_t camera_open_adj_fnum(void){
    int ret = 0;

    pa_ctrl->io_master_info.qup_client->i2c_client->addr = 0x6c;

    uint32_t gpio_avdd = 0;
    of_property_read_u32(pa_ctrl->soc_info.dev->of_node, "gpio-avdd", &gpio_avdd);
    gpio_request(gpio_avdd+512, "adj_aperture avdd gpio");
    gpio_direction_output(gpio_avdd+512, 1);

    msleep(2);
    gpio_direction_output(54+512, 1); //gpio+offset
    msleep(2);
    gpio_direction_output(153+512, 1);
    msleep(5);
    ret = camera_io_init(&pa_ctrl->io_master_info);
    if (ret < 0) {
        CAM_ERR(CAM_ACTUATOR, "aperture  cci init failed: rc: %d", ret);
    }

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

    if(ADJSTATEFIXMIN == dload)
    {
        camera_io_init(&pa_ctrl->io_master_info);
        camera_adj_aperture_stream_on();
        adj_setting.reg_addr = 0x10;
        adj_setting.reg_data = 0x00;
        ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);
        camera_adj_aperture_stream_off();
        camera_io_release(&pa_ctrl->io_master_info);
        return 0;
    }

    if((dload == ADJSTATEOPEN) && (!aperture_test_flag))
    {
    camera_open_adj_fnum();
        aperture_test_flag = 1;

        return 0;
    }

    if(!aperture_test_flag)
    {
        CAM_ERR(CAM_ACTUATOR, "aperture first open  dev ");
        return 0;
    }

    switch(dload )
    {
        case ADJSTATEMAX:
        camera_adj_aperture_stream_on();
        adj_setting.reg_addr = 0x10; //open max
        adj_setting.reg_data = 0xFFF;
        ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);
        camera_adj_aperture_stream_off();
        break;
        case ADJSTATEMID:
        camera_adj_aperture_stream_on();
        adj_setting.reg_addr = 0x10; //open mid
        adj_setting.reg_data = 0x800;
        ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);
        camera_adj_aperture_stream_off();
        break;
        break;
        case  ADJSTATEMIN:
        camera_adj_aperture_stream_on();
        adj_setting.reg_addr = 0x10; //open min
        adj_setting.reg_data = 0x00;
        ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);
        camera_adj_aperture_stream_off();
        break;
        case  ADJSTATEDEFAULT:
        camera_adj_aperture_stream_on();
        adj_setting.reg_addr = 0x10; //default
        adj_setting.reg_data = aperture_adcvalue;
        ret = camera_io_dev_write_continuous(&pa_ctrl->io_master_info, &adj_write_setting, 0);
        camera_adj_aperture_stream_off();
        break;
        case  ADJSTATEOPEN:
        break;
        case  ADJSTATECLOSE:
        camera_close_adj_fnum();
        aperture_test_flag = 0;
        break;
        case  ADJSTATESTREAMON:
        camera_adj_aperture_stream_on();
        camera_adj_aperture_stream_off();
        break;
    }

    return  ret;
}

static int  camera_state = ADJSTATEMAX;
static ssize_t camera_switch_adj_fnum_state( unsigned dload){
    int ret = 0;

    CAM_DBG(CAM_ACTUATOR, "aperture dload%d", dload);
    ret = camera_adj_aperture_switch_state(dload);

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

static int camera_var_aperture_open(struct inode *inode, struct file *file)
{
    struct cam_var_aperture_ctrl_t *a_ctrl = container_of(file->private_data,
    struct cam_var_aperture_ctrl_t, miscdev);

    cam_var_aperture_move_max_adc(a_ctrl);

    return 0;
}

static int camera_var_aperture_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations adj_aperture_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = NULL,
    .open = camera_var_aperture_open,
    .release = camera_var_aperture_release,
};

static struct cam_i3c_actuator_data {
    struct cam_var_aperture_ctrl_t                  *a_ctrl;
    struct completion                            probe_complete;
} g_i3c_actuator_data[MAX_CAMERAS];

struct completion *cam_var_aperture_get_i3c_completion(uint32_t index)
{
    return &g_i3c_actuator_data[index].probe_complete;
}

static int cam_var_aperture_subdev_close_internal(struct v4l2_subdev *sd,
    struct v4l2_subdev_fh *fh)
{
    struct cam_var_aperture_ctrl_t *a_ctrl =
        v4l2_get_subdevdata(sd);

    if (!a_ctrl) {
        CAM_ERR(CAM_ACTUATOR, "a_ctrl ptr is NULL");
        return -EINVAL;
    }

    mutex_lock(&(a_ctrl->actuator_mutex));
    cam_var_aperture_shutdown(a_ctrl);
    mutex_unlock(&(a_ctrl->actuator_mutex));

    return 0;
}

static int cam_var_aperture_subdev_close(struct v4l2_subdev *sd,
    struct v4l2_subdev_fh *fh)
{
    bool crm_active = cam_req_mgr_is_open();

    if (crm_active) {
        CAM_DBG(CAM_ACTUATOR,
            "CRM is ACTIVE, close should be from CRM");
        return 0;
    }

    return cam_var_aperture_subdev_close_internal(sd, fh);
}

static long cam_var_aperture_subdev_ioctl(struct v4l2_subdev *sd,
    unsigned int cmd, void *arg)
{
    int rc = 0;
    struct cam_var_aperture_ctrl_t *a_ctrl =
        v4l2_get_subdevdata(sd);

    switch (cmd) {
    case VIDIOC_CAM_CONTROL:
        rc = cam_var_aperture_driver_cmd(a_ctrl, arg);
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

        rc = cam_var_aperture_subdev_close_internal(sd, NULL);
        break;
    default:
        CAM_ERR(CAM_ACTUATOR, "Invalid ioctl cmd: %u", cmd);
        rc = -ENOIOCTLCMD;
        break;
    }
    return rc;
}

#ifdef CONFIG_COMPAT
static long cam_var_aperture_init_subdev_do_ioctl(struct v4l2_subdev *sd,
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
        rc = cam_var_aperture_subdev_ioctl(sd, cmd, &cmd_data);
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

static struct v4l2_subdev_core_ops cam_var_aperture_subdev_core_ops = {
    .ioctl = cam_var_aperture_subdev_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl32 = cam_var_aperture_init_subdev_do_ioctl,
#endif
};

static struct v4l2_subdev_ops cam_var_aperture_subdev_ops = {
    .core = &cam_var_aperture_subdev_core_ops,
};

static const struct v4l2_subdev_internal_ops cam_var_aperture_internal_ops = {
    .close = cam_var_aperture_subdev_close,
};

static int cam_var_aperture_init_subdev(struct cam_var_aperture_ctrl_t *a_ctrl)
{
    int rc = 0;

    a_ctrl->v4l2_dev_str.internal_ops =
        &cam_var_aperture_internal_ops;
    a_ctrl->v4l2_dev_str.ops =
        &cam_var_aperture_subdev_ops;//
    strscpy(a_ctrl->device_name, CAMX_APERTURE_DEV_NAME,
        sizeof(a_ctrl->device_name));
    a_ctrl->v4l2_dev_str.name =
        a_ctrl->device_name;
    a_ctrl->v4l2_dev_str.sd_flags =
        (V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS);
    a_ctrl->v4l2_dev_str.ent_function =
        CAM_APERTURE_DEVICE_TYPE;
        //
    a_ctrl->v4l2_dev_str.token = a_ctrl;
    a_ctrl->v4l2_dev_str.close_seq_prior =
         CAM_SD_CLOSE_MEDIUM_PRIORITY;

    rc = cam_register_subdev(&(a_ctrl->v4l2_dev_str));
    if (rc)
        CAM_ERR(CAM_ACTUATOR,
            "Fail with cam_register_subdev rc: %d", rc);

    return rc;
}

static int cam_var_aperture_i2c_component_bind(struct device *dev,
    struct device *master_dev, void *data)
{
    int32_t                          rc = 0;
    int32_t                          i = 0;
    struct i2c_client               *client;
    struct cam_var_aperture_ctrl_t      *a_ctrl;
    struct cam_hw_soc_info          *soc_info = NULL;
    struct cam_var_aperture_soc_private *soc_private = NULL;
    struct timespec64                ts_start, ts_end;
    long                             microsec = 0;
    struct device_node              *np = NULL;
    const char                      *drv_name;
    char  comp_name[COMPSIZE] = {0};
    struct platform_device *pdev = to_platform_device(dev);

    CAM_GET_TIMESTAMP(ts_start);
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

    soc_private = CAM_MEM_ZALLOC(sizeof(struct cam_var_aperture_soc_private),
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

    np = of_node_get(client->dev.of_node);
    drv_name = of_node_full_name(np);

    rc = cam_var_aperture_parse_dt(a_ctrl, &client->dev);
    if (rc < 0) {
        CAM_ERR(CAM_ACTUATOR, "failed: cam_sensor_parse_dt rc %d", rc);
        goto free_soc;
    }

    rc = cam_var_aperture_init_subdev(a_ctrl);
    if (rc)
        goto free_soc;

    if (soc_private->i2c_info.slave_addr != 0)
        a_ctrl->io_master_info.qup_client->i2c_client->addr =
            soc_private->i2c_info.slave_addr;

    a_ctrl->i2c_data.per_frame =
        CAM_MEM_ZALLOC(sizeof(struct i2c_settings_array) *
        MAX_PER_FRAME_ARRAY, GFP_KERNEL);
    if (a_ctrl->i2c_data.per_frame == NULL) {
        rc = -ENOMEM;
        goto unreg_subdev;
    }

    cam_sensor_module_add_i2c_device((void *) a_ctrl, CAM_SENSOR_APERTURE);//

    INIT_LIST_HEAD(&(a_ctrl->i2c_data.init_settings.list_head));

    for (i = 0; i < MAX_PER_FRAME_ARRAY; i++)
        INIT_LIST_HEAD(&(a_ctrl->i2c_data.per_frame[i].list_head));

    a_ctrl->bridge_intf.device_hdl = -1;
    a_ctrl->bridge_intf.link_hdl = -1;
    a_ctrl->bridge_intf.ops.get_dev_info =
        cam_var_aperture_publish_dev_info;
    a_ctrl->bridge_intf.ops.link_setup =
        cam_var_aperture_establish_link;
    a_ctrl->bridge_intf.ops.apply_req =
        cam_var_aperture_apply_request;
    a_ctrl->last_flush_req = 0;
    a_ctrl->cam_act_state = cam_var_aperture_INIT;
    CAM_GET_TIMESTAMP(ts_end);
    CAM_GET_TIMESTAMP_DIFF_IN_MICRO(ts_start, ts_end, microsec);
    cam_record_bind_latency(drv_name, microsec);
    of_node_put(np);

    find_compatible_for_platform_device(pdev, comp_name);
    if(!strcmp(comp_name, "zte,cam-i2c-aperture"))
    {
        pa_ctrl = a_ctrl;
    }

    camera_switch_init();
    a_ctrl->miscdev.minor = MISC_DYNAMIC_MINOR;
    a_ctrl->miscdev.name = "v4l-subdev_aperture";
    a_ctrl->miscdev.fops = &adj_aperture_fops;
    if (misc_register(&a_ctrl->miscdev) != 0)
    {
        CAM_ERR(CAM_ACTUATOR, "misc_register  fail");
        goto unreg_subdev;
    }

    return rc;

unreg_subdev:
    cam_unregister_subdev(&(a_ctrl->v4l2_dev_str));
free_soc:
    CAM_MEM_FREE(soc_private);
free_qup:
    CAM_MEM_FREE(a_ctrl->io_master_info.qup_client);
free_ctrl:
    CAM_MEM_FREE(a_ctrl);
    camera_switch_exit();

    return rc;
}

static void cam_var_aperture_i2c_component_unbind(struct device *dev,
    struct device *master_dev, void *data)
{
    struct i2c_client               *client = NULL;
    struct cam_var_aperture_ctrl_t      *a_ctrl = NULL;

    camera_switch_exit();
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

    CAM_INFO(CAM_ACTUATOR, "i2c remove invoked");
    mutex_lock(&(a_ctrl->actuator_mutex));
    cam_var_aperture_shutdown(a_ctrl);
    mutex_unlock(&(a_ctrl->actuator_mutex));

    cam_unregister_subdev(&(a_ctrl->v4l2_dev_str));
    if (!IS_ERR(a_ctrl->miscdev.this_device) &&
            a_ctrl->miscdev.this_device != NULL) {
        misc_deregister(&a_ctrl->miscdev);
    }

    /*Free Allocated Mem */
    CAM_MEM_FREE(a_ctrl->i2c_data.per_frame);
    a_ctrl->i2c_data.per_frame = NULL;
    a_ctrl->soc_info.soc_private = NULL;
    v4l2_set_subdevdata(&a_ctrl->v4l2_dev_str.sd, NULL);
    CAM_MEM_FREE(a_ctrl->io_master_info.qup_client);
    CAM_MEM_FREE(a_ctrl);
}

const static struct component_ops cam_var_aperture_i2c_component_ops = {
    .bind = cam_var_aperture_i2c_component_bind,
    .unbind = cam_var_aperture_i2c_component_unbind,
};

#if KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE
static int cam_var_aperture_driver_i2c_probe(struct i2c_client *client)
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

    CAM_DBG(CAM_ACTUATOR, "Adding sensor actuator component");

    rc = component_add(&client->dev, &cam_var_aperture_i2c_component_ops);
    if (rc)
        CAM_ERR(CAM_ACTUATOR, "failed to add component rc: %d", rc);

    return rc;
}
#else
static int32_t cam_var_aperture_driver_i2c_probe(struct i2c_client *client,
    const struct i2c_device_id *id)
{
    int rc = 0;

    CAM_ERR(CAM_ACTUATOR, "aperture  failed to add component rc: %d", rc);
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

    CAM_DBG(CAM_ACTUATOR, "Adding sensor actuator component");
    rc = component_add(&client->dev, &cam_var_aperture_i2c_component_ops);
    if (rc)
        CAM_ERR(CAM_ACTUATOR, "failed to add component rc: %d", rc);
    CAM_ERR(CAM_ACTUATOR, "aperture : component_add%d", rc);
    return rc;
}
#endif

#if KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE
void cam_var_aperture_driver_i2c_remove(
    struct i2c_client *client)
{
    component_del(&client->dev, &cam_var_aperture_i2c_component_ops);
}
#else
static int32_t cam_var_aperture_driver_i2c_remove(
    struct i2c_client *client)
{
    component_del(&client->dev, &cam_var_aperture_i2c_component_ops);
    return 0;
}
#endif

static int cam_var_aperture_platform_component_bind(struct device *dev,
    struct device *master_dev, void *data)
{
    int32_t                           rc = 0;
    int32_t                           i = 0;
    bool                              i3c_i2c_target;
    struct cam_var_aperture_ctrl_t       *a_ctrl = NULL;
    struct cam_var_aperture_soc_private  *soc_private = NULL;
    struct platform_device           *pdev = to_platform_device(dev);
    struct timespec64                 ts_start, ts_end;
    long                              microsec = 0;

    CAM_GET_TIMESTAMP(ts_start);

    i3c_i2c_target = of_property_read_bool(pdev->dev.of_node, "i3c-i2c-target");
    if (i3c_i2c_target)
        return 0;

    /* Create actuator control structure */
    a_ctrl = devm_kzalloc(&pdev->dev,
        sizeof(struct cam_var_aperture_ctrl_t), GFP_KERNEL);
    if (!a_ctrl)
        return -ENOMEM;
    CAM_ERR(CAM_ACTUATOR, "aperture  inxde %d", a_ctrl->soc_info.index);
    /*fill in platform device*/
    a_ctrl->v4l2_dev_str.pdev = pdev;
    a_ctrl->soc_info.pdev = pdev;
    a_ctrl->soc_info.dev = &pdev->dev;
    a_ctrl->soc_info.dev_name = pdev->name;
    a_ctrl->io_master_info.master_type = CCI_MASTER;

    a_ctrl->io_master_info.cci_client = CAM_MEM_ZALLOC(sizeof(
        struct cam_sensor_cci_client), GFP_KERNEL);
    if (!(a_ctrl->io_master_info.cci_client)) {
        rc = -ENOMEM;
        goto free_ctrl;
    }

    soc_private = CAM_MEM_ZALLOC(sizeof(struct cam_var_aperture_soc_private),
        GFP_KERNEL);
    if (!soc_private) {
        rc = -ENOMEM;
        goto free_cci_client;
    }
    a_ctrl->soc_info.soc_private = soc_private;
    soc_private->power_info.dev = &pdev->dev;

    a_ctrl->i2c_data.per_frame =
        CAM_MEM_ZALLOC(sizeof(struct i2c_settings_array) *
        MAX_PER_FRAME_ARRAY, GFP_KERNEL);
    if (a_ctrl->i2c_data.per_frame == NULL) {
        rc = -ENOMEM;
        goto free_soc;
    }

    cam_sensor_module_add_i2c_device((void *) a_ctrl, CAM_SENSOR_APERTURE);

    INIT_LIST_HEAD(&(a_ctrl->i2c_data.init_settings.list_head));

    for (i = 0; i < MAX_PER_FRAME_ARRAY; i++)
        INIT_LIST_HEAD(&(a_ctrl->i2c_data.per_frame[i].list_head));

    rc = cam_var_aperture_parse_dt(a_ctrl, &(pdev->dev));
    if (rc < 0) {
        CAM_ERR(CAM_ACTUATOR, "Paring actuator dt failed rc %d", rc);
        goto free_mem;
    }

    /* Fill platform device id*/
    pdev->id = a_ctrl->soc_info.index;
    CAM_ERR(CAM_ACTUATOR, "aperture  inxde %d", a_ctrl->soc_info.index);
    rc = cam_var_aperture_init_subdev(a_ctrl);
    if (rc)
        goto free_mem;

    a_ctrl->bridge_intf.device_hdl = -1;
    a_ctrl->bridge_intf.link_hdl = -1;
    a_ctrl->bridge_intf.ops.get_dev_info =
        cam_var_aperture_publish_dev_info;
    a_ctrl->bridge_intf.ops.link_setup =
        cam_var_aperture_establish_link;
    a_ctrl->bridge_intf.ops.apply_req =
        cam_var_aperture_apply_request;
    a_ctrl->bridge_intf.ops.flush_req =
        cam_var_aperture_flush_request;
    a_ctrl->last_flush_req = 0;

    platform_set_drvdata(pdev, a_ctrl);
    a_ctrl->cam_act_state = cam_var_aperture_INIT;
    CAM_DBG(CAM_ACTUATOR, "Component bound successfully %d",
        a_ctrl->soc_info.index);

    g_i3c_actuator_data[a_ctrl->soc_info.index].a_ctrl = a_ctrl;
    init_completion(&g_i3c_actuator_data[a_ctrl->soc_info.index].probe_complete);
    CAM_GET_TIMESTAMP(ts_end);
    CAM_GET_TIMESTAMP_DIFF_IN_MICRO(ts_start, ts_end, microsec);
    cam_record_bind_latency(pdev->name, microsec);

    return rc;

free_mem:
    CAM_MEM_FREE(a_ctrl->i2c_data.per_frame);
free_soc:
    CAM_MEM_FREE(soc_private);
free_cci_client:
    CAM_MEM_FREE(a_ctrl->io_master_info.cci_client);
free_ctrl:
    devm_kfree(&pdev->dev, a_ctrl);
    return rc;
}

static void cam_var_aperture_platform_component_unbind(struct device *dev,
    struct device *master_dev, void *data)
{
    struct cam_var_aperture_ctrl_t      *a_ctrl;
    bool                             i3c_i2c_target;
    struct platform_device *pdev = to_platform_device(dev);

    i3c_i2c_target = of_property_read_bool(pdev->dev.of_node, "i3c-i2c-target");
    if (i3c_i2c_target)
        return;

    a_ctrl = platform_get_drvdata(pdev);
    if (!a_ctrl) {
        CAM_ERR(CAM_ACTUATOR, "Actuator device is NULL");
        return;
    }

    mutex_lock(&(a_ctrl->actuator_mutex));
    cam_var_aperture_shutdown(a_ctrl);
    mutex_unlock(&(a_ctrl->actuator_mutex));
    cam_unregister_subdev(&(a_ctrl->v4l2_dev_str));

    CAM_MEM_FREE(a_ctrl->io_master_info.cci_client);
    a_ctrl->io_master_info.cci_client = NULL;
    CAM_MEM_FREE(a_ctrl->soc_info.soc_private);
    a_ctrl->soc_info.soc_private = NULL;
    CAM_MEM_FREE(a_ctrl->i2c_data.per_frame);
    a_ctrl->i2c_data.per_frame = NULL;
    v4l2_set_subdevdata(&a_ctrl->v4l2_dev_str.sd, NULL);
    platform_set_drvdata(pdev, NULL);
    devm_kfree(&pdev->dev, a_ctrl);
    CAM_INFO(CAM_ACTUATOR, "Actuator component unbinded");
}

const static struct component_ops cam_var_aperture_platform_component_ops = {
    .bind = cam_var_aperture_platform_component_bind,
    .unbind = cam_var_aperture_platform_component_unbind,
};

static int32_t cam_var_aperture_platform_remove(
    struct platform_device *pdev)
{
    component_del(&pdev->dev, &cam_var_aperture_platform_component_ops);
    return 0;
}

static const struct of_device_id cam_var_aperture_driver_dt_match[] = {
    {.compatible = "zte,actuator"},
    {}
};

static int32_t cam_var_aperture_driver_platform_probe(
    struct platform_device *pdev)
{
    int rc = 0;

    CAM_DBG(CAM_ACTUATOR, "Adding sensor actuator component");
    rc = component_add(&pdev->dev, &cam_var_aperture_platform_component_ops);
    if (rc)
        CAM_ERR(CAM_ACTUATOR, "failed to add component rc: %d", rc);

    return rc;
}

MODULE_DEVICE_TABLE(of, cam_var_aperture_driver_dt_match);

struct platform_driver cam_var_aperture_platform_driver = {
    .probe = cam_var_aperture_driver_platform_probe,
    .driver = {
        .name = "zte,actuator",
        .owner = THIS_MODULE,
        .of_match_table = cam_var_aperture_driver_dt_match,
        .suppress_bind_attrs = true,
    },
    .remove = cam_var_aperture_platform_remove,
};

static const struct i2c_device_id i2c_id[] = {
    {APERTURE_DRIVER_I2C, (kernel_ulong_t)NULL},
    { }
};

static const struct of_device_id cam_var_aperture_i2c_driver_dt_match[] = {
    {.compatible = "zte,cam-i2c-aperture"},
    {}
};
MODULE_DEVICE_TABLE(of, cam_var_aperture_i2c_driver_dt_match);

struct i2c_driver cam_var_aperture_i2c_driver = {
    .id_table = i2c_id,
    .probe  = cam_var_aperture_driver_i2c_probe,
    .remove = cam_var_aperture_driver_i2c_remove,
    .driver = {
        .of_match_table = cam_var_aperture_i2c_driver_dt_match,
        .owner = THIS_MODULE,
        .name = APERTURE_DRIVER_I2C,
        .suppress_bind_attrs = true,
    },
};

static struct i3c_device_id actuator_i3c_id[MAX_I3C_DEVICE_ID_ENTRIES + 1];

static int cam_var_aperture_i3c_driver_probe(struct i3c_device *client)
{
    int32_t                          rc = 0;
    struct cam_var_aperture_ctrl_t       *a_ctrl = NULL;
    uint32_t                          index;
    struct device                    *dev;

    if (!client) {
        CAM_INFO(CAM_ACTUATOR, "Null Client pointer");
        return -EINVAL;
    }

    dev = &client->dev;

    CAM_DBG(CAM_ACTUATOR, "Probe for I3C Slave %s", dev_name(dev));

    rc = of_property_read_u32(dev->of_node, "cell-index", &index);
    if (rc) {
        CAM_ERR(CAM_ACTUATOR, "device %s failed to read cell-index", dev_name(dev));
        return rc;
    }

    if (index >= MAX_CAMERAS) {
        CAM_ERR(CAM_ACTUATOR, "Invalid Cell-Index: %u for %s", index, dev_name(dev));
        return -EINVAL;
    }

    a_ctrl = g_i3c_actuator_data[index].a_ctrl;
    if (!a_ctrl) {
        CAM_ERR(CAM_ACTUATOR,
            "a_ctrl is null. I3C Probe before platfom driver probe for %s",
            dev_name(dev));
        return -EINVAL;
    }
    cam_sensor_utils_parse_pm_ctrl_flag(dev->of_node, &(a_ctrl->io_master_info));

    CAM_INFO(CAM_SENSOR,
        "master: %d (1-CCI, 2-I2C, 3-SPI, 4-I3C) pm_ctrl_client_enable: %d",
        a_ctrl->io_master_info.master_type,
        a_ctrl->io_master_info.qup_client->pm_ctrl_client_enable);

    a_ctrl->io_master_info.qup_client->i3c_client = client;
    a_ctrl->io_master_info.qup_client->i3c_wait_for_hotjoin = false;

    complete_all(&g_i3c_actuator_data[index].probe_complete);

    CAM_DBG(CAM_ACTUATOR, "I3C Probe Finished for %s", dev_name(dev));
    return rc;
}

#if (KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE)
static void cam_i3c_driver_remove(struct i3c_device *client)
{
    int32_t                        rc = 0;
    struct cam_var_aperture_ctrl_t     *a_ctrl = NULL;
    struct device                  *dev;
    uint32_t                       index;

    if (!client) {
        CAM_ERR(CAM_SENSOR, "I3C Driver Remove: Invalid input args");
        return;
    }

    dev = &client->dev;

    CAM_DBG(CAM_SENSOR, "driver remove for I3C Slave %s", dev_name(dev));

    rc = of_property_read_u32(dev->of_node, "cell-index", &index);
    if (rc) {
        CAM_ERR(CAM_UTIL, "device %s failed to read cell-index", dev_name(dev));
        return;
    }

    if (index >= MAX_CAMERAS) {
        CAM_ERR(CAM_SENSOR, "Invalid Cell-Index: %u for %s", index, dev_name(dev));
        return;
    }

    a_ctrl = g_i3c_actuator_data[index].a_ctrl;
    if (!a_ctrl) {
        CAM_ERR(CAM_SENSOR, "a_ctrl is null. I3C Probe before platfom driver probe for %s",
            dev_name(dev));
        return;
    }

    CAM_DBG(CAM_SENSOR, "I3C remove invoked for %s",
        (client ? dev_name(&client->dev) : "none"));
    CAM_MEM_FREE(a_ctrl->io_master_info.qup_client);
    a_ctrl->io_master_info.qup_client = NULL;
}

#else
static int cam_i3c_driver_remove(struct i3c_device *client)
{
    struct cam_var_aperture_ctrl_t     *a_ctrl = NULL;
    struct device                  *dev;
    uint32_t                       index;

    if (!client) {
        CAM_ERR(CAM_SENSOR, "I3C Driver Remove: Invalid input args");
        return -EINVAL;
    }

    dev = &client->dev;

    CAM_DBG(CAM_SENSOR, "driver remove for I3C Slave %s", dev_name(dev));

    rc = of_property_read_u32(dev->of_node, "cell-index", &index);
    if (rc) {
        CAM_ERR(CAM_UTIL, "device %s failed to read cell-index", dev_name(dev));
        return -EINVAL;
    }

    if (index >= MAX_CAMERAS) {
        CAM_ERR(CAM_SENSOR, "Invalid Cell-Index: %u for %s", index, dev_name(dev));
        return -EINVAL;
    }

    a_ctrl = g_i3c_actuator_data[index].a_ctrl;
    if (!a_ctrl) {
        CAM_ERR(CAM_SENSOR, "a_ctrl is null. I3C Probe before platfom driver probe for %s",
            dev_name(dev));
        return -EINVAL;
    }

    CAM_DBG(CAM_SENSOR, "I3C remove invoked for %s",
        (client ? dev_name(&client->dev) : "none"));
    CAM_MEM_FREE(a_ctrl->io_master_info.qup_client);
    a_ctrl->io_master_info.qup_client = NULL;
    return 0;
}
#endif

static struct i3c_driver cam_var_aperture_i3c_driver = {
    .id_table = actuator_i3c_id,
    .probe = cam_var_aperture_i3c_driver_probe,
    .remove = cam_i3c_driver_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = APERTURE_DRIVER_I3C,
        .of_match_table = cam_var_aperture_driver_dt_match,
        .suppress_bind_attrs = true,
    },
};

int cam_var_aperture_driver_init(void)
{
    int32_t rc = 0;
    struct device_node                      *dev;
    int num_entries = 0;

    rc = platform_driver_register(&cam_var_aperture_platform_driver);
    if (rc < 0) {
        CAM_ERR(CAM_ACTUATOR,
            "platform_driver_register failed rc = %d", rc);
        return rc;
    }

    rc = i2c_add_driver(&cam_var_aperture_i2c_driver);
    if (rc) {
        CAM_ERR(CAM_ACTUATOR, "i2c_add_driver failed rc = %d", rc);
        goto i2c_register_err;
    }

    memset(actuator_i3c_id, 0, sizeof(struct i3c_device_id) * (MAX_I3C_DEVICE_ID_ENTRIES + 1));

    dev = of_find_node_by_path(I3C_SENSOR_DEV_ID_DT_PATH);
    if (!dev) {
        CAM_DBG(CAM_ACTUATOR, "Couldnt Find the i3c-id-table dev node");
        return 0;
    }

    rc = cam_sensor_count_elems_i3c_device_id(dev, &num_entries,
        "i3c-actuator-id-table");
    if (rc)
        return 0;

    rc = cam_sensor_fill_i3c_device_id(dev, num_entries,
        "i3c-actuator-id-table", actuator_i3c_id);
    if (rc)
        goto i3c_register_err;

    rc = i3c_driver_register_with_owner(&cam_var_aperture_i3c_driver, THIS_MODULE);
    if (rc) {
        CAM_ERR(CAM_ACTUATOR, "i3c_driver registration failed, rc: %d", rc);
        goto i3c_register_err;
    }

    return 0;

i3c_register_err:
    i2c_del_driver(&cam_var_aperture_i2c_driver);
i2c_register_err:
    platform_driver_unregister(&cam_var_aperture_platform_driver);

    return rc;
}

void cam_var_aperture_driver_exit(void)
{
    struct device_node *dev;

    platform_driver_unregister(&cam_var_aperture_platform_driver);
    i2c_del_driver(&cam_var_aperture_i2c_driver);

    dev = of_find_node_by_path(I3C_SENSOR_DEV_ID_DT_PATH);
    if (!dev) {
        CAM_DBG(CAM_ACTUATOR, "Couldnt Find the i3c-id-table dev node");
        return;
    }

    i3c_driver_unregister(&cam_var_aperture_i3c_driver);
}

MODULE_DESCRIPTION("cam_var_aperture_driver");
MODULE_LICENSE("GPL v2");
