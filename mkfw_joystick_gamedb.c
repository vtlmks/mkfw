// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT


#ifndef MKFW_JOYSTICK
#error "MKFW_JOYSTICK_GAMEDB requires MKFW_JOYSTICK to be defined"
#endif

#include "mkfw_joystick_gamedb.h"

/* Mapping source types */
#define MKFW_GAMEDB_SRC_NONE   0
#define MKFW_GAMEDB_SRC_BUTTON 1
#define MKFW_GAMEDB_SRC_AXIS   2
#define MKFW_GAMEDB_SRC_HAT    3

struct mkfw_gamedb_bind {
	uint8_t type;       /* MKFW_GAMEDB_SRC_* */
	int8_t index;       /* button/axis index, or hat index */
	int8_t hat_mask;    /* hat bitmask: 1=up, 2=right, 4=down, 8=left */
	int8_t axis_invert; /* 1 = negate axis value */
};

struct mkfw_gamedb_mapping {
	uint8_t valid;
	struct mkfw_gamedb_bind buttons[MKFW_GAMEPAD_BUTTON_LAST];
	struct mkfw_gamedb_bind axes[MKFW_GAMEPAD_AXIS_LAST];
};

static struct mkfw_gamedb_mapping mkfw_gamedb_maps[MKFW_JOYSTICK_MAX_PADS];

/* Parse a number from string, advance pointer */
static int mkfw_gamedb_parse_int(const char **p) {
	int val = 0;
	while(**p >= '0' && **p <= '9') {
		val = val * 10 + (**p - '0');
		(*p)++;
	}
	return val;
}

