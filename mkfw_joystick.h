// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT


#pragma once

#include <stdint.h>

#define MKFW_JOYSTICK_MAX_PADS    4
#define MKFW_JOYSTICK_MAX_BUTTONS 32
#define MKFW_JOYSTICK_MAX_AXES    8
#define MKFW_JOYSTICK_NAME_LEN    256

struct mkfw_joystick_pad {
	uint8_t connected;
	uint8_t was_connected;
	char name[MKFW_JOYSTICK_NAME_LEN];
	uint16_t vendor_id;
	uint16_t product_id;

	int button_count;
	int axis_count;
	uint8_t buttons[MKFW_JOYSTICK_MAX_BUTTONS];
	uint8_t prev_buttons[MKFW_JOYSTICK_MAX_BUTTONS];
	float axes[MKFW_JOYSTICK_MAX_AXES];
	float hat_x;
	float hat_y;
};

typedef void (*mkfw_joystick_callback_t)(int pad_index, int connected);
