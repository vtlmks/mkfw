// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT


#include <linux/input.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#define MKFW_JOYSTICK_DEVPATH_LEN 280

struct mkfw_joystick_linux_pad {
	int fd;
	char devpath[MKFW_JOYSTICK_DEVPATH_LEN];

	/* Axis calibration from EVIOCGABS */
	struct {
		int code;
		int32_t minimum;
		int32_t maximum;
	} axis_map[MKFW_JOYSTICK_MAX_AXES];

	/* Button evdev code for each button index */
	int button_codes[MKFW_JOYSTICK_MAX_BUTTONS];

	/* Whether hat axes exist (vs D-pad buttons) */
	uint8_t has_hat;
};

static struct mkfw_joystick_pad mkfw_joystick_pads[MKFW_JOYSTICK_MAX_PADS];
static struct mkfw_joystick_linux_pad mkfw_joystick_linux[MKFW_JOYSTICK_MAX_PADS];
static mkfw_joystick_callback_t mkfw_joystick_cb;
static int mkfw_inotify_fd = -1;
static int mkfw_inotify_wd = -1;
static uint8_t mkfw_joystick_initialized;
static int mkfw_joystick_rescan_countdown;

#define MKFW_JOYSTICK_BIT_TEST(array, bit) \
	((array[(bit) / (sizeof(unsigned long) * 8)] >> ((bit) % (sizeof(unsigned long) * 8))) & 1UL)

static int mkfw_joystick_is_gamepad(int fd) {
	unsigned long evbits[(EV_MAX + 1 + sizeof(unsigned long) * 8 - 1) / (sizeof(unsigned long) * 8)] = {0};
	unsigned long keybits[(KEY_MAX + 1 + sizeof(unsigned long) * 8 - 1) / (sizeof(unsigned long) * 8)] = {0};

	if(ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits) < 0) return 0;

	/* Must support EV_ABS and EV_KEY */
	if(!MKFW_JOYSTICK_BIT_TEST(evbits, EV_ABS)) return 0;
	if(!MKFW_JOYSTICK_BIT_TEST(evbits, EV_KEY)) return 0;

	if(ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) < 0) return 0;

	/* Check for gamepad buttons (BTN_GAMEPAD = BTN_SOUTH = 0x130) */
	for(int code = BTN_GAMEPAD; code < BTN_GAMEPAD + 16; code++) {
		if(MKFW_JOYSTICK_BIT_TEST(keybits, code)) return 1;
	}

	return 0;
}

static float mkfw_joystick_normalize_axis(int32_t value, int32_t min, int32_t max) {
	if(max == min) return 0.0f;
	return 2.0f * (float)(value - min) / (float)(max - min) - 1.0f;
}

static int mkfw_joystick_find_free_slot(void) {
	for(int i = 0; i < MKFW_JOYSTICK_MAX_PADS; i++) {
		if(mkfw_joystick_linux[i].fd < 0) return i;
	}
	return -1;
}

static int mkfw_joystick_find_by_devpath(const char *devpath) {
	for(int i = 0; i < MKFW_JOYSTICK_MAX_PADS; i++) {
		if(mkfw_joystick_linux[i].fd >= 0 &&
		   strcmp(mkfw_joystick_linux[i].devpath, devpath) == 0) {
			return i;
		}
	}
	return -1;
}

