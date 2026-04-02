// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdlib.h>

#define PLATFORM(state) ((struct emscripten_mkfw_state *)(state)->platform)

struct emscripten_mkfw_state {
	uint8_t should_close;
	int32_t canvas_width;
	int32_t canvas_height;
	float aspect_ratio;
	double mouse_sensitivity;
	double accumulated_dx;
	double accumulated_dy;
	double fractional_dx;
	double fractional_dy;
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glctx;
	uint32_t current_cursor;
	uint8_t cursor_hidden;
	uint8_t mouse_constrained;
};

// ============================================================
// DOM keycode mapping
// ============================================================

// [=]===^=[ map_dom_keycode ]====================================================================^===[=]
static uint32_t map_dom_keycode(const char *code) {
	if(!code || !code[0]) {
		return 0;
	}

	// KeyA through KeyZ
	if(code[0] == 'K' && code[1] == 'e' && code[2] == 'y' && code[3] >= 'A' && code[3] <= 'Z' && code[4] == 0) {
		return 'a' + (code[3] - 'A');
	}

	// Digit0 through Digit9
	if(code[0] == 'D' && code[1] == 'i' && code[2] == 'g' && code[3] == 'i' && code[4] == 't' && code[5] >= '0' && code[5] <= '9' && code[6] == 0) {
		return '0' + (code[5] - '0');
	}

	// Numpad0 through Numpad9
	if(code[0] == 'N' && code[1] == 'u' && code[2] == 'm' && code[3] == 'p' && code[4] == 'a' && code[5] == 'd' && code[6] >= '0' && code[6] <= '9' && code[7] == 0) {
		return MKS_KEY_NUMPAD_0 + (code[6] - '0');
	}

	// F1 through F12
	if(code[0] == 'F') {
		if(code[1] >= '1' && code[1] <= '9' && code[2] == 0) {
			return MKS_KEY_F1 + (code[1] - '1');
		}
		if(code[1] == '1' && code[2] >= '0' && code[2] <= '2' && code[3] == 0) {
			return MKS_KEY_F10 + (code[2] - '0');
		}
	}

	if(strcmp(code, "Escape") == 0) { return MKS_KEY_ESCAPE; }
	if(strcmp(code, "Enter") == 0) { return MKS_KEY_RETURN; }
	if(strcmp(code, "NumpadEnter") == 0) { return MKS_KEY_NUMPAD_ENTER; }
	if(strcmp(code, "Tab") == 0) { return MKS_KEY_TAB; }
	if(strcmp(code, "Backspace") == 0) { return MKS_KEY_BACKSPACE; }
	if(strcmp(code, "Space") == 0) { return MKS_KEY_SPACE; }
	if(strcmp(code, "ArrowLeft") == 0) { return MKS_KEY_LEFT; }
	if(strcmp(code, "ArrowRight") == 0) { return MKS_KEY_RIGHT; }
	if(strcmp(code, "ArrowUp") == 0) { return MKS_KEY_UP; }
	if(strcmp(code, "ArrowDown") == 0) { return MKS_KEY_DOWN; }
	if(strcmp(code, "ShiftLeft") == 0) { return MKS_KEY_LSHIFT; }
	if(strcmp(code, "ShiftRight") == 0) { return MKS_KEY_RSHIFT; }
	if(strcmp(code, "ControlLeft") == 0) { return MKS_KEY_LCTRL; }
	if(strcmp(code, "ControlRight") == 0) { return MKS_KEY_RCTRL; }
	if(strcmp(code, "AltLeft") == 0) { return MKS_KEY_LALT; }
	if(strcmp(code, "AltRight") == 0) { return MKS_KEY_RALT; }
	if(strcmp(code, "MetaLeft") == 0) { return MKS_KEY_LSUPER; }
	if(strcmp(code, "MetaRight") == 0) { return MKS_KEY_RSUPER; }
	if(strcmp(code, "CapsLock") == 0) { return MKS_KEY_CAPSLOCK; }
	if(strcmp(code, "NumLock") == 0) { return MKS_KEY_NUMLOCK; }
	if(strcmp(code, "ScrollLock") == 0) { return MKS_KEY_SCROLLLOCK; }
	if(strcmp(code, "PrintScreen") == 0) { return MKS_KEY_PRINTSCREEN; }
	if(strcmp(code, "Pause") == 0) { return MKS_KEY_PAUSE; }
	if(strcmp(code, "ContextMenu") == 0) { return MKS_KEY_MENU; }
	if(strcmp(code, "Delete") == 0) { return MKS_KEY_DELETE; }
	if(strcmp(code, "Insert") == 0) { return MKS_KEY_INSERT; }
	if(strcmp(code, "Home") == 0) { return MKS_KEY_HOME; }
	if(strcmp(code, "End") == 0) { return MKS_KEY_END; }
	if(strcmp(code, "PageUp") == 0) { return MKS_KEY_PAGEUP; }
	if(strcmp(code, "PageDown") == 0) { return MKS_KEY_PAGEDOWN; }
	if(strcmp(code, "Comma") == 0) { return MKS_KEY_COMMA; }
	if(strcmp(code, "Period") == 0) { return MKS_KEY_PERIOD; }
	if(strcmp(code, "Slash") == 0) { return MKS_KEY_SLASH; }
	if(strcmp(code, "Semicolon") == 0) { return MKS_KEY_SEMICOLON; }
	if(strcmp(code, "Quote") == 0) { return MKS_KEY_APOSTROPHE; }
	if(strcmp(code, "BracketLeft") == 0) { return MKS_KEY_LEFT_BRACKET; }
	if(strcmp(code, "BracketRight") == 0) { return MKS_KEY_RIGHT_BRACKET; }
	if(strcmp(code, "Backslash") == 0) { return MKS_KEY_BACKSLASH; }
	if(strcmp(code, "Backquote") == 0) { return MKS_KEY_GRAVE; }
	if(strcmp(code, "Minus") == 0) { return MKS_KEY_MINUS; }
	if(strcmp(code, "Equal") == 0) { return MKS_KEY_EQUAL; }
	if(strcmp(code, "NumpadDecimal") == 0) { return MKS_KEY_NUMPAD_DECIMAL; }
	if(strcmp(code, "NumpadDivide") == 0) { return MKS_KEY_NUMPAD_DIVIDE; }
	if(strcmp(code, "NumpadMultiply") == 0) { return MKS_KEY_NUMPAD_MULTIPLY; }
	if(strcmp(code, "NumpadSubtract") == 0) { return MKS_KEY_NUMPAD_SUBTRACT; }
	if(strcmp(code, "NumpadAdd") == 0) { return MKS_KEY_NUMPAD_ADD; }

	return 0;
}

