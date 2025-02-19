/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <board_cfg.h>
#include <os_common_api.h>
#include <lvgl/lvgl.h>
#include <lvgl/lvgl_ext.h>
#include <lvgl/lvgl_bitmap_font.h>
#include <lvgl/lvgl_freetype_font.h>
#include <lvgl/demos/benchmark/lv_demo_benchmark.h>
#include <drivers/input/input_dev.h>
#include <mem_manager.h>
#include <sys_manager.h>
#include <thread_timer.h>
#include <sys_wakelock.h>
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#include "app_msg.h"

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void _disp_vsync_cb(const lvx_display_observer_t * observer, uint32_t timestamp);
static void _disp_state_cb(const lvx_display_observer_t * observer, uint32_t state);

/**********************
 *  STATIC VARIABLES
 **********************/

static const lvx_display_observer_t g_disp_observer = {
	.vsync_cb = _disp_vsync_cb,
	.state_cb = _disp_state_cb,
};

static uint8_t g_disp_active = 1;

static K_SEM_DEFINE(g_disp_vsync_sem, 0, 1);

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void _disp_vsync_cb(const lvx_display_observer_t *observer, uint32_t timestamp)
{
	k_sem_give(&g_disp_vsync_sem);
}

static void _disp_state_cb(const lvx_display_observer_t *observer, uint32_t state)
{
	switch (state) {
	case LVX_DISPLAY_STATE_OFF:
		app_msg_send(APP_NAME, MSG_UI, CMD_SCREEN_OFF);
		break;
	case LVX_DISPLAY_STATE_ON:
		app_msg_send(APP_NAME, MSG_UI, CMD_SCREEN_ON);
		break;
	default:
		break;
	}
}

static void _keypad_callback(struct device *dev, struct input_value *val)
{
	static uint32_t last_value = 0;

	SYS_LOG_INF("type: %d, code:%d, value:%d\n",
		val->keypad.type, val->keypad.code, val->keypad.value);

	if (last_value != val->keypad.value) {
		// screen on
		sys_wake_lock(FULL_WAKE_LOCK);
		sys_wake_unlock(FULL_WAKE_LOCK);

		last_value = val->keypad.value;
	}
}

static void _keypad_init(void)
{
	struct device *onoff_dev;

	onoff_dev = (struct device *)device_get_binding(CONFIG_INPUT_DEV_ACTS_ONOFF_KEY_NAME);
	if (!onoff_dev) {
		SYS_LOG_ERR("cannot found key dev %s", CONFIG_INPUT_DEV_ACTS_ONOFF_KEY_NAME);
		return;
	}

	input_dev_enable(onoff_dev);
	input_dev_register_notify(onoff_dev, _keypad_callback);
}

static void _lvgl_init(void)
{
	/* LVGL initialize */
	lvx_port_init();
	lvx_display_add_observer(&g_disp_observer);
	lvx_refr_set_manual(NULL);

	/* Set background color */
	lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
}

static void _process_ui_msg(struct app_msg *msg)
{
	switch (msg->cmd) {
	case CMD_SCREEN_OFF:
		g_disp_active = 0;
		break;
	case CMD_SCREEN_ON:
		g_disp_active = 1;
		lv_obj_invalidate(lv_screen_active());
		break;
	default:
		break;
	}
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int main(void)
{
	// init system
	mem_manager_init();
	system_init();

	// init app msg
	app_msg_init();

	// init lvgl
	_lvgl_init();

	// init key device
	_keypad_init();

	system_ready();

	lv_demo_benchmark();

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "init");
	dvfs_set_level(DVFS_LEVEL_NORMAL, "launcher");
#endif

	while (1) {
		struct app_msg msg;

		os_strace_void(SYS_TRACE_ID_GUI_TASK);
		uint32_t lv_timeout = lv_task_handler();
		os_strace_end_call(SYS_TRACE_ID_GUI_TASK);

		uint32_t tt_timeout = thread_timer_next_timeout();
		uint32_t timeout = MIN(lv_timeout, tt_timeout);

		int rc = poll_msg(&msg, &g_disp_vsync_sem, timeout);
		switch (rc) {
		case OS_POLL_MSG:
			SYS_LOG_INF("msg type=0x%x, cmd=0x%x", msg.type, msg.cmd);
			switch (msg.type) {
				case MSG_UI:
					_process_ui_msg(&msg);
					break;
			}
			if (msg.callback != NULL) {
				msg.callback(&msg, 0, NULL);
			}
			break;

		case OS_POLL_SEM:
			if (g_disp_active) {
				lvx_refr_all();
			}
			break;
		}

		thread_timer_handle_expired();
	}

	return 0;
}
