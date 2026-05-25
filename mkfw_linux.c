// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

// In unity mode this file is #included from mkfw.h; in library mode it
// is compiled standalone, so include mkfw.h to pick up the public
// types, MKFW_API / MKFW_VAR macros, error helpers, etc.
#include "mkfw.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/Xrandr.h>

#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/Xresource.h>

#include "mkfw_glx_mini.h"
#include "mkfw_linux_xlib_loader.h"
#include "mkfw_linux_xrandr_loader.h"
#include "mkfw_linux_xinput2_loader.h"

/* Storage for the cross-TU variables declared MKFW_VAR in mkfw.h.
 * Provided only in library / shared builds; in unity mode the
 * static declaration in the header is also the definition. */
#if defined(MKFW_BUILD_SHARED) || defined(MKFW_BUILD_LIBRARY)
mkfw_error_callback_t           mkfw_error_callback;
#endif

// Per-thread last-error storage.  __thread on Linux/glibc lowers to
// native ELF TLS with no extra runtime dependency, so we use it
// directly.  These live at file scope so the slot is allocated by the
// dynamic loader before main() and stays valid for the lifetime of
// each thread that ever touches the error API.
static __thread char    mkfw_linux_last_error_buf[512];
static __thread uint8_t mkfw_linux_last_error_set;

// [=]===^=[ mkfw_error ]=========================================================================[=]
MKFW_API void mkfw_error(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vsnprintf(mkfw_linux_last_error_buf, sizeof(mkfw_linux_last_error_buf), fmt, args);
	va_end(args);
	mkfw_linux_last_error_set = 1;
	if(mkfw_error_callback) {
		mkfw_error_callback(mkfw_linux_last_error_buf);
	}
}

// [=]===^=[ mkfw_get_last_error ]================================================================[=]
MKFW_API const char *mkfw_get_last_error(void) {
	return mkfw_linux_last_error_set ? mkfw_linux_last_error_buf : 0;
}

// [=]===^=[ mkfw_clear_last_error ]==============================================================[=]
MKFW_API void mkfw_clear_last_error(void) {
	mkfw_linux_last_error_buf[0] = 0;
	mkfw_linux_last_error_set = 0;
}

/* Platform casting macros */
#define PLATFORM(state) ((struct x11_mkfw_window *)(state)->platform)
#define CTX_PLATFORM(c) ((struct x11_mkfw_context *)(c)->platform)
#define WIN_CTX_PLATFORM(w) CTX_PLATFORM((w)->context)

struct x11_mkfw_context {
	Display *display;
	uint8_t libs_loaded;
	uint8_t signal_handlers_installed;
};

/* libXcursor minimal loader.  Used by mkfw_cursor_create_rgba; missing
 * libXcursor is non-fatal and simply makes custom cursor creation
 * return 0. */
typedef struct mkfw_xcursor_image {
	unsigned int version;
	unsigned int size;
	unsigned int width;
	unsigned int height;
	unsigned int xhot;
	unsigned int yhot;
	unsigned int delay;
	unsigned int *pixels;
} mkfw_xcursor_image;

typedef mkfw_xcursor_image *(*PFN_XcursorImageCreate)(int, int);
typedef void                (*PFN_XcursorImageDestroy)(mkfw_xcursor_image *);
typedef Cursor              (*PFN_XcursorImageLoadCursor)(Display *, const mkfw_xcursor_image *);

static PFN_XcursorImageCreate     mkfw_XcursorImageCreate;
static PFN_XcursorImageDestroy    mkfw_XcursorImageDestroy;
static PFN_XcursorImageLoadCursor mkfw_XcursorImageLoadCursor;

// [=]===^=[ load_xcursor_functions ]=============================================================[=]
static uint32_t load_xcursor_functions(void) {
	void *lib = dlopen("libXcursor.so.1", RTLD_LAZY);
	if(!lib) {
		return 0;
	}
	*(void **)&mkfw_XcursorImageCreate     = dlsym(lib, "XcursorImageCreate");
	*(void **)&mkfw_XcursorImageDestroy    = dlsym(lib, "XcursorImageDestroy");
	*(void **)&mkfw_XcursorImageLoadCursor = dlsym(lib, "XcursorImageLoadCursor");
	return mkfw_XcursorImageCreate && mkfw_XcursorImageDestroy && mkfw_XcursorImageLoadCursor;
}

struct mkfw_cursor {
	Cursor x_cursor;
};

struct x11_mkfw_window {
	Display *display;
	Window   window;
	GLXContext glctx;
	uint32_t graphics_api;
	float aspect_ratio;
	uint8_t cursor_locked;
	uint8_t cursor_visible;
	int32_t last_mouse_x;
	int32_t last_mouse_y;
	int32_t win_saved_width;
	int32_t win_saved_height;
	int32_t win_saved_x;
	int32_t win_saved_y;
	int32_t min_width;
	int32_t min_height;
	int32_t max_width;
	int32_t max_height;
	int32_t aspect_num;
	int32_t aspect_den;
	Atom wm_delete_window;
	Cursor hidden_cursor;
	uint8_t should_close;
	uint8_t in_window;
	int32_t xi_opcode;

	// Mouse delta smoothing (per-window)
	double last_mouse_dx;
	double last_mouse_dy;

	// Mouse delta accumulator (read-and-clear)
	double accumulated_dx;
	double accumulated_dy;
	double mouse_sensitivity;

	// Framebuffer size tracking (per-window)
	int32_t last_framebuffer_width;
	int32_t last_framebuffer_height;

	// XIM for Unicode text input
	XIM xim;
	XIC xic;

	// Cursor shapes
	Cursor cursors[MKFW_CURSOR_LAST];
	uint32_t current_cursor;
	Cursor active_custom_cursor;

	// Clipboard
	Atom clipboard_atom;
	Atom utf8_string_atom;
	Atom targets_atom;
	Atom mkfw_clipboard_atom;
	char *clipboard_text;

	// XDND drag-and-drop
	Atom xdnd_aware;
	Atom xdnd_enter;
	Atom xdnd_position;
	Atom xdnd_status;
	Atom xdnd_leave;
	Atom xdnd_drop;
	Atom xdnd_finished;
	Atom xdnd_selection;
	Atom xdnd_type_list;
	Atom text_uri_list;
	Window xdnd_source;
	uint8_t xdnd_has_uri_list;

	// Window state callback tracking
	Atom net_wm_state;
	Atom net_wm_state_maximized_horz;
	Atom net_wm_state_maximized_vert;
	Atom wm_state;
	Atom net_wm_state_demands_attention;
	uint8_t last_maximized;
	uint8_t last_minimized;
};

// USB HID Usage Page 7 scancode for each evdev key code (X11 keycode minus 8,
// on modern Linux with the evdev/libinput driver).  Entries are 0 for codes
// outside the standard PC101 + extended layout, which mkfw does not map.
static const uint8_t mkfw_evdev_to_hid[128] = {
	[1]   = 0x29,                                   // ESC
	[2]   = 0x1e, [3]  = 0x1f, [4]  = 0x20, [5]  = 0x21, [6]  = 0x22,
	[7]   = 0x23, [8]  = 0x24, [9]  = 0x25, [10] = 0x26, [11] = 0x27, // 1..0
	[12]  = 0x2d, [13] = 0x2e, [14] = 0x2a, [15] = 0x2b,             // - = BS Tab
	[16]  = 0x14, [17] = 0x1a, [18] = 0x08, [19] = 0x15, [20] = 0x17,
	[21]  = 0x1c, [22] = 0x18, [23] = 0x0c, [24] = 0x12, [25] = 0x13, // Q..P
	[26]  = 0x2f, [27] = 0x30, [28] = 0x28, [29] = 0xe0,             // [ ] Enter LCtrl
	[30]  = 0x04, [31] = 0x16, [32] = 0x07, [33] = 0x09, [34] = 0x0a,
	[35]  = 0x0b, [36] = 0x0d, [37] = 0x0e, [38] = 0x0f,             // A..L
	[39]  = 0x33, [40] = 0x34, [41] = 0x35, [42] = 0xe1, [43] = 0x31, // ; apostrophe grave LShift backslash
	[44]  = 0x1d, [45] = 0x1b, [46] = 0x06, [47] = 0x19, [48] = 0x05,
	[49]  = 0x11, [50] = 0x10,                                       // Z..M
	[51]  = 0x36, [52] = 0x37, [53] = 0x38, [54] = 0xe5,             // , . / RShift
	[55]  = 0x55, [56] = 0xe2, [57] = 0x2c, [58] = 0x39,             // KP* LAlt Space Caps
	[59]  = 0x3a, [60] = 0x3b, [61] = 0x3c, [62] = 0x3d, [63] = 0x3e,
	[64]  = 0x3f, [65] = 0x40, [66] = 0x41, [67] = 0x42, [68] = 0x43,// F1..F10
	[69]  = 0x53, [70] = 0x47,                                       // NumLock ScrollLock
	[71]  = 0x5f, [72] = 0x60, [73] = 0x61, [74] = 0x56,             // KP7 KP8 KP9 KP-
	[75]  = 0x5c, [76] = 0x5d, [77] = 0x5e, [78] = 0x57,             // KP4 KP5 KP6 KP+
	[79]  = 0x59, [80] = 0x5a, [81] = 0x5b, [82] = 0x62, [83] = 0x63,// KP1..KP0 KP.
	[87]  = 0x44, [88] = 0x45,                                       // F11 F12
	[96]  = 0x58, [97] = 0xe4, [98] = 0x54, [99] = 0x46,             // KPEnter RCtrl KP/ PrtScr
	[100] = 0xe6,                                                    // RAlt
	[102] = 0x4a, [103] = 0x52, [104] = 0x4b,                        // Home Up PageUp
	[105] = 0x50, [106] = 0x4f,                                      // Left Right
	[107] = 0x4d, [108] = 0x51, [109] = 0x4e,                        // End Down PageDown
	[110] = 0x49, [111] = 0x4c,                                      // Insert Delete
	[119] = 0x48,                                                    // Pause
	[125] = 0xe3, [126] = 0xe7, [127] = 0x65,                        // LSuper RSuper Menu
};

// [=]===^=[ x11_update_scancode ]================================================================[=]
static void x11_update_scancode(struct mkfw_window *state, uint32_t keycode, uint32_t down) {
	if(keycode < 8) {
		return;
	}
	uint32_t evdev = keycode - 8;
	if(evdev >= 128) {
		return;
	}
	uint8_t hid = mkfw_evdev_to_hid[evdev];
	if(hid) {
		state->scancode_state[hid] = down ? 1 : 0;
	}
}