/* Parse a hex digit */
static int mkfw_gamedb_hex_digit(char c) {
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'a' && c <= 'f') return c - 'a' + 10;
	if(c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

/* Parse 16-bit value from GUID at byte offset (little-endian in hex) */
static uint16_t mkfw_gamedb_guid_u16(const char *guid, int byte_offset) {
	int pos = byte_offset * 2;
	int lo_hi = mkfw_gamedb_hex_digit(guid[pos]);
	int lo_lo = mkfw_gamedb_hex_digit(guid[pos + 1]);
	int hi_hi = mkfw_gamedb_hex_digit(guid[pos + 2]);
	int hi_lo = mkfw_gamedb_hex_digit(guid[pos + 3]);
	if(lo_hi < 0 || lo_lo < 0 || hi_hi < 0 || hi_lo < 0) return 0;
	uint16_t lo = (uint16_t)((lo_hi << 4) | lo_lo);
	uint16_t hi = (uint16_t)((hi_hi << 4) | hi_lo);
	return (hi << 8) | lo;
}

/* Parse a source binding like "b0", "a2", "h0.4", "+a3", "-a3" */
static struct mkfw_gamedb_bind mkfw_gamedb_parse_source(const char *src) {
	struct mkfw_gamedb_bind bind = {0};
	const char *p = src;

	int invert = 0;
	if(*p == '+') { p++; }
	else if(*p == '-') { invert = 1; p++; }

	if(*p == 'b') {
		/* Button: b0, b1, etc. */
		p++;
		bind.type = MKFW_GAMEDB_SRC_BUTTON;
		bind.index = (int8_t)mkfw_gamedb_parse_int(&p);
	} else if(*p == 'a') {
		/* Axis: a0, a1, etc. */
		p++;
		bind.type = MKFW_GAMEDB_SRC_AXIS;
		bind.index = (int8_t)mkfw_gamedb_parse_int(&p);
		bind.axis_invert = (int8_t)invert;
	} else if(*p == 'h') {
		/* Hat: h0.1, h0.2, h0.4, h0.8 */
		p++;
		bind.type = MKFW_GAMEDB_SRC_HAT;
		bind.index = (int8_t)mkfw_gamedb_parse_int(&p);
		if(*p == '.') {
			p++;
			bind.hat_mask = (int8_t)mkfw_gamedb_parse_int(&p);
		}
	}

	return bind;
}

/* Map target name to gamepad button/axis enum. Returns -1 if unknown.
   Sets *is_axis = 1 if this is an axis target. */
static int mkfw_gamedb_target_id(const char *name, int len, int *is_axis) {
	*is_axis = 0;

	/* Buttons */
	if(len == 1 && name[0] == 'a') return MKFW_GAMEPAD_A;
	if(len == 1 && name[0] == 'b') return MKFW_GAMEPAD_B;
	if(len == 1 && name[0] == 'x') return MKFW_GAMEPAD_X;
	if(len == 1 && name[0] == 'y') return MKFW_GAMEPAD_Y;
	if(len == 4 && memcmp(name, "back", 4) == 0) return MKFW_GAMEPAD_BACK;
	if(len == 5 && memcmp(name, "start", 5) == 0) return MKFW_GAMEPAD_START;
	if(len == 5 && memcmp(name, "guide", 5) == 0) return MKFW_GAMEPAD_GUIDE;
	if(len == 12 && memcmp(name, "leftshoulder", 12) == 0) return MKFW_GAMEPAD_LEFT_BUMPER;
	if(len == 13 && memcmp(name, "rightshoulder", 13) == 0) return MKFW_GAMEPAD_RIGHT_BUMPER;
	if(len == 9 && memcmp(name, "leftstick", 9) == 0) return MKFW_GAMEPAD_LEFT_THUMB;
	if(len == 10 && memcmp(name, "rightstick", 10) == 0) return MKFW_GAMEPAD_RIGHT_THUMB;
	if(len == 4 && memcmp(name, "dpup", 4) == 0) return MKFW_GAMEPAD_DPAD_UP;
	if(len == 6 && memcmp(name, "dpdown", 6) == 0) return MKFW_GAMEPAD_DPAD_DOWN;
	if(len == 6 && memcmp(name, "dpleft", 6) == 0) return MKFW_GAMEPAD_DPAD_LEFT;
	if(len == 7 && memcmp(name, "dpright", 7) == 0) return MKFW_GAMEPAD_DPAD_RIGHT;

	/* Axes */
	if(len == 5 && memcmp(name, "leftx", 5) == 0) { *is_axis = 1; return MKFW_GAMEPAD_AXIS_LEFT_X; }
	if(len == 5 && memcmp(name, "lefty", 5) == 0) { *is_axis = 1; return MKFW_GAMEPAD_AXIS_LEFT_Y; }
	if(len == 6 && memcmp(name, "rightx", 6) == 0) { *is_axis = 1; return MKFW_GAMEPAD_AXIS_RIGHT_X; }
	if(len == 6 && memcmp(name, "righty", 6) == 0) { *is_axis = 1; return MKFW_GAMEPAD_AXIS_RIGHT_Y; }
	if(len == 11 && memcmp(name, "lefttrigger", 11) == 0) { *is_axis = 1; return MKFW_GAMEPAD_AXIS_LEFT_TRIGGER; }
	if(len == 12 && memcmp(name, "righttrigger", 12) == 0) { *is_axis = 1; return MKFW_GAMEPAD_AXIS_RIGHT_TRIGGER; }

	return -1;
}

/* Search the database for a matching controller and populate the mapping */
static void mkfw_gamedb_lookup(int pad_index) {
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[pad_index];
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[pad_index];

	memset(map, 0, sizeof(*map));

#ifdef _WIN32
	/* XInput always has a standardized layout - hardcode 1:1 mapping.
	   Button indices match mkfw_win32_joystick.c layout exactly. */
	map->valid = 1;

	/* Buttons -> raw button indices */
	map->buttons[MKFW_GAMEPAD_A]            = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 0, 0, 0};
	map->buttons[MKFW_GAMEPAD_B]            = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 1, 0, 0};
	map->buttons[MKFW_GAMEPAD_X]            = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 2, 0, 0};
	map->buttons[MKFW_GAMEPAD_Y]            = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 3, 0, 0};
	map->buttons[MKFW_GAMEPAD_LEFT_BUMPER]  = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 4, 0, 0};
	map->buttons[MKFW_GAMEPAD_RIGHT_BUMPER] = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 5, 0, 0};
	map->buttons[MKFW_GAMEPAD_BACK]         = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 6, 0, 0};
	map->buttons[MKFW_GAMEPAD_START]        = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 7, 0, 0};
	map->buttons[MKFW_GAMEPAD_LEFT_THUMB]   = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 8, 0, 0};
	map->buttons[MKFW_GAMEPAD_RIGHT_THUMB]  = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 9, 0, 0};
	map->buttons[MKFW_GAMEPAD_DPAD_UP]      = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 10, 0, 0};
	map->buttons[MKFW_GAMEPAD_DPAD_DOWN]    = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 11, 0, 0};
	map->buttons[MKFW_GAMEPAD_DPAD_LEFT]    = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 12, 0, 0};
	map->buttons[MKFW_GAMEPAD_DPAD_RIGHT]   = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_BUTTON, 13, 0, 0};
	map->buttons[MKFW_GAMEPAD_GUIDE]        = (struct mkfw_gamedb_bind){0, 0, 0, 0}; /* Not available via XInput */

	/* Axes -> raw axis indices */
	map->axes[MKFW_GAMEPAD_AXIS_LEFT_X]         = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_AXIS, 0, 0, 0};
	map->axes[MKFW_GAMEPAD_AXIS_LEFT_Y]         = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_AXIS, 1, 0, 0};
	map->axes[MKFW_GAMEPAD_AXIS_RIGHT_X]        = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_AXIS, 2, 0, 0};
	map->axes[MKFW_GAMEPAD_AXIS_RIGHT_Y]        = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_AXIS, 3, 0, 0};
	map->axes[MKFW_GAMEPAD_AXIS_LEFT_TRIGGER]    = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_AXIS, 4, 0, 0};
	map->axes[MKFW_GAMEPAD_AXIS_RIGHT_TRIGGER]   = (struct mkfw_gamedb_bind){MKFW_GAMEDB_SRC_AXIS, 5, 0, 0};
	return;
