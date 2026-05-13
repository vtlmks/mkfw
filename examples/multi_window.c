// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT
//
// Multi-window example: opens two windows on a single mkfw_context,
// pumps events for both from a single mkfw_poll_events() call, and
// renders an independent solid color into each window.  Closing
// either window terminates the application.

#include <stdio.h>

#include "../mkfw_gl_loader.h"
#include "../mkfw.h"

// [=]===^=[ render ]============================================================================^===[=]
static void render(struct mkfw_window *w, float r, float g, float b) {
	mkfw_window_attach_context(w);
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	mkfw_window_swap_buffers(w);
}

// [=]===^=[ on_error ]==========================================================================^===[=]
static void on_error(const char *message) {
	fprintf(stderr, "multi_window: %s\n", message);
}

// [=]===^=[ main ]==============================================================================^===[=]
int main(void) {
	mkfw_set_error_callback(on_error);

	struct mkfw_context *ctx = mkfw_init(0);
	if(!ctx) {
		return 1;
	}

	struct mkfw_window_options main_opts = {
		.width = 800, .height = 600, .title = "mkfw main window",
	};
	struct mkfw_window_options debug_opts = {
		.width = 320, .height = 240, .title = "mkfw debug overlay",
	};
	struct mkfw_window *main_w  = mkfw_window_create(ctx, &main_opts);
	struct mkfw_window *debug_w = mkfw_window_create(ctx, &debug_opts);
	if(!main_w || !debug_w) {
		mkfw_shutdown(ctx);
		return 1;
	}

	mkfw_gl_loader();

	while(!mkfw_window_should_close(main_w) && !mkfw_window_should_close(debug_w)) {
		mkfw_poll_events(ctx);   // one pump, both windows

		if(main_w->keyboard_state[MKFW_KEY_ESCAPE] || debug_w->keyboard_state[MKFW_KEY_ESCAPE]) {
			break;
		}

		render(main_w,  0.15f, 0.15f, 0.20f);
		render(debug_w, 0.40f, 0.10f, 0.10f);

		mkfw_window_update_input_state(main_w);
		mkfw_window_update_input_state(debug_w);
	}

	mkfw_window_destroy(debug_w);
	mkfw_window_destroy(main_w);
	mkfw_shutdown(ctx);
	return 0;
}
