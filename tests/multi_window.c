// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT
//
// Multi-window dispatch test.  Creates two windows on a single
// mkfw_context, pumps events for a short period, and verifies that
// the per-window framebuffer callback fires for each window
// independently.
//
// Pass criterion: exits 0 after both windows have observed at least
// one framebuffer event, or after a 1-second timeout.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../mkfw.h"

static uint32_t fb_events_a;
static uint32_t fb_events_b;

// [=]===^=[ on_resize_a ]========================================================================^===[=]
static void on_resize_a(struct mkfw_window *w, int32_t width, int32_t height, float aspect) {
	(void)w; (void)width; (void)height; (void)aspect;
	++fb_events_a;
}

// [=]===^=[ on_resize_b ]========================================================================^===[=]
static void on_resize_b(struct mkfw_window *w, int32_t width, int32_t height, float aspect) {
	(void)w; (void)width; (void)height; (void)aspect;
	++fb_events_b;
}

// [=]===^=[ on_error ]===========================================================================^===[=]
static void on_error(const char *message) {
	fprintf(stderr, "multi_window: mkfw_error: %s\n", message);
}

// [=]===^=[ main ]==============================================================================^===[=]
int main(void) {
	mkfw_set_error_callback(on_error);

	struct mkfw_context *ctx = mkfw_init(0);
	if(!ctx) {
		fprintf(stderr, "multi_window: mkfw_init failed\n");
		return 1;
	}

	struct mkfw_window_options opts_a = {
		.width = 320, .height = 240, .title = "mkfw multi A",
	};
	struct mkfw_window_options opts_b = {
		.width = 320, .height = 240, .title = "mkfw multi B",
	};
	struct mkfw_window *a = mkfw_window_create(ctx, &opts_a);
	struct mkfw_window *b = mkfw_window_create(ctx, &opts_b);
	if(!a || !b) {
		fprintf(stderr, "multi_window: window create failed\n");
		mkfw_shutdown(ctx);
		return 1;
	}
	mkfw_window_set_framebuffer_size_callback(a, on_resize_a);
	mkfw_window_set_framebuffer_size_callback(b, on_resize_b);

	uint64_t t0 = mkfw_get_time();
	while((fb_events_a == 0 || fb_events_b == 0)
	   && mkfw_get_time() - t0 < 1000000000ULL) {
		mkfw_poll_events(ctx);
		mkfw_sleep(10000000ULL);
	}

	mkfw_window_destroy(b);
	mkfw_window_destroy(a);
	mkfw_shutdown(ctx);

	if(fb_events_a == 0 || fb_events_b == 0) {
		fprintf(stderr, "multi_window: FAIL events a=%u b=%u\n", fb_events_a, fb_events_b);
		return 1;
	}
	printf("multi_window: ok (a=%u events, b=%u events)\n", fb_events_a, fb_events_b);
	return 0;
}
