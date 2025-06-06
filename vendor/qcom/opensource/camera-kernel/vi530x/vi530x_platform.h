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

#ifndef VI530X_PLATFORM_H
#define VI530X_PLATFORM_H

#include "vi530x.h"

int32_t vi530x_write_byte(VI530X_DEV dev, uint8_t reg, uint8_t data);
int32_t vi530x_read_byte(VI530X_DEV dev, uint8_t reg, uint8_t *data);
int32_t vi530x_write_multibytes(VI530X_DEV dev, uint8_t reg, uint8_t *data, int32_t count);
int32_t vi530x_read_multibytes(VI530X_DEV dev, uint8_t reg, uint8_t *data, int32_t count);
int32_t vi530x_write_reg_offset(VI530X_DEV dev, uint8_t reg, uint8_t offset, uint8_t data);
int32_t vi530x_read_reg_offset(VI530X_DEV dev, uint8_t reg, uint8_t offset, uint8_t *data);

 #endif