// [=]===^=[ update_modifier_state ]=============================================================^===[=]
static void update_modifier_state(struct mkfw_state *state, uint32_t key, uint32_t action) {
	uint32_t mod_bit = 0;
	uint32_t mod_index = 0;

	switch(key) {
		case MKS_KEY_LSHIFT: {
			mod_bit = MKS_MOD_LSHIFT;
			mod_index = MKS_MODIFIER_SHIFT;
		} break;

		case MKS_KEY_RSHIFT: {
			mod_bit = MKS_MOD_RSHIFT;
			mod_index = MKS_MODIFIER_SHIFT;
		} break;

		case MKS_KEY_LCTRL: {
			mod_bit = MKS_MOD_LCTRL;
			mod_index = MKS_MODIFIER_CTRL;
		} break;

		case MKS_KEY_RCTRL: {
			mod_bit = MKS_MOD_RCTRL;
			mod_index = MKS_MODIFIER_CTRL;
		} break;

		case MKS_KEY_LALT: {
			mod_bit = MKS_MOD_LALT;
			mod_index = MKS_MODIFIER_ALT;
		} break;

		case MKS_KEY_RALT: {
			mod_bit = MKS_MOD_RALT;
			mod_index = MKS_MODIFIER_ALT;
		} break;

		default: {
			return;
		}
	}

	if(action == MKS_PRESSED) {
		state->modifier_state[mod_index] |= mod_bit;
	} else {
		state->modifier_state[mod_index] &= ~mod_bit;
	}
}

