// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT
#define _GNU_SOURCE
#define MKFW_BUILD_LIBRARY

#ifndef MKFW_JOYSTICK_TEST_HEADER
#define MKFW_JOYSTICK_TEST_HEADER "../mkfw_joystick.h"
#endif
#ifndef MKFW_JOYSTICK_TEST_SOURCE
#define MKFW_JOYSTICK_TEST_SOURCE "../mkfw_linux_joystick.c"
#endif

#include MKFW_JOYSTICK_TEST_HEADER
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <linux/input.h>

struct test_device {
	uint8_t present;
	uint8_t accessible;
	uint32_t opens;
};

static struct test_device devices[3];
static char *paths[3] = {"/dev/input/event0", "/dev/input/event1", "/dev/input/event2"};
static uint8_t directory_token;
static struct dirent directory_entry;
static uint32_t directory_position;
static uint32_t directory_scans;
static uint32_t watch_mask;
static uint32_t notifications_size;
static uint8_t notifications[4096];
static uint8_t input_pending;
static uint32_t connections;
static uint32_t disconnections;
static uint32_t failures;
static uint32_t checks;

// [=]===^=[ test_check ]========================================================================[=]
static void test_check(uint8_t condition, char *message) {
	++checks;

	if(!condition) {
		fprintf(stderr, "FAIL: %s\n", message);
		++failures;

	}
}

// [=]===^=[ test_open ]=========================================================================[=]
static int test_open(const char *path, int flags, ...) {
	(void)flags;

	for(uint32_t index = 0; index < 3; ++index) {
		if(!strcmp(path, paths[index])) {
			++devices[index].opens;

			if(devices[index].present && devices[index].accessible) {
				return 200 + index;

			}

			errno = devices[index].present ? EACCES : ENOENT;
			return -1;

		}
	}

	errno = ENOENT;
	return -1;

}

// [=]===^=[ test_close ]========================================================================[=]
static int test_close(int descriptor) {
	(void)descriptor;
	return 0;

}

// [=]===^=[ test_opendir ]======================================================================[=]
static DIR *test_opendir(const char *path) {
	if(!strcmp(path, "/dev/input")) {
		++directory_scans;
		directory_position = 0;
		return (DIR *)&directory_token;

	}

	errno = ENOENT;
	return 0;

}

// [=]===^=[ test_readdir ]======================================================================[=]
static struct dirent *test_readdir(DIR *directory) {
	(void)directory;

	while(directory_position < 3) {
		uint32_t index = directory_position++;

		if(devices[index].present) {
			snprintf(directory_entry.d_name, sizeof(directory_entry.d_name), "event%u", index);
			return &directory_entry;

		}
	}

	return 0;

}

// [=]===^=[ test_closedir ]=====================================================================[=]
static int test_closedir(DIR *directory) {
	(void)directory;
	return 0;

}

// [=]===^=[ test_realpath ]=====================================================================[=]
static char *test_realpath(const char *path, char *resolved) {
	(void)path;
	(void)resolved;
	errno = ENOENT;
	return 0;

}

// [=]===^=[ test_ioctl ]========================================================================[=]
static int test_ioctl(int descriptor, unsigned long request, ...) {
	va_list arguments;
	void *output;
	uint32_t number = _IOC_NR(request);

	va_start(arguments, request);
	output = va_arg(arguments, void *);
	va_end(arguments);
	memset(output, 0, _IOC_SIZE(request));

	if(number == _IOC_NR(EVIOCGBIT(0, 0))) {
		unsigned long *bits = output;

		bits[0] = 1ul << EV_KEY;

		if(descriptor == 202) {
			bits[0] |= 1ul << EV_ABS;

		}
	} else if(number == _IOC_NR(EVIOCGBIT(EV_KEY, 0))) {
		unsigned long *bits = output;

		bits[BTN_SOUTH / (sizeof(unsigned long) * 8)] |= 1ul << (BTN_SOUTH % (sizeof(unsigned long) * 8));

	} else if(number == _IOC_NR(EVIOCGNAME(1))) {
		snprintf(output, _IOC_SIZE(request), "Isolated test pad");

	} else if(number >= _IOC_NR(EVIOCGABS(0)) && number <= _IOC_NR(EVIOCGABS(ABS_MAX))) {
		struct input_absinfo *axis = output;

		axis->minimum = -32768;
		axis->maximum = 32767;

	}

	return 0;

}

// [=]===^=[ test_read ]=========================================================================[=]
static ssize_t test_read(int descriptor, void *buffer, size_t size) {
	if(descriptor == 100 && notifications_size) {
		uint32_t length = notifications_size;

		test_check(size >= length, "Notification buffer contains complete records");
		memcpy(buffer, notifications, length);
		notifications_size = 0;
		return length;

	}

	if(descriptor == 202 && input_pending) {
		struct input_event event = {.type = EV_KEY, .code = BTN_SOUTH, .value = 1};

		test_check(size == sizeof(event), "Pad read requests an input event");
		memcpy(buffer, &event, sizeof(event));
		input_pending = 0;
		return sizeof(event);

	}

	errno = EAGAIN;
	return -1;

}

// [=]===^=[ test_write ]========================================================================[=]
static ssize_t test_write(int descriptor, const void *buffer, size_t size) {
	(void)descriptor;
	(void)buffer;
	return size;

}

// [=]===^=[ test_inotify_init1 ]================================================================[=]
static int test_inotify_init1(int flags) {
	test_check((flags & (IN_NONBLOCK | IN_CLOEXEC)) == (IN_NONBLOCK | IN_CLOEXEC), "Notification descriptor is nonblocking");
	return 100;

}

