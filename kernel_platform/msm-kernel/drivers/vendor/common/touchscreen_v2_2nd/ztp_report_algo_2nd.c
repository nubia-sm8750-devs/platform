
/***********************
 * file : ztp_report_algo_2nd.c
 **********************/
#include <linux/module.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include "ztp_common_2nd.h"

#define MAX_POINTS_SUPPORT 10
#define LONG_PRESS_MIN_COUNT 50
#define MAX_INSERT_POINTS 10
static void edge_point_report_2nd(int id);
static void edge_long_press_up_2nd(struct input_dev *input, u16 id);
static void point_report_reset_2nd(int id);
static bool is_have_other_point_down_2nd(int id);
static bool is_have_inside_point_down_2nd(void);
static bool is_have_half_screen_area_point_down_2nd(u8 half_screen_area);
void tpd_touch_release_2nd(struct input_dev *input, u16 id);

typedef struct point_info_2nd {
	int x;
	int y;
	u8 touch_major;
	u8 pressure;
} tpd_point_info_t_2nd;

typedef struct point_fifo_2nd {
	tpd_point_info_t_2nd point_data[MAX_INSERT_POINTS + 2];
	tpd_point_info_t_2nd first_report_point_data;
	tpd_point_info_t_2nd last_point;
	tpd_point_info_t_2nd mistake_touch_check_point;
	bool is_report_point_2nd;
	bool save_first_down_point;
	bool is_moving_in_limit_area;
	bool finger_down;
	bool edge_finger_down;
	bool temp_ctrl_zone_down;
	bool limit_area_log_print;
	bool is_inside_finger_down;
	bool jitter_check;
	bool mistake_touch_check;
	bool cancel_clean_edge_area_ponit;
	bool edge_area_move;
	unsigned long touch_down_timer;
	unsigned long edge_down_timer;
	unsigned long long_press_timer_2nd;
	unsigned long down_up_time;
	u16 single_ghost_detect_count;
	u16 multi_ghost_detect_count;
	struct input_dev *input;
} tpd_point_fifo_t_2nd;

tpd_point_fifo_t_2nd point_report_info[MAX_POINTS_SUPPORT];

#define tpd_idn_report_work_2nd(id)\
static void tpd_id##id##_report_work(struct work_struct *work)\
{\
	edge_long_press_up_2nd(tpd_cdev_2nd->input, id);\
}

tpd_idn_report_work_2nd(0)
tpd_idn_report_work_2nd(1)
tpd_idn_report_work_2nd(2)
tpd_idn_report_work_2nd(3)
tpd_idn_report_work_2nd(4)
tpd_idn_report_work_2nd(5)
tpd_idn_report_work_2nd(6)
tpd_idn_report_work_2nd(7)
tpd_idn_report_work_2nd(8)
tpd_idn_report_work_2nd(9)

static bool point_in_report_judge_area_2nd(u16 x, u16 y)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->display_rotation == mRotatin_0) {
		if ((x < cdev->long_pess_suppression_2nd[0] * 3  / 2) || (x > cdev->max_x - cdev->long_pess_suppression_2nd[1] * 3 / 2))
			return true;
	}
	return false;
}

u16 math_sqrt2_2nd(u32 value)
{
	u32 nsquare = 1;
	u32 ndelta = 3;

	while (nsquare <= value) {
		nsquare += ndelta;
		ndelta += 2;
	}
	return (ndelta / 2 -1);
}

u16 dis_between_2_point_2nd(u16 x1, u16 y1 ,u16 x2, u16 y2)
{
	s32 disx = 0, disy = 0;

	disx = 	(s32)(x1 - x2);
	disy = 	(s32)(y1 - y2);
	return math_sqrt2_2nd(disx * disx + disy * disy);
}

static bool angle_suppression_area_2nd(u16 level, u16 x, u16 y)
{
	u16 r = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (level == 0)
		return false;
	r = cdev->max_x / 10 * level;
	if (cdev->display_rotation == mRotatin_90) {
		if ((dis_between_2_point_2nd(x, y, 0, 0) <= r) && is_have_half_screen_area_point_down_2nd(0))
			return true;
		if (( dis_between_2_point_2nd(x, y, 0, cdev->max_y) < r) && is_have_half_screen_area_point_down_2nd(1))
			return true;
	} else if (cdev->display_rotation == mRotatin_270) {
		if ((dis_between_2_point_2nd(x, y, cdev->max_x, 0) < r)  && is_have_half_screen_area_point_down_2nd(0))
			return true;
		if ((dis_between_2_point_2nd(x, y, cdev->max_x, cdev->max_y) <= r)  && is_have_half_screen_area_point_down_2nd(1))
			return true;
	}
	return false;
}