// [=]===^=[ get_modifier_bits ]================================================================^===[=]
static uint32_t get_modifier_bits(struct mkfw_state *state) {
	return state->modifier_state[MKS_MODIFIER_SHIFT]
	     | state->modifier_state[MKS_MODIFIER_CTRL]
	     | state->modifier_state[MKS_MODIFIER_ALT];
}

// ============================================================
// DOM event callbacks
// ============================================================

// [=]===^=[ mkfw_em_on_key ]===================================================================^===[=]
static EM_BOOL mkfw_em_on_key(int event_type, const EmscriptenKeyboardEvent *e, void *user_data) {
	struct mkfw_state *state = (struct mkfw_state *)user_data;
	uint32_t key = map_dom_keycode(e->code);
	if(!key) {
		return EM_FALSE;
	}

	uint32_t action = (event_type == EMSCRIPTEN_EVENT_KEYDOWN) ? MKS_PRESSED : MKS_RELEASED;

	state->keyboard_state[key] = (action == MKS_PRESSED) ? 1 : 0;
	update_modifier_state(state, key, action);

	// Update generic shift/ctrl/alt keys
	if(key == MKS_KEY_LSHIFT || key == MKS_KEY_RSHIFT) {
		state->keyboard_state[MKS_KEY_SHIFT] = (action == MKS_PRESSED) ? 1 : (state->keyboard_state[MKS_KEY_LSHIFT] || state->keyboard_state[MKS_KEY_RSHIFT]);
	}
	if(key == MKS_KEY_LCTRL || key == MKS_KEY_RCTRL) {
		state->keyboard_state[MKS_KEY_CTRL] = (action == MKS_PRESSED) ? 1 : (state->keyboard_state[MKS_KEY_LCTRL] || state->keyboard_state[MKS_KEY_RCTRL]);
	}
	if(key == MKS_KEY_LALT || key == MKS_KEY_RALT) {
		state->keyboard_state[MKS_KEY_ALT] = (action == MKS_PRESSED) ? 1 : (state->keyboard_state[MKS_KEY_LALT] || state->keyboard_state[MKS_KEY_RALT]);
	}

	if(state->key_callback) {
		state->key_callback(state, key, action, get_modifier_bits(state));
	}

	// Pass char callback for printable keys on keydown
	if(action == MKS_PRESSED && state->char_callback) {
		if(e->charCode >= 32) {
			state->char_callback(state, e->charCode);
		} else if(key >= ' ' && key <= '~' && !(get_modifier_bits(state) & MKS_MOD_CTRL)) {
			state->char_callback(state, key);
		}
	}

	// Prevent default for game keys (arrows, space, tab, F keys)
	if(key == MKS_KEY_TAB || key == MKS_KEY_SPACE || key == MKS_KEY_BACKSPACE) {
		return EM_TRUE;
	}
	if(key >= MKS_KEY_LEFT && key <= MKS_KEY_DOWN) {
		return EM_TRUE;
	}
	if(key >= MKS_KEY_F1 && key <= MKS_KEY_F12) {
		return EM_TRUE;
	}

	return EM_FALSE;
}

// [=]===^=[ mkfw_em_on_mouse_move ]==========================================================^===[=]
static EM_BOOL mkfw_em_on_mouse_move(int event_type, const EmscriptenMouseEvent *e, void *user_data) {
	(void)event_type;
	struct mkfw_state *state = (struct mkfw_state *)user_data;

	state->mouse_x = e->targetX;
	state->mouse_y = e->targetY;

	double sensitivity = PLATFORM(state)->mouse_sensitivity;
	double dx = (double)e->movementX * sensitivity;
	double dy = (double)e->movementY * sensitivity;
	PLATFORM(state)->accumulated_dx += dx;
	PLATFORM(state)->accumulated_dy += dy;

	if(state->mouse_move_delta_callback) {
		state->mouse_move_delta_callback(state, (int32_t)e->movementX, (int32_t)e->movementY);
	}

	return EM_TRUE;
}