// [=]===^=[ map_x11_keysym ]=====================================================================[=]
static uint32_t map_x11_keysym(struct mkfw_window *state, KeySym keysym, int key_down) {
	uint32_t keycode = 0;

	// Handle number keys (XK_0 - XK_9)
	if(keysym >= XK_0 && keysym <= XK_9) {
		keycode = MKFW_KEY_0 + (keysym - XK_0);
	} else if(keysym >= 0x20 && keysym <= 0x7E) {
		keycode = keysym;
	}

	// Track modifier state
	switch(keysym) {
		case XK_Shift_L:
			state->keyboard_state[MKFW_KEY_LSHIFT] = key_down;
			break;
		case XK_Shift_R:
			state->keyboard_state[MKFW_KEY_RSHIFT] = key_down;
			break;
		case XK_Control_L:
			state->keyboard_state[MKFW_KEY_LCTRL] = key_down;
			break;
		case XK_Control_R:
			state->keyboard_state[MKFW_KEY_RCTRL] = key_down;
			break;
		case XK_Alt_L:
			state->keyboard_state[MKFW_KEY_LALT] = key_down;
			break;
		case XK_Alt_R:
			state->keyboard_state[MKFW_KEY_RALT] = key_down;
			break;
		case XK_Super_L:
			state->keyboard_state[MKFW_KEY_LSUPER] = key_down;
			break;
		case XK_Super_R:
			state->keyboard_state[MKFW_KEY_RSUPER] = key_down;
			break;
	}

	// Update combined modifier states from keyboard_state
	state->keyboard_state[MKFW_KEY_SHIFT] = state->keyboard_state[MKFW_KEY_LSHIFT] || state->keyboard_state[MKFW_KEY_RSHIFT];
	state->keyboard_state[MKFW_KEY_CTRL]  = state->keyboard_state[MKFW_KEY_LCTRL] || state->keyboard_state[MKFW_KEY_RCTRL];
	state->keyboard_state[MKFW_KEY_ALT]   = state->keyboard_state[MKFW_KEY_LALT] || state->keyboard_state[MKFW_KEY_RALT];

	// Handle non-extended special keys
	switch(keysym) {
		case XK_Escape:    keycode = MKFW_KEY_ESCAPE; break;
		case XK_BackSpace: keycode = MKFW_KEY_BACKSPACE; break;
		case XK_Tab:       keycode = MKFW_KEY_TAB; break;
		case XK_Return:    keycode = MKFW_KEY_RETURN; break;
		case XK_Caps_Lock: keycode = MKFW_KEY_CAPSLOCK; break;
		case XK_F1:        keycode = MKFW_KEY_F1; break;
		case XK_F2:        keycode = MKFW_KEY_F2; break;
		case XK_F3:        keycode = MKFW_KEY_F3; break;
		case XK_F4:        keycode = MKFW_KEY_F4; break;
		case XK_F5:        keycode = MKFW_KEY_F5; break;
		case XK_F6:        keycode = MKFW_KEY_F6; break;
		case XK_F7:        keycode = MKFW_KEY_F7; break;
		case XK_F8:        keycode = MKFW_KEY_F8; break;
		case XK_F9:        keycode = MKFW_KEY_F9; break;
		case XK_F10:       keycode = MKFW_KEY_F10; break;
		case XK_F11:       keycode = MKFW_KEY_F11; break;
		case XK_F12:       keycode = MKFW_KEY_F12; break;
	}

	// Handle extended keys
	switch(keysym) {
		case XK_Left:      keycode = MKFW_KEY_LEFT; break;
		case XK_Right:     keycode = MKFW_KEY_RIGHT; break;
		case XK_Up:        keycode = MKFW_KEY_UP; break;
		case XK_Down:      keycode = MKFW_KEY_DOWN; break;
		case XK_Insert:    keycode = MKFW_KEY_INSERT; break;
		case XK_Delete:    keycode = MKFW_KEY_DELETE; break;
		case XK_Home:      keycode = MKFW_KEY_HOME; break;
		case XK_End:       keycode = MKFW_KEY_END; break;
		case XK_Page_Up:   keycode = MKFW_KEY_PAGEUP; break;
		case XK_Page_Down: keycode = MKFW_KEY_PAGEDOWN; break;
		case XK_Num_Lock:  keycode = MKFW_KEY_NUMLOCK; break;
		case XK_Scroll_Lock: keycode = MKFW_KEY_SCROLLLOCK; break;
		case XK_Print:     keycode = MKFW_KEY_PRINTSCREEN; break;
		case XK_Pause:     keycode = MKFW_KEY_PAUSE; break;
		case XK_Menu:      keycode = MKFW_KEY_MENU; break;
	}

	// Handle numpad keys
	switch(keysym) {
		case XK_KP_0:        keycode = MKFW_KEY_NUMPAD_0; break;
		case XK_KP_1:        keycode = MKFW_KEY_NUMPAD_1; break;
		case XK_KP_2:        keycode = MKFW_KEY_NUMPAD_2; break;
		case XK_KP_3:        keycode = MKFW_KEY_NUMPAD_3; break;
		case XK_KP_4:        keycode = MKFW_KEY_NUMPAD_4; break;
		case XK_KP_5:        keycode = MKFW_KEY_NUMPAD_5; break;
		case XK_KP_6:        keycode = MKFW_KEY_NUMPAD_6; break;
		case XK_KP_7:        keycode = MKFW_KEY_NUMPAD_7; break;
		case XK_KP_8:        keycode = MKFW_KEY_NUMPAD_8; break;
		case XK_KP_9:        keycode = MKFW_KEY_NUMPAD_9; break;
		case XK_KP_Decimal:  keycode = MKFW_KEY_NUMPAD_DECIMAL; break;
		case XK_KP_Divide:   keycode = MKFW_KEY_NUMPAD_DIVIDE; break;
		case XK_KP_Multiply: keycode = MKFW_KEY_NUMPAD_MULTIPLY; break;
		case XK_KP_Subtract: keycode = MKFW_KEY_NUMPAD_SUBTRACT; break;
		case XK_KP_Add:      keycode = MKFW_KEY_NUMPAD_ADD; break;
		case XK_KP_Enter:    keycode = MKFW_KEY_NUMPAD_ENTER; break;
	}

	// Update key state
	if(keycode) {
		state->keyboard_state[keycode] = key_down;
	}

	// Call the key callback
	if(keycode && state->key_callback) {
		state->key_callback(state, keycode, key_down ? MKFW_PRESSED : MKFW_RELEASED, (state->keyboard_state[MKFW_KEY_SHIFT] ? MKFW_MOD_SHIFT : 0) | (state->keyboard_state[MKFW_KEY_CTRL] ? MKFW_MOD_CTRL : 0) | (state->keyboard_state[MKFW_KEY_ALT] ? MKFW_MOD_ALT : 0) | (state->keyboard_state[MKFW_KEY_LSUPER] ? MKFW_MOD_LSUPER : 0) | (state->keyboard_state[MKFW_KEY_RSUPER] ? MKFW_MOD_RSUPER : 0));
	}

	return keycode;
}

// [=]===^=[ enable_xi2_raw_input ]===============================================================[=]
static void enable_xi2_raw_input(struct mkfw_window *state) {
	int event, error;
	if(!XQueryExtension(PLATFORM(state)->display, "XInputExtension", &PLATFORM(state)->xi_opcode, &event, &error)) {
		mkfw_error("XInput2 not available on this X server");
		return;
	}

	int major = 2, minor = 0;
	if(XIQueryVersion(PLATFORM(state)->display, &major, &minor) == BadRequest) {
		mkfw_error("XInput2 version 2.0 not supported");
		return;
	}

	Window root = DefaultRootWindow(PLATFORM(state)->display);

	XIEventMask mask;
	unsigned char mask_bytes[(XI_LASTEVENT + 7) / 8] = {0};

	mask.deviceid = XIAllDevices;
	mask.mask_len = sizeof(mask_bytes);
	mask.mask = mask_bytes;
	XISetMask(mask.mask, XI_RawMotion);

	XISelectEvents(PLATFORM(state)->display, root, &mask, 1);
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_window_should_close ]==================================================================[=]
MKFW_API uint32_t mkfw_window_should_close(struct mkfw_window *state) {
	return PLATFORM(state)->should_close;
}

// [=]===^=[ mkfw_window_set_should_close ]==============================================================[=]
MKFW_API void mkfw_window_set_should_close(struct mkfw_window *state, int32_t value) {
	PLATFORM(state)->should_close = value;
}

// [=]===^=[ select_best_fbconfig_for ]===========================================================[=]
// Returns the chosen GLXFBConfig, or 0 on failure (mkfw_error has been fired).
static GLXFBConfig select_best_fbconfig_for(Display *display, int screen, int32_t transparent) {
	int fb_count = 0;
	GLXFBConfig *fbcs = glXChooseFBConfig(display, screen, 0, &fb_count);
	if(!fbcs || fb_count == 0) {
		mkfw_error("no framebuffer configs found");
		if(fbcs) {
			XFree(fbcs);
		}
		return 0;
	}

	GLXFBConfig best_fbconfig = 0;
	int highest_score = -1;

	for(int i = 0; i < fb_count; i++) {
		int drawable_type = 0;
		int doublebuffer  = 0;
		int red_size, green_size, blue_size, alpha_size;
		int depth_size, stencil_size;

		glXGetFBConfigAttrib(display, fbcs[i], GLX_DRAWABLE_TYPE, &drawable_type);
		glXGetFBConfigAttrib(display, fbcs[i], GLX_DOUBLEBUFFER, &doublebuffer);

		/* Skip configs that cannot render to windows */
		if(!(drawable_type & GLX_WINDOW_BIT)) {
			continue;
		}

		/* Fetch color/depth/stencil sizes */
		glXGetFBConfigAttrib(display, fbcs[i], GLX_ALPHA_SIZE, &alpha_size);
		if(transparent && alpha_size < 8) {
			continue;
		}

		/* For transparency we need a true 32-bit visual, not just alpha in the fbconfig */
		if(transparent) {
			XVisualInfo *tvi = glXGetVisualFromFBConfig(display, fbcs[i]);
			if(!tvi || tvi->depth != 32) {
				if(tvi) {
					XFree(tvi);
				}
				continue;
			}
			XFree(tvi);
		}

		glXGetFBConfigAttrib(display, fbcs[i], GLX_RED_SIZE, &red_size);
		glXGetFBConfigAttrib(display, fbcs[i], GLX_GREEN_SIZE, &green_size);
		glXGetFBConfigAttrib(display, fbcs[i], GLX_BLUE_SIZE, &blue_size);
		glXGetFBConfigAttrib(display, fbcs[i], GLX_ALPHA_SIZE, &alpha_size);
		glXGetFBConfigAttrib(display, fbcs[i], GLX_DEPTH_SIZE, &depth_size);
		glXGetFBConfigAttrib(display, fbcs[i], GLX_STENCIL_SIZE, &stencil_size);

		/* Score config */
		int score = 0;

		if(doublebuffer) {
			score += 100;
		}

		if(red_size >= 8 && green_size >= 8 && blue_size >= 8) {
			score += 50;
		}

		if(alpha_size >= 8) {
			score += 25;
		}

		if(depth_size >= 24) {
			score += 10;
		}

		if(stencil_size >= 8) {
			score += 5;
		}

		if(score > highest_score) {
			highest_score = score;
			best_fbconfig = fbcs[i];
		}
	}

	if(!best_fbconfig) {
		mkfw_error("no suitable framebuffer config supports window rendering, falling back to first");
		best_fbconfig = fbcs[0];
	}

	XFree(fbcs);
	return best_fbconfig;
}

// [=]===^=[ mkfw_window_detach_context ]================================================================[=]
MKFW_API void mkfw_window_detach_context(struct mkfw_window *state) {
	glXMakeCurrent(PLATFORM(state)->display, None, 0);
}

// [=]===^=[ mkfw_window_attach_context ]================================================================[=]
MKFW_API void mkfw_window_attach_context(struct mkfw_window *state) {
	glXMakeCurrent(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->glctx);
}

// [=]===^=[ mkfw_window_show ]===================================================================[=]
MKFW_API void mkfw_window_show(struct mkfw_window *state) {
	XMapWindow(PLATFORM(state)->display, PLATFORM(state)->window);
	XFlush(PLATFORM(state)->display);
	XSync(PLATFORM(state)->display, 0);
}

