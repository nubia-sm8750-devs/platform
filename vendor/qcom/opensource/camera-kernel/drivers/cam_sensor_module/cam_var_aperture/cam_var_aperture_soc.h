/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 */

#ifndef _CAM_VAR_APERTURE_SOC_H_
#define _CAM_VAR_APERTURE_SOC_H_

#include "cam_var_aperture_dev.h"

/**
 * @a_ctrl: Actuator ctrl structure
 *
 * This API parses actuator device tree
 */
int cam_var_aperture_parse_dt(struct cam_var_aperture_ctrl_t *a_ctrl,
    struct device *dev);

#endif /* _cam_var_aperture_SOC_H_ */