// [=]===^=[ mkfw_em_on_mouse_button ]================================================================^===[=]
static EM_BOOL mkfw_em_on_mouse_button(int event_type, const EmscriptenMouseEvent *e, void *user_data) {
	struct mkfw_state *state = (struct mkfw_state *)user_data;
	uint32_t action = (event_type == EMSCRIPTEN_EVENT_MOUSEDOWN) ? MKS_PRESSED : MKS_RELEASED;

	// DOM: 0=left, 1=middle, 2=right, 3=back, 4=forward (matches mkfw)
	uint8_t button = (uint8_t)e->button;
	if(button >= 5) {
		return EM_FALSE;
	}

	state->mouse_buttons[button] = (action == MKS_PRESSED) ? 1 : 0;

	if(state->mouse_button_callback) {
		state->mouse_button_callback(state, button, action);
	}

	return EM_TRUE;
}

// [=]===^=[ mkfw_em_on_wheel ]======================================================================^===[=]
static EM_BOOL mkfw_em_on_wheel(int event_type, const EmscriptenWheelEvent *e, void *user_data) {
	(void)event_type;
	struct mkfw_state *state = (struct mkfw_state *)user_data;

	if(state->scroll_callback) {
		double xoffset = 0.0;
		double yoffset = 0.0;

		// Normalize wheel delta to -1.0 / +1.0 per click
		if(e->deltaY < 0) {
			yoffset = 1.0;
		} else if(e->deltaY > 0) {
			yoffset = -1.0;
		}
		if(e->deltaX < 0) {
			xoffset = -1.0;
		} else if(e->deltaX > 0) {
			xoffset = 1.0;
		}

		state->scroll_callback(state, xoffset, yoffset);
	}

	return EM_TRUE;
}

// [=]===^=[ mkfw_em_on_resize ]=====================================================================^===[=]
static EM_BOOL mkfw_em_on_resize(int event_type, const EmscriptenUiEvent *e, void *user_data) {
	(void)event_type;
	struct mkfw_state *state = (struct mkfw_state *)user_data;
	int32_t w, h;
	double dw, dh;
	emscripten_get_element_css_size("#canvas", &dw, &dh);
	w = (int32_t)dw;
	h = (int32_t)dh;
	emscripten_set_canvas_element_size("#canvas", w, h);
	PLATFORM(state)->canvas_width = w;
	PLATFORM(state)->canvas_height = h;

	if(state->framebuffer_callback) {
		state->framebuffer_callback(state, w, h, PLATFORM(state)->aspect_ratio);
	}

	return EM_TRUE;
}

// [=]===^=[ mkfw_em_on_focus ]======================================================================^===[=]
static EM_BOOL mkfw_em_on_focus(int event_type, const EmscriptenFocusEvent *e, void *user_data) {
	(void)e;
	struct mkfw_state *state = (struct mkfw_state *)user_data;
	uint8_t focused = (event_type == EMSCRIPTEN_EVENT_FOCUS) ? 1 : 0;
	state->has_focus = focused;

	if(!focused) {
		memset(state->keyboard_state, 0, sizeof(state->keyboard_state));
		memset(state->modifier_state, 0, sizeof(state->modifier_state));
	}

	if(state->focus_callback) {
		state->focus_callback(state, focused);
	}

	return EM_TRUE;
}

// ============================================================
// Core API
// ============================================================

// [=]===^=[ mkfw_init ]=====================================================================^===[=]
static struct mkfw_state *mkfw_init(int32_t width, int32_t height) {
	struct mkfw_state *state = (struct mkfw_state *)calloc(1, sizeof(struct mkfw_state));
	if(!state) {
		mkfw_error("Failed to allocate mkfw_state");
		return 0;
	}

	struct emscripten_mkfw_state *platform = (struct emscripten_mkfw_state *)calloc(1, sizeof(struct emscripten_mkfw_state));
	if(!platform) {
		mkfw_error("Failed to allocate platform state");
		free(state);
		return 0;
	}

	state->platform = platform;
	platform->canvas_width = width;
	platform->canvas_height = height;
	platform->mouse_sensitivity = 1.0;

	emscripten_set_canvas_element_size("#canvas", width, height);

