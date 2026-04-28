/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _QCOM_MPAM_SLC_H
#define _QCOM_MPAM_SLC_H

struct qcom_slc_gear_val {
	uint32_t gear_val;
} __packed;

struct slc_parid_config {
	struct msc_query query;
	struct qcom_slc_gear_val gear_config;
} __packed;

#define MAX_PART_ID		10
#define SLC_INVALID_PARTID      (1 << 16) - 1

enum slc_clients_id {
	APPS,
	GPU,
	NSP,
	SLC_CLIENT_MAX,
};

enum gear_val {
	GEAR_VERY_HIGH,
	GEAR_HIGH,
	GEAR_MID,
	GEAR_LOW,
	GEAR_BYPASS,
	GEAR_MAX,
};

static char gear_index[][25] = {
	"SLC_GEAR_VERY_HIGH",
	"SLC_GEAR_HIGH",
	"SLC_GEAR_MID",
	"SLC_GEAR_LOW",
	"SLC_GEAR_BYPASS",
	"",
};

#define MAX_NUM_GEARS	GEAR_MAX

struct slc_partid_capability {
	uint8_t part_id;
	uint8_t num_gears;
	uint8_t part_id_gears[MAX_NUM_GEARS];
} __packed;

struct slc_client_info {
	uint16_t client_id;
	uint16_t num_part_id;
} __packed;

struct slc_client_capability {
	struct slc_client_info client_info;
	struct slc_partid_capability *slc_partid_cap;
	uint8_t enabled;
	const char *client_name;
} __packed;

struct qcom_slc_capability {
	uint32_t num_clients;
	struct slc_client_capability *slc_client_cap;
} __packed;

#endif /* _QCOM_MPAM_SLC_H */