static bool point_is_in_limit_area_2nd(u16 x, u16 y)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (angle_suppression_area_2nd(cdev->rotation_limit_level, x, y))
		return true;
	if (cdev->display_rotation == mRotatin_90 || cdev->display_rotation == mRotatin_270) {
		if ((x < cdev->edge_report_limit_2nd[0]) || (x > cdev->max_x - cdev->edge_report_limit_2nd[1]) ||
			(y < cdev->edge_report_limit_2nd[2]) || (y > cdev->max_y - cdev->edge_report_limit_2nd[3]))
			return true;
	} else {
		if ((x  < cdev->edge_report_limit_2nd[0]) || (x > cdev->max_x - cdev->edge_report_limit_2nd[1]))
			return true;
		if (point_in_report_judge_area_2nd(x, y) && is_have_inside_point_down_2nd()) {
			TPD_DBG("have other point down and tpd Press in judge area: x = %d, y = %d\n", x, y);
			return true;
		}
		if (cdev->edge_limit_pixel_level > 0) {
			if ((y > cdev->user_edge_limit[1]) &&
				(((x < cdev->user_edge_limit[0]) || (x > cdev->max_x - cdev->user_edge_limit[0]))))
				return true;
		}
	}
	return false;
}

static bool point_in_long_pess_suppression_area_2nd(u16 x, u16 y)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->edge_long_press_check_2nd == false)
		return false;

	if (cdev->display_rotation == mRotatin_90 || cdev->display_rotation == mRotatin_270) {
		if ((x < cdev->long_pess_suppression_2nd[0]) || (x > cdev->max_x - cdev->long_pess_suppression_2nd[1]) ||
			(y < cdev->long_pess_suppression_2nd[2]) || (y > cdev->max_y - cdev->long_pess_suppression_2nd[3]))
			return true;
		if (cdev->edge_limit_pixel_level > 0) {
			if ((y > cdev->user_edge_limit[1]) &&
				(((x < cdev->user_edge_limit[0]) || (x > cdev->max_x - cdev->user_edge_limit[0]))))
				return true;
		}
	} else {
		if ((x  < cdev->long_pess_suppression_2nd[0]) || (x > cdev->max_x - cdev->long_pess_suppression_2nd[1]))
			return true;
	}
	return false;
}

static bool is_ghost_ignore_edge_area_2nd(u16 x, u16 y)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->display_rotation == mRotatin_90 || cdev->display_rotation == mRotatin_270) {
		if (y < cdev->ghost_check_ignore_edge_area_2nd ||  y > cdev->max_y - cdev->ghost_check_ignore_edge_area_2nd
			 || x < cdev->ghost_check_ignore_edge_area_2nd || x > cdev->max_x - cdev->ghost_check_ignore_edge_area_2nd) {
			TPD_DBG("in ghost ignore edge area");
			return true;
		}
	} else {
		if (x < cdev->ghost_check_ignore_edge_area_2nd || x > cdev->max_x - cdev->ghost_check_ignore_edge_area_2nd) {
			TPD_DBG("in ghost ignore edge area");
			return true;
		}
	}
	return false;
}

static bool is_ghost_ignore_corner_area_2nd(u16 x, u16 y)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->display_rotation == mRotatin_90 || cdev->display_rotation == mRotatin_270) {
		if (((x < cdev->ghost_check_ignore_corner_y_2nd) || (x >  cdev->max_x - cdev->ghost_check_ignore_corner_y_2nd))
			 && ((y < cdev->ghost_check_ignore_corner_x_2nd) || (y > cdev->max_y - cdev->ghost_check_ignore_corner_x_2nd))) {
			TPD_DBG("in ghost_ignore_corner_area");
			return true;
		}
	} else {
		if (cdev->display_rotation == mRotatin_0) {
			if ((y > cdev->max_y - cdev->ghost_check_ignore_corner_y_2nd) 
				&& ((x < cdev->ghost_check_ignore_corner_x_2nd) || (x >  cdev->max_x - cdev->ghost_check_ignore_corner_x_2nd))) {
				TPD_DBG("in ghost_ignore_corner_area");
				return true;
			}
		}
		if (cdev->display_rotation == mRotatin_180) {
			if ((y < cdev->ghost_check_ignore_corner_y_2nd) 
				&& ((x < cdev->ghost_check_ignore_corner_x_2nd) || (x >  cdev->max_x - cdev->ghost_check_ignore_corner_x_2nd))) {
				TPD_DBG("in ghost_ignore_corner_area");
				return true;
			}
		}
	}
	return false;
}

static bool ghost_check_area_2nd(tpd_point_fifo_t_2nd *point)
{
	if (is_ghost_ignore_edge_area_2nd(point->first_report_point_data.x, point->first_report_point_data.y)
		|| is_ghost_ignore_corner_area_2nd(point->first_report_point_data.x, point->first_report_point_data.y)) {
			return false;
		}
	return true;
}

#ifdef CONFIG_TOUCHSCREEN_TEMP_CTRL_ZONE_NOTIFY_2nd
static temp_control_zeon_2nd(int x, int y)
{
	int ret = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if ((cdev->max_x / 2 - cdev->max_x / 10 < x) &&
	  	(cdev->max_x / 2 + cdev->max_x / 10 > x) &&
		(cdev->max_y  - cdev->max_y / 10 < y) ) {
		ret = 1;
	}

	return ret;
}