	EmscriptenWebGLContextAttributes attrs;
	emscripten_webgl_init_context_attributes(&attrs);
	attrs.majorVersion = 2;
	attrs.minorVersion = 0;
	attrs.alpha = 0;
	attrs.depth = 1;
	attrs.stencil = 1;
	attrs.antialias = 0;
	attrs.premultipliedAlpha = 0;
	attrs.preserveDrawingBuffer = 0;
	attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;

	platform->glctx = emscripten_webgl_create_context("#canvas", &attrs);
	if(platform->glctx <= 0) {
		mkfw_error("Failed to create WebGL2 context");
		free(platform);
		free(state);
		return 0;
	}
	emscripten_webgl_make_context_current(platform->glctx);

	// Register input callbacks
	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, state, EM_TRUE, mkfw_em_on_key);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, state, EM_TRUE, mkfw_em_on_key);
	emscripten_set_mousemove_callback("#canvas", state, EM_TRUE, mkfw_em_on_mouse_move);
	emscripten_set_mousedown_callback("#canvas", state, EM_TRUE, mkfw_em_on_mouse_button);
	emscripten_set_mouseup_callback("#canvas", state, EM_TRUE, mkfw_em_on_mouse_button);
	emscripten_set_wheel_callback("#canvas", state, EM_TRUE, mkfw_em_on_wheel);
	emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, state, EM_TRUE, mkfw_em_on_resize);
	emscripten_set_focusin_callback("#canvas", state, EM_TRUE, mkfw_em_on_focus);
	emscripten_set_focusout_callback("#canvas", state, EM_TRUE, mkfw_em_on_focus);

	state->has_focus = 1;
	state->mouse_in_window = 1;

	return state;
}

// [=]===^=[ mkfw_cleanup ]==================================================================^===[=]
static void mkfw_cleanup(struct mkfw_state *state) {
	if(!state) {
		return;
	}
	if(state->platform) {
		if(PLATFORM(state)->glctx > 0) {
			emscripten_webgl_destroy_context(PLATFORM(state)->glctx);
		}
		free(state->platform);
	}
	free(state);
}

// [=]===^=[ mkfw_should_close ]===============================================================^===[=]
static int32_t mkfw_should_close(struct mkfw_state *state) {
	return PLATFORM(state)->should_close;
}

// [=]===^=[ mkfw_set_should_close ]===========================================================^===[=]
static void mkfw_set_should_close(struct mkfw_state *state, int32_t value) {
	PLATFORM(state)->should_close = value ? 1 : 0;
}

// [=]===^=[ mkfw_show_window ]================================================================^===[=]
static void mkfw_show_window(struct mkfw_state *state) {
	(void)state;
}

// [=]===^=[ mkfw_hide_window ]================================================================^===[=]
static void mkfw_hide_window(struct mkfw_state *state) {
	(void)state;
}

// [=]===^=[ mkfw_pump_messages ]==============================================================^===[=]
static void mkfw_pump_messages(struct mkfw_state *state) {
	(void)state;
}

// [=]===^=[ mkfw_swap_buffers ]===============================================================^===[=]
static void mkfw_swap_buffers(struct mkfw_state *state) {
	(void)state;
}

// [=]===^=[ mkfw_set_window_title ]===========================================================^===[=]
static void mkfw_set_window_title(struct mkfw_state *state, const char *title) {
	(void)state;
	EM_ASM({ document.title = UTF8ToString($0); }, title);
}

// [=]===^=[ mkfw_set_window_size ]============================================================^===[=]
static void mkfw_set_window_size(struct mkfw_state *state, int32_t width, int32_t height) {
	PLATFORM(state)->canvas_width = width;
	PLATFORM(state)->canvas_height = height;
	emscripten_set_canvas_element_size("#canvas", width, height);
}

// [=]===^=[ mkfw_get_framebuffer_size ]======================================================^===[=]
static void mkfw_get_framebuffer_size(struct mkfw_state *state, int32_t *width, int32_t *height) {
	*width = PLATFORM(state)->canvas_width;
	*height = PLATFORM(state)->canvas_height;
}

// [=]===^=[ mkfw_set_swapinterval ]==========================================================^===[=]
static void mkfw_set_swapinterval(struct mkfw_state *state, uint32_t interval) {
	(void)state;
	emscripten_set_main_loop_timing(EM_TIMING_RAF, interval > 0 ? (int)interval : 1);
}

