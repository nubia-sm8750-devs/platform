#include "syna_tcm2.h"
#include "syna_tcm2_testing_items.h"
#include "testing/synaptics_touchcom_testing.h"
#include <linux/spi/spi.h>

#define SPI_NUM 4

extern int syna_dev_set_tp_report_rate(struct syna_tcm *tcm, int en, unsigned int resp_handling);
extern int syna_dev_set_sensibility_level(struct syna_tcm *tcm, int en, unsigned int resp_handling);
extern int syna_dev_set_follow_hand_level(struct syna_tcm *tcm, int en, unsigned int resp_handling);
extern int syna_dev_set_stability_level(struct syna_tcm *tcm, int en, unsigned int resp_handling);
extern int syna_dev_set_display_rotation(struct syna_tcm *tcm, int mrotation, unsigned int resp_handling);
extern int syna_dev_set_play_game(struct syna_tcm *tcm, int enable, unsigned int resp_handling);
extern int syna_testing_pt01_zte(struct syna_tcm *tcm);
extern int syna_testing_pt05_zte(struct syna_tcm *tcm);
extern int syna_testing_pt0a_zte(struct syna_tcm *tcm);
extern void syna_spi_hw_reset(struct syna_hw_interface *hw_if);
extern int syna_tcm_set_game_partition_config(struct tcm_dev *tcm_dev, unsigned char id,
	int len, unsigned char *value, unsigned int resp_reading);

static int tpd_init_tpinfo(struct ztp_device *cdev)
{
    struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;

    LOGI("%s: enter\n", __func__);
	snprintf(cdev->ic_tpinfo.tp_name, sizeof(cdev->ic_tpinfo.tp_name), "synaptics");
	cdev->ic_tpinfo.chip_model_id = TS_CHIP_SYNAPTICS;

	cdev->ic_tpinfo.firmware_ver = tcm->tcm_dev->packrat_number;
	cdev->ic_tpinfo.spi_num = SPI_NUM;

	return 0;
}

/* Started by AICoder, pid:8a310cc7b509b7014c2f0b4750eedd3e22a7c538 */
static int tpd_test_cmd_show(struct ztp_device *cdev, char *buf)
{
    ssize_t num_read_chars = 0;
    int i_len = 0;

    LOGI("%s: enter\n", __func__);
    i_len = snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d", 0, 16, 37, 0);
    num_read_chars = i_len;
    return num_read_chars;
}

static int tpd_test_cmd_store(struct ztp_device *cdev)
{
    struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;
	int ret = 0;

    if (tcm->pwr_state != PWR_ON) {
        LOGE("%s: error, change set in suspend!", __func__);
    } else {
		LOGI("%s: enter\n", __func__);
		ret = syna_testing_pt01_zte(tcm);
		if (ret < 0) {
			LOGE("Fail to run test pt01");
			goto reset;
		}
		ret = syna_testing_pt05_zte(tcm);
		if (ret < 0) {
			LOGE("Fail to run test pt05");
			goto reset;
		}
		ret = syna_testing_pt0a_zte(tcm);
		if (ret < 0) {
			LOGE("Fail to run test pt0a");
			goto reset;
		}
	}
reset:
	syna_spi_hw_reset(tcm->hw_if);

	LOGI("%s:end\n", __func__);
	return ret;
}
/* Ended by AICoder, pid:8a310cc7b509b7014c2f0b4750eedd3e22a7c538 */

static int tpd_set_tp_report_rate(struct ztp_device *cdev, int tp_report_rate_level)
{
	struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;
    struct syna_hw_attn_data *attn;
    struct tcm_dev *tcm_dev;
    unsigned int resp_handling;
	int ret = 0;

    LOGI("%s: enter\n", __func__);

 	if (!tcm)
		return -EINVAL;

	tcm_dev = tcm->tcm_dev;
	attn = &tcm->hw_if->bdata_attn;
	if ((attn->irq_id) && (attn->irq_enabled))
		resp_handling = CMD_RESPONSE_IN_ATTN;
	else
		resp_handling = tcm_dev->msg_data.command_polling_time;

	if (tp_report_rate_level > 5)
		tp_report_rate_level = 5;
	tcm->ztec.tp_report_rate = tp_report_rate_level;
	if (tcm->pwr_state != PWR_ON) {
		LOGE("%s: error, change set in suspend!", __func__);
	} else {
		/*0:in tp report mode->in 120Hz;
		  1:in tp report mode->in 240Hz;
		  2:in tp report mode->in 360Hz;*/
		ret = syna_dev_set_tp_report_rate(tcm, tp_report_rate_level, resp_handling);
		if (ret < 0)
			LOGE("set report rate mode failed!");
	}

	return 0;
}