static bool temp_control_zeon_down_2nd(void)
{
	int i = 0;

	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		if (point_report_info[i].temp_ctrl_zone_down)
			return true;
	}
	return false;
}

static void temp_control_zone_check_reset_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->start_temp_ctrl_timer) {
		cancel_delayed_work_sync(&cdev->temp_ctr_zone_work);
		TPD_DMESG("cancel temp control zone work");
		cdev->start_temp_ctrl_timer = false;
	}
	if (cdev->notify_temp_ctrl_zone_down) {
#ifdef CONFIG_TOUCHSCREEN_LCD_NOTIFY_2nd
		tpd_notifier_call_chain_2nd(TEMP_CTRL_ZONE_UP);
#endif
		TPD_DMESG("notify temp control zone up");
		cdev->notify_temp_ctrl_zone_down = false;
	}
}

static int temp_control_zone_check(tpd_point_fifo_t_2nd *point, u16 x, u16 y)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (!cdev->charger_mode) {
		temp_control_zone_check_reset_2nd();
		return 0;
	}
	if (temp_control_zeon_2nd(x, y))
		point->temp_ctrl_zone_down = true;
	else
		point->temp_ctrl_zone_down = false;
	if (!cdev->notify_temp_ctrl_zone_down && !cdev->start_temp_ctrl_timer  && point->temp_ctrl_zone_down) {
		queue_delayed_work(cdev->tpd_report_wq, &cdev->temp_ctr_zone_work, msecs_to_jiffies(30000));
		TPD_DMESG("start temp control zone timer");
		cdev->start_temp_ctrl_timer = true;
	} else if (!point->temp_ctrl_zone_down) {
		if (!temp_control_zeon_down_2nd()) {
			temp_control_zone_check_reset_2nd();
		}
	}
	return 0;
}
#endif

static bool is_long_pess_clean_area_2nd(u16 x, u16 y)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	if (cdev->edge_long_press_check_2nd == false) {
		return false;
	}
	if (cdev->display_rotation == mRotatin_90 || cdev->display_rotation == mRotatin_270) {
		if ((y < cdev->long_pess_suppression_2nd[2]) || (y > cdev->max_y - cdev->long_pess_suppression_2nd[3])) {
			return true;
		}
	} else {
		if ((x  < cdev->long_pess_suppression_2nd[0]) || (x > cdev->max_x - cdev->long_pess_suppression_2nd[1])) {
			return true;
		}
	}
	return false;
}

static bool is_need_clean_long_pess_area_down_2nd(tpd_point_fifo_t_2nd *point, u16 x, u16 y)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->edge_long_press_check_2nd == false || point->cancel_clean_edge_area_ponit) {
		return false;
	}
	if (cdev->display_rotation == mRotatin_90 || cdev->display_rotation == mRotatin_270) {
		if (y > cdev->max_y / 10 &&  y < cdev->max_y  * 9 / 10) {
			point->cancel_clean_edge_area_ponit = true;
		}
		if (point->first_report_point_data.y < cdev->max_y / 10 ||  point->first_report_point_data.y > cdev->max_y  * 9 / 10) {
			return true;
		} else {
			return false;
		}
	} else {
		if (x > cdev->max_x / 4 &&  x < cdev->max_x  * 3 / 4)
			point->cancel_clean_edge_area_ponit = true;
		if (point->first_report_point_data.x < cdev->max_x / 4 ||  point->first_report_point_data.x > cdev->max_x  * 3 / 4)
			return true;
	}
	return false;
}