// [=]===^=[ mkfw_get_swapinterval ]==========================================================^===[=]
static int32_t mkfw_get_swapinterval(struct mkfw_state *state) {
	(void)state;
	return 1;
}

// [=]===^=[ mkfw_gettime ]===================================================================^===[=]
static uint64_t mkfw_gettime(struct mkfw_state *state) {
	(void)state;
	return (uint64_t)(emscripten_get_now() * 1000000.0);
}

// [=]===^=[ mkfw_fullscreen ]================================================================^===[=]
static void mkfw_fullscreen(struct mkfw_state *state, int32_t enable) {
	if(enable) {
		EmscriptenFullscreenStrategy strategy;
		memset(&strategy, 0, sizeof(strategy));
		strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
		strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
		strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
		emscripten_request_fullscreen_strategy("#canvas", EM_TRUE, &strategy);
		state->is_fullscreen = 1;
	} else {
		emscripten_exit_fullscreen();
		state->is_fullscreen = 0;
	}
}

// [=]===^=[ mkfw_constrain_mouse ]============================================================^===[=]
static void mkfw_constrain_mouse(struct mkfw_state *state, int32_t constrain) {
	if(constrain) {
		emscripten_request_pointerlock("#canvas", EM_TRUE);
		PLATFORM(state)->mouse_constrained = 1;
	} else {
		emscripten_exit_pointerlock();
		PLATFORM(state)->mouse_constrained = 0;
	}
}

// [=]===^=[ mkfw_set_mouse_cursor ]==========================================================^===[=]
static void mkfw_set_mouse_cursor(struct mkfw_state *state, int32_t visible) {
	(void)state;
	if(visible) {
		EM_ASM({ Module['canvas'].style.cursor = 'auto'; });
	} else {
		EM_ASM({ Module['canvas'].style.cursor = 'none'; });
	}
}

// [=]===^=[ mkfw_set_cursor_shape ]==========================================================^===[=]
static void mkfw_set_cursor_shape(struct mkfw_state *state, uint32_t cursor) {
	(void)state;
	switch(cursor) {
		case MKFW_CURSOR_ARROW: {
			EM_ASM({ Module['canvas'].style.cursor = 'default'; });
		} break;

		case MKFW_CURSOR_TEXT_INPUT: {
			EM_ASM({ Module['canvas'].style.cursor = 'text'; });
		} break;

		case MKFW_CURSOR_RESIZE_ALL: {
			EM_ASM({ Module['canvas'].style.cursor = 'move'; });
		} break;

		case MKFW_CURSOR_RESIZE_NS: {
			EM_ASM({ Module['canvas'].style.cursor = 'ns-resize'; });
		} break;

		case MKFW_CURSOR_RESIZE_EW: {
			EM_ASM({ Module['canvas'].style.cursor = 'ew-resize'; });
		} break;

		case MKFW_CURSOR_RESIZE_NESW: {
			EM_ASM({ Module['canvas'].style.cursor = 'nesw-resize'; });
		} break;

		case MKFW_CURSOR_RESIZE_NWSE: {
			EM_ASM({ Module['canvas'].style.cursor = 'nwse-resize'; });
		} break;

		case MKFW_CURSOR_HAND: {
			EM_ASM({ Module['canvas'].style.cursor = 'pointer'; });
		} break;

		case MKFW_CURSOR_NOT_ALLOWED: {
			EM_ASM({ Module['canvas'].style.cursor = 'not-allowed'; });
		} break;

		default: {
			EM_ASM({ Module['canvas'].style.cursor = 'default'; });
		} break;
	}
}

// [=]===^=[ mkfw_set_mouse_sensitivity ]=====================================================^===[=]
static void mkfw_set_mouse_sensitivity(struct mkfw_state *state, double sensitivity) {
	PLATFORM(state)->mouse_sensitivity = sensitivity;
}

