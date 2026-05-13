// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT


#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mkfw_keys.h"

/* Public-API linkage macros.
 *
 *   MKFW_API marks functions; MKFW_VAR marks the cross-TU variables
 *   that the inline helpers in this header (and in companion
 *   subsystem headers) reference.
 *
 *   Default (header-only / unity build):    MKFW_API == MKFW_VAR == static
 *     The platform .c is #included from this header; every public
 *     symbol is file-scope in the consumer's translation unit.
 *
 *   Static library build:                   MKFW_API == MKFW_VAR == extern
 *     Define MKFW_BUILD_LIBRARY when compiling the .c files into a
 *     static .a / .lib.  Consumers link against the archive and
 *     reference the public symbols via the header declarations.
 *     The library .c files each provide the storage for their
 *     MKFW_VAR variables in a gated block.
 *
 *   Shared library build (Windows DLL):     MKFW_API == __declspec(dllexport)
 *     Define MKFW_BUILD_SHARED when compiling the DLL.
 *
 *   Shared library consumer (Windows DLL):  MKFW_API == __declspec(dllimport)
 *     Define MKFW_USE_SHARED when including this header from a TU
 *     that links against the import library.
 *
 * Internal helpers (file-scope to one .c) stay marked `static`
 * regardless of mode.
 */
#if defined(MKFW_BUILD_SHARED) && defined(_WIN32)
   #define MKFW_API __declspec(dllexport)
   #define MKFW_VAR __declspec(dllexport) extern
#elif defined(MKFW_USE_SHARED) && defined(_WIN32)
   #define MKFW_API __declspec(dllimport)
   #define MKFW_VAR __declspec(dllimport) extern
#elif defined(MKFW_BUILD_LIBRARY)
   #define MKFW_API extern
   #define MKFW_VAR extern
#else
   #define MKFW_API static
   #define MKFW_VAR static
#endif

/* Forward declarations */
struct mkfw_context;
struct mkfw_window;
struct mkfw_cursor;

/* Callback function pointers */
typedef void (*mkfw_key_callback_t)(struct mkfw_window *window, uint32_t key, uint32_t action, uint32_t modifier_bits);
typedef void (*mkfw_char_callback_t)(struct mkfw_window *window, uint32_t codepoint);
typedef void (*mkfw_scroll_callback_t)(struct mkfw_window *window, double xoffset, double yoffset);
typedef void (*mkfw_mouse_move_delta_callback_t)(struct mkfw_window *window, int32_t x, int32_t y);
typedef void (*mkfw_mouse_button_callback_t)(struct mkfw_window *window, uint8_t button, uint32_t action);
typedef void (*mkfw_framebuffer_callback_t)(struct mkfw_window *window, int32_t width, int32_t height, float aspect_ratio);
typedef void (*mkfw_focus_callback_t)(struct mkfw_window *window, uint8_t focused);
typedef void (*mkfw_drop_callback_t)(uint32_t count, const char **paths);
typedef void (*mkfw_window_state_callback_t)(struct mkfw_window *window, uint8_t maximized, uint8_t minimized);

/* Per-window state.  Owned by a mkfw_context; create with
 * mkfw_window_create and destroy with mkfw_window_destroy. */
struct mkfw_window {
	struct mkfw_context *context;

	// Input state
	uint8_t keyboard_state[MKFW_KEY_LAST];
	uint8_t prev_keyboard_state[MKFW_KEY_LAST];
	uint8_t scancode_state[256];
	uint8_t prev_scancode_state[256];
	uint8_t mouse_buttons[5];
	uint8_t previous_mouse_buttons[5];
	int32_t mouse_x;
	int32_t mouse_y;
	uint8_t is_fullscreen;
	uint8_t has_focus;
	uint8_t mouse_in_window;

	// Callbacks
	mkfw_key_callback_t key_callback;
	mkfw_char_callback_t char_callback;
	mkfw_scroll_callback_t scroll_callback;
	mkfw_mouse_move_delta_callback_t mouse_move_delta_callback;
	mkfw_mouse_button_callback_t mouse_button_callback;
	mkfw_framebuffer_callback_t framebuffer_callback;
	mkfw_focus_callback_t focus_callback;
	mkfw_drop_callback_t drop_callback;
	mkfw_window_state_callback_t window_state_callback;

	// Platform-specific state
	void *platform;

	// User data (for application use)
	void *user_data;
};