#endif

	/* Linux: search the DB by vendor:product from GUID */
	uint16_t vendor = pad->vendor_id;
	uint16_t product = pad->product_id;
	if(vendor == 0 && product == 0) return;

	const char *line = mkfw_gamecontrollerdb_data;
	while(*line) {
		/* Skip empty lines and comments */
		if(*line == '\n' || *line == '#') {
			while(*line && *line != '\n') line++;
			if(*line == '\n') line++;
			continue;
		}

		/* GUID is first 32 hex chars */
		if(strlen(line) < 34) break;

		/* Extract vendor (bytes 4-5) and product (bytes 8-9) from GUID */
		uint16_t db_vendor = mkfw_gamedb_guid_u16(line, 4);
		uint16_t db_product = mkfw_gamedb_guid_u16(line, 8);

		if(db_vendor == vendor && db_product == product) {
			/* Check platform tag */
			const char *plat = strstr(line, "platform:");
			if(plat) {
				plat += 9;
#ifdef __linux__
				if(strncmp(plat, "Linux", 5) != 0) goto next_line;
#elif _WIN32
				if(strncmp(plat, "Windows", 7) != 0) goto next_line;
#endif
			}

			/* Found a match. Skip GUID and name (after second comma) */
			const char *p = line;
			int commas = 0;
			while(*p && commas < 2) {
				if(*p == ',') commas++;
				p++;
			}

			/* Parse mappings: target:source,target:source,... */
			while(*p && *p != '\n') {
				/* Find target name */
				const char *target_start = p;
				while(*p && *p != ':' && *p != ',' && *p != '\n') p++;
				int target_len = (int)(p - target_start);

				if(*p != ':') {
					/* Skip non-mapping fields (like "platform:Linux") */
					while(*p && *p != ',' && *p != '\n') p++;
					if(*p == ',') p++;
					continue;
				}
				p++; /* skip ':' */

				/* Find source value */
				const char *source_start = p;
				while(*p && *p != ',' && *p != '\n') p++;
				int source_len = (int)(p - source_start);
				if(*p == ',') p++;

				/* Skip platform tag */
				if(target_len == 8 && memcmp(target_start, "platform", 8) == 0) continue;

				/* Parse source */
				char source_buf[32];
				if(source_len >= (int)sizeof(source_buf)) continue;
				memcpy(source_buf, source_start, source_len);
				source_buf[source_len] = 0;

				struct mkfw_gamedb_bind bind = mkfw_gamedb_parse_source(source_buf);

				/* Map to target */
				int is_axis = 0;
				int id = mkfw_gamedb_target_id(target_start, target_len, &is_axis);
				if(id >= 0) {
					if(is_axis) {
						map->axes[id] = bind;
					} else {
						map->buttons[id] = bind;
					}
				}
			}

			map->valid = 1;
			return;
		}

next_line:
		while(*line && *line != '\n') line++;
		if(*line == '\n') line++;
	}
}

