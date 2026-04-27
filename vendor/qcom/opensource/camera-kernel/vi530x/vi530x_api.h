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

#ifndef _VI530X_API_H_
#define _VI530X_API_H_
#include "vi530x.h"

VI530X_Error VI530X_Chip_PowerON(VI530X_DEV dev);
VI530X_Error VI530X_Chip_PowerOFF(VI530X_DEV dev);
void VI530X_Read_ChipVersion(VI530X_DEV dev);
void VI530X_Set_Period(VI530X_DEV dev, uint32_t period);
VI530X_Error VI530X_Single_Measure(VI530X_DEV dev);
VI530X_Error VI530X_Start_Continuous_Measure(VI530X_DEV dev);
VI530X_Error VI530X_Stop_Continuous_Measure(VI530X_DEV dev);
VI530X_Error VI530X_Get_Measure_Data(VI530X_DEV dev);
VI530X_Error VI530X_Get_Interrupt_State(VI530X_DEV dev);
VI530X_Error VI530X_Chip_Init(VI530X_DEV dev);
VI530X_Error VI530X_Start_XTalk_Calibration(VI530X_DEV dev);
VI530X_Error VI530X_Start_Offset_Calibration(VI530X_DEV dev);
VI530X_Error VI530X_Get_XTalk_Parameter(VI530X_DEV dev);
VI530X_Error VI530X_Config_XTalk_Parameter(VI530X_DEV dev);
void VI530X_Read_FW_Version(VI530X_DEV dev);

#endif