static int tpd_get_tp_report_rate(struct ztp_device *cdev)
{
	struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;

	cdev->tp_report_rate = tcm->ztec.tp_report_rate;

	return 0;
}

static int tpd_set_sensibility_level(struct ztp_device *cdev, u8 tp_sensibility_level)
{
	struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;
    struct syna_hw_attn_data *attn;
    struct tcm_dev *tcm_dev;
    unsigned int resp_handling;
	int ret = 0;

    LOGI("%s: enter\n", __func__);

 	if (!tcm)
		return -EINVAL;

	tcm_dev = tcm->tcm_dev;
	attn = &tcm->hw_if->bdata_attn;
	if ((attn->irq_id) && (attn->irq_enabled))
		resp_handling = CMD_RESPONSE_IN_ATTN;
	else
		resp_handling = tcm_dev->msg_data.command_polling_time;

	if (tp_sensibility_level > 4)
		tp_sensibility_level = 4;
	tcm->ztec.sensibility_level = tp_sensibility_level;
	if (tcm->pwr_state != PWR_ON) {
		LOGE("%s: error, change set in suspend!", __func__);
	} else {
		/*0:in tp report mode->in 120Hz;
		  1:in tp report mode->in 240Hz;
		  2:in tp report mode->in 360Hz;*/
		ret = syna_dev_set_sensibility_level(tcm, tp_sensibility_level, resp_handling);
		if (ret < 0)
			LOGE("set sensibility_level mode failed!");
	}

	return 0;
}

static int tpd_get_sensibility_level(struct ztp_device *cdev)
{
    struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;

	cdev->sensibility_level = tcm->ztec.sensibility_level;

	return 0;
}

static int tpd_get_wakegesture(struct ztp_device *cdev)
{
	struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;

	cdev->b_gesture_enable = tcm->ztec.is_wakeup_gesture;

	return 0;
}

static int tpd_enable_wakegesture(struct ztp_device *cdev, int enable)
{
	struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;

	//tcm->ztec.is_set_wakeup_in_suspend = enable;
	if (tcm->pwr_state != PWR_ON) {
		LOGE("%s: error, change set in suspend!", __func__);
	} else {
		tcm->ztec.is_wakeup_gesture = enable;
	}

	return 0;
}

static int tpd_set_follow_hand_level(struct ztp_device *cdev, int tp_follow_hand_level)
{
	struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;
    struct syna_hw_attn_data *attn;
    struct tcm_dev *tcm_dev;
    unsigned int resp_handling;
	int ret = 0;

    LOGI("%s: enter\n", __func__);

 	if (!tcm)
		return -EINVAL;

	tcm_dev = tcm->tcm_dev;
	attn = &tcm->hw_if->bdata_attn;
	if ((attn->irq_id) && (attn->irq_enabled))
		resp_handling = CMD_RESPONSE_IN_ATTN;
	else
		resp_handling = tcm_dev->msg_data.command_polling_time;

	if (tp_follow_hand_level > 4)
		tp_follow_hand_level = 4;
	tcm->ztec.follow_hand_level = tp_follow_hand_level;
	if (tcm->pwr_state != PWR_ON) {
		LOGE("%s: error, change set in suspend!", __func__);
	} else {
		/*0:in tp report mode->in 120Hz;
		  1:in tp report mode->in 240Hz;
		  2:in tp report mode->in 360Hz;*/
		ret = syna_dev_set_follow_hand_level(tcm, tp_follow_hand_level, resp_handling);
		if (ret < 0)
			LOGE("set follow_hand_level mode failed!");
	}

	return 0;
}

static int tpd_get_follow_hand_level(struct ztp_device *cdev)
{
    struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;

	cdev->follow_hand_level = tcm->ztec.follow_hand_level;

	return 0;
}

