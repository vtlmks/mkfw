// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#include "mkfw_joystick.h"

#include "mkfw_joystick_gamedb.h"
#include <string.h>

#define MKFW_GAMEDB_TYPE_MASK 0xc0
#define MKFW_GAMEDB_SRC_BUTTON 0x40
#define MKFW_GAMEDB_SRC_AXIS 0x80
#define MKFW_GAMEDB_SRC_HAT 0xc0
#define MKFW_GAMEDB_AXIS_INVERT 0x20
#define MKFW_GAMEDB_INDEX_MASK 0x1f

struct mkfw_gamedb_mapping {
	uint16_t vendor_id;
	uint16_t product_id;
	uint8_t valid;
	uint8_t buttons[MKFW_GAMEPAD_BUTTON_LAST];
	uint8_t axes[MKFW_GAMEPAD_AXIS_LAST];
};

static struct mkfw_gamedb_mapping mkfw_gamedb_maps[MKFW_JOYSTICK_MAX_PADS];

// [=]===^=[ mkfw_gamedb_lookup ]=================================================================[=]
static void mkfw_gamedb_lookup(uint32_t pad_index) {
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[pad_index];
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[pad_index];
	memset(map, 0, sizeof(*map));

#ifdef _WIN32
	*map = (struct mkfw_gamedb_mapping){
		.valid = 1,
		.buttons = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x00, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d},
		.axes = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85}
	};
	map->vendor_id = pad->vendor_id;
	map->product_id = pad->product_id;
#else
	for(size_t i = 0; i < sizeof(mkfw_gamecontrollerdb_data) / sizeof(mkfw_gamecontrollerdb_data[0]); ++i) {
		if(mkfw_gamecontrollerdb_data[i].vendor_id == pad->vendor_id && mkfw_gamecontrollerdb_data[i].product_id == pad->product_id) {
			memcpy(map->buttons, mkfw_gamecontrollerdb_data[i].buttons, sizeof(map->buttons));
			memcpy(map->axes, mkfw_gamecontrollerdb_data[i].axes, sizeof(map->axes));
			map->vendor_id = pad->vendor_id;
			map->product_id = pad->product_id;
			map->valid = 1;
			return;

		}
	}
#endif
}

// [=]===^=[ mkfw_gamedb_ensure_mapping ]=========================================================[=]
static void mkfw_gamedb_ensure_mapping(uint32_t pad_index) {
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[pad_index];
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[pad_index];

	if(pad->connected && (!map->valid || map->vendor_id != pad->vendor_id || map->product_id != pad->product_id)) {
		mkfw_gamedb_lookup(pad_index);

	}

	if(!pad->connected && map->valid) {
		memset(map, 0, sizeof(*map));

	}
}

// [=]===^=[ mkfw_gamepad_get_button ]================================================================[=]
MKFW_API uint32_t mkfw_gamepad_get_button(uint32_t pad_index, uint32_t gamepad_button) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0;

	}

	if(gamepad_button >= MKFW_GAMEPAD_BUTTON_LAST) {
		return 0;

	}

	if(!mkfw_joystick_pads[pad_index].connected) {
		return 0;

	}

	mkfw_gamedb_ensure_mapping(pad_index);
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[pad_index];

	if(!map->valid) {
		return 0;

	}

	uint8_t bind = map->buttons[gamepad_button];
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[pad_index];

	switch(bind & MKFW_GAMEDB_TYPE_MASK) {
		case MKFW_GAMEDB_SRC_BUTTON: {
			if((bind & MKFW_GAMEDB_INDEX_MASK) < pad->button_count) {
				return pad->buttons[bind & MKFW_GAMEDB_INDEX_MASK];

			}

			return 0;

		} break;

		case MKFW_GAMEDB_SRC_AXIS: {
			if((bind & MKFW_GAMEDB_INDEX_MASK) < pad->axis_count) {
				float v = pad->axes[bind & MKFW_GAMEDB_INDEX_MASK];

				return (v > 0.5f || v < -0.5f) ? 1 : 0;

			}

			return 0;

		} break;

		case MKFW_GAMEDB_SRC_HAT: {
			uint8_t hat_state = 0;

			if(pad->hat_y < -0.5f) {
				hat_state |= 1;

			}

			if(pad->hat_x > 0.5f) {
				hat_state |= 2;

			}

			if(pad->hat_y > 0.5f) {
				hat_state |= 4;

			}

			if(pad->hat_x < -0.5f) {
				hat_state |= 8;

			}

			return (hat_state & (bind & MKFW_GAMEDB_INDEX_MASK)) ? 1 : 0;

		} break;
	}

	return 0;

}

// [=]===^=[ mkfw_gamepad_is_button_pressed ]========================================================[=]
MKFW_API uint32_t mkfw_gamepad_is_button_pressed(uint32_t pad_index, uint32_t gamepad_button) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0;

	}

	if(gamepad_button >= MKFW_GAMEPAD_BUTTON_LAST) {
		return 0;

	}

	if(!mkfw_joystick_pads[pad_index].connected) {
		return 0;

	}

	mkfw_gamedb_ensure_mapping(pad_index);
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[pad_index];

	if(!map->valid) {
		return 0;

	}

	uint8_t bind = map->buttons[gamepad_button];
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[pad_index];

	if((bind & MKFW_GAMEDB_TYPE_MASK) == MKFW_GAMEDB_SRC_BUTTON) {
		if((bind & MKFW_GAMEDB_INDEX_MASK) < pad->button_count) {
			return pad->buttons[bind & MKFW_GAMEDB_INDEX_MASK] && !pad->prev_buttons[bind & MKFW_GAMEDB_INDEX_MASK];

		}
	}

	return 0;

}

// [=]===^=[ mkfw_gamepad_get_axis ]==================================================================[=]
MKFW_API float mkfw_gamepad_get_axis(uint32_t pad_index, uint32_t gamepad_axis) {
	if(pad_index >= MKFW_JOYSTICK_MAX_PADS) {
		return 0.0f;

	}

	if(gamepad_axis >= MKFW_GAMEPAD_AXIS_LAST) {
		return 0.0f;

	}

	if(!mkfw_joystick_pads[pad_index].connected) {
		return 0.0f;

	}

	mkfw_gamedb_ensure_mapping(pad_index);
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[pad_index];

	if(!map->valid) {
		return 0.0f;

	}

	uint8_t bind = map->axes[gamepad_axis];
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[pad_index];

	if((bind & MKFW_GAMEDB_TYPE_MASK) == MKFW_GAMEDB_SRC_AXIS) {
		if((bind & MKFW_GAMEDB_INDEX_MASK) < pad->axis_count) {
			float v = pad->axes[bind & MKFW_GAMEDB_INDEX_MASK];
			return bind & MKFW_GAMEDB_AXIS_INVERT ? -v : v;

		}
	} else if((bind & MKFW_GAMEDB_TYPE_MASK) == MKFW_GAMEDB_SRC_BUTTON) {
		if((bind & MKFW_GAMEDB_INDEX_MASK) < pad->button_count) {
			return pad->buttons[bind & MKFW_GAMEDB_INDEX_MASK] ? 1.0f : 0.0f;

		}
	}

	return 0.0f;

}
