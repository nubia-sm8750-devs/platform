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

#include<linux/string.h>
#include <linux/kernel.h>
#include <linux/firmware.h>

#include "vi530x_firmware.h"

uint8_t Firmware[FIRMWARE_NUM];

uint32_t LoadFirmware(VI530X_DEV dev)
{
	const struct firmware *vi530x_firmware;
	const char *fw_name = "VI5300-M40_G05_R03_V1.05.bin";
	uint32_t data_size;
	int err;

	err = request_firmware(&vi530x_firmware, fw_name, dev->dev);
	if (err  || !vi530x_firmware)
	{
		vi530x_errmsg("Firmware request failed!\n");
		return err;
	} else {
		vi530x_infomsg("Firmware request succeeded!\n");
	}
	data_size = (uint32_t)vi530x_firmware->size;
	vi530x_dbgmsg("vi530x_firmware size:%d\n", data_size);

	memcpy(Firmware, vi530x_firmware->data, vi530x_firmware->size);
	release_firmware(vi530x_firmware);
	return data_size;
}