static int tpd_set_stability_level(struct ztp_device *cdev, int tp_stability_level)
{
	struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;
    struct syna_hw_attn_data *attn;
    struct tcm_dev *tcm_dev;
    unsigned int resp_handling;
	int ret = 0;

    LOGI("%s: enter\n", __func__);

 	if (!tcm)
		return -EINVAL;

	tcm_dev = tcm->tcm_dev;
	attn = &tcm->hw_if->bdata_attn;
	if ((attn->irq_id) && (attn->irq_enabled))
		resp_handling = CMD_RESPONSE_IN_ATTN;
	else
		resp_handling = tcm_dev->msg_data.command_polling_time;

	if (tp_stability_level > 4)
		tp_stability_level = 4;
	tcm->ztec.stability_level = tp_stability_level;
	if (tcm->pwr_state != PWR_ON) {
		LOGE("%s: error, change set in suspend!", __func__);
	} else {
		/*0:in tp report mode->in 120Hz;
		  1:in tp report mode->in 240Hz;
		  2:in tp report mode->in 360Hz;*/
		ret = syna_dev_set_stability_level(tcm, tp_stability_level, resp_handling);
		if (ret < 0)
			LOGE("set stability_level mode failed!");
	}

	return 0;
}

static int tpd_get_stability_level(struct ztp_device *cdev)
{
    struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;

	cdev->stability_level = tcm->ztec.stability_level;

	return 0;
}

static int tpd_get_play_game(struct ztp_device *cdev)
{
    struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;

	cdev->rotation_limit_level = tcm->ztec.rotation_limit_level;

	return 0;
}

static int tpd_set_play_game(struct ztp_device *cdev, int enable)
{
	struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;
    struct syna_hw_attn_data *attn;
    struct tcm_dev *tcm_dev;
    unsigned int resp_handling;
	int ret = 0;

    LOGI("%s: enter\n", __func__);

 	if (!tcm)
		return -EINVAL;

	tcm_dev = tcm->tcm_dev;
	attn = &tcm->hw_if->bdata_attn;
	if ((attn->irq_id) && (attn->irq_enabled))
		resp_handling = CMD_RESPONSE_IN_ATTN;
	else
		resp_handling = tcm_dev->msg_data.command_polling_time;

	if (tcm->pwr_state != PWR_ON) {
		LOGE("%s: error, change set in suspend!", __func__);
	} else {
		if (tcm->ztec.is_play_game == enable) {
			LOGI("play no need reset");
		} else {
			tcm->ztec.is_play_game = enable;
			ret = syna_dev_set_play_game(tcm, enable, resp_handling);
			if (ret < 0)
				LOGE("set play_game mode failed!");
		}
	}

	return cdev->display_rotation;
}

static int tpd_get_rotation_limit_level(struct ztp_device *cdev)
{
    struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;

	cdev->rotation_limit_level = tcm->ztec.rotation_limit_level;

	return 0;
}

static int tpd_set_rotation_limit_level(struct ztp_device *cdev, int level)
{
	struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;
    struct syna_hw_attn_data *attn;
    struct tcm_dev *tcm_dev;
    unsigned int resp_handling;
	int ret = 0;

    LOGI("%s: enter\n", __func__);

 	if (!tcm)
		return -EINVAL;

	tcm_dev = tcm->tcm_dev;
	attn = &tcm->hw_if->bdata_attn;
	if ((attn->irq_id) && (attn->irq_enabled))
		resp_handling = CMD_RESPONSE_IN_ATTN;
	else
		resp_handling = tcm_dev->msg_data.command_polling_time;

	if (level > 3)
		level = 3;
	tcm->ztec.rotation_limit_level = level;

	if (tcm->pwr_state != PWR_ON) {
		LOGE("%s: error, change set in suspend!", __func__);
	} else {
		ret = syna_dev_set_display_rotation(tcm, cdev->display_rotation, resp_handling);
		if (ret) {
			LOGE("Write display rotation failed!");
		}
	}

	return 0;
}

