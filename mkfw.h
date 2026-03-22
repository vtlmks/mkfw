// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT


#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "mkfw_keys.h"

/* Forward declaration */
struct mkfw_state;

/* Callback function pointers */
typedef void (*key_callback_t)(struct mkfw_state *state, uint32_t key, uint32_t action, uint32_t modifier_bits);
typedef void (*char_callback_t)(struct mkfw_state *state, uint32_t codepoint);
typedef void (*scroll_callback_t)(struct mkfw_state *state, double xoffset, double yoffset);
typedef void (*mouse_move_delta_callback_t)(struct mkfw_state *state, int32_t x, int32_t y);
typedef void (*mouse_button_callback_t)(struct mkfw_state *state, uint8_t button, int action);
typedef void (*framebuffer_callback_t)(struct mkfw_state *state, int32_t width, int32_t height, float aspect_ratio);
typedef void (*focus_callback_t)(struct mkfw_state *state, uint8_t focused);
typedef void (*drop_callback_t)(uint32_t count, const char **paths);

/* Main state structure */
struct mkfw_state {
	// Shared input state
	uint8_t keyboard_state[MKS_KEY_LAST];
	uint8_t prev_keyboard_state[MKS_KEY_LAST];
	uint8_t modifier_state[MKS_MODIFIER_LAST];
	uint8_t prev_modifier_state[MKS_MODIFIER_LAST];
	uint8_t mouse_buttons[5];
	uint8_t previous_mouse_buttons[5];
	int32_t mouse_x;
	int32_t mouse_y;
	uint8_t is_fullscreen;
	uint8_t has_focus;
	uint8_t mouse_in_window;

	// Callbacks
	key_callback_t key_callback;
	char_callback_t char_callback;
	scroll_callback_t scroll_callback;
	mouse_move_delta_callback_t mouse_move_delta_callback;
	mouse_button_callback_t mouse_button_callback;
	framebuffer_callback_t framebuffer_callback;
	focus_callback_t focus_callback;
	drop_callback_t drop_callback;

	// Platform-specific state
	void *platform;

	// User data (for application use)
	void *user_data;
};

/* Error reporting callback */
typedef void (*mkfw_error_callback_t)(const char *message);
static mkfw_error_callback_t mkfw_error_callback;

__attribute__((format(printf, 1, 2)))
static inline void mkfw_error(const char *fmt, ...) {
	if(mkfw_error_callback) {
		char buf[512];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
		mkfw_error_callback(buf);
	}
}

/* OpenGL version configuration (call before mkfw_init, defaults to 3.1) */
static int mkfw_gl_major = 3;
static int mkfw_gl_minor = 1;

static inline void mkfw_set_gl_version(int major, int minor) {
	mkfw_gl_major = major;
	mkfw_gl_minor = minor;
}

/* Per-pixel transparency (call before mkfw_init) */
static int mkfw_transparent = 0;
static inline void mkfw_set_transparent(int enable) { mkfw_transparent = enable; }

/* Monitor information */
#define MKFW_MAX_MONITORS 16

struct mkfw_monitor {
	char name[128];
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	int32_t refresh_rate;
	uint8_t primary;
};

/* Suppress unused-function warnings for API functions the user may not call */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

/* Platform-specific implementation includes */
#ifdef _WIN32
#include "mkfw_win32.c"
#elif __linux__
#include "mkfw_linux.c"
#endif

/* Thread abstraction */
#ifdef _WIN32
	typedef HANDLE mkfw_thread;
	#define MKFW_THREAD_FUNC(name, arg) DWORD WINAPI name(LPVOID arg)

	static inline mkfw_thread mkfw_thread_create(LPTHREAD_START_ROUTINE func, void *arg) {
		return CreateThread(0, 0, func, arg, 0, 0);
	}

	static inline void mkfw_thread_join(mkfw_thread t) {
		WaitForSingleObject(t, INFINITE);
		CloseHandle(t);
	}