static bool is_report_point_2nd(u16 x, u16 y, u16 id, u8 touch_major, u8  pressure)
{
	tpd_point_fifo_t_2nd *point = &point_report_info[id];
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	u16 limit_pixel = cdev->max_x / 10;

	if (tpd_cdev_2nd->input == NULL) {
		TPD_DMESG("tpd_cdev_2nd->input is NULL\n");
		return false;
	}
	if (point->is_report_point_2nd) {
		if (is_need_clean_long_pess_area_down_2nd(point, x, y) == false)
			return true;
		if (is_long_pess_clean_area_2nd(x, y)) {
			if (!point->mistake_touch_check) {
				point->mistake_touch_check_point.x = x;
				point->mistake_touch_check_point.y = y;
				point->mistake_touch_check = true;
				point->edge_down_timer = jiffies;
			}
			if(jiffies_to_msecs(jiffies - point->edge_down_timer) > 800) {
				if ((abs(point->mistake_touch_check_point.x  - x) > 20
					|| abs(point->mistake_touch_check_point.y - y) > 20)) {
					point->mistake_touch_check_point.x = x;
					point->mistake_touch_check_point.y = y;
					point->edge_down_timer = jiffies;
					return true;
				}
				tpd_touch_release_2nd(tpd_cdev_2nd->input,  id);
				point->mistake_touch_check = false;
				return false;
			} else {
				return true;
			}
		}
		point->mistake_touch_check = false;
		return true;
	}
	if (point_is_in_limit_area_2nd(x, y) || point_in_long_pess_suppression_area_2nd(x, y)) {
		if ((!point_is_in_limit_area_2nd(x, y) && point_in_long_pess_suppression_area_2nd(x, y))) {
			if (point->limit_area_log_print == false) {
				point->limit_area_log_print = true;
				point->long_press_timer_2nd = jiffies;
				TPD_DMESG("tpd Press in long pess suppression area: id = %d, x = %d, y = %d\n", id, x, y);
			}
			if (is_have_inside_point_down_2nd()) {
				point->is_inside_finger_down = true;
			}
		} else {
			if (point->limit_area_log_print == false) {
				point->limit_area_log_print = true;
				TPD_DMESG("tpd Press in limit area: id = %d, x = %d, y = %d\n", id, x, y);
			}
		}
		if (point->save_first_down_point == false) {
			point->point_data[0].x = x;
			point->point_data[0].y = y;
			point->point_data[0].touch_major = touch_major;
			point->point_data[0].pressure = pressure;
			point->save_first_down_point = true;
			return false;
		}
		if (abs(point->point_data[0].x - x) > limit_pixel
			|| abs(point->point_data[0].y - y) > limit_pixel) {
			goto save_last_ponit;

		} else {
			return false;
		}
	}
save_last_ponit:
	if (point->save_first_down_point == false) {
		point->is_moving_in_limit_area = false;
		return true;
	}
	point->point_data[MAX_INSERT_POINTS + 1].x = x;
	point->point_data[MAX_INSERT_POINTS + 1].y = y;
	point->point_data[MAX_INSERT_POINTS + 1].touch_major = touch_major;
	point->point_data[MAX_INSERT_POINTS + 1].pressure = pressure;
	point->is_moving_in_limit_area = true;
	return true;
}

static void tpd_touch_report_2nd(struct input_dev *input, u16 x, u16 y, u16 id, u8 touch_major, u8  pressure)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	mutex_lock(&cdev->report_mutex);
	input_mt_slot(input, id);
	input_mt_report_slot_state(input, MT_TOOL_FINGER, true);
	input_report_key(input, BTN_TOUCH, 1);
	input_report_abs(input, ABS_MT_POSITION_X, x);
	input_report_abs(input, ABS_MT_POSITION_Y, y);
	if (pressure)
		input_report_abs(input, ABS_MT_PRESSURE, pressure);
	if (touch_major)
		input_report_abs(input, ABS_MT_TOUCH_MAJOR, touch_major);
	mutex_unlock(&cdev->report_mutex);
}

static void tpd_touch_report_nolock_2nd(struct input_dev *input, u16 x, u16 y, u16 id, u8 touch_major, u8  pressure)
{
	input_mt_slot(input, id);
	input_mt_report_slot_state(input, MT_TOOL_FINGER, true);
	input_report_key(input, BTN_TOUCH, 1);
	input_report_abs(input, ABS_MT_POSITION_X, x);
	input_report_abs(input, ABS_MT_POSITION_Y, y);
	if (pressure)
		input_report_abs(input, ABS_MT_PRESSURE, pressure);
	if (touch_major)
		input_report_abs(input, ABS_MT_TOUCH_MAJOR, touch_major);
}

static bool is_have_other_point_down_2nd(int id)
{
	int i = 0;

	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		if (i == id)
			continue;
		if ((point_report_info[i].finger_down || point_report_info[i].edge_finger_down))
			return true;
	}
	return false;
}

static bool is_have_inside_point_down_2nd(void)
{
	int i = 0;

	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		if (point_report_info[i].finger_down)
			return true;
	}
	return false;
}

static bool is_have_half_screen_area_point_down_2nd(u8 half_screen_area)
{
	int i = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		if (point_report_info[i].finger_down) {
			if (half_screen_area == 0) {
				if (point_report_info[i].last_point.y < cdev->max_y / 2)
					return true;
			} else {
				if (point_report_info[i].last_point.y > cdev->max_y / 2)
					return true;
			}
		}
	}
	return false;
}

static void edge_long_press_up_2nd(struct input_dev *input, u16 id)
{
	tpd_point_fifo_t_2nd *point = &point_report_info[id];
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;


	if (point->edge_finger_down == false) {
		return;
	}
	mutex_lock(&cdev->report_mutex);
	input_mt_slot(input, id);
	input_mt_report_slot_state(input, MT_TOOL_FINGER, false);
	if (!is_have_other_point_down_2nd(id)) {
		input_report_key(input, BTN_TOUCH, 0);
	}
	input_sync(input);
	mutex_unlock(&cdev->report_mutex);
	point->edge_finger_down = false;
	TPD_DMESG("tpd touch up id: %d, coord [%d:%d]\n",
		id, point->point_data[0].x, point->point_data[0].y);

}