// [=]===^=[ mkfw_get_and_clear_mouse_delta ]=================================================^===[=]
static void mkfw_get_and_clear_mouse_delta(struct mkfw_state *state, int32_t *dx, int32_t *dy) {
	double total_dx = PLATFORM(state)->accumulated_dx + PLATFORM(state)->fractional_dx;
	double total_dy = PLATFORM(state)->accumulated_dy + PLATFORM(state)->fractional_dy;
	int32_t ix = (int32_t)total_dx;
	int32_t iy = (int32_t)total_dy;
	PLATFORM(state)->fractional_dx = total_dx - (double)ix;
	PLATFORM(state)->fractional_dy = total_dy - (double)iy;
	PLATFORM(state)->accumulated_dx = 0.0;
	PLATFORM(state)->accumulated_dy = 0.0;
	*dx = ix;
	*dy = iy;
}

// [=]===^=[ mkfw_get_cursor_position ]=======================================================^===[=]
static void mkfw_get_cursor_position(struct mkfw_state *state, int32_t *x, int32_t *y) {
	*x = state->mouse_x;
	*y = state->mouse_y;
}

// [=]===^=[ mkfw_get_monitors ]=============================================================^===[=]
static int32_t mkfw_get_monitors(struct mkfw_state *state, struct mkfw_monitor *out, int32_t max) {
	if(max < 1) {
		return 0;
	}
	memset(&out[0], 0, sizeof(struct mkfw_monitor));
	snprintf(out[0].name, sizeof(out[0].name), "Browser Canvas");
	out[0].x = 0;
	out[0].y = 0;
	out[0].width = PLATFORM(state)->canvas_width;
	out[0].height = PLATFORM(state)->canvas_height;
	out[0].refresh_rate = 60;
	out[0].primary = 1;
	return 1;
}

// ============================================================
// No-op functions (desktop-only features)
// ============================================================

// [=]===^=[ mkfw_sleep ]====================================================================^===[=]
static void mkfw_sleep(uint64_t nanoseconds) {
	(void)nanoseconds;
}

// [=]===^=[ mkfw_set_window_position ]======================================================^===[=]
static void mkfw_set_window_position(struct mkfw_state *state, int32_t x, int32_t y) {
	(void)state;
	(void)x;
	(void)y;
}

// [=]===^=[ mkfw_get_window_position ]======================================================^===[=]
static void mkfw_get_window_position(struct mkfw_state *state, int32_t *x, int32_t *y) {
	(void)state;
	*x = 0;
	*y = 0;
}

// [=]===^=[ mkfw_maximize_window ]==========================================================^===[=]
static void mkfw_maximize_window(struct mkfw_state *state) {
	(void)state;
}

// [=]===^=[ mkfw_minimize_window ]==========================================================^===[=]
static void mkfw_minimize_window(struct mkfw_state *state) {
	(void)state;
}

// [=]===^=[ mkfw_is_minimized ]============================================================^===[=]
static int32_t mkfw_is_minimized(struct mkfw_state *state) {
	(void)state;
	return 0;
}

// [=]===^=[ mkfw_is_maximized ]============================================================^===[=]
static int32_t mkfw_is_maximized(struct mkfw_state *state) {
	(void)state;
	return 0;
}

// [=]===^=[ mkfw_restore_window ]===========================================================^===[=]
static void mkfw_restore_window(struct mkfw_state *state) {
	(void)state;
}

// [=]===^=[ mkfw_set_window_decorated ]=====================================================^===[=]
static void mkfw_set_window_decorated(struct mkfw_state *state, int32_t decorated) {
	(void)state;
	(void)decorated;
}

// [=]===^=[ mkfw_set_window_opacity ]======================================================^===[=]
static void mkfw_set_window_opacity(struct mkfw_state *state, float opacity) {
	(void)state;
	(void)opacity;
}

// [=]===^=[ mkfw_set_window_icon ]==========================================================^===[=]
static void mkfw_set_window_icon(struct mkfw_state *state, int32_t width, int32_t height, const uint8_t *rgba) {
	(void)state;
	(void)width;
	(void)height;
	(void)rgba;
}

// [=]===^=[ mkfw_set_window_resizable ]====================================================^===[=]
static void mkfw_set_window_resizable(struct mkfw_state *state, int32_t resizable) {
	(void)state;
	(void)resizable;
}

