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

#ifndef VI530X_FIRMWARE_H
#define VI530X_FIRMWARE_H

#include "vi530x.h"
#define FIRMWARE_NUM 8192
extern uint8_t Firmware[FIRMWARE_NUM];
uint32_t LoadFirmware(VI530X_DEV dev);

#endif