#else
	#include <pthread.h>
	typedef pthread_t mkfw_thread;
	#define MKFW_THREAD_FUNC(name, arg) void *name(void *arg)

	static inline mkfw_thread mkfw_thread_create(void *(*func)(void *), void *arg) {
		pthread_t t;
		if(pthread_create(&t, 0, func, arg)) {
			return 0;
		}
		return t;
	}

	static inline void mkfw_thread_join(mkfw_thread t) {
		pthread_join(t, 0);
	}
#endif

/* Audio subsystem - optional */
#ifdef MKFW_AUDIO
#ifdef _WIN32
#include "mkfw_win32_audio.c"
#elif __linux__
#include "mkfw_linux_audio.c"
#endif
#endif

/* Timer subsystem - optional */
#ifdef MKFW_TIMER
#ifdef _WIN32
#include "mkfw_win32_timer.c"
#elif __linux__
#include "mkfw_linux_timer.c"
#endif
#endif

/* Joystick subsystem - optional */
#ifdef MKFW_JOYSTICK
#include "mkfw_joystick.h"
#ifdef _WIN32
#include "mkfw_win32_joystick.c"
#elif __linux__
#include "mkfw_linux_joystick.c"
#endif
#ifdef MKFW_JOYSTICK_GAMEDB
#include "mkfw_joystick_gamedb.c"
#endif
#endif

/* Inline helper functions - placed after platform includes so struct is defined */
static inline void mkfw_update_input_state(struct mkfw_state *state) {
	memcpy(state->prev_keyboard_state, state->keyboard_state, sizeof(state->keyboard_state));
	memcpy(state->prev_modifier_state, state->modifier_state, sizeof(state->modifier_state));
	memcpy(state->previous_mouse_buttons, state->mouse_buttons, sizeof(state->mouse_buttons));
}

static inline void mkfw_set_error_callback(mkfw_error_callback_t callback) { mkfw_error_callback = callback; }
static inline void mkfw_set_user_data(struct mkfw_state *state, void *user_data) { state->user_data = user_data; }
static inline void *mkfw_get_user_data(struct mkfw_state *state) { return state->user_data; }
static inline void mkfw_set_key_callback(struct mkfw_state *state, key_callback_t callback) { state->key_callback = callback; }
static inline void mkfw_set_char_callback(struct mkfw_state *state, char_callback_t callback) { state->char_callback = callback; }
static inline void mkfw_set_scroll_callback(struct mkfw_state *state, scroll_callback_t callback) { state->scroll_callback = callback; }
static inline void mkfw_set_mouse_move_delta_callback(struct mkfw_state *state, mouse_move_delta_callback_t callback) { state->mouse_move_delta_callback = callback; }
static inline void mkfw_set_mouse_button_callback(struct mkfw_state *state, mouse_button_callback_t callback) { state->mouse_button_callback = callback; }
static inline void mkfw_set_framebuffer_size_callback(struct mkfw_state *state, framebuffer_callback_t callback) { state->framebuffer_callback = callback; }
static inline void mkfw_set_focus_callback(struct mkfw_state *state, focus_callback_t callback) { state->focus_callback = callback; }
static inline void mkfw_set_drop_callback(struct mkfw_state *state, drop_callback_t callback) { state->drop_callback = callback; mkfw_enable_drop(state, callback != 0); }
static inline int mkfw_is_key_pressed(struct mkfw_state *state, uint8_t key) { return state->keyboard_state[key] && !state->prev_keyboard_state[key]; }
static inline int mkfw_was_key_released(struct mkfw_state *state, uint8_t key) { return !state->keyboard_state[key] && state->prev_keyboard_state[key]; }
static inline uint8_t mkfw_is_button_pressed(struct mkfw_state *state, uint8_t button) { return state->mouse_buttons[button] && !state->previous_mouse_buttons[button]; }
static inline uint8_t mkfw_was_button_released(struct mkfw_state *state, uint8_t button) { return !state->mouse_buttons[button] && state->previous_mouse_buttons[button]; }