void get_interpolation_point_2nd(u16 id)
{
	int i = 0;
	s16 move_distance_x = 0;
	s16 move_distance_y = 0;
	tpd_point_fifo_t_2nd *point = &point_report_info[id];

	move_distance_x = point->point_data[MAX_INSERT_POINTS + 1].x - point->point_data[0].x;
	move_distance_y = point->point_data[MAX_INSERT_POINTS + 1].y - point->point_data[0].y;
	for (i = 1; i < MAX_INSERT_POINTS + 1; i++) {
		point->point_data[i].x = point->point_data[0].x + i * move_distance_x / (MAX_INSERT_POINTS + 1);
		point->point_data[i].y = point->point_data[0].y + i * move_distance_y / (MAX_INSERT_POINTS + 1);
		point->point_data[i].touch_major = point->point_data[MAX_INSERT_POINTS + 1].touch_major;
		point->point_data[i].pressure = point->point_data[MAX_INSERT_POINTS + 1].pressure;
	}
}

void tpd_touch_press_2nd(struct input_dev *input, u16 x, u16 y, u16 id, u8 touch_major, u8  pressure)
{
	int i = 0;
	tpd_point_fifo_t_2nd *point = NULL;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (cdev->display_rotation) {
		if (x == 0)
			x = x + 1;
		if (y == 0)
			y = y + 1;
	}

	if (input == NULL || id >= MAX_POINTS_SUPPORT) {
		TPD_DMESG("input is NULL? id = %d", id);
		return;
	}
	if (tpd_cdev_2nd->input == NULL) {
		tpd_cdev_2nd->input = input;
		TPD_DMESG("tpd_cdev_2nd->input is NULL\n");
		return;
	}
#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
	cdev->touch_press = true;
#endif
	mutex_lock(&cdev->report_down_mutex);
	point = &point_report_info[id];
#ifdef CONFIG_TOUCHSCREEN_TEMP_CTRL_ZONE_NOTIFY_2nd
	temp_control_zone_check(point, x, y);
#endif
	if (is_report_point_2nd(x, y, id, touch_major, pressure) == false) {
		mutex_unlock(&cdev->report_down_mutex);
		return;
	}

	point->is_report_point_2nd = true;
	if (point->is_moving_in_limit_area) {
		get_interpolation_point_2nd(id);
		for (i = 0; i < MAX_INSERT_POINTS + 2; i++) {
			if (point->finger_down == false) {
				cdev->point_down_num++;
				point->finger_down = true;
				point->touch_down_timer = jiffies;
				point->edge_area_move = true;
				point->first_report_point_data.x = point->point_data[0].x;
				point->first_report_point_data.y = point->point_data[0].y;
				point_report_reset_2nd(id);
				TPD_DMESG("tpd touch down id: %d, coord [%d:%d]\n",
					id, point->point_data[i].x, point->point_data[i].y);
			}
			tpd_touch_report_2nd(input, point->point_data[i].x, point->point_data[i].y,
				id, touch_major, pressure);
			if (i < MAX_INSERT_POINTS + 1) {
				input_sync(input);
				usleep_range(1000, 1500);
			}
		}
	} else {
		if (cdev->tp_jitter_check_2nd) {
			if (point->finger_down == false) {
				point->finger_down = true;
				cdev->point_down_num++;
				point_report_reset_2nd(id);
				point->first_report_point_data.x = x;
				point->first_report_point_data.y = y;
				point->touch_down_timer = jiffies;
				point->jitter_check = true;
				TPD_DMESG("tpd touch down id: %d, coord [%d:%d]. jitter check %d pixe\n",
					id, x, y, cdev->tp_jitter_check_2nd);
				tpd_touch_report_2nd(input, x, y, id, touch_major, pressure);
			} else {
				if (point->jitter_check) {
					if (jiffies_to_msecs(jiffies - point->touch_down_timer) > 100) {
						if (abs(point->first_report_point_data.x - x) > cdev->tp_jitter_check_2nd
							|| abs(point->first_report_point_data.y - y) > cdev->tp_jitter_check_2nd) {
							tpd_touch_report_2nd(input, x, y, id, touch_major, pressure);
							point->jitter_check = false;
						}
					} else if (abs(point->first_report_point_data.x - x) > cdev->tp_jitter_check_2nd * 3
							|| abs(point->first_report_point_data.y - y) > cdev->tp_jitter_check_2nd * 3) {
						tpd_touch_report_2nd(input, x, y, id, touch_major, pressure);
						point->jitter_check = false;
					}
				} else {
					tpd_touch_report_2nd(input, x, y, id, touch_major, pressure);
				}
			}
		}else {

			if (point->finger_down == false) {
				point->finger_down = true;
				cdev->point_down_num++;
				point->first_report_point_data.x = x;
				point->first_report_point_data.y = y;
				point_report_reset_2nd(id);
				point->touch_down_timer = jiffies;
				TPD_DMESG("tpd touch down id: %d, coord [%d:%d]\n", id, x, y);
			}
			tpd_touch_report_2nd(input, x, y, id, touch_major, pressure);
		}
	}
	point->last_point.x = x;
	point->last_point.y = y;
	point->is_moving_in_limit_area = false;
	mutex_unlock(&cdev->report_down_mutex);
}
EXPORT_SYMBOL_GPL(tpd_touch_press_2nd);

