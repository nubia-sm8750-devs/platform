/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */


#ifndef _CAM_ADJ_APERTURE_CORE_H_
#define _CAM_ADJ_APERTURE_CORE_H_

#include "cam_adj_aperture_dev.h"

typedef enum  APERTURE_ELEVEL
{
    APERTURE_ELEVEL_L1 = 1,
    APERTURE_ELEVEL_L2,
    APERTURE_ELEVEL_L3,
    APERTURE_ELEVEL_L4,
    APERTURE_ELEVEL_L5,
    APERTURE_ELEVEL_L6,
    APERTURE_ELEVEL_L7,
    APERTURE_ELEVEL_L8,
    APERTURE_ELEVEL_L9,
    APERTURELEVEL_INVALID = -1,
}APERTURE_ELEVEL;

typedef enum  APERTURE_STATE
{
    APERTURE_OFF,
    APERTURE_ON
}APERTURE_STATE;

typedef struct APERTURE_CTRL
{
    APERTURE_ELEVEL   aperture_level;
    unsigned  int     aperture_adc_value;
}APERTURE_CTRL;

/**
 * @power_info: power setting info to control the power
 *
 * This API construct the default actuator power setting.
 *
 * @return Status of operation. Negative in case of error. Zero otherwise.
 */
int32_t cam_adj_aperture_construct_default_power_setting(
	struct cam_sensor_power_ctrl_t *power_info);

/**
 * @apply: Req mgr structure for applying request
 *
 * This API applies the request that is mentioned
 */
int32_t cam_adj_aperture_apply_request(struct cam_req_mgr_apply_request *apply);

/**
 * @info: Sub device info to req mgr
 *
 * This API publish the subdevice info to req mgr
 */
int32_t cam_adj_aperture_publish_dev_info(struct cam_req_mgr_device_info *info);

/**
 * @flush: Req mgr structure for flushing request
 *
 * This API flushes the request that is mentioned
 */
int cam_adj_aperture_flush_request(struct cam_req_mgr_flush_request *flush);


/**
 * @link: Link setup info
 *
 * This API establishes link actuator subdevice with req mgr
 */
int32_t cam_adj_aperture_establish_link(
	struct cam_req_mgr_core_dev_link_setup *link);

/**
 * @a_ctrl: Actuator ctrl structure
 * @arg:    Camera control command argument
 *
 * This API handles the camera control argument reached to actuator
 */
int32_t cam_adj_aperture_driver_cmd(struct cam_adj_aperture_ctrl_t *a_ctrl, void *arg);
/**
 * @a_ctrl: Actuator ctrl structure
 *
 * This API handles the shutdown ioctl/close
 */
void cam_adj_aperture_shutdown(struct cam_adj_aperture_ctrl_t *a_ctrl);

struct completion *cam_adj_aperture_get_i3c_completion(uint32_t index);

int32_t cam_zte_adj_aperture_driver_cmd(struct cam_adj_aperture_ctrl_t *a_ctrl,
	void *arg);

ssize_t camera_open_zte_adj_aperture(struct cam_adj_aperture_ctrl_t *pa_ctrl);

ssize_t camera_close_zte_adj_aperture(struct cam_adj_aperture_ctrl_t *pa_ctrl);
void  camera_zte_aperture_init_kfifo(void);
void  cam_zte_adj_aperture_thread_resources_close(struct cam_adj_aperture_ctrl_t *a_ctrl);
#endif /* _CAM_ACTUATOR_CORE_H_ */