/* Error reporting.
 *
 * mkfw never writes to stdout/stderr.  Errors surface through two
 * channels that are populated together:
 *
 *   - A thread-local buffer holding the last error message, read via
 *     mkfw_get_last_error() (returns 0 if no error has been recorded
 *     on this thread, or after mkfw_clear_last_error()).
 *   - An optional user callback installed with mkfw_set_error_callback().
 *
 * mkfw_error() is internal and not part of the public API surface,
 * but is declared here because the inline platform .c is included
 * into the same translation unit.  All sites that report failure
 * call mkfw_error() with a printf-style format string. */
typedef void (*mkfw_error_callback_t)(const char *message);
MKFW_VAR mkfw_error_callback_t mkfw_error_callback;

#ifdef _MSC_VER
	#define MKFW_THREAD_LOCAL __declspec(thread)
#else
	#define MKFW_THREAD_LOCAL __thread
#endif

MKFW_VAR MKFW_THREAD_LOCAL char mkfw_last_error_buf[512];
MKFW_VAR MKFW_THREAD_LOCAL uint8_t mkfw_last_error_set;

__attribute__((format(printf, 1, 2)))
static inline void mkfw_error(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vsnprintf(mkfw_last_error_buf, sizeof(mkfw_last_error_buf), fmt, args);
	va_end(args);
	mkfw_last_error_set = 1;
	if(mkfw_error_callback) {
		mkfw_error_callback(mkfw_last_error_buf);
	}
}

static inline const char *mkfw_get_last_error(void) {
	return mkfw_last_error_set ? mkfw_last_error_buf : 0;
}

static inline void mkfw_clear_last_error(void) {
	mkfw_last_error_buf[0] = 0;
	mkfw_last_error_set = 0;
}

// sscanf("%d.%d") pulls in __isoc23_sscanf on GCC 13+, requiring glibc 2.38
static inline uint32_t mkfw_parse_version(const char *str, int32_t *major, int32_t *minor) {
	const char *p = str;
	int32_t maj = 0, min = 0;
	if(*p < '0' || *p > '9') {
		return 0;
	}
	while(*p >= '0' && *p <= '9') {
		maj = maj * 10 + (*p++ - '0');
	}
	if(*p++ != '.') {
		return 0;
	}
	if(*p < '0' || *p > '9') {
		return 0;
	}
	while(*p >= '0' && *p <= '9') {
		min = min * 10 + (*p++ - '0');
	}
	*major = maj;
	*minor = min;
	return 1;
}

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

/* Library-level handle.  Created with mkfw_init, destroyed with
 * mkfw_shutdown.  Owns the platform display connection, loaded
 * function pointers, monitor cache, shared atoms, shared cursor
 * handles, and all windows created against it. */
#define MKFW_MAX_WINDOWS 16

struct mkfw_context {
	void *platform;

	struct mkfw_window *windows[MKFW_MAX_WINDOWS];
	uint32_t window_count;

	struct mkfw_monitor monitors[MKFW_MAX_MONITORS];
	uint32_t monitor_count;
};

/* Library init options.  Pass 0 to use defaults for every field. */
struct mkfw_options {
	uint32_t version;        // 0 = current
};

/* Window-creation flags */
#define MKFW_WIN_TRANSPARENT  (1u << 0)
#define MKFW_WIN_HIDDEN       (1u << 1)

/* Graphics-API selection for window creation.
 *
 * MKFW_GFX_GL (the default) creates a GLX/WGL context honouring
 * gl_major/gl_minor in the options struct.  MKFW_GFX_NONE skips all
 * rendering setup; the window is created in a state suitable for the
 * caller to manage a Vulkan surface, a Direct2D surface, or any other
 * API mkfw does not know about.  Reach the underlying platform
 * handles with mkfw_window_get_native_handles().
 *
 * MKFW_GFX_GLES and MKFW_GFX_VULKAN are reserved for future use.
 * Passing them today fails window creation with an error. */
enum mkfw_graphics_api {
	MKFW_GFX_GL = 0,
	MKFW_GFX_GLES,
	MKFW_GFX_VULKAN,
	MKFW_GFX_NONE,
};

/* Window creation options.  Pass 0 to use defaults for every field.
 * The version field must be 0 for now; future revisions may add
 * fields and bump the version. */
