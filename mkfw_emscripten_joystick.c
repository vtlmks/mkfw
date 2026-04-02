// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#include <emscripten/html5.h>

static struct mkfw_joystick_pad mkfw_joystick_pads[MKFW_JOYSTICK_MAX_PADS];
static mkfw_joystick_callback_t mkfw_joystick_cb;
static uint8_t mkfw_joystick_initialized;

// [=]===^=[ mkfw_em_on_gamepad_connected ]=============================================================^===[=]
static EM_BOOL mkfw_em_on_gamepad_connected(int event_type, const EmscriptenGamepadEvent *e, void *user_data) {
	(void)user_data;
	int32_t idx = e->index;
	if(idx < 0 || idx >= MKFW_JOYSTICK_MAX_PADS) {
		return EM_FALSE;
	}

	if(event_type == EMSCRIPTEN_EVENT_GAMEPADCONNECTED) {
		memset(&mkfw_joystick_pads[idx], 0, sizeof(struct mkfw_joystick_pad));
		mkfw_joystick_pads[idx].connected = 1;
		snprintf(mkfw_joystick_pads[idx].name, MKFW_JOYSTICK_NAME_LEN, "%s", e->id);

		int32_t nb = e->numButtons;
		if(nb > MKFW_JOYSTICK_MAX_BUTTONS) {
			nb = MKFW_JOYSTICK_MAX_BUTTONS;
		}
		mkfw_joystick_pads[idx].button_count = nb;

		int32_t na = e->numAxes;
		if(na > MKFW_JOYSTICK_MAX_AXES) {
			na = MKFW_JOYSTICK_MAX_AXES;
		}
		mkfw_joystick_pads[idx].axis_count = na;

		if(mkfw_joystick_cb) {
			mkfw_joystick_cb(idx, 1);
		}
	} else {
		mkfw_joystick_pads[idx].connected = 0;
		if(mkfw_joystick_cb) {
			mkfw_joystick_cb(idx, 0);
		}
	}

	return EM_TRUE;
}

// [=]===^=[ mkfw_joystick_init ]================================================================^===[=]
static void mkfw_joystick_init(void) {
	if(mkfw_joystick_initialized) {
		return;
	}
	mkfw_joystick_initialized = 1;
	memset(mkfw_joystick_pads, 0, sizeof(mkfw_joystick_pads));

	emscripten_set_gamepadconnected_callback(0, EM_TRUE, mkfw_em_on_gamepad_connected);
	emscripten_set_gamepaddisconnected_callback(0, EM_TRUE, mkfw_em_on_gamepad_connected);
}

// [=]===^=[ mkfw_joystick_shutdown ]============================================================^===[=]
static void mkfw_joystick_shutdown(void) {
	if(!mkfw_joystick_initialized) {
		return;
	}
	mkfw_joystick_initialized = 0;
	memset(mkfw_joystick_pads, 0, sizeof(mkfw_joystick_pads));
}

// [=]===^=[ mkfw_joystick_update ]=============================================================^===[=]
static void mkfw_joystick_update(void) {
	if(!mkfw_joystick_initialized) {
		return;
	}

	emscripten_sample_gamepad_data();

	int32_t num = emscripten_get_num_gamepads();
	if(num > MKFW_JOYSTICK_MAX_PADS) {
		num = MKFW_JOYSTICK_MAX_PADS;
	}

	for(int32_t i = 0; i < MKFW_JOYSTICK_MAX_PADS; ++i) {
		// Save previous button state for edge detection
		memcpy(mkfw_joystick_pads[i].prev_buttons, mkfw_joystick_pads[i].buttons, sizeof(mkfw_joystick_pads[i].buttons));

		EmscriptenGamepadEvent ge;
		if(i < num && emscripten_get_gamepad_status(i, &ge) == EMSCRIPTEN_RESULT_SUCCESS && ge.connected) {
			uint8_t was_connected = mkfw_joystick_pads[i].connected;
			mkfw_joystick_pads[i].connected = 1;

			if(!was_connected) {
				snprintf(mkfw_joystick_pads[i].name, MKFW_JOYSTICK_NAME_LEN, "%s", ge.id);
				int32_t nb = ge.numButtons;
				if(nb > MKFW_JOYSTICK_MAX_BUTTONS) {
					nb = MKFW_JOYSTICK_MAX_BUTTONS;
				}
				mkfw_joystick_pads[i].button_count = nb;
				int32_t na = ge.numAxes;
				if(na > MKFW_JOYSTICK_MAX_AXES) {
					na = MKFW_JOYSTICK_MAX_AXES;
				}
				mkfw_joystick_pads[i].axis_count = na;
				if(mkfw_joystick_cb) {
					mkfw_joystick_cb(i, 1);
				}
			}

			// Read buttons
			for(int32_t b = 0; b < mkfw_joystick_pads[i].button_count; ++b) {
				mkfw_joystick_pads[i].buttons[b] = ge.digitalButton[b] ? 1 : 0;
			}

			// Read axes
			for(int32_t a = 0; a < mkfw_joystick_pads[i].axis_count; ++a) {
				mkfw_joystick_pads[i].axes[a] = (float)ge.axis[a];
			}

			// Standard gamepad mapping: axes 0/1 = left stick, 2/3 = right stick
			// D-pad is typically buttons 12-15 in standard mapping
			mkfw_joystick_pads[i].hat_x = 0.0f;
			mkfw_joystick_pads[i].hat_y = 0.0f;
			if(ge.mapping[0] == 's') {
				// "standard" mapping - d-pad is buttons 12-15
				if(ge.numButtons > 15) {
					if(ge.digitalButton[14]) {
						mkfw_joystick_pads[i].hat_x = -1.0f;
					}
					if(ge.digitalButton[15]) {
						mkfw_joystick_pads[i].hat_x = 1.0f;
					}
					if(ge.digitalButton[12]) {
						mkfw_joystick_pads[i].hat_y = -1.0f;
					}
					if(ge.digitalButton[13]) {
						mkfw_joystick_pads[i].hat_y = 1.0f;
					}
				}
			}
		} else {
			if(mkfw_joystick_pads[i].connected) {
				mkfw_joystick_pads[i].connected = 0;
				if(mkfw_joystick_cb) {
					mkfw_joystick_cb(i, 0);
				}
			}
		}
	}
}

// [=]===^=[ mkfw_joystick_rumble_platform ]====================================================^===[=]
static void mkfw_joystick_rumble_platform(int pad_index, float low_freq, float high_freq, uint32_t duration_ms) {
	(void)pad_index;
	(void)low_freq;
	(void)high_freq;
	(void)duration_ms;
}