static inline const char *mkfw_get_key_name(uint32_t key) {
	if(key >= 'a' && key <= 'z') {
		static char letter[2];
		letter[0] = (char)(key - 32);
		letter[1] = 0;
		return letter;
	}
	if(key >= '0' && key <= '9') {
		static char digit[2];
		digit[0] = (char)key;
		digit[1] = 0;
		return digit;
	}
	switch(key) {
		case MKS_KEY_SPACE:       return "Space";
		case MKS_KEY_APOSTROPHE:  return "'";
		case MKS_KEY_COMMA:       return ",";
		case MKS_KEY_MINUS:       return "-";
		case MKS_KEY_PERIOD:      return ".";
		case MKS_KEY_SLASH:       return "/";
		case MKS_KEY_SEMICOLON:   return ";";
		case MKS_KEY_EQUAL:       return "=";
		case MKS_KEY_LEFT_BRACKET: return "[";
		case MKS_KEY_BACKSLASH:   return "\\";
		case MKS_KEY_RIGHT_BRACKET: return "]";
		case MKS_KEY_GRAVE:       return "`";
		case MKS_KEY_ESCAPE:      return "Escape";
		case MKS_KEY_RETURN:      return "Enter";
		case MKS_KEY_TAB:         return "Tab";
		case MKS_KEY_BACKSPACE:   return "Backspace";
		case MKS_KEY_INSERT:      return "Insert";
		case MKS_KEY_DELETE:      return "Delete";
		case MKS_KEY_LEFT:        return "Left";
		case MKS_KEY_RIGHT:       return "Right";
		case MKS_KEY_UP:          return "Up";
		case MKS_KEY_DOWN:        return "Down";
		case MKS_KEY_HOME:        return "Home";
		case MKS_KEY_END:         return "End";
		case MKS_KEY_PAGEUP:      return "PageUp";
		case MKS_KEY_PAGEDOWN:    return "PageDown";
		case MKS_KEY_CAPSLOCK:    return "CapsLock";
		case MKS_KEY_NUMLOCK:     return "NumLock";
		case MKS_KEY_SCROLLLOCK:  return "ScrollLock";
		case MKS_KEY_PRINTSCREEN: return "PrintScreen";
		case MKS_KEY_PAUSE:       return "Pause";
		case MKS_KEY_MENU:        return "Menu";
		case MKS_KEY_F1:          return "F1";
		case MKS_KEY_F2:          return "F2";
		case MKS_KEY_F3:          return "F3";
		case MKS_KEY_F4:          return "F4";
		case MKS_KEY_F5:          return "F5";
		case MKS_KEY_F6:          return "F6";
		case MKS_KEY_F7:          return "F7";
		case MKS_KEY_F8:          return "F8";
		case MKS_KEY_F9:          return "F9";
		case MKS_KEY_F10:         return "F10";
		case MKS_KEY_F11:         return "F11";
		case MKS_KEY_F12:         return "F12";
		case MKS_KEY_SHIFT:       return "Shift";
		case MKS_KEY_LSHIFT:      return "Left Shift";
		case MKS_KEY_RSHIFT:      return "Right Shift";
		case MKS_KEY_CTRL:        return "Ctrl";
		case MKS_KEY_LCTRL:       return "Left Ctrl";
		case MKS_KEY_RCTRL:       return "Right Ctrl";
		case MKS_KEY_ALT:         return "Alt";
		case MKS_KEY_LALT:        return "Left Alt";
		case MKS_KEY_RALT:        return "Right Alt";
		case MKS_KEY_LSUPER:      return "Left Super";
		case MKS_KEY_RSUPER:      return "Right Super";
		case MKS_KEY_NUMPAD_0:    return "Numpad 0";
		case MKS_KEY_NUMPAD_1:    return "Numpad 1";
		case MKS_KEY_NUMPAD_2:    return "Numpad 2";
		case MKS_KEY_NUMPAD_3:    return "Numpad 3";
		case MKS_KEY_NUMPAD_4:    return "Numpad 4";
		case MKS_KEY_NUMPAD_5:    return "Numpad 5";
		case MKS_KEY_NUMPAD_6:    return "Numpad 6";
		case MKS_KEY_NUMPAD_7:    return "Numpad 7";
		case MKS_KEY_NUMPAD_8:    return "Numpad 8";
		case MKS_KEY_NUMPAD_9:    return "Numpad 9";
		case MKS_KEY_NUMPAD_DECIMAL:  return "Numpad .";
		case MKS_KEY_NUMPAD_DIVIDE:   return "Numpad /";
		case MKS_KEY_NUMPAD_MULTIPLY: return "Numpad *";
		case MKS_KEY_NUMPAD_SUBTRACT: return "Numpad -";
		case MKS_KEY_NUMPAD_ADD:      return "Numpad +";
		case MKS_KEY_NUMPAD_ENTER:    return "Numpad Enter";
		default:                  return "Unknown";
	}
}