struct mkfw_window_options {
	uint32_t version;        // 0 = current
	int32_t  width;          // 0 = 1280
	int32_t  height;         // 0 = 720
	const char *title;       // 0 = "mkfw"
	int32_t  gl_major;       // 0 = 3
	int32_t  gl_minor;       // 0 = 1
	uint32_t flags;          // MKFW_WIN_*
	uint32_t graphics_api;   // MKFW_GFX_*; 0 = MKFW_GFX_GL
};

/* Native platform handles for callers that need to integrate with
 * APIs mkfw does not own (Vulkan surfaces, EGL, Direct2D, ...).
 *
 *   Linux:  display     -> Display *      (X11 connection)
 *           window      -> Window         (X11 XID; cast via uintptr_t)
 *           gl_context  -> GLXContext     (0 if graphics_api != MKFW_GFX_GL)
 *
 *   Win32:  display     -> HINSTANCE
 *           window      -> HWND
 *           gl_context  -> HGLRC          (0 if graphics_api != MKFW_GFX_GL)
 *
 * The handles remain valid until the window is destroyed.  mkfw does
 * not include the platform headers from this file, so callers cast
 * the void * / uintptr_t to the appropriate platform type at the use
 * site after including the relevant header. */
struct mkfw_native_handles {
	void     *display;
	uintptr_t window;
	void     *gl_context;
};

/* Suppress unused-function warnings for API functions the user may not call */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

/* Platform-specific implementation includes (header-only / unity build only) */
#if !defined(MKFW_BUILD_LIBRARY) && !defined(MKFW_USE_SHARED)
#ifdef _WIN32
#include "mkfw_win32.c"
#elif defined(__linux__)
#include "mkfw_linux.c"
#endif
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
#elif defined(__linux__)
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

/* Forward declarations of every MKFW_API function defined in the
 * platform .c.  In unity mode the .c above already declared them via
 * its definitions; in library mode these are the only declarations
 * the consumer's TU sees. */

/* Context / library lifecycle */
MKFW_API struct mkfw_context *mkfw_init(struct mkfw_options *opts);
MKFW_API void                 mkfw_shutdown(struct mkfw_context *ctx);
MKFW_API int32_t              mkfw_get_monitors(struct mkfw_context *ctx, struct mkfw_monitor *out, int32_t max);
MKFW_API uint32_t             mkfw_query_max_gl_version(int32_t *major, int32_t *minor);

/* Event pumping */
MKFW_API void                 mkfw_poll_events(struct mkfw_context *ctx);
MKFW_API void                 mkfw_wait_events(struct mkfw_context *ctx);
MKFW_API void                 mkfw_wait_events_timeout(struct mkfw_context *ctx, uint64_t nanoseconds);

/* Time */
MKFW_API uint64_t             mkfw_get_time(void);
MKFW_API void                 mkfw_sleep(uint64_t nanoseconds);

/* Window lifecycle */
MKFW_API struct mkfw_window  *mkfw_window_create(struct mkfw_context *ctx, struct mkfw_window_options *opts);
MKFW_API void                 mkfw_window_destroy(struct mkfw_window *state);
MKFW_API void                 mkfw_window_show(struct mkfw_window *state);
MKFW_API void                 mkfw_window_hide(struct mkfw_window *state);
MKFW_API uint32_t             mkfw_window_should_close(struct mkfw_window *state);
MKFW_API void                 mkfw_window_set_should_close(struct mkfw_window *state, int32_t value);

/* Window attributes */
MKFW_API void                 mkfw_window_set_title(struct mkfw_window *state, const char *title);
MKFW_API void                 mkfw_window_set_size(struct mkfw_window *state, int32_t width, int32_t height);
MKFW_API void                 mkfw_window_get_framebuffer_size(struct mkfw_window *state, int32_t *width, int32_t *height);
MKFW_API void                 mkfw_window_set_position(struct mkfw_window *state, int32_t x, int32_t y);
MKFW_API void                 mkfw_window_get_position(struct mkfw_window *state, int32_t *x, int32_t *y);
MKFW_API void                 mkfw_window_set_size_limits(struct mkfw_window *state, int32_t min_w, int32_t min_h, int32_t max_w, int32_t max_h);
MKFW_API void                 mkfw_window_set_aspect_ratio(struct mkfw_window *state, int32_t num, int32_t den);
MKFW_API void                 mkfw_window_set_resizable(struct mkfw_window *state, int32_t resizable);
MKFW_API void                 mkfw_window_set_decorated(struct mkfw_window *state, int32_t decorated);
MKFW_API void                 mkfw_window_set_opacity(struct mkfw_window *state, float opacity);
MKFW_API void                 mkfw_window_set_icon(struct mkfw_window *state, int32_t width, int32_t height, const uint8_t *rgba);
MKFW_API void                 mkfw_window_set_fullscreen(struct mkfw_window *state, int32_t enable);
MKFW_API void                 mkfw_window_minimize(struct mkfw_window *state);
MKFW_API void                 mkfw_window_maximize(struct mkfw_window *state);
MKFW_API void                 mkfw_window_restore(struct mkfw_window *state);
MKFW_API uint32_t             mkfw_window_is_minimized(struct mkfw_window *state);
MKFW_API uint32_t             mkfw_window_is_maximized(struct mkfw_window *state);
MKFW_API void                 mkfw_window_request_attention(struct mkfw_window *state);
MKFW_API float                mkfw_window_get_content_scale(struct mkfw_window *state);

