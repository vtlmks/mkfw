// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

// In unity mode this file is #included from mkfw_joystick.h.
// In library mode it is compiled standalone; the include below pulls
// in the public types, MKFW_API / MKFW_VAR macros, and the cross-TU
// variable declarations.
#include "mkfw_joystick.h"

#include <xinput.h>

/* Cross-TU storage: declared as MKFW_VAR in mkfw_joystick.h.  In unity
 * mode the header's static declaration is the definition; in library
 * mode the storage block below provides it. */
#if defined(MKFW_BUILD_SHARED) || defined(MKFW_BUILD_LIBRARY)
struct mkfw_joystick_pad      mkfw_joystick_pads[MKFW_JOYSTICK_MAX_PADS];
mkfw_joystick_callback_t      mkfw_joystick_cb;
#endif

static uint8_t mkfw_joystick_initialized;

// [=]===^=[ mkfw_joystick_apply_deadzone ]=======================================================[=]
static float mkfw_joystick_apply_deadzone(int16_t value, int16_t deadzone) {
	if(value > deadzone) {
		return (float)(value - deadzone) / (float)(32767 - deadzone);
	} else if(value < -deadzone) {
		return (float)(value + deadzone) / (float)(32767 - deadzone);
	}
	return 0.0f;
}

// [=]===^=[ mkfw_joystick_normalize_trigger ]====================================================[=]
static float mkfw_joystick_normalize_trigger(uint8_t value, uint8_t threshold) {
	if(value > threshold) {
		return (float)(value - threshold) / (float)(255 - threshold);
	}
	return 0.0f;
}

// [=]===^=[ mkfw_joystick_init ]=================================================================[=]
MKFW_API void mkfw_joystick_init(void) {
	memset(mkfw_joystick_pads, 0, sizeof(mkfw_joystick_pads));
	mkfw_joystick_cb = 0;
	mkfw_joystick_initialized = 1;
}

// [=]===^=[ mkfw_joystick_shutdown ]=============================================================[=]
MKFW_API void mkfw_joystick_shutdown(void) {
	memset(mkfw_joystick_pads, 0, sizeof(mkfw_joystick_pads));
	mkfw_joystick_cb = 0;
	mkfw_joystick_initialized = 0;
}