static void mkfw_joystick_try_open(const char *devpath) {
	/* Already open? */
	if(mkfw_joystick_find_by_devpath(devpath) >= 0) return;

	int slot = mkfw_joystick_find_free_slot();
	if(slot < 0) return;

	int fd = open(devpath, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if(fd < 0) return;

	if(!mkfw_joystick_is_gamepad(fd)) {
		close(fd);
		return;
	}

	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[slot];
	struct mkfw_joystick_linux_pad *lpad = &mkfw_joystick_linux[slot];

	memset(pad, 0, sizeof(*pad));
	memset(lpad, 0, sizeof(*lpad));
	lpad->fd = fd;
	snprintf(lpad->devpath, MKFW_JOYSTICK_DEVPATH_LEN, "%s", devpath);

	/* Get device name */
	if(ioctl(fd, EVIOCGNAME(MKFW_JOYSTICK_NAME_LEN), pad->name) < 0) {
		snprintf(pad->name, MKFW_JOYSTICK_NAME_LEN, "Unknown Gamepad");
	}

	/* Get vendor/product IDs */
	struct input_id id;
	if(ioctl(fd, EVIOCGID, &id) == 0) {
		pad->vendor_id = id.vendor;
		pad->product_id = id.product;
	}

	/* Enumerate axes */
	unsigned long absbits[(ABS_MAX + 1 + sizeof(unsigned long) * 8 - 1) / (sizeof(unsigned long) * 8)] = {0};
	ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits);

	/* Axis codes in ascending order (0x00-0x05) to match SDL's sequential scan. */
	static const int axis_codes[] = { ABS_X, ABS_Y, ABS_Z, ABS_RX, ABS_RY, ABS_RZ };
	pad->axis_count = 0;
	for(int a = 0; a < 6 && pad->axis_count < MKFW_JOYSTICK_MAX_AXES; a++) {
		if(MKFW_JOYSTICK_BIT_TEST(absbits, axis_codes[a])) {
			struct input_absinfo absinfo;
			if(ioctl(fd, EVIOCGABS(axis_codes[a]), &absinfo) == 0) {
				int idx = pad->axis_count++;
				lpad->axis_map[idx].code = axis_codes[a];
				lpad->axis_map[idx].minimum = absinfo.minimum;
				lpad->axis_map[idx].maximum = absinfo.maximum;
			}
		}
	}

	/* Check for hat (D-pad as axes) */
	lpad->has_hat = MKFW_JOYSTICK_BIT_TEST(absbits, ABS_HAT0X) &&
	                MKFW_JOYSTICK_BIT_TEST(absbits, ABS_HAT0Y);

	/* Enumerate buttons */
	unsigned long keybits[(KEY_MAX + 1 + sizeof(unsigned long) * 8 - 1) / (sizeof(unsigned long) * 8)] = {0};
	ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);

	/* Button codes in ascending order (0x130-0x13e) to match SDL's
	   sequential scan.  BTN_C and BTN_Z are included because hid-generic
	   maps all HID buttons sequentially through the full range. */
	static const int btn_codes[] = {
		BTN_SOUTH, BTN_EAST, BTN_C, BTN_NORTH, BTN_WEST, BTN_Z,
		BTN_TL, BTN_TR, BTN_TL2, BTN_TR2,
		BTN_SELECT, BTN_START, BTN_MODE, BTN_THUMBL, BTN_THUMBR
	};

	pad->button_count = 0;
	for(int b = 0; b < (int)(sizeof(btn_codes) / sizeof(btn_codes[0])) && pad->button_count < MKFW_JOYSTICK_MAX_BUTTONS; b++) {
		if(MKFW_JOYSTICK_BIT_TEST(keybits, btn_codes[b])) {
			int idx = pad->button_count++;
			lpad->button_codes[idx] = btn_codes[b];
		}
	}

	/* D-pad buttons (BTN_DPAD_* or BTN_TRIGGER_HAPPY*) are handled
	   in the event loop by converting them to hat_x/hat_y values,
	   so they work with the hat-based GAMEDB mappings (dpup:h0.1 etc). */

	pad->connected = 1;

	if(mkfw_joystick_cb) {
		mkfw_joystick_cb(slot, 1);
	}
}

static void mkfw_joystick_close_pad(int slot) {
	struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[slot];
	struct mkfw_joystick_linux_pad *lpad = &mkfw_joystick_linux[slot];

	if(lpad->fd >= 0) {
		close(lpad->fd);
		lpad->fd = -1;
	}

	pad->connected = 0;
	memset(pad->buttons, 0, sizeof(pad->buttons));
	memset(pad->axes, 0, sizeof(pad->axes));
	pad->hat_x = 0.0f;
	pad->hat_y = 0.0f;
	pad->button_count = 0;
	pad->axis_count = 0;
	pad->name[0] = 0;
}

static void mkfw_joystick_scan_devices(void) {
	DIR *dir = opendir("/dev/input");
	if(!dir) return;

	struct dirent *entry;
	while((entry = readdir(dir)) != 0) {
		if(strncmp(entry->d_name, "event", 5) != 0) continue;

		char path[MKFW_JOYSTICK_DEVPATH_LEN];
		snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);
		mkfw_joystick_try_open(path);
	}

	closedir(dir);
}

static void mkfw_joystick_init(void) {
	memset(mkfw_joystick_pads, 0, sizeof(mkfw_joystick_pads));
	memset(mkfw_joystick_linux, 0, sizeof(mkfw_joystick_linux));
	mkfw_joystick_cb = 0;

	for(int i = 0; i < MKFW_JOYSTICK_MAX_PADS; i++) {
		mkfw_joystick_linux[i].fd = -1;
	}

	/* Set up inotify for hotplug */
	mkfw_inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	if(mkfw_inotify_fd >= 0) {
		mkfw_inotify_wd = inotify_add_watch(mkfw_inotify_fd, "/dev/input", IN_CREATE | IN_DELETE);
	}

	mkfw_joystick_scan_devices();
	mkfw_joystick_initialized = 1;
}