void tpd_touch_release_2nd(struct input_dev *input, u16 id)
{
	tpd_point_fifo_t_2nd *point = NULL;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	u8 ghost_check_time = 25;

	if (input == NULL || id >= MAX_POINTS_SUPPORT) {
		TPD_DMESG("input is NULL? id = %d", id);
		return;
	}
#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
	cdev->touch_press = false;
#endif
	point = &point_report_info[id];
	if (point->finger_down) {
		mutex_lock(&cdev->report_mutex);
		input_mt_slot(input, id);
		input_mt_report_slot_state(input, MT_TOOL_FINGER, false);
		point->down_up_time = jiffies_to_msecs(jiffies - point->touch_down_timer);
		TPD_DMESG("tpd touch up id: %d, coord [%d:%d],down-up time:%lu.\n",
			id, point->last_point.x, point->last_point.y, point->down_up_time);
		mutex_unlock(&cdev->report_mutex);

		if ((point->down_up_time < cdev->ghost_check_start_time_2nd) && !point->edge_area_move){
			if (!cdev->start_ghost_check_timer) {
				TPD_DMESG("start ghost check timer.");
				cdev->start_ghost_check_timer = true;
				queue_delayed_work(cdev->tpd_wq, &cdev->ghost_check_work, msecs_to_jiffies(2000));
			}
			if (cdev->point_down_num > 2) {
				ghost_check_time = cdev->ghost_check_multi_time_2nd;
			} else {
				ghost_check_time = cdev->ghost_check_single_time_2nd;
			}
			if (ghost_check_area_2nd(point)) {
				if (point->down_up_time < ghost_check_time) {
					point->single_ghost_detect_count++;
					point->multi_ghost_detect_count++;
				} else {
					point->multi_ghost_detect_count++;
				}
			}
			TPD_DMESG("touch id(%d):single_ghost_detect_count:%d,multi_ghost_detect_count:%d",
				id, point->single_ghost_detect_count, point->multi_ghost_detect_count);
		}
		cdev->point_down_num--;
	}
	if (cdev->edge_long_press_check_2nd && !point->is_inside_finger_down && (point->long_press_timer_2nd != 0)
		&& (jiffies_to_msecs(jiffies - point->long_press_timer_2nd) < cdev->edge_long_press_timer_2nd)) {
		edge_point_report_2nd(id);
	}
	point->edge_area_move = false;
	point->long_press_timer_2nd = 0;
	point->finger_down = false;
	point->is_report_point_2nd = false;
	point->save_first_down_point = false;
	point->limit_area_log_print = false;
	point->is_inside_finger_down = false;
	point->jitter_check = false;
	point->mistake_touch_check = false;
	point->cancel_clean_edge_area_ponit = false;
#ifdef CONFIG_TOUCHSCREEN_TEMP_CTRL_ZONE_NOTIFY_2nd
	point->temp_ctrl_zone_down = false;
	if (!temp_control_zeon_down_2nd()) {
		temp_control_zone_check_reset_2nd();
	}
#endif
#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
	tpd_clean_diffdata_2nd();
#endif
}
EXPORT_SYMBOL_GPL(tpd_touch_release_2nd);

bool tp_ghost_check_2nd(void)
{
	int i = 0;
	int len = 0;
	u8 ghost_point_num = 0;
	u16 ghost_count = 0;
	u16 ghost_check_count = 5;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	char *log_buffer = NULL;
	int point_down_num;

	log_buffer = vmalloc(ZLOG_INFO_LEN);
	if (!log_buffer) {
		TPD_DMESG("log_buffer malloc fail");
		return false;
	}
	TPD_DMESG("enter");
	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		if (point_report_info[i].multi_ghost_detect_count > 0) {
			ghost_point_num ++;
		}
	}
	point_down_num = cdev->point_down_num > ghost_point_num ? cdev->point_down_num : ghost_point_num;
	if (point_down_num > 2) {
		ghost_check_count = cdev->ghost_check_multi_count_2nd;
	} else {
		ghost_check_count = cdev->ghost_check_single_count_2nd;
	}
	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		if (i == cdev->ghost_check_ignore_id_2nd)
			continue;
		if (point_report_info[i].single_ghost_detect_count >= ghost_check_count) {
			len += snprintf(log_buffer + len, ZLOG_INFO_LEN - len,
				"single ghost detect,touch id:%d, count:%d ", i, point_report_info[i].single_ghost_detect_count);
			goto ghost_point_report_log;
		}
		if (point_report_info[i].multi_ghost_detect_count > 0) {
			ghost_count += point_report_info[i].multi_ghost_detect_count;
			if ((ghost_point_num > 5) && (ghost_count > (ghost_point_num * ghost_check_count))) {
				len += snprintf(log_buffer + len, ZLOG_INFO_LEN - len,
					"multi ghost detect,ghost_count:%d. ", ghost_count);
				goto ghost_point_report_log;
			}
		}
	}
	vfree(log_buffer);
	return false;