// [=]===^=[ mkfw_joystick_update ]===============================================================[=]
MKFW_API void mkfw_joystick_update(void) {
	if(!mkfw_joystick_initialized) {
		return;
	}

	for(int i = 0; i < MKFW_JOYSTICK_MAX_PADS; i++) {
		struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[i];

		memcpy(pad->prev_buttons, pad->buttons, sizeof(pad->buttons));
		pad->was_connected = pad->connected;

		XINPUT_STATE state;
		DWORD result = XInputGetState(i, &state);

		if(result == ERROR_SUCCESS) {
			pad->connected = 1;

			if(!pad->was_connected) {
				snprintf(pad->name, MKFW_JOYSTICK_NAME_LEN, "XInput Controller %d", i);
				pad->vendor_id = 0;
				pad->product_id = 0;
				pad->button_count = 14;
				pad->axis_count = 6;
			}

			/* Buttons: 0=A, 1=B, 2=X, 3=Y, 4=LB, 5=RB, 6=Back, 7=Start,
			   8=LStick, 9=RStick, 10=DPadUp, 11=DPadDown, 12=DPadLeft, 13=DPadRight */
			WORD w = state.Gamepad.wButtons;
			pad->buttons[0]  = !!(w & XINPUT_GAMEPAD_A);
			pad->buttons[1]  = !!(w & XINPUT_GAMEPAD_B);
			pad->buttons[2]  = !!(w & XINPUT_GAMEPAD_X);
			pad->buttons[3]  = !!(w & XINPUT_GAMEPAD_Y);
			pad->buttons[4]  = !!(w & XINPUT_GAMEPAD_LEFT_SHOULDER);
			pad->buttons[5]  = !!(w & XINPUT_GAMEPAD_RIGHT_SHOULDER);
			pad->buttons[6]  = !!(w & XINPUT_GAMEPAD_BACK);
			pad->buttons[7]  = !!(w & XINPUT_GAMEPAD_START);
			pad->buttons[8]  = !!(w & XINPUT_GAMEPAD_LEFT_THUMB);
			pad->buttons[9]  = !!(w & XINPUT_GAMEPAD_RIGHT_THUMB);
			pad->buttons[10] = !!(w & XINPUT_GAMEPAD_DPAD_UP);
			pad->buttons[11] = !!(w & XINPUT_GAMEPAD_DPAD_DOWN);
			pad->buttons[12] = !!(w & XINPUT_GAMEPAD_DPAD_LEFT);
			pad->buttons[13] = !!(w & XINPUT_GAMEPAD_DPAD_RIGHT);

			/* Axes: 0=LX, 1=LY, 2=RX, 3=RY, 4=LTrigger, 5=RTrigger */
			pad->axes[0] = mkfw_joystick_apply_deadzone(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			pad->axes[1] = mkfw_joystick_apply_deadzone(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			pad->axes[2] = mkfw_joystick_apply_deadzone(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			pad->axes[3] = mkfw_joystick_apply_deadzone(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			pad->axes[4] = mkfw_joystick_normalize_trigger(state.Gamepad.bLeftTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
			pad->axes[5] = mkfw_joystick_normalize_trigger(state.Gamepad.bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

			/* D-pad as hat (screen coordinates: Y-down) */
			pad->hat_x = 0.0f;
			pad->hat_y = 0.0f;
			if(w & XINPUT_GAMEPAD_DPAD_LEFT) {
				pad->hat_x = -1.0f;
			}
			if(w & XINPUT_GAMEPAD_DPAD_RIGHT) {
				pad->hat_x = 1.0f;
			}
			if(w & XINPUT_GAMEPAD_DPAD_UP) {
				pad->hat_y = -1.0f;
			}
			if(w & XINPUT_GAMEPAD_DPAD_DOWN) {
				pad->hat_y = 1.0f;
			}

			if(!pad->was_connected && mkfw_joystick_cb) {
				mkfw_joystick_cb(i, 1);
			}
		} else {
			if(pad->was_connected && mkfw_joystick_cb) {
				mkfw_joystick_cb(i, 0);
			}
			memset(pad->buttons, 0, sizeof(pad->buttons));
			memset(pad->axes, 0, sizeof(pad->axes));
			pad->hat_x = 0.0f;
			pad->hat_y = 0.0f;
			pad->connected = 0;
			pad->button_count = 0;
			pad->axis_count = 0;
			pad->name[0] = 0;
		}
	}
}

// [=]===^=[ mkfw_joystick_get_battery ]==========================================================[=]
MKFW_API float mkfw_joystick_get_battery(uint32_t pad_index) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS || !mkfw_joystick_pads[pad_index].connected) {
		return -1.0f;
	}
	XINPUT_BATTERY_INFORMATION info;
	if(XInputGetBatteryInformation((DWORD)pad_index, BATTERY_DEVTYPE_GAMEPAD, &info) != ERROR_SUCCESS) {
		return -1.0f;
	}
	// Wired and unknown/disconnected pads report -1: caller can tell "no
	// battery to monitor" from "battery present, X% remaining".
	if(info.BatteryType != BATTERY_TYPE_ALKALINE && info.BatteryType != BATTERY_TYPE_NIMH) {
		return -1.0f;
	}
	switch(info.BatteryLevel) {
		case BATTERY_LEVEL_EMPTY:  return 0.0f;
		case BATTERY_LEVEL_LOW:    return 1.0f / 3.0f;
		case BATTERY_LEVEL_MEDIUM: return 2.0f / 3.0f;
		case BATTERY_LEVEL_FULL:   return 1.0f;
	}
	return -1.0f;
}

// [=]===^=[ mkfw_joystick_rumble_stop_proc ]=====================================================[=]
// Threadpool timer callback that zeros vibration after the
// time-limited rumble_platform duration expires.
static VOID CALLBACK mkfw_joystick_rumble_stop_proc(PTP_CALLBACK_INSTANCE inst, PVOID context, PTP_TIMER timer) {
	(void)inst;
	DWORD pad_index = (DWORD)(UINT_PTR)context;
	XINPUT_VIBRATION zero = {0, 0};
	XInputSetState(pad_index, &zero);
	CloseThreadpoolTimer(timer);
}

// [=]===^=[ mkfw_joystick_rumble_set_platform ]==================================================[=]
MKFW_API void mkfw_joystick_rumble_set_platform(uint32_t pad_index, float low_freq, float high_freq) {
	XINPUT_VIBRATION vibration;
	vibration.wLeftMotorSpeed  = (WORD)(low_freq  * 65535.0f);
	vibration.wRightMotorSpeed = (WORD)(high_freq * 65535.0f);
	XInputSetState(pad_index, &vibration);
}

// [=]===^=[ mkfw_joystick_rumble_platform ]======================================================[=]
MKFW_API void mkfw_joystick_rumble_platform(uint32_t pad_index, float low_freq, float high_freq, uint32_t duration_ms) {
	mkfw_joystick_rumble_set_platform(pad_index, low_freq, high_freq);
	if(duration_ms == 0) {
		return;
	}
	PTP_TIMER timer = CreateThreadpoolTimer(mkfw_joystick_rumble_stop_proc, (PVOID)(UINT_PTR)pad_index, 0);
	if(!timer) {
		return;
	}
	// FILETIME is in 100ns ticks; negative = relative.
	ULARGE_INTEGER due;
	due.QuadPart = (ULONGLONG)(-(LONGLONG)duration_ms * 10000LL);
	FILETIME ft;
	ft.dwLowDateTime  = due.LowPart;
	ft.dwHighDateTime = due.HighPart;
	SetThreadpoolTimer(timer, &ft, 0, 0);
}
