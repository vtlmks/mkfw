// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#define _GNU_SOURCE
#define MKFW_JOYSTICK_GAMEDB
#include "../mkfw_joystick.h"

// [=]===^=[ main ]==============================================================================[=]
int main(void) {
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[0];
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[0];

	pad->connected = 1;
	pad->button_count = MKFW_JOYSTICK_MAX_BUTTONS;
	pad->axis_count = MKFW_JOYSTICK_MAX_AXES;

#ifdef _WIN32
	mkfw_gamedb_ensure_mapping(0);

	if(!map->valid || map->buttons[MKFW_GAMEPAD_GUIDE] != 0 || map->buttons[MKFW_GAMEPAD_DPAD_RIGHT] != 0x4d || map->axes[MKFW_GAMEPAD_AXIS_RIGHT_TRIGGER] != 0x85) {
		return 1;

	}
#else
	for(size_t i = 0; i < sizeof(mkfw_gamecontrollerdb_data) / sizeof(mkfw_gamecontrollerdb_data[0]); ++i) {
		pad->vendor_id = mkfw_gamecontrollerdb_data[i].vendor_id;
		pad->product_id = mkfw_gamecontrollerdb_data[i].product_id;
		mkfw_gamedb_ensure_mapping(0);

		if(!map->valid || map->vendor_id != pad->vendor_id || map->product_id != pad->product_id || memcmp(map->buttons, mkfw_gamecontrollerdb_data[i].buttons, sizeof(map->buttons)) || memcmp(map->axes, mkfw_gamecontrollerdb_data[i].axes, sizeof(map->axes))) {
			return 1;

		}
	}
#endif

	map->buttons[MKFW_GAMEPAD_A] = 0x42;
	map->buttons[MKFW_GAMEPAD_B] = 0x81;
	map->buttons[MKFW_GAMEPAD_DPAD_UP] = 0xc1;
	map->axes[MKFW_GAMEPAD_AXIS_LEFT_X] = 0xa1;
	map->axes[MKFW_GAMEPAD_AXIS_LEFT_TRIGGER] = 0x42;
	pad->buttons[2] = 1;
	pad->axes[1] = 0.75f;
	pad->hat_y = -1.0f;

	if(!mkfw_gamepad_get_button(0, MKFW_GAMEPAD_A) || !mkfw_gamepad_is_button_pressed(0, MKFW_GAMEPAD_A) || !mkfw_gamepad_get_button(0, MKFW_GAMEPAD_B) || !mkfw_gamepad_get_button(0, MKFW_GAMEPAD_DPAD_UP) || mkfw_gamepad_get_axis(0, MKFW_GAMEPAD_AXIS_LEFT_X) != -0.75f || mkfw_gamepad_get_axis(0, MKFW_GAMEPAD_AXIS_LEFT_TRIGGER) != 1.0f) {
		return 2;

	}

	pad->prev_buttons[2] = 1;
	pad->button_count = 2;
	pad->axis_count = 1;

	if(mkfw_gamepad_is_button_pressed(0, MKFW_GAMEPAD_A) || mkfw_gamepad_get_button(0, MKFW_GAMEPAD_A) || mkfw_gamepad_get_button(0, MKFW_GAMEPAD_B) || mkfw_gamepad_get_axis(0, MKFW_GAMEPAD_AXIS_LEFT_X) || mkfw_gamepad_get_axis(0, MKFW_GAMEPAD_AXIS_LEFT_TRIGGER)) {
		return 3;

	}

	pad->connected = 0;
	mkfw_gamedb_ensure_mapping(0);

	if(map->valid || mkfw_gamepad_get_button(0, MKFW_GAMEPAD_A) || mkfw_gamepad_get_axis(0, MKFW_GAMEPAD_AXIS_LEFT_X) || mkfw_gamepad_get_button(MKFW_JOYSTICK_MAX_PADS, 0) || mkfw_gamepad_get_axis(0, MKFW_GAMEPAD_AXIS_LAST)) {
		return 4;

	}

#ifndef _WIN32
	pad->connected = 1;
	pad->vendor_id = 0;
	pad->product_id = 0;
	mkfw_gamedb_ensure_mapping(0);

	if(map->valid) {
		return 5;

	}
#endif

	return 0;

}