// [=]===^=[ mkfw_set_window_min_size_and_aspect ]===========================================^===[=]
static void mkfw_set_window_min_size_and_aspect(struct mkfw_state *state, int32_t min_width, int32_t min_height, float aspect_width, float aspect_height) {
	(void)min_width;
	(void)min_height;
	(void)aspect_width;
	(void)aspect_height;
	PLATFORM(state)->aspect_ratio = (aspect_height > 0.0f) ? (aspect_width / aspect_height) : 0.0f;
}

// [=]===^=[ mkfw_set_cursor_position ]=====================================================^===[=]
static void mkfw_set_cursor_position(struct mkfw_state *state, int32_t x, int32_t y) {
	(void)state;
	(void)x;
	(void)y;
}

// [=]===^=[ mkfw_set_clipboard_text ]=======================================================^===[=]
static void mkfw_set_clipboard_text(struct mkfw_state *state, const char *text) {
	(void)state;
	(void)text;
}

// [=]===^=[ mkfw_get_clipboard_text ]=======================================================^===[=]
static const char *mkfw_get_clipboard_text(struct mkfw_state *state) {
	(void)state;
	return "";
}

// [=]===^=[ mkfw_enable_drop ]==============================================================^===[=]
static void mkfw_enable_drop(struct mkfw_state *state, uint8_t enable) {
	(void)state;
	(void)enable;
}

// [=]===^=[ mkfw_query_max_gl_version ]=====================================================^===[=]
__attribute__((error("mkfw_query_max_gl_version not available on Emscripten -- WebGL2 is fixed at ES 3.0")))
static int mkfw_query_max_gl_version(int *major, int *minor);

// [=]===^=[ mkfw_attach_context ]===========================================================^===[=]
__attribute__((error("mkfw_attach_context not available on Emscripten -- single-threaded, single context")))
static void mkfw_attach_context(struct mkfw_state *state);

// [=]===^=[ mkfw_detach_context ]===========================================================^===[=]
__attribute__((error("mkfw_detach_context not available on Emscripten -- single-threaded, single context")))
static void mkfw_detach_context(struct mkfw_state *state);

// ============================================================
// Timer stubs (when MKFW_TIMER is defined)
// ============================================================

#ifdef MKFW_TIMER
struct mkfw_timer_handle {
	int32_t dummy;
};

// [=]===^=[ mkfw_timer_init ]================================================================^===[=]
static void mkfw_timer_init(void) {
}

// [=]===^=[ mkfw_timer_shutdown ]============================================================^===[=]
static void mkfw_timer_shutdown(void) {
}

// [=]===^=[ mkfw_timer_new ]================================================================^===[=]
static struct mkfw_timer_handle *mkfw_timer_new(uint64_t interval_ns) {
	(void)interval_ns;
	return 0;
}

// [=]===^=[ mkfw_timer_wait ]===============================================================^===[=]
static uint32_t mkfw_timer_wait(struct mkfw_timer_handle *t) {
	(void)t;
	return 1;
}

// [=]===^=[ mkfw_timer_set_interval ]========================================================^===[=]
static void mkfw_timer_set_interval(struct mkfw_timer_handle *t, uint64_t interval_ns) {
	(void)t;
	(void)interval_ns;
}

// [=]===^=[ mkfw_timer_destroy ]=============================================================^===[=]
static void mkfw_timer_destroy(struct mkfw_timer_handle *t) {
	(void)t;
}
#endif

// ============================================================
// mkfw_run (Emscripten main loop)
// ============================================================

static struct mkfw_state *mkfw_run_window;
static void (*mkfw_run_frame)(struct mkfw_state *);

// [=]===^=[ mkfw_run_loop_body ]============================================================^===[=]
static void mkfw_run_loop_body(void *arg) {
	(void)arg;
	if(mkfw_run_window && mkfw_run_frame) {
		mkfw_run_frame(mkfw_run_window);
	}
}

// [=]===^=[ mkfw_run ]======================================================================^===[=]
static void mkfw_run(struct mkfw_state *state, void (*frame)(struct mkfw_state *)) {
	mkfw_run_window = state;
	mkfw_run_frame = frame;
	emscripten_set_main_loop_arg(mkfw_run_loop_body, 0, 0, 1);
}