// [=]===^=[ mkfw_window_hide ]===================================================================[=]
MKFW_API void mkfw_window_hide(struct mkfw_window *state) {
	XUnmapWindow(PLATFORM(state)->display, PLATFORM(state)->window);
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_query_max_gl_version ]==========================================================[=]
MKFW_API uint32_t mkfw_query_max_gl_version(int32_t *major, int32_t *minor) {
	load_x11_functions();
	Display *dpy = XOpenDisplay(0);
	if(!dpy) {
		mkfw_error("mkfw_query_max_gl_version: unable to open X display");
		return 0;
	}

	load_glx_functions(dpy);

	int screen = DefaultScreen(dpy);
	GLXFBConfig fb_config = select_best_fbconfig_for(dpy, screen, 0);
	if(!fb_config) {
		XCloseDisplay(dpy);
		return 0;
	}

	XVisualInfo *vi = glXGetVisualFromFBConfig(dpy, fb_config);
	if(!vi) {
		mkfw_error("mkfw_query_max_gl_version: glXGetVisualFromFBConfig returned 0");
		XCloseDisplay(dpy);
		return 0;
	}

	Window root = RootWindow(dpy, screen);
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa = {0};
	swa.colormap = cmap;
	Window win = XCreateWindow(dpy, root, 0, 0, 1, 1, 0, vi->depth, InputOutput, vi->visual, CWColormap, &swa);

	// Request 3.1 compat -- drivers return the max supported version
	int ctx_attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 1,
		GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		0
	};

	GLXContext ctx = glXCreateContextAttribsARB(dpy, fb_config, 0, 1, ctx_attribs);
	if(!ctx) {
		mkfw_error("mkfw_query_max_gl_version: glXCreateContextAttribsARB failed to create probe context");
		XDestroyWindow(dpy, win);
		XFreeColormap(dpy, cmap);
		XFree(vi);
		XCloseDisplay(dpy);
		return 0;
	}

	glXMakeCurrent(dpy, win, ctx);

	typedef const unsigned char *(*PFNGLGETSTRINGPROC)(unsigned int);
	PFNGLGETSTRINGPROC pglGetString = (PFNGLGETSTRINGPROC)glXGetProcAddress((const unsigned char *)"glGetString");
	uint32_t result = 0;
	if(pglGetString) {
		const char *version = (const char *)pglGetString(0x1F02); // GL_VERSION
		if(version) {
			int32_t tmp_major, tmp_minor;
			result = mkfw_parse_version(version, &tmp_major, &tmp_minor);
			if(result) {
				if(major) {
					*major = tmp_major;
				}
				if(minor) {
					*minor = tmp_minor;
				}
			}
		}
	}

	glXMakeCurrent(dpy, None, 0);
	glXDestroyContext(dpy, ctx);
	XDestroyWindow(dpy, win);
	XFreeColormap(dpy, cmap);
	XFree(vi);
	XCloseDisplay(dpy);

	return result;
}

// Forward declaration for monitor query used inside mkfw_init
static int32_t mkfw_query_monitors_into(Display *dpy, struct mkfw_monitor *out, int32_t max);

// [=]===^=[ mkfw_init ]==========================================================================[=]
MKFW_API struct mkfw_context *mkfw_init(struct mkfw_options *opts) {
	(void)opts;  // No fields yet beyond version

	struct mkfw_context *ctx = (struct mkfw_context *)calloc(1, sizeof(struct mkfw_context));
	if(!ctx) {
		mkfw_error("mkfw_init: out of memory");
		return 0;
	}
	ctx->platform = calloc(1, sizeof(struct x11_mkfw_context));
	if(!ctx->platform) {
		mkfw_error("mkfw_init: out of memory");
		free(ctx);
		return 0;
	}

	load_x11_functions();
	load_xrandr_functions();
	load_xinput2_functions();
	load_xcursor_functions();   // optional: missing libXcursor disables custom cursors
	CTX_PLATFORM(ctx)->libs_loaded = 1;

	XInitThreads();

	CTX_PLATFORM(ctx)->display = XOpenDisplay(0);
	if(!CTX_PLATFORM(ctx)->display) {
		mkfw_error("unable to open X display");
		free(ctx->platform);
		free(ctx);
		return 0;
	}

	load_glx_functions(CTX_PLATFORM(ctx)->display);

	XrmInitialize();

	// Cache monitors so callers can query before creating a window
	ctx->monitor_count = (uint32_t)mkfw_query_monitors_into(CTX_PLATFORM(ctx)->display, ctx->monitors, MKFW_MAX_MONITORS);

	return ctx;
}

// [=]===^=[ mkfw_window_create ]=================================================================[=]
MKFW_API struct mkfw_window *mkfw_window_create(struct mkfw_context *ctx, struct mkfw_window_options *opts) {
	if(!ctx) {
		mkfw_error("mkfw_window_create: ctx is null");
		return 0;
	}
	if(ctx->window_count >= MKFW_MAX_WINDOWS) {
		mkfw_error("mkfw_window_create: context already at MKFW_MAX_WINDOWS (%d)", MKFW_MAX_WINDOWS);
		return 0;
	}

	struct mkfw_window_options defaults = {0};
	if(!opts) {
		opts = &defaults;
	}
	uint32_t graphics_api = opts->graphics_api;
	if(graphics_api == MKFW_GFX_GLES || graphics_api == MKFW_GFX_VULKAN) {
		mkfw_error("graphics_api %u not supported (only MKFW_GFX_GL and MKFW_GFX_NONE)", graphics_api);
		return 0;
	}

	int32_t width    = opts->width    > 0 ? opts->width    : 1280;
	int32_t height   = opts->height   > 0 ? opts->height   : 720;
	int32_t gl_major = opts->gl_major;
	int32_t gl_minor = opts->gl_minor;
	if(gl_major == 0 && graphics_api == MKFW_GFX_GL) {
		if(!mkfw_query_max_gl_version(&gl_major, &gl_minor)) {
			mkfw_error("mkfw_window_create: unable to query driver's maximum OpenGL version");
			return 0;
		}
	}
	const char *title = opts->title ? opts->title : "mkfw";
	int32_t transparent = (opts->flags & MKFW_WIN_TRANSPARENT) ? 1 : 0;
	uint32_t gl_profile_bit = (opts->gl_profile == MKFW_GL_PROFILE_COMPAT)
		? GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
		: GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
	const char *gl_profile_name = (opts->gl_profile == MKFW_GL_PROFILE_COMPAT) ? "Compatibility" : "Core";

	struct mkfw_window *state = (struct mkfw_window *)calloc(1, sizeof(struct mkfw_window));
	if(!state) {
		mkfw_error("mkfw_window_create: out of memory");
		return 0;
	}
	state->context = ctx;

	state->platform = calloc(1, sizeof(struct x11_mkfw_window));
	if(!state->platform) {
		mkfw_error("mkfw_window_create: out of memory");
		free(state);
		return 0;
	}

	PLATFORM(state)->mouse_sensitivity = 1.0;
	PLATFORM(state)->xi_opcode = -1;
	PLATFORM(state)->display = CTX_PLATFORM(ctx)->display;
	PLATFORM(state)->graphics_api = graphics_api;
	PLATFORM(state)->cursor_visible = 1;

	Display *display = CTX_PLATFORM(ctx)->display;
	int screen = DefaultScreen(display);
	Window root = RootWindow(display, screen);

	XVisualInfo vi_storage = {0};
	XVisualInfo *vi = 0;
	GLXFBConfig fb_config = 0;

	if(graphics_api == MKFW_GFX_GL) {
		fb_config = select_best_fbconfig_for(display, screen, transparent);
		if(!fb_config) {
			free(state->platform);
			free(state);
			return 0;
		}
		vi = glXGetVisualFromFBConfig(display, fb_config);
		if(!vi) {
			mkfw_error("unable to get a visual from framebuffer config");
			free(state->platform);
			free(state);
			return 0;
		}
	} else {
		// Non-GL windows get the screen default visual; transparency hint
		// is the caller's responsibility (they own the rendering surface).
		vi_storage.visual = DefaultVisual(display, screen);
		vi_storage.depth  = DefaultDepth(display, screen);
		vi = &vi_storage;
	}

	Colormap cmap = XCreateColormap(display, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask | PropertyChangeMask;
	swa.border_pixel = 0;
	swa.background_pixmap = None;

	PLATFORM(state)->window = XCreateWindow(display, root, 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask, &swa);

	XStoreName(display, PLATFORM(state)->window, title);

	XClassHint *class_hint = XAllocClassHint();
	if(class_hint) {
		class_hint->res_name = (char *)"mkfw";
		class_hint->res_class = (char *)"MKFW";
		XSetClassHint(display, PLATFORM(state)->window, class_hint);
		XFree(class_hint);
	}

	PLATFORM(state)->wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, PLATFORM(state)->window, &PLATFORM(state)->wm_delete_window, 1);
	enable_xi2_raw_input(state);

	if(graphics_api == MKFW_GFX_GL) {
		if(!glXCreateContextAttribsARB) {
			mkfw_error("glXCreateContextAttribsARB not supported");
			XFree(vi);
			XDestroyWindow(display, PLATFORM(state)->window);
			free(state->platform);
			free(state);
			return 0;
		}

		int ctx_attribs[] = {
			GLX_CONTEXT_MAJOR_VERSION_ARB, gl_major,
			GLX_CONTEXT_MINOR_VERSION_ARB, gl_minor,
			GLX_CONTEXT_PROFILE_MASK_ARB,  (int)gl_profile_bit,
			0
		};

		PLATFORM(state)->glctx = glXCreateContextAttribsARB(display, fb_config, 0, 1, ctx_attribs);
		if(!PLATFORM(state)->glctx) {
			int32_t max_major = 0, max_minor = 0;
			mkfw_query_max_gl_version(&max_major, &max_minor);
			if(max_major > 0) {
				mkfw_error("OpenGL %d.%d %s Profile not available (driver supports up to %d.%d)",
					gl_major, gl_minor, gl_profile_name, max_major, max_minor);
			} else {
				mkfw_error("OpenGL %d.%d %s Profile not available", gl_major, gl_minor, gl_profile_name);
			}
			XFree(vi);
			XDestroyWindow(display, PLATFORM(state)->window);
			free(state->platform);
			free(state);
			return 0;
		}

		glXMakeCurrent(display, PLATFORM(state)->window, PLATFORM(state)->glctx);
		XFree(vi);
	}

	PLATFORM(state)->xim = XOpenIM(display, 0, 0, 0);
	if(PLATFORM(state)->xim) {
		PLATFORM(state)->xic = XCreateIC(PLATFORM(state)->xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, PLATFORM(state)->window, XNFocusWindow, PLATFORM(state)->window, (char *)0);
	}

	PLATFORM(state)->cursors[MKFW_CURSOR_ARROW]       = XCreateFontCursor(display, XC_left_ptr);
	PLATFORM(state)->cursors[MKFW_CURSOR_TEXT_INPUT]  = XCreateFontCursor(display, XC_xterm);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_ALL]  = XCreateFontCursor(display, XC_fleur);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_NS]   = XCreateFontCursor(display, XC_sb_v_double_arrow);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_EW]   = XCreateFontCursor(display, XC_sb_h_double_arrow);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_NESW] = XCreateFontCursor(display, XC_bottom_left_corner);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_NWSE] = XCreateFontCursor(display, XC_bottom_right_corner);
	PLATFORM(state)->cursors[MKFW_CURSOR_HAND]        = XCreateFontCursor(display, XC_hand2);
	PLATFORM(state)->cursors[MKFW_CURSOR_NOT_ALLOWED] = XCreateFontCursor(display, XC_X_cursor);

	PLATFORM(state)->clipboard_atom      = XInternAtom(display, "CLIPBOARD", False);
	PLATFORM(state)->utf8_string_atom    = XInternAtom(display, "UTF8_STRING", False);
	PLATFORM(state)->targets_atom        = XInternAtom(display, "TARGETS", False);
	PLATFORM(state)->mkfw_clipboard_atom = XInternAtom(display, "MKFW_CLIPBOARD", False);

	PLATFORM(state)->xdnd_aware     = XInternAtom(display, "XdndAware", False);
	PLATFORM(state)->xdnd_enter     = XInternAtom(display, "XdndEnter", False);
	PLATFORM(state)->xdnd_position  = XInternAtom(display, "XdndPosition", False);
	PLATFORM(state)->xdnd_status    = XInternAtom(display, "XdndStatus", False);
	PLATFORM(state)->xdnd_leave     = XInternAtom(display, "XdndLeave", False);
	PLATFORM(state)->xdnd_drop      = XInternAtom(display, "XdndDrop", False);
	PLATFORM(state)->xdnd_finished  = XInternAtom(display, "XdndFinished", False);
	PLATFORM(state)->xdnd_selection = XInternAtom(display, "XdndSelection", False);
	PLATFORM(state)->xdnd_type_list = XInternAtom(display, "XdndTypeList", False);
	PLATFORM(state)->text_uri_list  = XInternAtom(display, "text/uri-list", False);

	PLATFORM(state)->net_wm_state                = XInternAtom(display, "_NET_WM_STATE", False);
	PLATFORM(state)->net_wm_state_maximized_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	PLATFORM(state)->net_wm_state_maximized_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	PLATFORM(state)->wm_state                    = XInternAtom(display, "WM_STATE", False);
	PLATFORM(state)->net_wm_state_demands_attention = XInternAtom(display, "_NET_WM_STATE_DEMANDS_ATTENTION", False);

	state->has_focus = 1;

	ctx->windows[ctx->window_count++] = state;

	if(!(opts->flags & MKFW_WIN_HIDDEN)) {
		XMapWindow(display, PLATFORM(state)->window);
		XFlush(display);
	}

	return state;
}