ghost_point_report_log:
	len += snprintf(log_buffer + len, ZLOG_INFO_LEN - len, "point_down_num: %d.", point_down_num);
	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		if (point_report_info[i].multi_ghost_detect_count) {
			len += snprintf(log_buffer + len, ZLOG_INFO_LEN - len,
				" point[%d] down: %d, %d. ", i, point_report_info[i].first_report_point_data.x,
				point_report_info[i].first_report_point_data.y);
			len += snprintf(log_buffer + len, ZLOG_INFO_LEN - len,
				" point[%d] up: %d, %d. ", i, point_report_info[i].last_point.x,
				point_report_info[i].last_point.y);
		}
	};
	TPD_DMESG("%s", log_buffer);
#ifdef CONFIG_VENDOR_ZTE_LOG_EXCEPTION_2nd
	tpd_print_zlog_2nd(log_buffer);
#endif
	tpd_zlog_record_notify_2nd(TP_GHOST_ERROR_NO);
	vfree(log_buffer);
	return true;
}

void ghost_check_reset_2nd(void)
{
	int i = 0;

	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		point_report_info[i].single_ghost_detect_count = 0;
		point_report_info[i].multi_ghost_detect_count = 0;
	}
}

void tpd_clean_all_event_2nd(void)
{
	int i = 0;

	for (i = 0; i < MAX_POINTS_SUPPORT; i++) {
		point_report_info[i].finger_down = false;
		point_report_info[i].edge_finger_down = false;
#ifdef CONFIG_TOUCHSCREEN_TEMP_CTRL_ZONE_NOTIFY_2nd
		point_report_info[i].temp_ctrl_zone_down = false;
#endif
		point_report_info[i].is_report_point_2nd = false;
		point_report_info[i].save_first_down_point = false;
		point_report_info[i].is_moving_in_limit_area = false;
		point_report_info[i].limit_area_log_print = false;
		point_report_info[i].is_inside_finger_down = false;
		point_report_info[i].jitter_check = false;
		point_report_info[i].mistake_touch_check = false;
		point_report_info[i].cancel_clean_edge_area_ponit = false;
		point_report_info[i].edge_area_move = false;
	}
#ifdef CONFIG_TOUCHSCREEN_TEMP_CTRL_ZONE_NOTIFY_2nd
	temp_control_zone_check_reset_2nd();
#endif
#ifdef CONFIG_TOUCHSCREEN_KNUCKLE
	tpd_clean_diffdata_2nd();
#endif
}
EXPORT_SYMBOL_GPL(tpd_clean_all_event_2nd);

static void edge_point_report_2nd(int id)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
	tpd_point_fifo_t_2nd *point = &point_report_info[id];

	TPD_DMESG("tpd id:%d", id);
	if (!cdev->tpd_report_wq) {
		TPD_DMESG("tpd_report_wq is null");
		return;
	}
	if (is_have_inside_point_down_2nd()) {
		TPD_DMESG("have inside point down");
		return;
	}
	if (tpd_cdev_2nd->input == NULL) {
		TPD_DMESG("tpd_cdev_2nd->input is NULL\n");
		return;
	}
	mutex_lock(&cdev->report_mutex);
	tpd_touch_report_nolock_2nd(tpd_cdev_2nd->input, point->point_data[0].x, point->point_data[0].y,
				id, point->point_data[0].touch_major, point->point_data[0].pressure);
	input_sync(tpd_cdev_2nd->input);
	point->edge_finger_down = true;
	TPD_DMESG("tpd touch down id: %d, coord [%d:%d]\n",
		id, point->point_data[0].x, point->point_data[0].y);
	mutex_unlock(&cdev->report_mutex);

	switch (id) {
	case 0:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work0, msecs_to_jiffies(50));
		break;
	case 1:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work1, msecs_to_jiffies(50));
		break;
	case 2:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work2, msecs_to_jiffies(50));
		break;
	case 3:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work3, msecs_to_jiffies(50));
		break;
	case 4:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work4, msecs_to_jiffies(50));
		break;
	case 5:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work5, msecs_to_jiffies(50));
		break;
	case 6:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work6, msecs_to_jiffies(50));
		break;
	case 7:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work7, msecs_to_jiffies(50));
		break;
	case 8:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work8, msecs_to_jiffies(50));
		break;
	case 9:
		queue_delayed_work(cdev->tpd_report_wq, &cdev->tpd_report_work9, msecs_to_jiffies(50));
		break;
	default:
		TPD_DMESG("error id %d", id);
	}
}

static void point_report_reset_2nd(int id)
{
	tpd_point_fifo_t_2nd *point = &point_report_info[id];
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (tpd_cdev_2nd->input == NULL) {
		TPD_DMESG("tpd_cdev_2nd->input is NULL\n");
		return;
	}
	if (point->edge_finger_down) {
		TPD_DMESG("tpd touch up id: %d\n", id);
		point->edge_finger_down = false;
		mutex_lock(&cdev->report_mutex);
		input_mt_slot(tpd_cdev_2nd->input, id);
		input_mt_report_slot_state(tpd_cdev_2nd->input, MT_TOOL_FINGER, false);
		input_sync(tpd_cdev_2nd->input);
		mutex_unlock(&cdev->report_mutex);
		usleep_range(1000, 1100);
	}
}

