// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mkfw_glx_mini.h"

/* Platform casting macro */
#define PLATFORM(state) ((struct x11_mkfw_state *)(state)->platform)

struct x11_mkfw_state {
	Display *display;
	Window   window;
	GLXContext glctx;
	float aspect_ratio;
	uint8_t mouse_constrained;
	int32_t last_mouse_x;
	int32_t last_mouse_y;
	int32_t win_saved_width;
	int32_t win_saved_height;
	int32_t win_saved_x;
	int32_t win_saved_y;
	int32_t hide_mouse_x;
	int32_t hide_mouse_y;
	int32_t min_width;
	int32_t min_height;
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

	// Clipboard
	Atom clipboard_atom;
	Atom utf8_string_atom;
	Atom targets_atom;
	Atom mkfw_clipboard_atom;
	char *clipboard_text;
};

static uint32_t map_x11_keysym(struct mkfw_state *state, KeySym keysym, int key_down) {
	uint32_t keycode = 0;

	// Handle number keys (XK_0 - XK_9)
	if(keysym >= XK_0 && keysym <= XK_9) {
		keycode = MKS_KEY_0 + (keysym - XK_0);
	} else if(keysym >= 0x20 && keysym <= 0x7E) {
		keycode = keysym;
	}

	// Track modifier state
	switch(keysym) {
		case XK_Shift_L:
			state->keyboard_state[MKS_KEY_LSHIFT] = key_down;
			break;
		case XK_Shift_R:
			state->keyboard_state[MKS_KEY_RSHIFT] = key_down;
			break;
		case XK_Control_L:
			state->keyboard_state[MKS_KEY_LCTRL] = key_down;
			break;
		case XK_Control_R:
			state->keyboard_state[MKS_KEY_RCTRL] = key_down;
			break;
		case XK_Alt_L:
			state->keyboard_state[MKS_KEY_LALT] = key_down;
			break;
		case XK_Alt_R:
			state->keyboard_state[MKS_KEY_RALT] = key_down;
			break;
		case XK_Super_L:
			state->keyboard_state[MKS_KEY_LSUPER] = key_down;
			break;
		case XK_Super_R:
			state->keyboard_state[MKS_KEY_RSUPER] = key_down;
			break;
	}

	// Update combined modifier states from keyboard_state
	state->keyboard_state[MKS_KEY_SHIFT] = state->keyboard_state[MKS_KEY_LSHIFT] || state->keyboard_state[MKS_KEY_RSHIFT];
	state->keyboard_state[MKS_KEY_CTRL]  = state->keyboard_state[MKS_KEY_LCTRL] || state->keyboard_state[MKS_KEY_RCTRL];
	state->keyboard_state[MKS_KEY_ALT]   = state->keyboard_state[MKS_KEY_LALT] || state->keyboard_state[MKS_KEY_RALT];

	// Update modifier_state array for compatibility
	state->modifier_state[MKS_MODIFIER_SHIFT] = state->keyboard_state[MKS_KEY_SHIFT];
	state->modifier_state[MKS_MODIFIER_CTRL] = state->keyboard_state[MKS_KEY_CTRL];
	state->modifier_state[MKS_MODIFIER_ALT] = state->keyboard_state[MKS_KEY_ALT];

	// Handle non-extended special keys
	switch(keysym) {
		case XK_Escape:    keycode = MKS_KEY_ESCAPE; break;
		case XK_BackSpace: keycode = MKS_KEY_BACKSPACE; break;
		case XK_Tab:       keycode = MKS_KEY_TAB; break;
		case XK_Return:    keycode = MKS_KEY_RETURN; break;
		case XK_Caps_Lock: keycode = MKS_KEY_CAPSLOCK; break;
		case XK_F1:        keycode = MKS_KEY_F1; break;
		case XK_F2:        keycode = MKS_KEY_F2; break;
		case XK_F3:        keycode = MKS_KEY_F3; break;
		case XK_F4:        keycode = MKS_KEY_F4; break;
		case XK_F5:        keycode = MKS_KEY_F5; break;
		case XK_F6:        keycode = MKS_KEY_F6; break;
		case XK_F7:        keycode = MKS_KEY_F7; break;
		case XK_F8:        keycode = MKS_KEY_F8; break;
		case XK_F9:        keycode = MKS_KEY_F9; break;
		case XK_F10:       keycode = MKS_KEY_F10; break;
		case XK_F11:       keycode = MKS_KEY_F11; break;
		case XK_F12:       keycode = MKS_KEY_F12; break;
	}

	// Handle extended keys
	switch(keysym) {
		case XK_Left:      keycode = MKS_KEY_LEFT; break;
		case XK_Right:     keycode = MKS_KEY_RIGHT; break;
		case XK_Up:        keycode = MKS_KEY_UP; break;
		case XK_Down:      keycode = MKS_KEY_DOWN; break;
		case XK_Insert:    keycode = MKS_KEY_INSERT; break;
		case XK_Delete:    keycode = MKS_KEY_DELETE; break;
		case XK_Home:      keycode = MKS_KEY_HOME; break;
		case XK_End:       keycode = MKS_KEY_END; break;
		case XK_Page_Up:   keycode = MKS_KEY_PAGEUP; break;
		case XK_Page_Down: keycode = MKS_KEY_PAGEDOWN; break;
		case XK_Num_Lock:  keycode = MKS_KEY_NUMLOCK; break;
		case XK_Scroll_Lock: keycode = MKS_KEY_SCROLLLOCK; break;
		case XK_Print:     keycode = MKS_KEY_PRINTSCREEN; break;
		case XK_Pause:     keycode = MKS_KEY_PAUSE; break;
		case XK_Menu:      keycode = MKS_KEY_MENU; break;
	}

	// Handle numpad keys
	switch(keysym) {
		case XK_KP_0:        keycode = MKS_KEY_NUMPAD_0; break;
		case XK_KP_1:        keycode = MKS_KEY_NUMPAD_1; break;
		case XK_KP_2:        keycode = MKS_KEY_NUMPAD_2; break;
		case XK_KP_3:        keycode = MKS_KEY_NUMPAD_3; break;
		case XK_KP_4:        keycode = MKS_KEY_NUMPAD_4; break;
		case XK_KP_5:        keycode = MKS_KEY_NUMPAD_5; break;
		case XK_KP_6:        keycode = MKS_KEY_NUMPAD_6; break;
		case XK_KP_7:        keycode = MKS_KEY_NUMPAD_7; break;
		case XK_KP_8:        keycode = MKS_KEY_NUMPAD_8; break;
		case XK_KP_9:        keycode = MKS_KEY_NUMPAD_9; break;
		case XK_KP_Decimal:  keycode = MKS_KEY_NUMPAD_DECIMAL; break;
		case XK_KP_Divide:   keycode = MKS_KEY_NUMPAD_DIVIDE; break;
		case XK_KP_Multiply: keycode = MKS_KEY_NUMPAD_MULTIPLY; break;
		case XK_KP_Subtract: keycode = MKS_KEY_NUMPAD_SUBTRACT; break;
		case XK_KP_Add:      keycode = MKS_KEY_NUMPAD_ADD; break;
		case XK_KP_Enter:    keycode = MKS_KEY_NUMPAD_ENTER; break;
	}

	// Update key state
	if(keycode) {
		state->keyboard_state[keycode] = key_down;
	}

	// Call the key callback
	if(keycode && state->key_callback) {
		state->key_callback(state, keycode, key_down ? MKS_PRESSED : MKS_RELEASED, (state->keyboard_state[MKS_KEY_SHIFT] ? MKS_MOD_SHIFT : 0) | (state->keyboard_state[MKS_KEY_CTRL] ? MKS_MOD_CTRL : 0) | (state->keyboard_state[MKS_KEY_ALT] ? MKS_MOD_ALT : 0) | (state->keyboard_state[MKS_KEY_LSUPER] ? MKS_MOD_LSUPER : 0) | (state->keyboard_state[MKS_KEY_RSUPER] ? MKS_MOD_RSUPER : 0));
	}

	return keycode;
}