static void mkfw_joystick_shutdown(void) {
	for(int i = 0; i < MKFW_JOYSTICK_MAX_PADS; i++) {
		if(mkfw_joystick_linux[i].fd >= 0) {
			close(mkfw_joystick_linux[i].fd);
			mkfw_joystick_linux[i].fd = -1;
		}
	}

	if(mkfw_inotify_wd >= 0) {
		inotify_rm_watch(mkfw_inotify_fd, mkfw_inotify_wd);
		mkfw_inotify_wd = -1;
	}
	if(mkfw_inotify_fd >= 0) {
		close(mkfw_inotify_fd);
		mkfw_inotify_fd = -1;
	}

	memset(mkfw_joystick_pads, 0, sizeof(mkfw_joystick_pads));
	memset(mkfw_joystick_linux, 0, sizeof(mkfw_joystick_linux));
	mkfw_joystick_initialized = 0;
}

static void mkfw_joystick_check_hotplug(void) {
	if(mkfw_inotify_fd < 0) return;

	char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
	ssize_t len = read(mkfw_inotify_fd, buf, sizeof(buf));
	if(len <= 0) return;

	char *ptr = buf;
	while(ptr < buf + len) {
		struct inotify_event *event = (struct inotify_event *)ptr;

		if(event->len > 0 && strncmp(event->name, "event", 5) == 0) {
			char path[MKFW_JOYSTICK_DEVPATH_LEN];
			snprintf(path, sizeof(path), "/dev/input/%s", event->name);

			if(event->mask & IN_CREATE) {
				/* Device file may not be ready yet (udev permissions).
				   Try now, and schedule rescans for the next ~1 second. */
				mkfw_joystick_try_open(path);
				mkfw_joystick_rescan_countdown = 60;
			} else if(event->mask & IN_DELETE) {
				int slot = mkfw_joystick_find_by_devpath(path);
				if(slot >= 0) {
					mkfw_joystick_close_pad(slot);
					if(mkfw_joystick_cb) {
						mkfw_joystick_cb(slot, 0);
					}
				}
			}
		}

		ptr += sizeof(struct inotify_event) + event->len;
	}
}

static void mkfw_joystick_update(void) {
	if(!mkfw_joystick_initialized) return;

	mkfw_joystick_check_hotplug();

	/* Retry device scan after inotify CREATE events, since the device
	   may not have been ready (permissions/initialization) on first try. */
	if(mkfw_joystick_rescan_countdown > 0) {
		mkfw_joystick_rescan_countdown--;
		mkfw_joystick_scan_devices();
	}

	for(int i = 0; i < MKFW_JOYSTICK_MAX_PADS; i++) {
		struct mkfw_joystick_pad *pad = &mkfw_joystick_pads[i];
		struct mkfw_joystick_linux_pad *lpad = &mkfw_joystick_linux[i];

		memcpy(pad->prev_buttons, pad->buttons, sizeof(pad->buttons));
		pad->was_connected = pad->connected;

		if(lpad->fd < 0) continue;

		struct input_event ev;
		int had_error = 0;

		while(read(lpad->fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
			if(ev.type == EV_KEY) {
				for(int b = 0; b < pad->button_count; b++) {
					if(lpad->button_codes[b] == (int)ev.code) {
						pad->buttons[b] = ev.value ? 1 : 0;
						break;
					}
				}
				/* D-pad buttons → hat values.  Handles both BTN_DPAD_*
				   (hid-generic) and BTN_TRIGGER_HAPPY* (xpad dpad_to_buttons). */
				if(!lpad->has_hat) {
					if(ev.code == BTN_DPAD_UP || ev.code == BTN_TRIGGER_HAPPY3) {
						pad->hat_y = ev.value ? -1.0f : 0.0f;
					} else if(ev.code == BTN_DPAD_DOWN || ev.code == BTN_TRIGGER_HAPPY4) {
						pad->hat_y = ev.value ? 1.0f : 0.0f;
					} else if(ev.code == BTN_DPAD_LEFT || ev.code == BTN_TRIGGER_HAPPY1) {
						pad->hat_x = ev.value ? -1.0f : 0.0f;
					} else if(ev.code == BTN_DPAD_RIGHT || ev.code == BTN_TRIGGER_HAPPY2) {
						pad->hat_x = ev.value ? 1.0f : 0.0f;
					}
				}
			} else if(ev.type == EV_ABS) {
				if(ev.code == ABS_HAT0X) {
					pad->hat_x = (float)ev.value;
				} else if(ev.code == ABS_HAT0Y) {
					pad->hat_y = (float)ev.value;
				} else {
					for(int a = 0; a < pad->axis_count; a++) {
						if(lpad->axis_map[a].code == (int)ev.code) {
							pad->axes[a] = mkfw_joystick_normalize_axis(
								ev.value,
								lpad->axis_map[a].minimum,
								lpad->axis_map[a].maximum
							);
							break;
						}
					}
				}
			}
		}

		/* Check for actual errors (not just EAGAIN from non-blocking) */
		if(errno != EAGAIN && errno != EWOULDBLOCK && errno != 0) {
			had_error = 1;
		}

		if(had_error) {
			mkfw_joystick_close_pad(i);
			if(mkfw_joystick_cb) {
				mkfw_joystick_cb(i, 0);
			}
		}
	}
}