/* Joystick query helpers */
#ifdef MKFW_JOYSTICK
static inline int mkfw_joystick_connected(int pad_index) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0;
	return mkfw_joystick_pads[pad_index].connected;
}
static inline const char *mkfw_joystick_name(int pad_index) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return "";
	return mkfw_joystick_pads[pad_index].name;
}
static inline int mkfw_joystick_button(int pad_index, int button_index) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0;
	if(button_index < 0 || button_index >= MKFW_JOYSTICK_MAX_BUTTONS) return 0;
	return mkfw_joystick_pads[pad_index].buttons[button_index];
}
static inline int mkfw_joystick_button_pressed(int pad_index, int button_index) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0;
	if(button_index < 0 || button_index >= MKFW_JOYSTICK_MAX_BUTTONS) return 0;
	return mkfw_joystick_pads[pad_index].buttons[button_index] &&
	       !mkfw_joystick_pads[pad_index].prev_buttons[button_index];
}
static inline int mkfw_joystick_button_released(int pad_index, int button_index) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0;
	if(button_index < 0 || button_index >= MKFW_JOYSTICK_MAX_BUTTONS) return 0;
	return !mkfw_joystick_pads[pad_index].buttons[button_index] &&
	       mkfw_joystick_pads[pad_index].prev_buttons[button_index];
}
static inline float mkfw_joystick_axis(int pad_index, int axis_index) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0.0f;
	if(axis_index < 0 || axis_index >= MKFW_JOYSTICK_MAX_AXES) return 0.0f;
	return mkfw_joystick_pads[pad_index].axes[axis_index];
}
static inline float mkfw_joystick_hat_x(int pad_index) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0.0f;
	return mkfw_joystick_pads[pad_index].hat_x;
}
static inline float mkfw_joystick_hat_y(int pad_index) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0.0f;
	return mkfw_joystick_pads[pad_index].hat_y;
}
static inline int mkfw_joystick_button_count(int pad_index) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0;
	return mkfw_joystick_pads[pad_index].button_count;
}
static inline int mkfw_joystick_axis_count(int pad_index) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0;
	return mkfw_joystick_pads[pad_index].axis_count;
}
static inline void mkfw_joystick_set_callback(mkfw_joystick_callback_t callback) {
	mkfw_joystick_cb = callback;
}
static inline void mkfw_joystick_rumble(int pad_index, float low_freq, float high_freq, uint32_t duration_ms) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return;
	mkfw_joystick_rumble_platform(pad_index, low_freq, high_freq, duration_ms);
}
#else
/* Stub macros when joystick is not enabled */
#define mkfw_joystick_init() ((void)0)
#define mkfw_joystick_shutdown() ((void)0)
#define mkfw_joystick_update() ((void)0)
#define mkfw_joystick_rumble(idx, low, high, ms) ((void)0)
#define mkfw_joystick_connected(idx) (0)
#define mkfw_joystick_name(idx) ("")
#define mkfw_joystick_button(idx, btn) (0)
#define mkfw_joystick_button_pressed(idx, btn) (0)
#define mkfw_joystick_button_released(idx, btn) (0)
#define mkfw_joystick_axis(idx, axis) (0.0f)
#define mkfw_joystick_hat_x(idx) (0.0f)
#define mkfw_joystick_hat_y(idx) (0.0f)
#define mkfw_joystick_button_count(idx) (0)
#define mkfw_joystick_axis_count(idx) (0)
#define mkfw_joystick_set_callback(cb) ((void)0)
#endif

#ifndef MKFW_JOYSTICK_GAMEDB
#define mkfw_gamepad_button(idx, btn) (0)
#define mkfw_gamepad_button_pressed(idx, btn) (0)
#define mkfw_gamepad_axis(idx, axis) (0.0f)
#endif


#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