/* Rendering / GL */
MKFW_API void                 mkfw_window_attach_context(struct mkfw_window *state);
MKFW_API void                 mkfw_window_detach_context(struct mkfw_window *state);
MKFW_API void                 mkfw_window_swap_buffers(struct mkfw_window *state);
MKFW_API void                 mkfw_window_set_swap_interval(struct mkfw_window *state, uint32_t interval);
MKFW_API int32_t              mkfw_window_get_swap_interval(struct mkfw_window *state);

/* Mouse + cursor */
MKFW_API void                 mkfw_window_set_mouse_sensitivity(struct mkfw_window *state, double sensitivity);
MKFW_API void                 mkfw_window_get_and_clear_mouse_delta(struct mkfw_window *state, int32_t *dx, int32_t *dy);
MKFW_API void                 mkfw_window_get_cursor_position(struct mkfw_window *state, int32_t *x, int32_t *y);
MKFW_API void                 mkfw_window_set_cursor_position(struct mkfw_window *state, int32_t x, int32_t y);
MKFW_API void                 mkfw_window_set_cursor_visible(struct mkfw_window *state, uint32_t visible);
MKFW_API void                 mkfw_window_set_cursor_locked(struct mkfw_window *state, uint32_t locked);
MKFW_API uint32_t             mkfw_window_is_cursor_visible(struct mkfw_window *state);
MKFW_API uint32_t             mkfw_window_is_cursor_locked(struct mkfw_window *state);
MKFW_API void                 mkfw_window_set_cursor_shape(struct mkfw_window *state, uint32_t cursor);
MKFW_API struct mkfw_cursor  *mkfw_cursor_create_rgba(struct mkfw_context *ctx, uint32_t width, uint32_t height, uint8_t *rgba, int32_t hotspot_x, int32_t hotspot_y);
MKFW_API void                 mkfw_cursor_destroy(struct mkfw_context *ctx, struct mkfw_cursor *cursor);
MKFW_API void                 mkfw_window_set_custom_cursor(struct mkfw_window *state, struct mkfw_cursor *cursor);

/* Clipboard, drop, native handles */
MKFW_API void                 mkfw_window_set_clipboard_text(struct mkfw_window *state, const char *text);
MKFW_API char                *mkfw_window_get_clipboard_text(struct mkfw_window *state);
MKFW_API void                 mkfw_window_enable_drop(struct mkfw_window *state, uint8_t enable);
MKFW_API void                 mkfw_window_get_native_handles(struct mkfw_window *state, struct mkfw_native_handles *out);

/* Inline helper functions - placed after platform includes so struct is defined */
static inline void mkfw_window_update_input_state(struct mkfw_window *state) {
	memcpy(state->prev_keyboard_state, state->keyboard_state, sizeof(state->keyboard_state));
	memcpy(state->prev_scancode_state, state->scancode_state, sizeof(state->scancode_state));
	memcpy(state->previous_mouse_buttons, state->mouse_buttons, sizeof(state->mouse_buttons));
}

/* Release a drop-callback path array.  The drop callback receives a
 * malloc'd array of malloc'd UTF-8 strings; this helper frees them in
 * the canonical way.  Callers may instead free each path and the array
 * directly if they want to retain a path without copying it first. */
static inline void mkfw_drop_paths_free(uint32_t count, const char **paths) {
	if(!paths) {
		return;
	}
	for(uint32_t i = 0; i < count; ++i) {
		free((char *)paths[i]);
	}
	free((char **)paths);
}