#ifdef CONFIG_TOUCHSCREEN_POINT_REPORT_CHECK_2nd
static void ts_point_report_check_2nd(struct work_struct *work)
{
	int id = 0;
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if (tpd_cdev_2nd->input) {
		TPD_DMESG("Release all touch");
		mutex_lock(&cdev->report_mutex);
		for (id = MAX_POINTS_SUPPORT - 1; id >= 0; id--) {
			input_mt_slot(tpd_cdev_2nd->input, id);
			input_mt_report_slot_state(tpd_cdev_2nd->input, MT_TOOL_FINGER, false);
		}
		input_report_key(tpd_cdev_2nd->input, BTN_TOUCH, 0);
		input_sync(tpd_cdev_2nd->input);
		mutex_unlock(&cdev->report_mutex);
		tpd_clean_all_event_2nd();
	}
}
#endif

#ifdef CONFIG_TOUCHSCREEN_PSENSOR_REPORT_CHECK_2nd
static void ts_psensor_report_check_2nd(struct work_struct *work)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	if(cdev->tp_psensor_report_check_2nd)
		cdev->tp_psensor_report_check_2nd(cdev);
}
#endif

#ifdef CONFIG_TOUCHSCREEN_TEMP_CTRL_ZONE_NOTIFY_2nd
static void temp_ctrl_zone_down_notify_2nd(struct work_struct *work)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;
#ifdef CONFIG_TOUCHSCREEN_LCD_NOTIFY_2nd
	tpd_notifier_call_chain_2nd(TEMP_CTRL_ZONE_DOWN);
#endif
	TPD_DMESG("notify temp control zone down");
	cdev->start_temp_ctrl_timer = false;
	cdev->notify_temp_ctrl_zone_down = true;
}
#endif

int tpd_report_work_init_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	cdev->tpd_report_wq = create_singlethread_workqueue("tpd_report_wq");

	if (!cdev->tpd_report_wq) {
		goto err_create_tpd_report_wq_failed;
	}
	INIT_DELAYED_WORK(&cdev->tpd_report_work0, tpd_id0_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work1, tpd_id1_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work2, tpd_id2_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work3, tpd_id3_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work4, tpd_id4_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work5, tpd_id5_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work6, tpd_id6_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work7, tpd_id7_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work8, tpd_id8_report_work);
	INIT_DELAYED_WORK(&cdev->tpd_report_work9, tpd_id9_report_work);
#ifdef CONFIG_TOUCHSCREEN_POINT_REPORT_CHECK_2nd
	INIT_DELAYED_WORK(&cdev->point_report_check_work, ts_point_report_check_2nd);
#endif
#ifdef CONFIG_TOUCHSCREEN_PSENSOR_REPORT_CHECK_2nd
	INIT_DELAYED_WORK(&cdev->psensor_report_check_work, ts_psensor_report_check_2nd);
#endif
#ifdef CONFIG_TOUCHSCREEN_TEMP_CTRL_ZONE_NOTIFY_2nd
	INIT_DELAYED_WORK(&cdev->temp_ctr_zone_work, temp_ctrl_zone_down_notify_2nd);
#endif
	return 0;
err_create_tpd_report_wq_failed:
	TPD_DMESG("create tpd report workqueue failed\n");
	return -ENOMEM;

}

void tpd_report_work_deinit_2nd(void)
{
	struct ztp_device_2nd *cdev = tpd_cdev_2nd;

	TPD_DMESG("enter");
	cancel_delayed_work_sync(&cdev->tpd_report_work0);
	cancel_delayed_work_sync(&cdev->tpd_report_work1);
	cancel_delayed_work_sync(&cdev->tpd_report_work2);
	cancel_delayed_work_sync(&cdev->tpd_report_work3);
	cancel_delayed_work_sync(&cdev->tpd_report_work4);
	cancel_delayed_work_sync(&cdev->tpd_report_work5);
	cancel_delayed_work_sync(&cdev->tpd_report_work6);
	cancel_delayed_work_sync(&cdev->tpd_report_work7);
	cancel_delayed_work_sync(&cdev->tpd_report_work8);
	cancel_delayed_work_sync(&cdev->tpd_report_work9);
#ifdef CONFIG_TOUCHSCREEN_POINT_REPORT_CHECK_2nd
	cancel_delayed_work_sync(&cdev->point_report_check_work);
#endif
#ifdef CONFIG_TOUCHSCREEN_PSENSOR_REPORT_CHECK_2nd
	cancel_delayed_work_sync(&cdev->psensor_report_check_work);
#endif
#ifdef CONFIG_TOUCHSCREEN_TEMP_CTRL_ZONE_NOTIFY_2nd
	cancel_delayed_work_sync(&cdev->temp_ctr_zone_work);
#endif
}