// [=]===^=[ test_inotify_add_watch ]============================================================[=]
static int test_inotify_add_watch(int descriptor, const char *path, uint32_t mask) {
	test_check(descriptor == 100 && !strcmp(path, "/dev/input"), "Only the input directory is watched");
	watch_mask = mask;
	return 1;

}

// [=]===^=[ test_inotify_rm_watch ]=============================================================[=]
static int test_inotify_rm_watch(int descriptor, int watch) {
	test_check(descriptor == 100 && watch == 1, "Input watch is removed during shutdown");
	return 0;

}

#define open test_open
#define close test_close
#define opendir test_opendir
#define readdir test_readdir
#define closedir test_closedir
#define realpath test_realpath
#define ioctl test_ioctl
#define read test_read
#define write test_write
#define inotify_init1 test_inotify_init1
#define inotify_add_watch test_inotify_add_watch
#define inotify_rm_watch test_inotify_rm_watch
#include MKFW_JOYSTICK_TEST_SOURCE
#undef open
#undef close
#undef opendir
#undef readdir
#undef closedir
#undef realpath
#undef ioctl
#undef read
#undef write
#undef inotify_init1
#undef inotify_add_watch
#undef inotify_rm_watch

// [=]===^=[ test_notify ]=======================================================================[=]
static void test_notify(uint32_t mask, char *name) {
	struct inotify_event event = {.wd = 1, .mask = mask, .len = 16};

	memcpy(notifications + notifications_size, &event, sizeof(event));
	snprintf((char *)notifications + notifications_size + sizeof(event), event.len, "%s", name);
	notifications_size += sizeof(event) + event.len;

}

// [=]===^=[ test_connection ]===================================================================[=]
static void test_connection(uint32_t pad, uint32_t connected) {
	test_check(pad == 0, "The test pad occupies the first free slot");

	if(connected) {
		++connections;

	} else {
		++disconnections;

	}
}

// [=]===^=[ main ]==============================================================================[=]
int main(void) {
	devices[0] = (struct test_device){.present = 1, .accessible = 1};
	devices[1] = (struct test_device){.present = 1, .accessible = 1};
	mkfw_joystick_init();
	mkfw_joystick_cb = test_connection;
	test_check(directory_scans == 1 && devices[0].opens == 1 && devices[1].opens == 1, "Initialization discovers existing devices once");
	test_check((watch_mask & IN_ATTRIB) != 0, "Hotplug watches permission and ACL changes");
	devices[2] = (struct test_device){.present = 1, .accessible = 1};
	test_notify(IN_CREATE, "event2");
	mkfw_joystick_update();
	test_check(mkfw_joystick_pads[0].connected && devices[2].opens == 1 && connections == 1, "New gamepad opens immediately");

	for(uint32_t frame = 0; frame < 60; ++frame) {
		mkfw_joystick_update();

	}

	test_check(directory_scans == 1 && devices[0].opens == 1 && devices[1].opens == 1 && devices[2].opens == 1, "Successful hotplug never schedules frame-by-frame directory rescans");
	test_notify(IN_ATTRIB, "event2");
	mkfw_joystick_update();
	test_check(devices[2].opens == 1 && connections == 1, "Metadata notifications do not reopen a connected pad");
	input_pending = 1;
	mkfw_joystick_update();
	test_check(mkfw_joystick_pads[0].buttons[0] == 1, "Input events remain available after hotplug");
	devices[2].present = 0;
	test_notify(IN_DELETE, "event2");
	mkfw_joystick_update();
	test_check(!mkfw_joystick_pads[0].connected && !mkfw_joystick_pads[0].buttons[0] && disconnections == 1, "Removal clears input state and reports disconnection");
	devices[2].present = 1;
	devices[2].accessible = 0;
	test_notify(IN_CREATE, "event2");
	mkfw_joystick_update();
	test_check(!mkfw_joystick_pads[0].connected && devices[2].opens == 3, "A node created before udev grants access remains pending");

	for(uint32_t frame = 0; frame < 60; ++frame) {
		mkfw_joystick_update();

	}

	test_check(directory_scans == 1 && devices[2].opens == 3 && devices[0].opens == 1 && devices[1].opens == 1, "Permission race causes no polling retries or unrelated device probes");
	devices[2].accessible = 1;
	test_notify(IN_ATTRIB, "event2");
	mkfw_joystick_update();
	test_check(mkfw_joystick_pads[0].connected && devices[2].opens == 4 && connections == 2, "Permission notification retries only the new node");
	test_notify(IN_CREATE, "event2");
	test_notify(IN_ATTRIB, "event2");
	mkfw_joystick_update();
	test_check(devices[2].opens == 4 && connections == 2, "Batched duplicate notifications preserve the existing connection");
	test_notify(IN_ATTRIB, "event1");
	test_notify(IN_CREATE, "js0");
	mkfw_joystick_update();
	test_check(devices[0].opens == 1 && devices[1].opens == 2 && devices[2].opens == 4 && directory_scans == 1, "Notifications probe only the named event node");
	test_notify(IN_DELETE, "event2");
	test_notify(IN_CREATE, "event2");
	mkfw_joystick_update();
	test_check(mkfw_joystick_pads[0].connected && connections == 3 && disconnections == 2, "Removal and reconnection work in one notification batch");
	mkfw_joystick_shutdown();
	test_check(!mkfw_joystick_initialized && mkfw_inotify_fd == -1 && !mkfw_joystick_pads[0].connected, "Shutdown releases hotplug state");
	printf("joystick_hotplug: checks=%u failures=%u\n", checks, failures);
	return failures ? 1 : 0;

}
