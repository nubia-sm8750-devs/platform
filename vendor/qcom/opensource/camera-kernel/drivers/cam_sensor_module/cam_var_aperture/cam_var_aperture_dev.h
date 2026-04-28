/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2019, 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */


#ifndef _CAM_VAR_APERTURE_DEV_H_
#define _CAM_VAR_APERTURE_DEV_H_

#include <cam_sensor_io.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/iommu.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-subdev.h>
#include <cam_cci_dev.h>
#include <cam_sensor_cmn_header.h>
#include <cam_subdev.h>
#include "cam_sensor_util.h"
#include "cam_soc_util.h"
#include "cam_debug_util.h"
#include "cam_context.h"
#include <linux/miscdevice.h>

#define NUM_MASTERS 2
#define NUM_QUEUES 2

#define APERTURE_DRIVER_I2C                    "cam-i2c-aperture"
#define CAMX_APERTURE_DEV_NAME                 "cam-aperture-driver"
#define APERTURE_DRIVER_I3C                    "i3c_camera_aperture"


#define MSM_ACTUATOR_MAX_VREGS (10)
#define ACTUATOR_MAX_POLL_COUNT 10

enum cam_var_aperture_apply_state_t {
    ACT_APPLY_SETTINGS_NOW_APERTURE,
    ACT_APPLY_SETTINGS_LATER_APERTURE,
};

enum cam_var_aperture_state {
    cam_var_aperture_INIT,
    cam_var_aperture_ACQUIRE,
    cam_var_aperture_CONFIG,
    cam_var_aperture_START,
};



/**
 * struct cam_var_aperture_i2c_info_t - I2C info
 * @slave_addr      :   slave address
 * @i2c_freq_mode   :   i2c frequency mode
 */
struct cam_var_aperture_i2c_info_t {
    uint16_t slave_addr;
    uint8_t i2c_freq_mode;
};

struct cam_var_aperture_soc_private {
    struct cam_var_aperture_i2c_info_t i2c_info;
    struct cam_sensor_power_ctrl_t power_info;
};

/**
 * struct actuator_intf_params
 * @device_hdl: Device Handle
 * @session_hdl: Session Handle
 * @ops: KMD operations
 * @crm_cb: Callback API pointers
 */
struct aperture_intf_params {
    int32_t device_hdl;
    int32_t session_hdl;
    int32_t link_hdl;
    struct cam_req_mgr_kmd_ops ops;
    struct cam_req_mgr_crm_cb *crm_cb;
};

/**
 * struct cam_var_aperture_ctrl_t
 * @device_name: Device name
 * @i2c_driver: I2C device info
 * @pdev: Platform device
 * @cci_i2c_master: I2C structure
 * @io_master_info: Information about the communication master
 * @actuator_mutex: Actuator mutex
 * @is_i3c_device : A Flag to indicate whether this actuator is I3C device
 * @act_apply_state: Actuator settings aRegulator config
 * @id: Cell Index
 * @res_apply_state: Actuator settings apply state
 * @cam_act_state:   Actuator state
 * @gconf: GPIO config
 * @pinctrl_info: Pinctrl information
 * @v4l2_dev_str: V4L2 device structure
 * @i2c_data: I2C register settings structure
 * @act_info: Sensor query cap structure
 * @of_node: Node ptr
 * @last_flush_req: Last request to flush
 */
struct cam_var_aperture_ctrl_t {
    char device_name[CAM_CTX_DEV_NAME_MAX_LENGTH];
    struct i2c_driver *i2c_driver;
    enum cci_i2c_master_t cci_i2c_master;
    enum cci_device_num cci_num;
    struct camera_io_master io_master_info;
    struct cam_hw_soc_info soc_info;
    struct mutex actuator_mutex;
    bool is_i3c_device;
    uint32_t id;
    enum cam_var_aperture_apply_state_t setting_apply_state;
    enum cam_var_aperture_state cam_act_state;
    uint8_t cam_pinctrl_status;
    struct cam_subdev v4l2_dev_str;
    struct i2c_data_settings i2c_data;
    struct cam_actuator_query_cap act_info;
    struct aperture_intf_params bridge_intf;
    uint32_t last_flush_req;
    struct miscdevice miscdev;
};

/**
 * @brief : API to register Actuator hw to platform framework.
 * @return struct platform_device pointer on on success, or ERR_PTR() on error.
 */
int cam_var_aperture_driver_init(void);

/**
 * @brief : API to remove Actuator Hw from platform framework.
 */
void cam_var_aperture_driver_exit(void);
#endif /* _cam_var_aperture_DEV_H_ */