/* Ensure mapping is populated for a connected pad (lazy lookup) */
static void mkfw_gamedb_ensure_mapping(int pad_index) {
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[pad_index];
	if(!map->valid && mkfw_joystick_pads[pad_index].connected) {
		mkfw_gamedb_lookup(pad_index);
	}
	/* Clear mapping when pad disconnects */
	if(!mkfw_joystick_pads[pad_index].connected && map->valid) {
		memset(map, 0, sizeof(*map));
	}
}

/* Read a button value through the mapping */
static int mkfw_gamepad_button(int pad_index, int gamepad_button) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0;
	if(gamepad_button < 0 || gamepad_button >= MKFW_GAMEPAD_BUTTON_LAST) return 0;
	if(!mkfw_joystick_pads[pad_index].connected) return 0;

	mkfw_gamedb_ensure_mapping(pad_index);
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[pad_index];
	if(!map->valid) return 0;

	struct mkfw_gamedb_bind *bind = &map->buttons[gamepad_button];
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[pad_index];

	switch(bind->type) {
	case MKFW_GAMEDB_SRC_BUTTON:
		if(bind->index >= 0 && bind->index < pad->button_count)
			return pad->buttons[bind->index];
		return 0;
	case MKFW_GAMEDB_SRC_AXIS:
		if(bind->index >= 0 && bind->index < pad->axis_count) {
			float v = pad->axes[bind->index];
			return (v > 0.5f || v < -0.5f) ? 1 : 0;
		}
		return 0;
	case MKFW_GAMEDB_SRC_HAT:
		/* Hat bits: 1=up, 2=right, 4=down, 8=left */
		{
			uint8_t hat_state = 0;
			if(pad->hat_y < -0.5f) hat_state |= 1;
			if(pad->hat_x >  0.5f) hat_state |= 2;
			if(pad->hat_y >  0.5f) hat_state |= 4;
			if(pad->hat_x < -0.5f) hat_state |= 8;
			return (hat_state & bind->hat_mask) ? 1 : 0;
		}
	}
	return 0;
}

/* Read a button press edge through the mapping */
static int mkfw_gamepad_button_pressed(int pad_index, int gamepad_button) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0;
	if(gamepad_button < 0 || gamepad_button >= MKFW_GAMEPAD_BUTTON_LAST) return 0;
	if(!mkfw_joystick_pads[pad_index].connected) return 0;

	mkfw_gamedb_ensure_mapping(pad_index);
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[pad_index];
	if(!map->valid) return 0;

	struct mkfw_gamedb_bind *bind = &map->buttons[gamepad_button];
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[pad_index];

	if(bind->type == MKFW_GAMEDB_SRC_BUTTON) {
		if(bind->index >= 0 && bind->index < pad->button_count)
			return pad->buttons[bind->index] && !pad->prev_buttons[bind->index];
	}
	return 0;
}

/* Read an axis value through the mapping */
static float mkfw_gamepad_axis(int pad_index, int gamepad_axis) {
	if(pad_index < 0 || pad_index >= MKFW_JOYSTICK_MAX_PADS) return 0.0f;
	if(gamepad_axis < 0 || gamepad_axis >= MKFW_GAMEPAD_AXIS_LAST) return 0.0f;
	if(!mkfw_joystick_pads[pad_index].connected) return 0.0f;

	mkfw_gamedb_ensure_mapping(pad_index);
	struct mkfw_gamedb_mapping *map = &mkfw_gamedb_maps[pad_index];
	if(!map->valid) return 0.0f;

	struct mkfw_gamedb_bind *bind = &map->axes[gamepad_axis];
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[pad_index];

	if(bind->type == MKFW_GAMEDB_SRC_AXIS) {
		if(bind->index >= 0 && bind->index < pad->axis_count) {
			float v = pad->axes[bind->index];
			return bind->axis_invert ? -v : v;
		}
	} else if(bind->type == MKFW_GAMEDB_SRC_BUTTON) {
		/* Button mapped to axis (e.g. triggers as buttons) */
		if(bind->index >= 0 && bind->index < pad->button_count) {
			return pad->buttons[bind->index] ? 1.0f : 0.0f;
		}
	}
	return 0.0f;
}