// [=]===^=[ x11_active_cursor ]==================================================================[=]
static Cursor x11_active_cursor(struct mkfw_window *state) {
	if(PLATFORM(state)->active_custom_cursor) {
		return PLATFORM(state)->active_custom_cursor;
	}
	return PLATFORM(state)->cursors[PLATFORM(state)->current_cursor];
}

// [=]===^=[ mkfw_window_set_cursor_visible ]=====================================================[=]
MKFW_API void mkfw_window_set_cursor_visible(struct mkfw_window *state, uint32_t visible) {
	PLATFORM(state)->cursor_visible = visible ? 1 : 0;
	if(visible) {
		XDefineCursor(PLATFORM(state)->display, PLATFORM(state)->window, x11_active_cursor(state));
	} else {
		if(!PLATFORM(state)->hidden_cursor) {
			Pixmap pixmap = XCreatePixmap(PLATFORM(state)->display, PLATFORM(state)->window, 1, 1, 1);
			XColor black = {0};
			PLATFORM(state)->hidden_cursor = XCreatePixmapCursor(PLATFORM(state)->display, pixmap, pixmap, &black, &black, 0, 0);
			XFreePixmap(PLATFORM(state)->display, pixmap);
		}
		XDefineCursor(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->hidden_cursor);
	}
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_window_set_cursor_locked ]======================================================[=]
MKFW_API void mkfw_window_set_cursor_locked(struct mkfw_window *state, uint32_t locked) {
	if(locked) {
		int result = XGrabPointer(PLATFORM(state)->display, PLATFORM(state)->window, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask, GrabModeAsync, GrabModeAsync, PLATFORM(state)->window, None, CurrentTime);
		if(result != GrabSuccess) {
			mkfw_error("failed to grab pointer");
			return;
		}
		PLATFORM(state)->cursor_locked = 1;
	} else {
		XUngrabPointer(PLATFORM(state)->display, CurrentTime);
		PLATFORM(state)->cursor_locked = 0;
	}
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_window_is_cursor_visible ]======================================================[=]
MKFW_API uint32_t mkfw_window_is_cursor_visible(struct mkfw_window *state) {
	return PLATFORM(state)->cursor_visible;
}

// [=]===^=[ mkfw_window_is_cursor_locked ]=======================================================[=]
MKFW_API uint32_t mkfw_window_is_cursor_locked(struct mkfw_window *state) {
	return PLATFORM(state)->cursor_locked;
}

// [=]===^=[ mkfw_window_get_native_handles ]=====================================================[=]
MKFW_API void mkfw_window_get_native_handles(struct mkfw_window *state, struct mkfw_native_handles *out) {
	out->display    = (void *)PLATFORM(state)->display;
	out->window     = (uintptr_t)PLATFORM(state)->window;
	out->gl_context = (void *)PLATFORM(state)->glctx;
}

