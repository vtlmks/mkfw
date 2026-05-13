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

	uint32_t button_count;
	uint32_t axis_count;
	uint8_t buttons[MKFW_JOYSTICK_MAX_BUTTONS];
	uint8_t prev_buttons[MKFW_JOYSTICK_MAX_BUTTONS];
	float axes[MKFW_JOYSTICK_MAX_AXES];
	float hat_x;
	float hat_y;
};

typedef void (*mkfw_joystick_callback_t)(uint32_t pad_index, uint32_t connected);

/* Broad controller archetype, derived from USB vendor ID.  Useful
 * for picking the right button-prompt graphics ("press X" vs
 * "press Cross").  Layout-neutral input still goes through the
 * MKFW_GAMEPAD_* abstraction via mkfw_joystick_gamedb.h. */
enum {
	MKFW_JOYSTICK_TYPE_GENERIC = 0,
	MKFW_JOYSTICK_TYPE_XBOX,
	MKFW_JOYSTICK_TYPE_PLAYSTATION,
	MKFW_JOYSTICK_TYPE_SWITCH,
};

/* Cross-TU storage: declared here, defined in the platform .c either
 * via the unity include below or via the gated block in library mode. */
MKFW_VAR struct mkfw_joystick_pad   mkfw_joystick_pads[MKFW_JOYSTICK_MAX_PADS];
MKFW_VAR mkfw_joystick_callback_t   mkfw_joystick_cb;

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#if !defined(MKFW_BUILD_LIBRARY) && !defined(MKFW_USE_SHARED)
#ifdef _WIN32
#include "mkfw_win32_joystick.c"
#elif defined(__linux__)
#include "mkfw_linux_joystick.c"
#endif

#ifdef MKFW_JOYSTICK_GAMEDB
#include "mkfw_joystick_gamedb.c"
#endif
#endif

/* Forward declarations of every MKFW_API function defined in the
 * platform .c.  Same rationale as in mkfw.h. */
MKFW_API void  mkfw_joystick_init(void);
MKFW_API void  mkfw_joystick_shutdown(void);
MKFW_API void  mkfw_joystick_update(void);
MKFW_API void  mkfw_joystick_rumble_platform(uint32_t pad_index, float low_freq, float high_freq, uint32_t duration_ms);
MKFW_API void  mkfw_joystick_rumble_set_platform(uint32_t pad_index, float low_freq, float high_freq);
MKFW_API float mkfw_joystick_get_battery(uint32_t pad_index);

#ifdef MKFW_JOYSTICK_GAMEDB
MKFW_API uint32_t mkfw_gamepad_get_button(uint32_t pad_index, uint32_t gamepad_button);
MKFW_API uint32_t mkfw_gamepad_is_button_pressed(uint32_t pad_index, uint32_t gamepad_button);
MKFW_API float    mkfw_gamepad_get_axis(uint32_t pad_index, uint32_t gamepad_axis);
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

/* Continuous rumble: latches the given magnitudes until called again.
 * Pass (0.0f, 0.0f) to stop.  Use this for effects whose intensity
 * is driven each frame (engine revs, collision feedback); use the
 * time-limited mkfw_joystick_rumble for fire-and-forget pulses. */
static inline void mkfw_joystick_rumble_set(uint32_t pad_index, float low_freq, float high_freq) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return;
	}
	mkfw_joystick_rumble_set_platform(pad_index, low_freq, high_freq);
}

/* Broad controller archetype derived from USB vendor ID.  Returns
 * MKFW_JOYSTICK_TYPE_GENERIC if the pad isn't connected or the
 * vendor isn't one of the four well-known platforms. */
static inline uint32_t mkfw_joystick_get_type(uint32_t pad_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS || !mkfw_joystick_pads[pad_index].connected) {
		return MKFW_JOYSTICK_TYPE_GENERIC;
	}
	switch(mkfw_joystick_pads[pad_index].vendor_id) {
		case 0x045E: return MKFW_JOYSTICK_TYPE_XBOX;        // Microsoft
		case 0x054C: return MKFW_JOYSTICK_TYPE_PLAYSTATION; // Sony
		case 0x057E: return MKFW_JOYSTICK_TYPE_SWITCH;      // Nintendo
		default:     return MKFW_JOYSTICK_TYPE_GENERIC;
	}
}

#ifndef MKFW_JOYSTICK_GAMEDB
#define mkfw_gamepad_get_button(idx, btn) (0)
#define mkfw_gamepad_is_button_pressed(idx, btn) (0)
#define mkfw_gamepad_get_axis(idx, axis) (0.0f)
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
