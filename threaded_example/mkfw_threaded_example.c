// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

// Threaded rendering example for MKFW
//
// Demonstrates how to decouple the render loop from the OS message pump.
// On Windows, dragging or resizing a window blocks the main thread inside
// DispatchMessage (the system runs a modal loop). If rendering happens on
// the main thread, the frame freezes until the user releases the mouse.
//
// The fix: pump messages on the main thread, render on a separate thread.
//
//   Main thread:   mkfw_pump_messages() in a loop  (never blocks long)
//   Render thread: attach GL context, render, swap  (runs independently)
//
// This pattern works with any windowing library, not just MKFW.
//
// Compile with: ./build_threaded.sh

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../mkfw_gl_loader.h"
#include "../mkfw.h"

struct app_state {
	struct mkfw_state *window;
	volatile int32_t running;
	float clear_r;
	float clear_g;
	float clear_b;
};

// [=]===^=[ on_key ]=========================================================================^===[=]
static void on_key(struct mkfw_state *window, uint32_t key, uint32_t action, uint32_t mods) {
	struct app_state *app = (struct app_state *)mkfw_get_user_data(window);

	if(key == MKS_KEY_ESCAPE && action == MKS_PRESSED) {
		app->running = 0;
	}
}

// [=]===^=[ on_resize ]======================================================================^===[=]
static void on_resize(struct mkfw_state *window, int32_t w, int32_t h, float aspect) {
	glViewport(0, 0, w, h);
}

// [=]===^=[ render_thread_func ]=============================================================^===[=]
static MKFW_THREAD_FUNC(render_thread_func, arg) {
	struct app_state *app = (struct app_state *)arg;
	struct mkfw_state *window = app->window;

	// Make the GL context current on this thread
	mkfw_attach_context(window);
	mkfw_gl_loader();
	mkfw_set_swapinterval(window, 1);

	float t = 0.0f;

	while(app->running) {
		int32_t w, h;
		mkfw_get_framebuffer_size(window, &w, &h);
		glViewport(0, 0, w, h);

		// Animate the clear color so it's obvious rendering never stalls
		t += 0.01f;
		app->clear_r = 0.1f + 0.05f * sinf(t * 1.1f);
		app->clear_g = 0.1f + 0.05f * sinf(t * 1.3f);
		app->clear_b = 0.2f + 0.1f * sinf(t * 0.7f);

		glClearColor(app->clear_r, app->clear_g, app->clear_b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// --- your draw calls go here ---

		mkfw_swap_buffers(window);
		mkfw_update_input_state(window);
	}

	return 0;
}

// [=]===^=[ main ]===========================================================================^===[=]
int main(void) {
	struct app_state app = {0};

	// Create window (GL context is current on main thread after init)
	app.window = mkfw_init(1280, 720);
	if(!app.window) {
		fprintf(stderr, "Failed to create window\n");
		return 1;
	}

	mkfw_set_window_title(app.window, "MKFW Threaded Rendering");
	mkfw_set_window_min_size_and_aspect(app.window, 640, 360, 0.0f, 0.0f);
	mkfw_set_user_data(app.window, &app);
	mkfw_set_key_callback(app.window, on_key);
	mkfw_set_framebuffer_size_callback(app.window, on_resize);
	mkfw_show_window(app.window);

	// Release the GL context from the main thread so the render thread can use it.
	// A GL context can only be current on one thread at a time.
	mkfw_detach_context(app.window);

	// Spawn the render thread
	app.running = 1;
	mkfw_thread render_thread = mkfw_thread_create(render_thread_func, &app);

	// Main thread: pump OS messages until the app is done.
	// On Windows this is the thread that owns the window, so it must
	// process messages. Dragging/resizing will block here, but the
	// render thread keeps drawing independently.
	while(app.running && !mkfw_should_close(app.window)) {
		mkfw_pump_messages(app.window);
		mkfw_sleep(5000000); // 5ms — don't burn CPU on message polling
	}

	// Signal the render thread to stop and wait for it
	app.running = 0;
	mkfw_thread_join(render_thread);

	mkfw_cleanup(app.window);
	return 0;
}