// [=]===^=[ mkfw_window_set_fullscreen ]====================================================================[=]
MKFW_API void mkfw_window_set_fullscreen(struct mkfw_window *state, int32_t enable) {
	if(enable && !state->is_fullscreen) {
		// Save the current geometry
		XWindowAttributes attrs;
		Window dummy_child;
		int root_x, root_y;

		XGetWindowAttributes(PLATFORM(state)->display, PLATFORM(state)->window, &attrs);
		PLATFORM(state)->win_saved_width = attrs.width;
		PLATFORM(state)->win_saved_height = attrs.height;
		XTranslateCoordinates(PLATFORM(state)->display, PLATFORM(state)->window, DefaultRootWindow(PLATFORM(state)->display), 0, 0, &root_x, &root_y, &dummy_child);
		PLATFORM(state)->win_saved_x = root_x;
		PLATFORM(state)->win_saved_y = root_y;

		// Go fullscreen using _NET_WM_STATE
		XEvent xev = {0};
		Atom wm_state = XInternAtom(PLATFORM(state)->display, "_NET_WM_STATE", False);
		Atom fullscreen = XInternAtom(PLATFORM(state)->display, "_NET_WM_STATE_FULLSCREEN", False);

		xev.type = ClientMessage;
		xev.xclient.window = PLATFORM(state)->window;
		xev.xclient.message_type = wm_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
		xev.xclient.data.l[1] = fullscreen;
		xev.xclient.data.l[2] = 0;

		XSendEvent(PLATFORM(state)->display, DefaultRootWindow(PLATFORM(state)->display), False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);

		state->is_fullscreen = 1;

	} else if(!enable && state->is_fullscreen) {
		// Restore from fullscreen
		XEvent xev = {0};
		Atom wm_state = XInternAtom(PLATFORM(state)->display, "_NET_WM_STATE", False);
		Atom fullscreen = XInternAtom(PLATFORM(state)->display, "_NET_WM_STATE_FULLSCREEN", False);

		xev.type = ClientMessage;
		xev.xclient.window = PLATFORM(state)->window;
		xev.xclient.message_type = wm_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = 0; // _NET_WM_STATE_REMOVE
		xev.xclient.data.l[1] = fullscreen;
		xev.xclient.data.l[2] = 0;

		XSendEvent(PLATFORM(state)->display, DefaultRootWindow(PLATFORM(state)->display), False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
		XMoveResizeWindow(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->win_saved_x, PLATFORM(state)->win_saved_y, PLATFORM(state)->win_saved_width, PLATFORM(state)->win_saved_height);
		state->is_fullscreen = 0;
	}

	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_window_enable_drop ]===================================================================[=]
MKFW_API void mkfw_window_enable_drop(struct mkfw_window *state, uint8_t enable) {
	if(enable) {
		Atom version = 5;
		XChangeProperty(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->xdnd_aware, XA_ATOM, 32, PropModeReplace, (uint8_t *)&version, 1);
	} else {
		XDeleteProperty(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->xdnd_aware);
	}
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ xdnd_percent_decode ]================================================================[=]
static uint32_t xdnd_percent_decode(const char *src, uint32_t src_len, char *dst) {
	uint32_t out = 0;
	for(uint32_t i = 0; i < src_len; ++i) {
		if(src[i] == '%' && i + 2 < src_len) {
			uint8_t hi = (uint8_t)src[i + 1];
			uint8_t lo = (uint8_t)src[i + 2];
			uint8_t val = 0;
			if(hi >= '0' && hi <= '9') {
				val = (hi - '0') << 4;
			} else if(hi >= 'A' && hi <= 'F') {
				val = (hi - 'A' + 10) << 4;
			} else if(hi >= 'a' && hi <= 'f') {
				val = (hi - 'a' + 10) << 4;
			}
			if(lo >= '0' && lo <= '9') {
				val |= (lo - '0');
			} else if(lo >= 'A' && lo <= 'F') {
				val |= (lo - 'A' + 10);
			} else if(lo >= 'a' && lo <= 'f') {
				val |= (lo - 'a' + 10);
			}
			dst[out++] = (char)val;
			i += 2;
		} else {
			dst[out++] = src[i];
		}
	}
	dst[out] = 0;
	return out;
}

// [=]===^=[ xdnd_parse_uri_list ]================================================================[=]
static void xdnd_parse_uri_list(struct mkfw_window *state, const char *data, uint32_t len) {
	uint32_t count = 0;
	uint32_t capacity = 8;
	char **paths = (char **)malloc(capacity * sizeof(char *));
	char *decoded = (char *)malloc(len + 1);

	const char *p = data;
	const char *end = data + len;
	while(p < end) {
		const char *line_end = p;
		while(line_end < end && *line_end != '\r' && *line_end != '\n') {
			++line_end;
		}
		uint32_t line_len = (uint32_t)(line_end - p);

		if(line_len > 0 && p[0] != '#') {
			const char *path_start = p;
			uint32_t path_len = line_len;
			if(line_len >= 7 && memcmp(p, "file://", 7) == 0) {
				path_start = p + 7;
				path_len = line_len - 7;
			}
			uint32_t decoded_len = xdnd_percent_decode(path_start, path_len, decoded);
			char *path = (char *)malloc(decoded_len + 1);
			memcpy(path, decoded, decoded_len + 1);
			if(count >= capacity) {
				capacity *= 2;
				paths = (char **)realloc(paths, capacity * sizeof(char *));
			}
			paths[count++] = path;
		}

		p = line_end;
		while(p < end && (*p == '\r' || *p == '\n')) {
			++p;
		}
	}

	free(decoded);

	if(count > 0 && state->drop_callback) {
		// Ownership of `paths` and its contents passes to the callback;
		// the consumer must release with mkfw_drop_paths_free or equivalent.
		state->drop_callback(count, (const char **)paths);
	} else {
		for(uint32_t i = 0; i < count; ++i) {
			free(paths[i]);
		}
		free(paths);
	}
}

// [=]===^=[ mkfw_window_is_minimized ]=================================================================[=]
MKFW_API uint32_t mkfw_window_is_minimized(struct mkfw_window *state) {
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *data = 0;
	XGetWindowProperty(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->wm_state, 0, 2, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &data);
	uint32_t result = 0;
	if(data && nitems >= 1) {
		long state_val = *(long *)data;
		result = (state_val == 3) ? 1 : 0;
	}
	if(data) {
		XFree(data);
	}
	return result;
}

// [=]===^=[ mkfw_window_is_maximized ]=================================================================[=]
MKFW_API uint32_t mkfw_window_is_maximized(struct mkfw_window *state) {
	Display *dpy = PLATFORM(state)->display;
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *data = 0;
	XGetWindowProperty(dpy, PLATFORM(state)->window, PLATFORM(state)->net_wm_state, 0, 1024, False, XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, &data);
	uint32_t result = 0;
	if(data) {
		Atom *atoms = (Atom *)data;
		uint8_t has_h = 0, has_v = 0;
		for(unsigned long i = 0; i < nitems; ++i) {
			if(atoms[i] == PLATFORM(state)->net_wm_state_maximized_horz) {
				has_h = 1;
			}
			if(atoms[i] == PLATFORM(state)->net_wm_state_maximized_vert) {
				has_v = 1;
			}
		}
		result = (has_h && has_v) ? 1 : 0;
		XFree(data);
	}
	return result;
}

// Forward declaration so process_window_event can be called from mkfw_poll_events
static void process_window_event(struct mkfw_window *state, XEvent *event_ptr);

// [=]===^=[ find_window_for_event ]==============================================================[=]
static struct mkfw_window *find_window_for_event(struct mkfw_context *ctx, XEvent *event) {
	for(uint32_t i = 0; i < ctx->window_count; ++i) {
		if(PLATFORM(ctx->windows[i])->window == event->xany.window) {
			return ctx->windows[i];
		}
	}
	// XInput2 generic events have window == 0; deliver them to the focused window
	if(event->type == GenericEvent) {
		for(uint32_t i = 0; i < ctx->window_count; ++i) {
			if(PLATFORM(ctx->windows[i])->in_window) {
				return ctx->windows[i];
			}
		}
		if(ctx->window_count > 0) {
			return ctx->windows[0];
		}
	}
	return 0;
}

// [=]===^=[ mkfw_poll_events ]===================================================================[=]
MKFW_API void mkfw_poll_events(struct mkfw_context *ctx) {
	if(!ctx || !CTX_PLATFORM(ctx)->display) {
		return;
	}
	XEvent event;
	while(XPending(CTX_PLATFORM(ctx)->display)) {
		XNextEvent(CTX_PLATFORM(ctx)->display, &event);
		struct mkfw_window *target = find_window_for_event(ctx, &event);
		if(target) {
			process_window_event(target, &event);
		}
	}
}

// [=]===^=[ process_window_event ]===============================================================[=]
static void process_window_event(struct mkfw_window *state, XEvent *event_ptr) {
	XEvent event = *event_ptr;
	{
		// Handle XInput2 generic events if relevant
		if(event.type == GenericEvent && PLATFORM(state)->in_window) {
			if(event.xcookie.extension == PLATFORM(state)->xi_opcode && XGetEventData(PLATFORM(state)->display, &event.xcookie)) {
				if(event.xcookie.evtype == XI_RawMotion) {
					XIRawEvent* re = (XIRawEvent*)event.xcookie.data;
					double dx = 0.0;
					double dy = 0.0;

					int idx = 0;
					int nValuators = re->valuators.mask_len * 8;
					for(int i = 0; i < nValuators; i++) {
						if(XIMaskIsSet(re->valuators.mask, i)) {
							double val = re->raw_values[idx++];

							if(i == 0) { dx = val; }
							if(i == 1) { dy = val; }
						}
					}

					if(PLATFORM(state)->cursor_locked) {
						if(dx * dx + dy * dy < 0.1) {
							dx = PLATFORM(state)->last_mouse_dx;
							dy = PLATFORM(state)->last_mouse_dy;
						}
					}

					PLATFORM(state)->last_mouse_dx = dx;
					PLATFORM(state)->last_mouse_dy = dy;

					// Accumulate for read-and-clear API with sensitivity scaling
					PLATFORM(state)->accumulated_dx += dx * PLATFORM(state)->mouse_sensitivity;
					PLATFORM(state)->accumulated_dy += dy * PLATFORM(state)->mouse_sensitivity;

					if(state->mouse_move_delta_callback) {
						state->mouse_move_delta_callback(state, (int)dx, (int)dy);
					}
				}
				XFreeEventData(PLATFORM(state)->display, &event.xcookie);
			}
		}

		// Normal X11 events
		switch(event.type) {

			case EnterNotify: {
				PLATFORM(state)->in_window = true;
				state->mouse_in_window = 1;
			} break;

			case LeaveNotify: {
				PLATFORM(state)->in_window = false;
				state->mouse_in_window = 0;
			} break;

			case FocusIn: {
				state->has_focus = 1;
				if(PLATFORM(state)->xic) {
					XSetICFocus(PLATFORM(state)->xic);
				}
				if(state->focus_callback) {
					state->focus_callback(state, 1);
				}
			} break;

			case FocusOut: {
				state->has_focus = 0;
				if(PLATFORM(state)->xic) {
					XUnsetICFocus(PLATFORM(state)->xic);
				}
				if(state->focus_callback) {
					state->focus_callback(state, 0);
				}
			} break;

			case KeyPress: {
				x11_update_scancode(state, event.xkey.keycode, 1);
				map_x11_keysym(state, XLookupKeysym(&event.xkey, 0), 1);

				if(state->char_callback) {
					char buf[64];
					KeySym keysym;
					Status xim_status;
					int32_t len = 0;
					if(PLATFORM(state)->xic) {
						len = Xutf8LookupString(PLATFORM(state)->xic, &event.xkey, buf, sizeof(buf) - 1, &keysym, &xim_status);
					} else {
						len = XLookupString(&event.xkey, buf, sizeof(buf) - 1, &keysym, 0);
					}
					if(len > 0) {
						buf[len] = 0;
						uint32_t ci = 0;
						while(ci < (uint32_t)len) {
							uint32_t codepoint = 0;
							uint8_t c = (uint8_t)buf[ci];
							if(c < 0x80) {
								codepoint = c;
								ci += 1;
							} else if((c & 0xE0) == 0xC0) {
								codepoint = (c & 0x1F) << 6;
								if(ci + 1 < (uint32_t)len) {
									codepoint |= ((uint8_t)buf[ci + 1] & 0x3F);
								}
								ci += 2;
							} else if((c & 0xF0) == 0xE0) {
								codepoint = (c & 0x0F) << 12;
								if(ci + 1 < (uint32_t)len) {
									codepoint |= ((uint8_t)buf[ci + 1] & 0x3F) << 6;
								}
								if(ci + 2 < (uint32_t)len) {
									codepoint |= ((uint8_t)buf[ci + 2] & 0x3F);
								}
								ci += 3;
							} else if((c & 0xF8) == 0xF0) {
								codepoint = (c & 0x07) << 18;
								if(ci + 1 < (uint32_t)len) {
									codepoint |= ((uint8_t)buf[ci + 1] & 0x3F) << 12;
								}
								if(ci + 2 < (uint32_t)len) {
									codepoint |= ((uint8_t)buf[ci + 2] & 0x3F) << 6;
								}
								if(ci + 3 < (uint32_t)len) {
									codepoint |= ((uint8_t)buf[ci + 3] & 0x3F);
								}
								ci += 4;
							} else {
								ci += 1;
								continue;
							}
							if(codepoint == 8 || codepoint >= 32) {
								state->char_callback(state, codepoint);
							}
						}
					}
				}
			} break;

			case KeyRelease: {
				x11_update_scancode(state, event.xkey.keycode, 0);
				map_x11_keysym(state, XLookupKeysym(&event.xkey, 0), 0);
			} break;

			case SelectionRequest: {
				XSelectionRequestEvent *req = &event.xselectionrequest;
				XSelectionEvent reply;
				memset(&reply, 0, sizeof(reply));
				reply.type = SelectionNotify;
				reply.requestor = req->requestor;
				reply.selection = req->selection;
				reply.target = req->target;
				reply.time = req->time;
				reply.property = None;

				if(PLATFORM(state)->clipboard_text) {
					if(req->target == PLATFORM(state)->targets_atom) {
						Atom targets[] = { PLATFORM(state)->utf8_string_atom, XA_STRING };
						XChangeProperty(PLATFORM(state)->display, req->requestor, req->property, XA_ATOM, 32, PropModeReplace, (uint8_t *)targets, 2);
						reply.property = req->property;

					} else if(req->target == PLATFORM(state)->utf8_string_atom || req->target == XA_STRING) {
						XChangeProperty(PLATFORM(state)->display, req->requestor, req->property, req->target, 8, PropModeReplace, (uint8_t *)PLATFORM(state)->clipboard_text, strlen(PLATFORM(state)->clipboard_text));
						reply.property = req->property;
					}
				}

				XSendEvent(PLATFORM(state)->display, req->requestor, False, 0, (XEvent *)&reply);
			} break;

			case ButtonPress: {
				uint32_t xbtn = event.xbutton.button;

				if(xbtn == 4) {
					if(state->scroll_callback) {
						state->scroll_callback(state, 0.0, 1.0);
					}

				} else if(xbtn == 5) {
					if(state->scroll_callback) {
						state->scroll_callback(state, 0.0, -1.0);
					}

				} else if(xbtn == 6) {
					if(state->scroll_callback) {
						state->scroll_callback(state, -1.0, 0.0);
					}

				} else if(xbtn == 7) {
					if(state->scroll_callback) {
						state->scroll_callback(state, 1.0, 0.0);
					}

				} else {
					uint8_t mapped = 0;
					if(xbtn == 1) {
						mapped = MKFW_MOUSE_LEFT;
					} else if(xbtn == 2) {
						mapped = MKFW_MOUSE_MIDDLE;
					} else if(xbtn == 3) {
						mapped = MKFW_MOUSE_RIGHT;
					} else if(xbtn == 8) {
						mapped = MKFW_MOUSE_EXTRA1;
					} else if(xbtn == 9) {
						mapped = MKFW_MOUSE_EXTRA2;
					} else {
						break;
					}
					state->mouse_buttons[mapped] = 1;
					if(state->mouse_button_callback) {
						state->mouse_button_callback(state, mapped, MKFW_PRESSED);
					}
				}
			} break;

			case ButtonRelease: {
				uint32_t xbtn = event.xbutton.button;
				uint8_t mapped = 0;
				if(xbtn == 1) {
					mapped = MKFW_MOUSE_LEFT;
				} else if(xbtn == 2) {
					mapped = MKFW_MOUSE_MIDDLE;
				} else if(xbtn == 3) {
					mapped = MKFW_MOUSE_RIGHT;
				} else if(xbtn == 8) {
					mapped = MKFW_MOUSE_EXTRA1;
				} else if(xbtn == 9) {
					mapped = MKFW_MOUSE_EXTRA2;
				} else {
					break;
				}
				state->mouse_buttons[mapped] = 0;
				if(state->mouse_button_callback) {
					state->mouse_button_callback(state, mapped, MKFW_RELEASED);
				}
			} break;

			case MotionNotify: {
				if(PLATFORM(state)->in_window) {
					PLATFORM(state)->last_mouse_x = event.xmotion.x;
					PLATFORM(state)->last_mouse_y = event.xmotion.y;

					// Update absolute mouse position
					state->mouse_x = event.xmotion.x;
					state->mouse_y = event.xmotion.y;

					if(PLATFORM(state)->cursor_locked && !PLATFORM(state)->cursor_visible) {
						XWindowAttributes attrs;
						XGetWindowAttributes(PLATFORM(state)->display, PLATFORM(state)->window, &attrs);
						int center_x = attrs.width / 2;
						int center_y = attrs.height / 2;
						if (event.xmotion.x != center_x || event.xmotion.y != center_y) { // Only re-center hidden+locked FPS cursors
							XWarpPointer(PLATFORM(state)->display, None, PLATFORM(state)->window, 0, 0, 0, 0, center_x, center_y);
							XSync(PLATFORM(state)->display, False);
							PLATFORM(state)->last_mouse_x = center_x;
							PLATFORM(state)->last_mouse_y = center_y;
						}
					}
				}
			} break;

			case ConfigureNotify: {
				int new_width  = event.xconfigure.width;
				int new_height = event.xconfigure.height;

				if(new_width == PLATFORM(state)->last_framebuffer_width && new_height == PLATFORM(state)->last_framebuffer_height) {
					break;
				}
				PLATFORM(state)->last_framebuffer_width  = new_width;
				PLATFORM(state)->last_framebuffer_height = new_height;

				if(state->framebuffer_callback) {
					state->framebuffer_callback(state, PLATFORM(state)->last_framebuffer_width, PLATFORM(state)->last_framebuffer_height, PLATFORM(state)->aspect_ratio);
				}
			} break;

			case ClientMessage: {
				Atom msg_type = event.xclient.message_type;

				if((Atom)event.xclient.data.l[0] == PLATFORM(state)->wm_delete_window) {
					PLATFORM(state)->should_close = 1;

				} else if(msg_type == PLATFORM(state)->xdnd_enter) {
					PLATFORM(state)->xdnd_source = (Window)event.xclient.data.l[0];
					PLATFORM(state)->xdnd_has_uri_list = 0;
					uint8_t more_than_three = (event.xclient.data.l[1] >> 0) & 1;
					if(more_than_three) {
						Atom actual_type;
						int actual_format;
						unsigned long nitems, bytes_after;
						unsigned char *type_data = 0;
						XGetWindowProperty(PLATFORM(state)->display, PLATFORM(state)->xdnd_source, PLATFORM(state)->xdnd_type_list, 0, 1024, False, XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, &type_data);
						if(type_data) {
							Atom *types = (Atom *)type_data;
							for(unsigned long ti = 0; ti < nitems; ++ti) {
								if(types[ti] == PLATFORM(state)->text_uri_list) {
									PLATFORM(state)->xdnd_has_uri_list = 1;
									break;
								}
							}
							XFree(type_data);
						}
					} else {
						for(uint32_t ti = 0; ti < 3; ++ti) {
							if((Atom)event.xclient.data.l[2 + ti] == PLATFORM(state)->text_uri_list) {
								PLATFORM(state)->xdnd_has_uri_list = 1;
								break;
							}
						}
					}

				} else if(msg_type == PLATFORM(state)->xdnd_position) {
					XEvent reply;
					memset(&reply, 0, sizeof(reply));
					reply.xclient.type = ClientMessage;
					reply.xclient.window = PLATFORM(state)->xdnd_source;
					reply.xclient.message_type = PLATFORM(state)->xdnd_status;
					reply.xclient.format = 32;
					reply.xclient.data.l[0] = (long)PLATFORM(state)->window;
					reply.xclient.data.l[1] = PLATFORM(state)->xdnd_has_uri_list ? 1 : 0;
					reply.xclient.data.l[2] = 0;
					reply.xclient.data.l[3] = 0;
					reply.xclient.data.l[4] = 0;
					XSendEvent(PLATFORM(state)->display, PLATFORM(state)->xdnd_source, False, NoEventMask, &reply);
					XFlush(PLATFORM(state)->display);

				} else if(msg_type == PLATFORM(state)->xdnd_leave) {
					PLATFORM(state)->xdnd_source = 0;
					PLATFORM(state)->xdnd_has_uri_list = 0;

				} else if(msg_type == PLATFORM(state)->xdnd_drop) {
					if(PLATFORM(state)->xdnd_has_uri_list) {
						XConvertSelection(PLATFORM(state)->display, PLATFORM(state)->xdnd_selection, PLATFORM(state)->text_uri_list, PLATFORM(state)->xdnd_selection, PLATFORM(state)->window, CurrentTime);
						XFlush(PLATFORM(state)->display);
					} else {
						XEvent reply;
						memset(&reply, 0, sizeof(reply));
						reply.xclient.type = ClientMessage;
						reply.xclient.window = PLATFORM(state)->xdnd_source;
						reply.xclient.message_type = PLATFORM(state)->xdnd_finished;
						reply.xclient.format = 32;
						reply.xclient.data.l[0] = (long)PLATFORM(state)->window;
						reply.xclient.data.l[1] = 0;
						reply.xclient.data.l[2] = 0;
						XSendEvent(PLATFORM(state)->display, PLATFORM(state)->xdnd_source, False, NoEventMask, &reply);
						XFlush(PLATFORM(state)->display);
						PLATFORM(state)->xdnd_source = 0;
					}
				}
			} break;

			case SelectionNotify: {
				if(event.xselection.selection == PLATFORM(state)->xdnd_selection) {
					Atom actual_type;
					int actual_format;
					unsigned long nitems, bytes_after;
					unsigned char *data = 0;
					XGetWindowProperty(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->xdnd_selection, 0, 1024 * 1024, True, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &data);
					if(data) {
						xdnd_parse_uri_list(state, (const char *)data, (uint32_t)(nitems * (actual_format / 8)));
						XFree(data);
					}

					XEvent reply;
					memset(&reply, 0, sizeof(reply));
					reply.xclient.type = ClientMessage;
					reply.xclient.window = PLATFORM(state)->xdnd_source;
					reply.xclient.message_type = PLATFORM(state)->xdnd_finished;
					reply.xclient.format = 32;
					reply.xclient.data.l[0] = (long)PLATFORM(state)->window;
					reply.xclient.data.l[1] = 1;
					reply.xclient.data.l[2] = 0;
					XSendEvent(PLATFORM(state)->display, PLATFORM(state)->xdnd_source, False, NoEventMask, &reply);
					XFlush(PLATFORM(state)->display);
					PLATFORM(state)->xdnd_source = 0;
					PLATFORM(state)->xdnd_has_uri_list = 0;
				}
			} break;

			case PropertyNotify: {
				if(!state->window_state_callback) {
					break;
				}
				Atom prop = event.xproperty.atom;
				if(prop == PLATFORM(state)->net_wm_state || prop == PLATFORM(state)->wm_state) {
					uint8_t maximized = (uint8_t)mkfw_window_is_maximized(state);
					uint8_t minimized = (uint8_t)mkfw_window_is_minimized(state);
					if(maximized != PLATFORM(state)->last_maximized || minimized != PLATFORM(state)->last_minimized) {
						PLATFORM(state)->last_maximized = maximized;
						PLATFORM(state)->last_minimized = minimized;
						state->window_state_callback(state, maximized, minimized);
					}
				}
			} break;
		}
	}
}

// [=]===^=[ mkfw_window_swap_buffers ]==================================================================[=]
MKFW_API void mkfw_window_swap_buffers(struct mkfw_window *state) {
	glXSwapBuffers(PLATFORM(state)->display, PLATFORM(state)->window);
}

// [=]===^=[ x11_apply_size_hints ]===============================================================[=]
static void x11_apply_size_hints(struct mkfw_window *state) {
	XSizeHints *hints = XAllocSizeHints();
	if(!hints) {
		mkfw_error("failed to allocate XSizeHints");
		return;
	}

	if(PLATFORM(state)->min_width > 0 || PLATFORM(state)->min_height > 0) {
		hints->flags |= PMinSize;
		hints->min_width  = PLATFORM(state)->min_width;
		hints->min_height = PLATFORM(state)->min_height;
	}
	if(PLATFORM(state)->max_width > 0 || PLATFORM(state)->max_height > 0) {
		hints->flags |= PMaxSize;
		hints->max_width  = PLATFORM(state)->max_width  > 0 ? PLATFORM(state)->max_width  : 0x7fffffff;
		hints->max_height = PLATFORM(state)->max_height > 0 ? PLATFORM(state)->max_height : 0x7fffffff;
	}
	if(PLATFORM(state)->aspect_num > 0 && PLATFORM(state)->aspect_den > 0) {
		hints->flags |= PAspect;
		hints->min_aspect.x = PLATFORM(state)->aspect_num;
		hints->min_aspect.y = PLATFORM(state)->aspect_den;
		hints->max_aspect.x = PLATFORM(state)->aspect_num;
		hints->max_aspect.y = PLATFORM(state)->aspect_den;
	}

	XSetWMNormalHints(PLATFORM(state)->display, PLATFORM(state)->window, hints);
	XFree(hints);
}

// [=]===^=[ mkfw_window_set_size_limits ]========================================================[=]
MKFW_API void mkfw_window_set_size_limits(struct mkfw_window *state, int32_t min_width, int32_t min_height, int32_t max_width, int32_t max_height) {
	PLATFORM(state)->min_width  = min_width;
	PLATFORM(state)->min_height = min_height;
	PLATFORM(state)->max_width  = max_width;
	PLATFORM(state)->max_height = max_height;
	x11_apply_size_hints(state);
}

// [=]===^=[ mkfw_window_set_aspect_ratio ]=======================================================[=]
MKFW_API void mkfw_window_set_aspect_ratio(struct mkfw_window *state, int32_t num, int32_t den) {
	PLATFORM(state)->aspect_num = num;
	PLATFORM(state)->aspect_den = den;
	PLATFORM(state)->aspect_ratio = (num > 0 && den > 0) ? ((float)num / (float)den) : 0.0f;
	x11_apply_size_hints(state);
}

// [=]===^=[ mkfw_window_set_title ]==============================================================[=]
MKFW_API void mkfw_window_set_title(struct mkfw_window *state, const char *title) {
	if(!title) {
		return;
	}
	XStoreName(PLATFORM(state)->display, PLATFORM(state)->window, title);

	// Set _NET_WM_NAME for modern window managers
	Atom net_wm_name = XInternAtom(PLATFORM(state)->display, "_NET_WM_NAME", 0);
	Atom utf8_string = XInternAtom(PLATFORM(state)->display, "UTF8_STRING", 0);

	if(net_wm_name && utf8_string) {
		XChangeProperty(PLATFORM(state)->display, PLATFORM(state)->window, net_wm_name, utf8_string, 8, PropModeReplace, (uint8_t *)title, strlen(title));
	}

	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_window_set_resizable ]==========================================================[=]
MKFW_API void mkfw_window_set_resizable(struct mkfw_window *state, int32_t resizable) {
	if(resizable) {
		// Restore caller-supplied size/aspect constraints, if any.
		x11_apply_size_hints(state);
		XFlush(PLATFORM(state)->display);
		return;
	}

	// Non-resizable: pin to the current size by setting min == max.
	XSizeHints *hints = XAllocSizeHints();
	if(!hints) {
		mkfw_error("failed to allocate XSizeHints");
		return;
	}

	XWindowAttributes attrs;
	XGetWindowAttributes(PLATFORM(state)->display, PLATFORM(state)->window, &attrs);

	hints->flags = PMinSize | PMaxSize;
	hints->min_width  = attrs.width;
	hints->min_height = attrs.height;
	hints->max_width  = attrs.width;
	hints->max_height = attrs.height;

	XSetWMNormalHints(PLATFORM(state)->display, PLATFORM(state)->window, hints);
	XFree(hints);
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_window_set_decorated ]==========================================================[=]
MKFW_API void mkfw_window_set_decorated(struct mkfw_window *state, int32_t decorated) {
	struct motif_hints {
		unsigned long flags;
		unsigned long functions;
		unsigned long decorations;
		long input_mode;
		unsigned long status;
	};
	Atom motif = XInternAtom(PLATFORM(state)->display, "_MOTIF_WM_HINTS", False);
	struct motif_hints hints = {0};
	hints.flags = 2;
	hints.decorations = decorated ? 1 : 0;
	XChangeProperty(PLATFORM(state)->display, PLATFORM(state)->window, motif, motif, 32, PropModeReplace, (unsigned char *)&hints, 5);
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_window_set_opacity ]============================================================[=]
MKFW_API void mkfw_window_set_opacity(struct mkfw_window *state, float opacity) {
	Display *dpy = PLATFORM(state)->display;
	Atom net_wm_opacity = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
	if(opacity >= 1.0f) {
		XDeleteProperty(dpy, PLATFORM(state)->window, net_wm_opacity);
	} else {
		uint32_t val = (uint32_t)((double)opacity * (double)0xffffffff);
		XChangeProperty(dpy, PLATFORM(state)->window, net_wm_opacity, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&val, 1);
	}
	XFlush(dpy);
}

// [=]===^=[ mkfw_window_set_size ]===============================================================[=]
MKFW_API void mkfw_window_set_size(struct mkfw_window *state, int32_t width, int32_t height) {
	XResizeWindow(PLATFORM(state)->display, PLATFORM(state)->window, width, height);
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_window_get_framebuffer_size ]==========================================================[=]
MKFW_API void mkfw_window_get_framebuffer_size(struct mkfw_window *state, int32_t *width, int32_t *height) {
	Window root;
	int x, y;
	unsigned int w, h, border, depth;
	XGetGeometry(PLATFORM(state)->display, PLATFORM(state)->window, &root, &x, &y, &w, &h, &border, &depth);
	if(width) {
		*width = (int32_t)w;
	}
	if(height) {
		*height = (int32_t)h;
	}
}

// [=]===^=[ mkfw_window_set_swap_interval ]==============================================================[=]
MKFW_API void mkfw_window_set_swap_interval(struct mkfw_window *state, uint32_t interval) {
	typedef int (*PFNGLXSWAPINTERVALEXTPROC)(Display*, GLXDrawable, int);
	PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress((const unsigned char *)"glXSwapIntervalEXT");

	if(glXSwapIntervalEXT) {
		glXSwapIntervalEXT(PLATFORM(state)->display, glXGetCurrentDrawable(), interval);
	}
}

// [=]===^=[ mkfw_window_get_swap_interval ]==============================================================[=]
MKFW_API int32_t mkfw_window_get_swap_interval(struct mkfw_window *state) {
	typedef void (*PFNGLXQUERYDRAWABLEPROC)(Display *, GLXDrawable, int, unsigned int *);
	PFNGLXQUERYDRAWABLEPROC pglXQueryDrawable = (PFNGLXQUERYDRAWABLEPROC)glXGetProcAddress((const unsigned char *)"glXQueryDrawable");
	if(pglXQueryDrawable) {
		unsigned int interval = 0;
		pglXQueryDrawable(PLATFORM(state)->display, PLATFORM(state)->window, 0x20f1, &interval);
		return (int32_t)interval;
	}
	return 0;
}

// [=]===^=[ mkfw_get_time ]=======================================================================[=]
MKFW_API uint64_t mkfw_get_time(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// [=]===^=[ mkfw_window_set_icon ]===============================================================[=]
MKFW_API void mkfw_window_set_icon(struct mkfw_window *state, int32_t width, int32_t height, const uint8_t *rgba) {
	if(!rgba) {
		return;
	}
	// _NET_WM_ICON format: [width, height, argb_pixels...]
	// Each pixel is a uint32_t in 0xAARRGGBB format (native long)
	size_t pixel_count = (size_t)width * (size_t)height;
	size_t data_len = 2 + pixel_count;
	unsigned long *data = (unsigned long *)malloc(data_len * sizeof(unsigned long));
	data[0] = width;
	data[1] = height;
	for(size_t i = 0; i < pixel_count; ++i) {
		uint32_t r = rgba[i * 4 + 0];
		uint32_t g = rgba[i * 4 + 1];
		uint32_t b = rgba[i * 4 + 2];
		uint32_t a = rgba[i * 4 + 3];
		data[2 + i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
	Atom net_wm_icon = XInternAtom(PLATFORM(state)->display, "_NET_WM_ICON", False);
	XChangeProperty(PLATFORM(state)->display, PLATFORM(state)->window, net_wm_icon, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, data_len);
	XFlush(PLATFORM(state)->display);
	free(data);
}

// [=]===^=[ mkfw_query_monitors_into ]===========================================================[=]
// Queries XRandR and writes into the caller-supplied buffer. Used internally
// at mkfw_init to populate the context's monitor cache.
static int32_t mkfw_query_monitors_into(Display *dpy, struct mkfw_monitor *out, int32_t max) {
	if(!out || max <= 0) {
		return 0;
	}
	Window root = DefaultRootWindow(dpy);
	XRRScreenResources *sr = XRRGetScreenResourcesCurrent(dpy, root);
	if(!sr) {
		mkfw_error("monitor query: XRRGetScreenResourcesCurrent returned 0");
		return 0;
	}

	RROutput primary_output = XRRGetOutputPrimary(dpy, root);

	int32_t count = 0;
	for(int i = 0; i < sr->ncrtc && count < max; ++i) {
		XRRCrtcInfo *ci = XRRGetCrtcInfo(dpy, sr, sr->crtcs[i]);
		if(!ci || ci->mode == None || ci->noutput == 0) {
			if(ci) {
				XRRFreeCrtcInfo(ci);
			}
			continue;
		}
		struct mkfw_monitor *m = &out[count];
		memset(m, 0, sizeof(*m));
		m->x = ci->x;
		m->y = ci->y;
		m->width = ci->width;
		m->height = ci->height;
		m->primary = 0;

		// Check if any output on this crtc is the primary
		for(int k = 0; k < ci->noutput; ++k) {
			if(ci->outputs[k] == primary_output) {
				m->primary = 1;
				break;
			}
		}

		// Get output name
		XRROutputInfo *oi = XRRGetOutputInfo(dpy, sr, ci->outputs[0]);
		if(oi) {
			snprintf(m->name, sizeof(m->name), "%s", oi->name);
			XRRFreeOutputInfo(oi);
		}

		// Get refresh rate from mode info
		for(int j = 0; j < sr->nmode; ++j) {
			if(sr->modes[j].id == ci->mode) {
				XRRModeInfo *mi = &sr->modes[j];
				if(mi->hTotal && mi->vTotal) {
					m->refresh_rate = (int32_t)((double)mi->dotClock / ((double)mi->hTotal * (double)mi->vTotal) + 0.5);
				}
				break;
			}
		}

		++count;
		XRRFreeCrtcInfo(ci);
	}
	XRRFreeScreenResources(sr);

	for(int32_t i = 1; i < count; ++i) {
		if(out[i].primary) {
			struct mkfw_monitor tmp = out[0];
			out[0] = out[i];
			out[i] = tmp;
			break;
		}
	}

	return count;
}

// [=]===^=[ mkfw_get_monitors ]==================================================================[=]
MKFW_API int32_t mkfw_get_monitors(struct mkfw_context *ctx, struct mkfw_monitor *out, int32_t max) {
	if(!ctx || !out || max <= 0) {
		return 0;
	}
	int32_t n = (int32_t)ctx->monitor_count;
	if(n > max) {
		n = max;
	}
	for(int32_t i = 0; i < n; ++i) {
		out[i] = ctx->monitors[i];
	}
	return n;
}

// [=]===^=[ mkfw_window_set_position ]===========================================================[=]
MKFW_API void mkfw_window_set_position(struct mkfw_window *state, int32_t x, int32_t y) {
	XMoveWindow(PLATFORM(state)->display, PLATFORM(state)->window, x, y);
}

// [=]===^=[ mkfw_window_get_position ]===========================================================[=]
MKFW_API void mkfw_window_get_position(struct mkfw_window *state, int32_t *x, int32_t *y) {
	Window child;
	int32_t tx, ty;
	XTranslateCoordinates(PLATFORM(state)->display, PLATFORM(state)->window, DefaultRootWindow(PLATFORM(state)->display), 0, 0, &tx, &ty, &child);
	if(x) {
		*x = tx;
	}
	if(y) {
		*y = ty;
	}
}

// [=]===^=[ mkfw_window_maximize ]===============================================================[=]
MKFW_API void mkfw_window_maximize(struct mkfw_window *state) {
	Display *dpy = PLATFORM(state)->display;
	XEvent ev = {0};
	ev.xclient.type = ClientMessage;
	ev.xclient.window = PLATFORM(state)->window;
	ev.xclient.message_type = PLATFORM(state)->net_wm_state;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
	ev.xclient.data.l[1] = PLATFORM(state)->net_wm_state_maximized_horz;
	ev.xclient.data.l[2] = PLATFORM(state)->net_wm_state_maximized_vert;
	ev.xclient.data.l[3] = 1; // source: application
	XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
	XFlush(dpy);
}

// [=]===^=[ mkfw_window_minimize ]===============================================================[=]
MKFW_API void mkfw_window_minimize(struct mkfw_window *state) {
	XIconifyWindow(PLATFORM(state)->display, PLATFORM(state)->window, DefaultScreen(PLATFORM(state)->display));
}

// [=]===^=[ mkfw_window_restore ]================================================================[=]
MKFW_API void mkfw_window_restore(struct mkfw_window *state) {
	Display *dpy = PLATFORM(state)->display;
	XEvent ev = {0};
	ev.xclient.type = ClientMessage;
	ev.xclient.window = PLATFORM(state)->window;
	ev.xclient.message_type = PLATFORM(state)->net_wm_state;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 0; // _NET_WM_STATE_REMOVE
	ev.xclient.data.l[1] = PLATFORM(state)->net_wm_state_maximized_horz;
	ev.xclient.data.l[2] = PLATFORM(state)->net_wm_state_maximized_vert;
	ev.xclient.data.l[3] = 1;
	XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
	XMapWindow(dpy, PLATFORM(state)->window);
	XFlush(dpy);
}

// [=]===^=[ mkfw_window_get_content_scale ]=============================================================[=]
MKFW_API float mkfw_window_get_content_scale(struct mkfw_window *state) {
	Display *dpy = PLATFORM(state)->display;
	char *rms = XResourceManagerString(dpy);
	if(rms) {
		XrmDatabase db = XrmGetStringDatabase(rms);
		if(db) {
			char *type = 0;
			XrmValue value;
			if(XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)) {
				if(type && strcmp(type, "String") == 0) {
					float dpi = (float)atof(value.addr);
					XrmDestroyDatabase(db);
					if(dpi > 0.0f) {
						return dpi / 96.0f;
					}
				}
			}
			XrmDestroyDatabase(db);
		}
	}
	int screen = DefaultScreen(dpy);
	float width_px = (float)DisplayWidth(dpy, screen);
	float width_mm = (float)DisplayWidthMM(dpy, screen);
	if(width_mm > 0.0f) {
		float dpi = width_px / (width_mm / 25.4f);
		return dpi / 96.0f;
	}
	return 1.0f;
}

// [=]===^=[ mkfw_window_request_attention ]=============================================================[=]
MKFW_API void mkfw_window_request_attention(struct mkfw_window *state) {
	Display *dpy = PLATFORM(state)->display;
	XEvent ev = {0};
	ev.xclient.type = ClientMessage;
	ev.xclient.window = PLATFORM(state)->window;
	ev.xclient.message_type = PLATFORM(state)->net_wm_state;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
	ev.xclient.data.l[1] = PLATFORM(state)->net_wm_state_demands_attention;
	ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = 1; // source: application
	XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
	XFlush(dpy);
}

// [=]===^=[ mkfw_wait_events ]==================================================================[=]
MKFW_API void mkfw_wait_events(struct mkfw_context *ctx) {
	if(!ctx || !CTX_PLATFORM(ctx)->display) {
		return;
	}
	Display *dpy = CTX_PLATFORM(ctx)->display;
	if(!XPending(dpy)) {
		struct pollfd pfd;
		pfd.fd = ConnectionNumber(dpy);
		pfd.events = POLLIN;
		poll(&pfd, 1, -1);
	}
	mkfw_poll_events(ctx);
}

// [=]===^=[ mkfw_wait_events_timeout ]===========================================================[=]
MKFW_API void mkfw_wait_events_timeout(struct mkfw_context *ctx, uint64_t nanoseconds) {
	if(!ctx || !CTX_PLATFORM(ctx)->display) {
		return;
	}
	Display *dpy = CTX_PLATFORM(ctx)->display;
	if(!XPending(dpy)) {
		struct pollfd pfd;
		pfd.fd = ConnectionNumber(dpy);
		pfd.events = POLLIN;
		poll(&pfd, 1, (int)(nanoseconds / 1000000));
	}
	mkfw_poll_events(ctx);
}

// [=]===^=[ mkfw_window_destroy ]================================================================[=]
MKFW_API void mkfw_window_destroy(struct mkfw_window *state) {
	if(!state) {
		return;
	}

	if(PLATFORM(state)->cursor_locked) {
		XUngrabPointer(PLATFORM(state)->display, CurrentTime);
	}
	if(PLATFORM(state)->hidden_cursor) {
		XFreeCursor(PLATFORM(state)->display, PLATFORM(state)->hidden_cursor);
	}

	if(PLATFORM(state)->xic) {
		XDestroyIC(PLATFORM(state)->xic);
	}
	if(PLATFORM(state)->xim) {
		XCloseIM(PLATFORM(state)->xim);
	}
	for(uint32_t i = 0; i < MKFW_CURSOR_LAST; ++i) {
		if(PLATFORM(state)->cursors[i]) {
			XFreeCursor(PLATFORM(state)->display, PLATFORM(state)->cursors[i]);
		}
	}
	free(PLATFORM(state)->clipboard_text);

	if(PLATFORM(state)->glctx) {
		glXMakeCurrent(PLATFORM(state)->display, None, 0);
		glXDestroyContext(PLATFORM(state)->display, PLATFORM(state)->glctx);
	}
	XDestroyWindow(PLATFORM(state)->display, PLATFORM(state)->window);

	// Unlink from its context's window list
	struct mkfw_context *ctx = state->context;
	if(ctx) {
		for(uint32_t i = 0; i < ctx->window_count; ++i) {
			if(ctx->windows[i] == state) {
				for(uint32_t j = i + 1; j < ctx->window_count; ++j) {
					ctx->windows[j - 1] = ctx->windows[j];
				}
				ctx->windows[ctx->window_count - 1] = 0;
				--ctx->window_count;
				break;
			}
		}
	}

	free(state->platform);
	free(state);
}

// [=]===^=[ mkfw_shutdown ]======================================================================[=]
MKFW_API void mkfw_shutdown(struct mkfw_context *ctx) {
	if(!ctx) {
		return;
	}

	// Destroy any remaining windows
	while(ctx->window_count > 0) {
		mkfw_window_destroy(ctx->windows[ctx->window_count - 1]);
	}

	if(CTX_PLATFORM(ctx)->display) {
		XCloseDisplay(CTX_PLATFORM(ctx)->display);
	}
	free(ctx->platform);
	free(ctx);
}

// [=]===^=[ mkfw_sleep ]=========================================================================[=]
MKFW_API void mkfw_sleep(uint64_t nanoseconds) {
	struct timespec ts;
	ts.tv_sec = nanoseconds / 1000000000;
	ts.tv_nsec = nanoseconds % 1000000000;
	nanosleep(&ts, 0);
}

// [=]===^=[ mkfw_window_set_mouse_sensitivity ]=========================================================[=]
MKFW_API void mkfw_window_set_mouse_sensitivity(struct mkfw_window *state, double sensitivity) {
	PLATFORM(state)->mouse_sensitivity = sensitivity;
}

// [=]===^=[ mkfw_window_get_and_clear_mouse_delta ]=====================================================[=]
MKFW_API void mkfw_window_get_and_clear_mouse_delta(struct mkfw_window *state, int32_t *dx, int32_t *dy) {
	int32_t tdx = (int32_t)PLATFORM(state)->accumulated_dx;
	int32_t tdy = (int32_t)PLATFORM(state)->accumulated_dy;
	PLATFORM(state)->accumulated_dx -= (double)tdx;
	PLATFORM(state)->accumulated_dy -= (double)tdy;
	if(dx) {
		*dx = tdx;
	}
	if(dy) {
		*dy = tdy;
	}
}

// [=]===^=[ mkfw_window_set_cursor_shape ]=============================================================[=]
MKFW_API void mkfw_window_set_cursor_shape(struct mkfw_window *state, uint32_t cursor) {
	if(cursor >= MKFW_CURSOR_LAST) {
		cursor = MKFW_CURSOR_ARROW;
	}
	PLATFORM(state)->current_cursor = cursor;
	PLATFORM(state)->active_custom_cursor = 0;
	if(PLATFORM(state)->cursor_visible) {
		XDefineCursor(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->cursors[cursor]);
		XFlush(PLATFORM(state)->display);
	}
}

// [=]===^=[ mkfw_cursor_create_rgba ]============================================================[=]
MKFW_API struct mkfw_cursor *mkfw_cursor_create_rgba(struct mkfw_context *ctx, uint32_t width, uint32_t height, uint8_t *rgba, int32_t hotspot_x, int32_t hotspot_y) {
	if(!mkfw_XcursorImageCreate) {
		mkfw_error("libXcursor not loaded; custom cursors unavailable");
		return 0;
	}
	mkfw_xcursor_image *img = mkfw_XcursorImageCreate((int)width, (int)height);
	if(!img) {
		return 0;
	}
	img->xhot = (unsigned int)hotspot_x;
	img->yhot = (unsigned int)hotspot_y;

	// libXcursor pixels are 0xAARRGGBB premultiplied alpha.
	for(uint32_t i = 0; i < width * height; ++i) {
		uint32_t r = rgba[i * 4 + 0];
		uint32_t g = rgba[i * 4 + 1];
		uint32_t b = rgba[i * 4 + 2];
		uint32_t a = rgba[i * 4 + 3];
		uint32_t pr = (r * a) / 255;
		uint32_t pg = (g * a) / 255;
		uint32_t pb = (b * a) / 255;
		img->pixels[i] = (a << 24) | (pr << 16) | (pg << 8) | pb;
	}

	Cursor xc = mkfw_XcursorImageLoadCursor(CTX_PLATFORM(ctx)->display, img);
	mkfw_XcursorImageDestroy(img);
	if(!xc) {
		return 0;
	}

	struct mkfw_cursor *c = (struct mkfw_cursor *)malloc(sizeof(struct mkfw_cursor));
	if(!c) {
		XFreeCursor(CTX_PLATFORM(ctx)->display, xc);
		return 0;
	}
	c->x_cursor = xc;
	return c;
}

// [=]===^=[ mkfw_cursor_destroy ]================================================================[=]
MKFW_API void mkfw_cursor_destroy(struct mkfw_context *ctx, struct mkfw_cursor *cursor) {
	if(!cursor) {
		return;
	}
	XFreeCursor(CTX_PLATFORM(ctx)->display, cursor->x_cursor);
	free(cursor);
}

// [=]===^=[ mkfw_window_set_custom_cursor ]======================================================[=]
MKFW_API void mkfw_window_set_custom_cursor(struct mkfw_window *state, struct mkfw_cursor *cursor) {
	PLATFORM(state)->active_custom_cursor = cursor ? cursor->x_cursor : 0;
	if(PLATFORM(state)->cursor_visible) {
		XDefineCursor(PLATFORM(state)->display, PLATFORM(state)->window, x11_active_cursor(state));
		XFlush(PLATFORM(state)->display);
	}
}

// [=]===^=[ mkfw_window_get_cursor_position ]===========================================================[=]
MKFW_API void mkfw_window_get_cursor_position(struct mkfw_window *state, int32_t *x, int32_t *y) {
	Window root_ret, child_ret;
	int root_x, root_y, win_x, win_y;
	unsigned int mask;
	XQueryPointer(PLATFORM(state)->display, PLATFORM(state)->window, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask);
	if(x) {
		*x = win_x;
	}
	if(y) {
		*y = win_y;
	}
}

// [=]===^=[ mkfw_window_set_cursor_position ]===========================================================[=]
MKFW_API void mkfw_window_set_cursor_position(struct mkfw_window *state, int32_t x, int32_t y) {
	XWarpPointer(PLATFORM(state)->display, None, PLATFORM(state)->window, 0, 0, 0, 0, x, y);
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_window_set_clipboard_text ]===========================================================[=]
MKFW_API void mkfw_window_set_clipboard_text(struct mkfw_window *state, const char *text) {
	free(PLATFORM(state)->clipboard_text);
	PLATFORM(state)->clipboard_text = 0;
	if(text) {
		size_t len = strlen(text);
		PLATFORM(state)->clipboard_text = (char *)malloc(len + 1);
		memcpy(PLATFORM(state)->clipboard_text, text, len + 1);
	}
	XSetSelectionOwner(PLATFORM(state)->display, PLATFORM(state)->clipboard_atom, PLATFORM(state)->window, CurrentTime);
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_window_get_clipboard_text ]===========================================================[=]
// Returns a malloc'd UTF-8 string the caller must release with free(),
// or 0 if the clipboard is empty / unavailable.
MKFW_API char *mkfw_window_get_clipboard_text(struct mkfw_window *state) {
	Window owner = XGetSelectionOwner(PLATFORM(state)->display, PLATFORM(state)->clipboard_atom);
	if(owner == None) {
		return 0;
	}

	if(owner == PLATFORM(state)->window) {
		if(!PLATFORM(state)->clipboard_text) {
			return 0;
		}
		size_t len = strlen(PLATFORM(state)->clipboard_text);
		char *copy = (char *)malloc(len + 1);
		memcpy(copy, PLATFORM(state)->clipboard_text, len + 1);
		return copy;
	}

	XConvertSelection(PLATFORM(state)->display, PLATFORM(state)->clipboard_atom, PLATFORM(state)->utf8_string_atom, PLATFORM(state)->mkfw_clipboard_atom, PLATFORM(state)->window, CurrentTime);
	XFlush(PLATFORM(state)->display);

	XEvent ev;
	for(uint32_t i = 0; i < 50; ++i) {
		if(XCheckTypedWindowEvent(PLATFORM(state)->display, PLATFORM(state)->window, SelectionNotify, &ev)) {
			if(ev.xselection.property == None) {
				return 0;
			}

			Atom actual_type;
			int actual_format;
			unsigned long nitems, bytes_after;
			unsigned char *data = 0;

			XGetWindowProperty(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->mkfw_clipboard_atom, 0, 1024 * 1024, True, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &data);

			if(data) {
				size_t len = nitems * (actual_format / 8);
				char *copy = (char *)malloc(len + 1);
				memcpy(copy, data, len);
				copy[len] = 0;
				XFree(data);
				return copy;
			}

			return 0;
		}
		struct timespec ts = { 0, 10000000 };
		nanosleep(&ts, 0);
	}

	return 0;
}

