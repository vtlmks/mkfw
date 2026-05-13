// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT
//
// mkfw smoke test.  Brings up every subsystem the library exposes,
// runs them for a short interval, and tears everything down cleanly.
// Pass criterion: exits 0 with no output on stderr.
//
// Build: ../tests/build_tests.sh (also called from CI).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../mkfw.h"
#include "../mkfw_audio.h"
#include "../mkfw_timer.h"
#include "../mkfw_joystick.h"

static uint32_t failures;

#define EXPECT(cond, msg) do { \
	if(!(cond)) { \
		fprintf(stderr, "smoke: FAIL %s:%d %s\n", __FILE__, __LINE__, (msg)); \
		++failures; \
	} \
} while(0)

// [=]===^=[ on_error ]===========================================================================^===[=]
static void on_error(const char *message) {
	fprintf(stderr, "smoke: mkfw_error: %s\n", message);
}

// [=]===^=[ on_audio ]===========================================================================^===[=]
static void on_audio(void *userdata, float *buffer, uint32_t frames) {
	(void)userdata;
	(void)buffer;
	(void)frames;
	// Silent producer: caller already zeroed the buffer.
}

// [=]===^=[ main ]==============================================================================^===[=]
int main(void) {
	mkfw_set_error_callback(on_error);

	// --- Context + window ------------------------------------------
	struct mkfw_context *ctx = mkfw_init(0);
	EXPECT(ctx != 0, "mkfw_init failed");
	if(!ctx) {
		return 1;
	}

	struct mkfw_window_options wopts = {
		.width  = 640,
		.height = 360,
		.title  = "mkfw smoke",
		.flags  = MKFW_WIN_HIDDEN,    // don't pop a window on CI
	};
	struct mkfw_window *win = mkfw_window_create(ctx, &wopts);
	EXPECT(win != 0, "mkfw_window_create failed");
	if(!win) {
		mkfw_shutdown(ctx);
		return 1;
	}

	// --- Monitor query ---------------------------------------------
	struct mkfw_monitor mons[MKFW_MAX_MONITORS];
	int32_t mcount = mkfw_get_monitors(ctx, mons, MKFW_MAX_MONITORS);
	EXPECT(mcount >= 0, "mkfw_get_monitors returned negative");

	// --- Event pump ------------------------------------------------
	uint64_t t0 = mkfw_get_time();
	while(mkfw_get_time() - t0 < 200000000ULL) {   // ~200ms
		mkfw_poll_events(ctx);
		mkfw_window_update_input_state(win);
		mkfw_sleep(10000000ULL);
	}

	// --- Native handles --------------------------------------------
	struct mkfw_native_handles nh = {0};
	mkfw_window_get_native_handles(win, &nh);
	EXPECT(nh.display != 0, "native display handle is 0");
	EXPECT(nh.window  != 0, "native window  handle is 0");

	// --- Last-error ------------------------------------------------
	mkfw_clear_last_error();
	EXPECT(mkfw_get_last_error() == 0, "last-error should be clear");

	// --- Audio -----------------------------------------------------
	struct mkfw_audio_options aopts = {0};
	if(mkfw_audio_init(&aopts)) {
		struct mkfw_audio_info info;
		mkfw_audio_info(&info);
		EXPECT(info.sample_rate > 0, "audio sample_rate is 0");
		EXPECT(info.channels    > 0, "audio channels is 0");
		mkfw_audio_set_callback(on_audio, 0);
		mkfw_sleep(100000000ULL);   // 100ms of silent playback
		mkfw_audio_set_callback(0, 0);
		mkfw_audio_shutdown();
	} else {
		fprintf(stderr, "smoke: audio init failed (acceptable on headless CI), continuing\n");
	}

	// --- Timer -----------------------------------------------------
	mkfw_timer_init();
	struct mkfw_timer_handle *timer = mkfw_timer_create(16666666ULL);   // ~60Hz
	EXPECT(timer != 0, "mkfw_timer_create returned 0");
	if(timer) {
		mkfw_timer_wait(timer);
		mkfw_timer_destroy(timer);
	}
	mkfw_timer_shutdown();

	// --- Joystick --------------------------------------------------
	mkfw_joystick_init();
	mkfw_joystick_update();
	mkfw_joystick_shutdown();

	// --- Teardown --------------------------------------------------
	mkfw_window_destroy(win);
	mkfw_shutdown(ctx);

	if(failures) {
		fprintf(stderr, "smoke: %u failures\n", failures);
		return 1;
	}
	printf("smoke: ok\n");
	return 0;
}