static inline uint32_t mkfw_window_get_modifiers(struct mkfw_window *state) {
	return (state->keyboard_state[MKFW_KEY_LSHIFT] ? MKFW_MOD_LSHIFT : 0)
		 | (state->keyboard_state[MKFW_KEY_RSHIFT] ? MKFW_MOD_RSHIFT : 0)
		 | (state->keyboard_state[MKFW_KEY_LCTRL]  ? MKFW_MOD_LCTRL  : 0)
		 | (state->keyboard_state[MKFW_KEY_RCTRL]  ? MKFW_MOD_RCTRL  : 0)
		 | (state->keyboard_state[MKFW_KEY_LALT]   ? MKFW_MOD_LALT   : 0)
		 | (state->keyboard_state[MKFW_KEY_RALT]   ? MKFW_MOD_RALT   : 0)
		 | (state->keyboard_state[MKFW_KEY_LSUPER] ? MKFW_MOD_LSUPER : 0)
		 | (state->keyboard_state[MKFW_KEY_RSUPER] ? MKFW_MOD_RSUPER : 0);
}

static inline void mkfw_set_error_callback(mkfw_error_callback_t callback) { mkfw_error_callback = callback; }
static inline void mkfw_window_set_user_data(struct mkfw_window *state, void *user_data) { state->user_data = user_data; }
static inline void *mkfw_window_get_user_data(struct mkfw_window *state) { return state->user_data; }
static inline void mkfw_window_set_key_callback(struct mkfw_window *state, mkfw_key_callback_t callback) { state->key_callback = callback; }
static inline void mkfw_window_set_char_callback(struct mkfw_window *state, mkfw_char_callback_t callback) { state->char_callback = callback; }
static inline void mkfw_window_set_scroll_callback(struct mkfw_window *state, mkfw_scroll_callback_t callback) { state->scroll_callback = callback; }
static inline void mkfw_window_set_mouse_move_delta_callback(struct mkfw_window *state, mkfw_mouse_move_delta_callback_t callback) { state->mouse_move_delta_callback = callback; }
static inline void mkfw_window_set_mouse_button_callback(struct mkfw_window *state, mkfw_mouse_button_callback_t callback) { state->mouse_button_callback = callback; }
static inline void mkfw_window_set_framebuffer_size_callback(struct mkfw_window *state, mkfw_framebuffer_callback_t callback) { state->framebuffer_callback = callback; }
static inline void mkfw_window_set_focus_callback(struct mkfw_window *state, mkfw_focus_callback_t callback) { state->focus_callback = callback; }
static inline void mkfw_window_set_drop_callback(struct mkfw_window *state, mkfw_drop_callback_t callback) { state->drop_callback = callback; mkfw_window_enable_drop(state, callback != 0); }
static inline void mkfw_window_set_state_callback(struct mkfw_window *state, mkfw_window_state_callback_t callback) { state->window_state_callback = callback; }
static inline uint32_t mkfw_window_is_key_pressed(struct mkfw_window *state, uint8_t key) { return state->keyboard_state[key] && !state->prev_keyboard_state[key]; }
static inline uint32_t mkfw_window_was_key_released(struct mkfw_window *state, uint8_t key) { return !state->keyboard_state[key] && state->prev_keyboard_state[key]; }
static inline uint32_t mkfw_window_is_scancode_down(struct mkfw_window *state, uint8_t scancode) { return state->scancode_state[scancode]; }
static inline uint32_t mkfw_window_is_scancode_pressed(struct mkfw_window *state, uint8_t scancode) { return state->scancode_state[scancode] && !state->prev_scancode_state[scancode]; }
static inline uint32_t mkfw_window_was_scancode_released(struct mkfw_window *state, uint8_t scancode) { return !state->scancode_state[scancode] && state->prev_scancode_state[scancode]; }
static inline uint32_t mkfw_window_is_button_pressed(struct mkfw_window *state, uint8_t button) { return state->mouse_buttons[button] && !state->previous_mouse_buttons[button]; }
static inline uint32_t mkfw_window_was_button_released(struct mkfw_window *state, uint8_t button) { return !state->mouse_buttons[button] && state->previous_mouse_buttons[button]; }

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
		case MKFW_KEY_SPACE:       return "Space";
		case MKFW_KEY_APOSTROPHE:  return "'";
		case MKFW_KEY_COMMA:       return ",";
		case MKFW_KEY_MINUS:       return "-";
		case MKFW_KEY_PERIOD:      return ".";
		case MKFW_KEY_SLASH:       return "/";
		case MKFW_KEY_SEMICOLON:   return ";";
		case MKFW_KEY_EQUAL:       return "=";
		case MKFW_KEY_LEFT_BRACKET: return "[";
		case MKFW_KEY_BACKSLASH:   return "\\";
		case MKFW_KEY_RIGHT_BRACKET: return "]";
		case MKFW_KEY_GRAVE:       return "`";
		case MKFW_KEY_ESCAPE:      return "Escape";
		case MKFW_KEY_RETURN:      return "Enter";
		case MKFW_KEY_TAB:         return "Tab";
		case MKFW_KEY_BACKSPACE:   return "Backspace";
		case MKFW_KEY_INSERT:      return "Insert";
		case MKFW_KEY_DELETE:      return "Delete";
		case MKFW_KEY_LEFT:        return "Left";
		case MKFW_KEY_RIGHT:       return "Right";
		case MKFW_KEY_UP:          return "Up";
		case MKFW_KEY_DOWN:        return "Down";
		case MKFW_KEY_HOME:        return "Home";
		case MKFW_KEY_END:         return "End";
		case MKFW_KEY_PAGEUP:      return "PageUp";
		case MKFW_KEY_PAGEDOWN:    return "PageDown";
		case MKFW_KEY_CAPSLOCK:    return "CapsLock";
		case MKFW_KEY_NUMLOCK:     return "NumLock";
		case MKFW_KEY_SCROLLLOCK:  return "ScrollLock";
		case MKFW_KEY_PRINTSCREEN: return "PrintScreen";
		case MKFW_KEY_PAUSE:       return "Pause";
		case MKFW_KEY_MENU:        return "Menu";
		case MKFW_KEY_F1:          return "F1";
		case MKFW_KEY_F2:          return "F2";
		case MKFW_KEY_F3:          return "F3";
		case MKFW_KEY_F4:          return "F4";
		case MKFW_KEY_F5:          return "F5";
		case MKFW_KEY_F6:          return "F6";
		case MKFW_KEY_F7:          return "F7";
		case MKFW_KEY_F8:          return "F8";
		case MKFW_KEY_F9:          return "F9";
		case MKFW_KEY_F10:         return "F10";
		case MKFW_KEY_F11:         return "F11";
		case MKFW_KEY_F12:         return "F12";
		case MKFW_KEY_SHIFT:       return "Shift";
		case MKFW_KEY_LSHIFT:      return "Left Shift";
		case MKFW_KEY_RSHIFT:      return "Right Shift";
		case MKFW_KEY_CTRL:        return "Ctrl";
		case MKFW_KEY_LCTRL:       return "Left Ctrl";
		case MKFW_KEY_RCTRL:       return "Right Ctrl";
		case MKFW_KEY_ALT:         return "Alt";
		case MKFW_KEY_LALT:        return "Left Alt";
		case MKFW_KEY_RALT:        return "Right Alt";
		case MKFW_KEY_LSUPER:      return "Left Super";
		case MKFW_KEY_RSUPER:      return "Right Super";
		case MKFW_KEY_NUMPAD_0:    return "Numpad 0";
		case MKFW_KEY_NUMPAD_1:    return "Numpad 1";
		case MKFW_KEY_NUMPAD_2:    return "Numpad 2";
		case MKFW_KEY_NUMPAD_3:    return "Numpad 3";
		case MKFW_KEY_NUMPAD_4:    return "Numpad 4";
		case MKFW_KEY_NUMPAD_5:    return "Numpad 5";
		case MKFW_KEY_NUMPAD_6:    return "Numpad 6";
		case MKFW_KEY_NUMPAD_7:    return "Numpad 7";
		case MKFW_KEY_NUMPAD_8:    return "Numpad 8";
		case MKFW_KEY_NUMPAD_9:    return "Numpad 9";
		case MKFW_KEY_NUMPAD_DECIMAL:  return "Numpad .";
		case MKFW_KEY_NUMPAD_DIVIDE:   return "Numpad /";
		case MKFW_KEY_NUMPAD_MULTIPLY: return "Numpad *";
		case MKFW_KEY_NUMPAD_SUBTRACT: return "Numpad -";
		case MKFW_KEY_NUMPAD_ADD:      return "Numpad +";
		case MKFW_KEY_NUMPAD_ENTER:    return "Numpad Enter";
		default:                  return "Unknown";
	}
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
