// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT
//
// mkfw joystick subsystem.  Include this header in addition to
// mkfw.h to use gamepad input.  The implementation is unity-built
// alongside the application; no separate compilation required.
//
//   #include "mkfw.h"
//   #include "mkfw_joystick.h"
//
// Define MKFW_JOYSTICK_GAMEDB before including to also pull in the
// SDL GameController DB mappings for normalised button / axis names.

#pragma once

#include <stdint.h>

#include "mkfw.h"

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

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifdef _WIN32
#include "mkfw_win32_joystick.c"
#elif defined(__linux__)
#include "mkfw_linux_joystick.c"
#endif

#ifdef MKFW_JOYSTICK_GAMEDB
#include "mkfw_joystick_gamedb.c"
#endif

static inline uint32_t mkfw_joystick_is_connected(uint32_t pad_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0;
	}
	return mkfw_joystick_pads[pad_index].connected;
}

static inline const char *mkfw_joystick_get_name(uint32_t pad_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return "";
	}
	return mkfw_joystick_pads[pad_index].name;
}

static inline uint32_t mkfw_joystick_get_button(uint32_t pad_index, uint32_t button_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0;
	}
	if(button_index >= MKFW_JOYSTICK_MAX_BUTTONS) {
		return 0;
	}
	return mkfw_joystick_pads[pad_index].buttons[button_index];
}

static inline uint32_t mkfw_joystick_is_button_pressed(uint32_t pad_index, uint32_t button_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0;
	}
	if(button_index >= MKFW_JOYSTICK_MAX_BUTTONS) {
		return 0;
	}
	return mkfw_joystick_pads[pad_index].buttons[button_index] &&
	       !mkfw_joystick_pads[pad_index].prev_buttons[button_index];
}

static inline uint32_t mkfw_joystick_was_button_released(uint32_t pad_index, uint32_t button_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0;
	}
	if(button_index >= MKFW_JOYSTICK_MAX_BUTTONS) {
		return 0;
	}
	return !mkfw_joystick_pads[pad_index].buttons[button_index] &&
	       mkfw_joystick_pads[pad_index].prev_buttons[button_index];
}

static inline float mkfw_joystick_get_axis(uint32_t pad_index, uint32_t axis_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0.0f;
	}
	if(axis_index >= MKFW_JOYSTICK_MAX_AXES) {
		return 0.0f;
	}
	return mkfw_joystick_pads[pad_index].axes[axis_index];
}

static inline float mkfw_joystick_get_hat_x(uint32_t pad_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0.0f;
	}
	return mkfw_joystick_pads[pad_index].hat_x;
}

static inline float mkfw_joystick_get_hat_y(uint32_t pad_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0.0f;
	}
	return mkfw_joystick_pads[pad_index].hat_y;
}

static inline uint32_t mkfw_joystick_get_button_count(uint32_t pad_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0;
	}
	return mkfw_joystick_pads[pad_index].button_count;
}

static inline uint32_t mkfw_joystick_get_axis_count(uint32_t pad_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0;
	}
	return mkfw_joystick_pads[pad_index].axis_count;
}

static inline void mkfw_joystick_set_callback(mkfw_joystick_callback_t callback) {
	mkfw_joystick_cb = callback;
}

static inline void mkfw_joystick_rumble(uint32_t pad_index, float low_freq, float high_freq, uint32_t duration_ms) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return;
	}
	mkfw_joystick_rumble_platform(pad_index, low_freq, high_freq, duration_ms);
}

#ifndef MKFW_JOYSTICK_GAMEDB
#define mkfw_gamepad_get_button(idx, btn) (0)
#define mkfw_gamepad_is_button_pressed(idx, btn) (0)
#define mkfw_gamepad_get_axis(idx, axis) (0.0f)
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