static int tpd_set_display_rotation(struct ztp_device *cdev, int mrotation)
{
	struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;
    struct syna_hw_attn_data *attn;
    struct tcm_dev *tcm_dev;
    unsigned int resp_handling;
	int ret = 0;
	int retry = 0;

    LOGI("%s: enter\n", __func__);

 	if (!tcm)
		return -EINVAL;

	tcm_dev = tcm->tcm_dev;
	attn = &tcm->hw_if->bdata_attn;
	if ((attn->irq_id) && (attn->irq_enabled))
		resp_handling = CMD_RESPONSE_IN_ATTN;
	else
		resp_handling = tcm_dev->msg_data.command_polling_time;

	cdev->display_rotation = mrotation;
	tcm->ztec.display_rotation = mrotation;
	LOGI("%s:display_rotation=%d", __func__, cdev->display_rotation);

	while(retry < 5) {
		if(tcm->pwr_state == PWR_ON) {
			ret = syna_dev_set_display_rotation(tcm, cdev->display_rotation, resp_handling);
			if (ret < 0) {
				LOGE("set display_rotation mode failed!");
			}
			break;
		} else {
			LOGE("%s: still suspend mode %d\n", __func__, retry);
			msleep(200);
			retry++;
		}
	}

	if (retry >= 5) {
		LOGE("%s: error, no need retry!", __func__);
		return -EINVAL;
	}

	return cdev->display_rotation;
}

static char* trim(char* str)
{
    if (!str) return NULL;
    char* end;
    while (isspace(*str)) str++;
    if (*str == '\0') return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = '\0';
    return str;
}

static char* my_strdup(const char* s)
{
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = kmalloc(len, GFP_KERNEL);
    if (dup) memcpy(dup, s, len);
    return dup;
}

#define SYNA_RESOLUTION 10
static void change_coordinate(struct syna_tcm *tcm, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t out[4])
{
	LOGI("syna mrotation partition is %d", tcm->ztec.display_rotation);
	if (tcm->ztec.display_rotation == 1) {
		out[0] = (1504 - y2)*SYNA_RESOLUTION;
		out[1] = x1*SYNA_RESOLUTION;
		out[2] = (1504 - y1)*SYNA_RESOLUTION;
		out[3] = x2*SYNA_RESOLUTION;
	} else if (tcm->ztec.display_rotation == 3) {
		out[0] = y1*10;
		out[1] = (2400 - x2)*SYNA_RESOLUTION;
		out[2] = y2*10;
		out[3] = (2400 - x1)*SYNA_RESOLUTION;
	} else {
		out[0] = 0;
		out[1] = 0;
		out[2] = 0;
		out[3] = 0;
	}
}

char* string_change(struct syna_tcm *tcm, char *buffer, int *out_len)
{
    if (!buffer || *buffer == '\0') {
        char* empty = kmalloc(1, GFP_KERNEL);
        if (empty) *empty = '\0';
        *out_len = 0;
        return empty;
    }

    char* input_copy = my_strdup(buffer);
    if (!input_copy) {
        *out_len = 0;
        return NULL;
    }

    size_t max_len = strlen(buffer) * 3;
    unsigned char* output = kmalloc(max_len, GFP_KERNEL);
    if (!output) {
        kfree(input_copy);
        *out_len = 0;
        return NULL;
    }

    int output_index = 0;

    // Split input at colon
    char* colon_ptr = strchr(input_copy, ':');
    char* part1 = input_copy;
    char* part2 = colon_ptr ? colon_ptr + 1 : NULL;
    if (colon_ptr) *colon_ptr = '\0';

    // Process part1 (hex bytes before colon)
    if (part1 && *part1) {
        char* token;
        for (token = strsep(&part1, ","); token; token = strsep(&part1, ",")) {
            char* clean_token = trim(token);
            if (*clean_token) {
                unsigned char byte_val = (unsigned char)simple_strtoul(clean_token, NULL, 16);
                output[output_index++] = byte_val;
            }
        }
    }

    // Process part2 (segments after colon)
    if (part2 && *part2) {
        char* segment;
        int is_first_segment = 1;
        while ((segment = strsep(&part2, "."))) {
            char* clean_segment = trim(segment);
            if (!*clean_segment) continue;

            // Handle segment identifier
            char* seg_id = NULL;
            char* numbers_part = clean_segment;
            if (!is_first_segment) {
                char* colon_seg = strchr(clean_segment, ':');
                if (colon_seg) {
                    *colon_seg = '\0';
                    seg_id = trim(clean_segment);
                    numbers_part = trim(colon_seg + 1);
                }
            }

            // Add segment identifier if present
            if (seg_id) {
                unsigned char id_val = (unsigned char)simple_strtoul(seg_id, NULL, 16);
                output[output_index++] = id_val;
            }

            // Process four integers in segment
            char* numbers[4];
            int count = 0;
            char* num_token = numbers_part;
            for (count = 0; count < 4; count++) {
                char* num = strsep(&num_token, ",");
                if (!num) break;
                numbers[count] = trim(num);
            }

            if (count == 4) {
                for (int i = 0; i < 4; i++) {
					uint16_t tp_point[4];
               		uint16_t x1 = (uint16_t)simple_strtoul(numbers[0], NULL, 10);
                	uint16_t y1 = (uint16_t)simple_strtoul(numbers[1], NULL, 10);
                	uint16_t x2 = (uint16_t)simple_strtoul(numbers[2], NULL, 10);
                	uint16_t y2 = (uint16_t)simple_strtoul(numbers[3], NULL, 10);

                	change_coordinate(tcm,x1, y1, x2, y2, tp_point);

                    output[output_index++] = tp_point[i] & 0xFF;         // low
                    output[output_index++] = (tp_point[i] >> 8) & 0xFF;  // high
                }
            }
            is_first_segment = 0;
        }
    }

    kfree(input_copy);
    *out_len = output_index;
    return (char*)output;
}