/* Set Raw Motion events for all devices on the root window */
static void enable_xi2_raw_input(struct mkfw_state *state) {
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


/* Returns whether user requested shutdown (ESC or window close) */
static int32_t mkfw_should_close(struct mkfw_state *state) {
	return PLATFORM(state)->should_close;
}

/* Force close */
static void mkfw_set_should_close(struct mkfw_state *state, int32_t value) {
	PLATFORM(state)->should_close = value;
}

/* Signal handlers removed - applications should set up their own if needed */
static void setup_signal_handlers(struct mkfw_state *state) {
	// Signal handlers can't easily access per-window state
	// Applications should set up their own signal handling if needed
	(void)state;
}

/* Select the best framebuffer config */
static GLXFBConfig select_best_fbconfig(Display *display, int screen) {
	int fb_count = 0;
	GLXFBConfig *fbcs = glXChooseFBConfig(display, screen, 0, &fb_count);
	if(!fbcs || fb_count == 0) {
		mkfw_error("no framebuffer configs found");
		exit(EXIT_FAILURE);
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

static void mkfw_detach_context(struct mkfw_state *state) {
	glXMakeCurrent(PLATFORM(state)->display, None, 0);
}

static void mkfw_attach_context(struct mkfw_state *state) {
	glXMakeCurrent(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->glctx);
}

static void mkfw_show_window(struct mkfw_state *state) {
	XMapWindow(PLATFORM(state)->display, PLATFORM(state)->window);
	XFlush(PLATFORM(state)->display);
	XSync(PLATFORM(state)->display, 0);
}

// Query the maximum OpenGL version supported by the driver.
// Returns 1 on success (major/minor filled), 0 on failure.
// This creates a temporary display connection and context, then cleans up.
static int mkfw_query_max_gl_version(int *major, int *minor) {
	Display *dpy = XOpenDisplay(0);
	if(!dpy) return 0;

	load_glx_functions(dpy);

	int screen = DefaultScreen(dpy);
	GLXFBConfig fb_config = select_best_fbconfig(dpy, screen);

	XVisualInfo *vi = glXGetVisualFromFBConfig(dpy, fb_config);
	if(!vi) {
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
		XDestroyWindow(dpy, win);
		XFreeColormap(dpy, cmap);
		XFree(vi);
		XCloseDisplay(dpy);
		return 0;
	}

	glXMakeCurrent(dpy, win, ctx);

	typedef const unsigned char *(*PFNGLGETSTRINGPROC)(unsigned int);
	PFNGLGETSTRINGPROC pglGetString = (PFNGLGETSTRINGPROC)glXGetProcAddress((const GLubyte *)"glGetString");
	int result = 0;
	if(pglGetString) {
		const char *version = (const char *)pglGetString(0x1F02); // GL_VERSION
		if(version) {
			result = (sscanf(version, "%d.%d", major, minor) == 2);
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

// Platform initialization
static struct mkfw_state *mkfw_init(int32_t width, int32_t height) {
	struct mkfw_state *state = (struct mkfw_state *)calloc(1, sizeof(struct mkfw_state));
	if(!state) {
		return 0;
	}

	state->platform = calloc(1, sizeof(struct x11_mkfw_state));
	if(!state->platform) {
		free(state);
		return 0;
	}

	PLATFORM(state)->mouse_sensitivity = 1.0;
	PLATFORM(state)->xi_opcode = -1;

	XInitThreads();
	setup_signal_handlers(state);
	PLATFORM(state)->display = XOpenDisplay(0);
	if(!PLATFORM(state)->display) {
		mkfw_error("unable to open X display");
		free(state->platform);
		free(state);
		return 0;
	}

	int screen = DefaultScreen(PLATFORM(state)->display);
	Window root = RootWindow(PLATFORM(state)->display, screen);

	load_glx_functions(PLATFORM(state)->display);

	GLXFBConfig fb_config = select_best_fbconfig(PLATFORM(state)->display, screen);

	XVisualInfo *vi = glXGetVisualFromFBConfig(PLATFORM(state)->display, fb_config);
	if(!vi) {
		mkfw_error("unable to get a visual from framebuffer config");
		XCloseDisplay(PLATFORM(state)->display);
		free(state->platform);
		free(state);
		return 0;
	}

	Colormap cmap = XCreateColormap(PLATFORM(state)->display, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask;

	PLATFORM(state)->window = XCreateWindow(PLATFORM(state)->display, root, 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

	XStoreName(PLATFORM(state)->display, PLATFORM(state)->window, "MKFW");

	// Set WM_CLASS
	XClassHint *class_hint = XAllocClassHint();
	if(class_hint) {
		class_hint->res_name = (char *)"mkfw";
		class_hint->res_class = (char *)"MKFW";
		XSetClassHint(PLATFORM(state)->display, PLATFORM(state)->window, class_hint);
		XFree(class_hint);
	}

	PLATFORM(state)->wm_delete_window = XInternAtom(PLATFORM(state)->display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(PLATFORM(state)->display, PLATFORM(state)->window, &PLATFORM(state)->wm_delete_window, 1);
	enable_xi2_raw_input(state);

	// Create OpenGL context (glXCreateContextAttribsARB loaded by load_glx_functions above)
	if(!glXCreateContextAttribsARB) {
		mkfw_error("glXCreateContextAttribsARB not supported");
		XFree(vi);
		XDestroyWindow(PLATFORM(state)->display, PLATFORM(state)->window);
		XCloseDisplay(PLATFORM(state)->display);
		free(state->platform);
		free(state);
		return 0;
	}

	int ctx_attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, mkfw_gl_major,
		GLX_CONTEXT_MINOR_VERSION_ARB, mkfw_gl_minor,
		GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		0
	};

	PLATFORM(state)->glctx = glXCreateContextAttribsARB(PLATFORM(state)->display, fb_config, 0, 1, ctx_attribs);
	if(!PLATFORM(state)->glctx) {
		// Try to determine max supported version for the error message
		int fallback_attribs[] = {
			GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
			GLX_CONTEXT_MINOR_VERSION_ARB, 1,
			GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			0
		};
		GLXContext query_ctx = glXCreateContextAttribsARB(PLATFORM(state)->display, fb_config, 0, 1, fallback_attribs);
		if(query_ctx) {
			glXMakeCurrent(PLATFORM(state)->display, PLATFORM(state)->window, query_ctx);
			typedef const unsigned char *(*PFNGLGETSTRINGPROC)(unsigned int);
			PFNGLGETSTRINGPROC pglGetString = (PFNGLGETSTRINGPROC)glXGetProcAddress((const GLubyte *)"glGetString");
			int max_major = 0, max_minor = 0;
			if(pglGetString) {
				const char *ver = (const char *)pglGetString(0x1F02);
				if(ver) sscanf(ver, "%d.%d", &max_major, &max_minor);
			}
			glXMakeCurrent(PLATFORM(state)->display, None, 0);
			glXDestroyContext(PLATFORM(state)->display, query_ctx);
			mkfw_error("OpenGL %d.%d Compatibility Profile not available (driver supports up to %d.%d)",
				mkfw_gl_major, mkfw_gl_minor, max_major, max_minor);
		} else {
			mkfw_error("OpenGL %d.%d Compatibility Profile not available", mkfw_gl_major, mkfw_gl_minor);
		}
		XFree(vi);
		XDestroyWindow(PLATFORM(state)->display, PLATFORM(state)->window);
		XCloseDisplay(PLATFORM(state)->display);
		free(state->platform);
		free(state);
		return 0;
	}

	glXMakeCurrent(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->glctx);
	XFree(vi);

	// XIM for Unicode text input
	PLATFORM(state)->xim = XOpenIM(PLATFORM(state)->display, 0, 0, 0);
	if(PLATFORM(state)->xim) {
		PLATFORM(state)->xic = XCreateIC(PLATFORM(state)->xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, PLATFORM(state)->window, XNFocusWindow, PLATFORM(state)->window, (char *)0);
	}

	// Cursor shapes
	PLATFORM(state)->cursors[MKFW_CURSOR_ARROW]       = XCreateFontCursor(PLATFORM(state)->display, XC_left_ptr);
	PLATFORM(state)->cursors[MKFW_CURSOR_TEXT_INPUT]   = XCreateFontCursor(PLATFORM(state)->display, XC_xterm);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_ALL]   = XCreateFontCursor(PLATFORM(state)->display, XC_fleur);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_NS]    = XCreateFontCursor(PLATFORM(state)->display, XC_sb_v_double_arrow);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_EW]    = XCreateFontCursor(PLATFORM(state)->display, XC_sb_h_double_arrow);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_NESW]  = XCreateFontCursor(PLATFORM(state)->display, XC_bottom_left_corner);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_NWSE]  = XCreateFontCursor(PLATFORM(state)->display, XC_bottom_right_corner);
	PLATFORM(state)->cursors[MKFW_CURSOR_HAND]         = XCreateFontCursor(PLATFORM(state)->display, XC_hand2);
	PLATFORM(state)->cursors[MKFW_CURSOR_NOT_ALLOWED]  = XCreateFontCursor(PLATFORM(state)->display, XC_X_cursor);

	// Clipboard atoms
	PLATFORM(state)->clipboard_atom      = XInternAtom(PLATFORM(state)->display, "CLIPBOARD", False);
	PLATFORM(state)->utf8_string_atom    = XInternAtom(PLATFORM(state)->display, "UTF8_STRING", False);
	PLATFORM(state)->targets_atom        = XInternAtom(PLATFORM(state)->display, "TARGETS", False);
	PLATFORM(state)->mkfw_clipboard_atom = XInternAtom(PLATFORM(state)->display, "MKFW_CLIPBOARD", False);

	state->has_focus = 1;

	return state;
}

// Constrain mouse pointer to window if needed
static void mkfw_constrain_mouse(struct mkfw_state *state, int32_t constrain) {
	PLATFORM(state)->mouse_constrained = constrain;

	XWindowAttributes attrs;
	XGetWindowAttributes(PLATFORM(state)->display, PLATFORM(state)->window, &attrs);

	if(constrain) {
		int result = XGrabPointer(PLATFORM(state)->display, PLATFORM(state)->window, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask, GrabModeAsync, GrabModeAsync, PLATFORM(state)->window, None, CurrentTime);

		if(result != GrabSuccess) {
			mkfw_error("failed to grab pointer");
			return;
		}

		int center_x = attrs.width / 2;
		int center_y = attrs.height / 2;
		XWarpPointer(PLATFORM(state)->display, None, PLATFORM(state)->window, 0, 0, 0, 0, center_x, center_y);
	} else {
		XUngrabPointer(PLATFORM(state)->display, CurrentTime);
	}

	XFlush(PLATFORM(state)->display);
}

// Show/hide mouse cursor
static void mkfw_set_mouse_cursor(struct mkfw_state *state, int32_t visible) {
	if(visible) {
		if(PLATFORM(state)->mouse_constrained) { // Only warp back if the cursor was previously hidden
			mkfw_constrain_mouse(state, 0);
			XWarpPointer(PLATFORM(state)->display, None, PLATFORM(state)->window, 0, 0, 0, 0, PLATFORM(state)->hide_mouse_x, PLATFORM(state)->hide_mouse_y);
		}
		XUndefineCursor(PLATFORM(state)->display, PLATFORM(state)->window);
	} else {
		Window root, child;
		int root_x, root_y, win_x, win_y;
		unsigned int mask;

		if(XQueryPointer(PLATFORM(state)->display, PLATFORM(state)->window, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask)) {
			PLATFORM(state)->hide_mouse_x = win_x;
			PLATFORM(state)->hide_mouse_y = win_y;
		}

		mkfw_constrain_mouse(state, 1);
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

// Fullscreen toggling (unchanged except for referencing raw input if needed)
static void mkfw_fullscreen(struct mkfw_state *state, int32_t enable) {
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

		mkfw_set_mouse_cursor(state, 0);
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
		mkfw_set_mouse_cursor(state, 1);
		state->is_fullscreen = 0;
	}

	XFlush(PLATFORM(state)->display);
}

// Process X events and raw XInput2 events
static void mkfw_pump_messages(struct mkfw_state *state) {
	XEvent event;
	while(XPending(PLATFORM(state)->display)) {
		XNextEvent(PLATFORM(state)->display, &event);

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

					if(PLATFORM(state)->mouse_constrained) {
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
						mapped = MOUSE_BUTTON_LEFT;
					} else if(xbtn == 2) {
						mapped = MOUSE_BUTTON_MIDDLE;
					} else if(xbtn == 3) {
						mapped = MOUSE_BUTTON_RIGHT;
					} else if(xbtn == 8) {
						mapped = MOUSE_BUTTON_EXTRA1;
					} else if(xbtn == 9) {
						mapped = MOUSE_BUTTON_EXTRA2;
					} else {
						break;
					}
					state->mouse_buttons[mapped] = 1;
					if(state->mouse_button_callback) {
						state->mouse_button_callback(state, mapped, MKS_PRESSED);
					}
				}
			} break;

			case ButtonRelease: {
				uint32_t xbtn = event.xbutton.button;
				uint8_t mapped = 0;
				if(xbtn == 1) {
					mapped = MOUSE_BUTTON_LEFT;
				} else if(xbtn == 2) {
					mapped = MOUSE_BUTTON_MIDDLE;
				} else if(xbtn == 3) {
					mapped = MOUSE_BUTTON_RIGHT;
				} else if(xbtn == 8) {
					mapped = MOUSE_BUTTON_EXTRA1;
				} else if(xbtn == 9) {
					mapped = MOUSE_BUTTON_EXTRA2;
				} else {
					break;
				}
				state->mouse_buttons[mapped] = 0;
				if(state->mouse_button_callback) {
					state->mouse_button_callback(state, mapped, MKS_RELEASED);
				}
			} break;

			case MotionNotify: {
				if(PLATFORM(state)->in_window) {
					PLATFORM(state)->last_mouse_x = event.xmotion.x;
					PLATFORM(state)->last_mouse_y = event.xmotion.y;

					// Update absolute mouse position
					state->mouse_x = event.xmotion.x;
					state->mouse_y = event.xmotion.y;

					if(PLATFORM(state)->mouse_constrained) {
						XWindowAttributes attrs;
						XGetWindowAttributes(PLATFORM(state)->display, PLATFORM(state)->window, &attrs);
						int center_x = attrs.width / 2;
						int center_y = attrs.height / 2;
						if (event.xmotion.x != center_x || event.xmotion.y != center_y) { // Only warp if pointer is NOT at the center
							XWarpPointer(PLATFORM(state)->display, None, PLATFORM(state)->window, 0, 0, 0, 0, center_x, center_y);
							XSync(PLATFORM(state)->display, False); // Sync is still important
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
				if((Atom)event.xclient.data.l[0] == PLATFORM(state)->wm_delete_window) {
					PLATFORM(state)->should_close = 1;
				}
			} break;
		}
	}
}

// Swap the front/back buffers
static void mkfw_swap_buffers(struct mkfw_state *state) {
	glXSwapBuffers(PLATFORM(state)->display, PLATFORM(state)->window);
}

static void mkfw_set_window_min_size_and_aspect(struct mkfw_state *state, int32_t min_width, int32_t min_height, float aspect_width, float aspect_height) {
	XSizeHints *hints = XAllocSizeHints();
	if(!hints) {
		mkfw_error("failed to allocate XSizeHints");
		return;
	}

	PLATFORM(state)->aspect_ratio = aspect_width / aspect_height;
	PLATFORM(state)->min_width = min_width;
	PLATFORM(state)->min_height = min_height;

	// Set both minimum size and aspect ratio
	hints->flags = PMinSize | PAspect;
	hints->min_width = min_width;
	hints->min_height = min_height;
	hints->min_aspect.x = aspect_width;
	hints->min_aspect.y = aspect_height;
	hints->max_aspect.x = aspect_width;
	hints->max_aspect.y = aspect_height;

	XSetWMNormalHints(PLATFORM(state)->display, PLATFORM(state)->window, hints);
	XFree(hints);
}

static void mkfw_set_window_title(struct mkfw_state *state, const char *title) {
	XStoreName(PLATFORM(state)->display, PLATFORM(state)->window, title);

	// Set _NET_WM_NAME for modern window managers
	Atom net_wm_name = XInternAtom(PLATFORM(state)->display, "_NET_WM_NAME", 0);
	Atom utf8_string = XInternAtom(PLATFORM(state)->display, "UTF8_STRING", 0);

	if(net_wm_name && utf8_string) {
		XChangeProperty(PLATFORM(state)->display, PLATFORM(state)->window, net_wm_name, utf8_string, 8, PropModeReplace, (uint8_t *)title, strlen(title));
	}

	XFlush(PLATFORM(state)->display);
}

static void mkfw_set_window_resizable(struct mkfw_state *state, int32_t resizable) {
	XSizeHints *hints = XAllocSizeHints();
	if(!hints) {
		mkfw_error("failed to allocate XSizeHints");
		return;
	}

	if(resizable) {	// Make window resizable - set minimum size only
		hints->flags = PMinSize;
		hints->min_width = PLATFORM(state)->min_width > 0 ? PLATFORM(state)->min_width : 100;
		hints->min_height = PLATFORM(state)->min_height > 0 ? PLATFORM(state)->min_height : 100;

		if(PLATFORM(state)->aspect_ratio > 0.0f) {		// Preserve aspect ratio if it was set
			hints->flags |= PAspect;
			hints->min_aspect.x = (int)(PLATFORM(state)->aspect_ratio * 1000);
			hints->min_aspect.y = 1000;
			hints->max_aspect.x = (int)(PLATFORM(state)->aspect_ratio * 1000);
			hints->max_aspect.y = 1000;
		}

	} else {				// Make window non-resizable - set min and max to current size
		XWindowAttributes attrs;
		XGetWindowAttributes(PLATFORM(state)->display, PLATFORM(state)->window, &attrs);

		hints->flags = PMinSize | PMaxSize;
		hints->min_width = attrs.width;
		hints->min_height = attrs.height;
		hints->max_width = attrs.width;
		hints->max_height = attrs.height;
	}

	XSetWMNormalHints(PLATFORM(state)->display, PLATFORM(state)->window, hints);
	XFree(hints);
	XFlush(PLATFORM(state)->display);
}


static void mkfw_get_framebuffer_size(struct mkfw_state *state, int32_t *width, int32_t *height) {
	Window root;
	int x, y;
	unsigned int w, h, border, depth;
	XGetGeometry(PLATFORM(state)->display, PLATFORM(state)->window, &root, &x, &y, &w, &h, &border, &depth);
	*width = (int)w;
	*height = (int)h;
}

static void mkfw_set_swapinterval(struct mkfw_state *state, uint32_t interval) {
	typedef int (*PFNGLXSWAPINTERVALEXTPROC)(Display*, GLXDrawable, int);
	PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress((const GLubyte *)"glXSwapIntervalEXT");

	if(glXSwapIntervalEXT) {
		glXSwapIntervalEXT(PLATFORM(state)->display, glXGetCurrentDrawable(), interval);
	}
}

static uint64_t mkfw_gettime(struct mkfw_state *state __attribute__((unused))) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Cleanup routines
static void mkfw_cleanup(struct mkfw_state *state) {
	if(!state) return;

	mkfw_set_mouse_cursor(state, 1);
	mkfw_constrain_mouse(state, 0);

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

	glXMakeCurrent(PLATFORM(state)->display, None, 0);
	glXDestroyContext(PLATFORM(state)->display, PLATFORM(state)->glctx);
	XDestroyWindow(PLATFORM(state)->display, PLATFORM(state)->window);
	XCloseDisplay(PLATFORM(state)->display);

	free(state->platform);
	free(state);
}

static void mkfw_sleep(uint64_t nanoseconds) {
	struct timespec ts;
	ts.tv_sec = nanoseconds / 1000000000;
	ts.tv_nsec = nanoseconds % 1000000000;
	nanosleep(&ts, 0);
}

static void mkfw_set_mouse_sensitivity(struct mkfw_state *state, double sensitivity) {
	PLATFORM(state)->mouse_sensitivity = sensitivity;
}

static void mkfw_get_and_clear_mouse_delta(struct mkfw_state *state, int32_t *dx, int32_t *dy) {
	*dx = (int32_t)PLATFORM(state)->accumulated_dx;
	*dy = (int32_t)PLATFORM(state)->accumulated_dy;
	// Keep fractional remainder for next frame
	PLATFORM(state)->accumulated_dx -= (double)*dx;
	PLATFORM(state)->accumulated_dy -= (double)*dy;
}

// [=]===^=[ mkfw_set_cursor_shape ]========================================[=]
static void mkfw_set_cursor_shape(struct mkfw_state *state, uint32_t cursor) {
	if(cursor >= MKFW_CURSOR_LAST) {
		cursor = MKFW_CURSOR_ARROW;
	}
	PLATFORM(state)->current_cursor = cursor;
	XDefineCursor(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->cursors[cursor]);
	XFlush(PLATFORM(state)->display);
}

// [=]===^=[ mkfw_set_clipboard_text ]======================================[=]
static void mkfw_set_clipboard_text(struct mkfw_state *state, const char *text) {
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

// [=]===^=[ mkfw_get_clipboard_text ]======================================[=]
static const char *mkfw_get_clipboard_text(struct mkfw_state *state) {
	Window owner = XGetSelectionOwner(PLATFORM(state)->display, PLATFORM(state)->clipboard_atom);
	if(owner == None) {
		return "";
	}

	if(owner == PLATFORM(state)->window) {
		return PLATFORM(state)->clipboard_text ? PLATFORM(state)->clipboard_text : "";
	}

	XConvertSelection(PLATFORM(state)->display, PLATFORM(state)->clipboard_atom, PLATFORM(state)->utf8_string_atom, PLATFORM(state)->mkfw_clipboard_atom, PLATFORM(state)->window, CurrentTime);
	XFlush(PLATFORM(state)->display);

	XEvent ev;
	for(uint32_t i = 0; i < 50; ++i) {
		if(XCheckTypedWindowEvent(PLATFORM(state)->display, PLATFORM(state)->window, SelectionNotify, &ev)) {
			if(ev.xselection.property == None) {
				return "";
			}

			Atom actual_type;
			int actual_format;
			unsigned long nitems, bytes_after;
			unsigned char *data = 0;

			XGetWindowProperty(PLATFORM(state)->display, PLATFORM(state)->window, PLATFORM(state)->mkfw_clipboard_atom, 0, 1024 * 1024, True, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &data);

			if(data) {
				free(PLATFORM(state)->clipboard_text);
				size_t len = nitems * (actual_format / 8);
				PLATFORM(state)->clipboard_text = (char *)malloc(len + 1);
				memcpy(PLATFORM(state)->clipboard_text, data, len);
				PLATFORM(state)->clipboard_text[len] = 0;
				XFree(data);
				return PLATFORM(state)->clipboard_text;
			}

			return "";
		}
		struct timespec ts = { 0, 10000000 };
		nanosleep(&ts, 0);
	}

	return "";
}