static int tpd_set_game_partition(struct ztp_device *cdev, char *data_buf)
{
    struct syna_tcm *tcm = (struct syna_tcm *)cdev->private;
    struct syna_hw_attn_data *attn;
    struct tcm_dev *tcm_dev;
    unsigned int resp_handling;
    int out_len = 0;
    unsigned char* output = NULL;
	int retval;

    LOGI("%s: enter\n", __func__);

    if (!tcm)
        return -EINVAL;

    tcm_dev = tcm->tcm_dev;
    attn = &tcm->hw_if->bdata_attn;
    if ((attn->irq_id) && (attn->irq_enabled))
        resp_handling = CMD_RESPONSE_IN_ATTN;
    else
        resp_handling = tcm_dev->msg_data.command_polling_time;

    LOGI("%s", data_buf);

    output = (unsigned char*)string_change(tcm, data_buf, &out_len);

	if (tcm->pwr_state != PWR_ON) {
		LOGE("%s: error, change set in suspend!", __func__);
	} else {
		retval = syna_tcm_set_game_partition_config(tcm->tcm_dev,
				0x03,
				out_len,
				output,
				resp_handling);
		if (retval < 0) {
			LOGE("set game_partition failed");
			return retval;
		}
	}

    return 0;
}

void syna_tpd_register_fw_class(struct syna_tcm *tcm)
{
	LOGI("%s: entry", __func__);

	tpd_cdev->private = (void *)tcm;

    tpd_cdev->get_tpinfo = tpd_init_tpinfo;

	tpd_cdev->tp_self_test = tpd_test_cmd_store;
	tpd_cdev->get_tp_self_test_result = tpd_test_cmd_show;

    tpd_cdev->get_tp_report_rate = tpd_get_tp_report_rate;
	tpd_cdev->set_tp_report_rate = tpd_set_tp_report_rate;

	tpd_cdev->get_sensibility = tpd_get_sensibility_level;
	tpd_cdev->set_sensibility = tpd_set_sensibility_level;

	tpd_cdev->get_gesture = tpd_get_wakegesture;
	tpd_cdev->wake_gesture = tpd_enable_wakegesture;

	tpd_cdev->get_follow_hand_level = tpd_get_follow_hand_level;
	tpd_cdev->set_follow_hand_level = tpd_set_follow_hand_level;

	tpd_cdev->get_stability_level = tpd_get_stability_level;
	tpd_cdev->set_stability_level = tpd_set_stability_level;

	tpd_cdev->get_rotation_limit_level = tpd_get_rotation_limit_level;
	tpd_cdev->set_rotation_limit_level = tpd_set_rotation_limit_level;
	tpd_cdev->set_display_rotation = tpd_set_display_rotation;

	tpd_cdev->get_play_game = tpd_get_play_game;
	tpd_cdev->set_play_game = tpd_set_play_game;

	tpd_cdev->set_game_partition = tpd_set_game_partition;

	tpd_cdev->max_x = tcm->input_dev_params.max_x;
	tpd_cdev->max_y = tcm->input_dev_params.max_y;

	LOGI("%s: end", __func__);
}
