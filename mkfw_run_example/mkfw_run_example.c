// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

// mkfw_run() example -- cross-platform frame callback
//
// Demonstrates the mkfw_run() API which works on Linux, Windows, and Emscripten.
// Instead of writing a manual while loop (which can't work on Emscripten), the
// user provides a frame callback that mkfw calls each frame.
//
// On Linux:      single-thread while loop, vsync paces the frame
// On Windows:    render thread calls frame(), main thread pumps messages
// On Emscripten: requestAnimationFrame calls frame() at display refresh rate
//
// The frame callback may run on a render thread (Windows) or the main thread
// (Linux/Emscripten). Window event callbacks (key, mouse, resize) always fire
// on the main thread. Use atomics for shared state between callbacks and frame().
//
// Compile:
//   Linux:      ./build_desktop.sh
//   Emscripten: ./build_emscripten.sh

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../mkfw_gl_loader.h"
#include "../mkfw.h"

struct app_state {
	struct mkfw_state *window;
	float time;
	int32_t fullscreen;
	int32_t fb_width;
	int32_t fb_height;
};

static struct app_state app;

// [=]===^=[ on_key ]============================================================================^===[=]
static void on_key(struct mkfw_state *window, uint32_t key, uint32_t action, uint32_t mods) {
	(void)mods;
	(void)window;

	if(key == MKS_KEY_ESCAPE && action == MKS_PRESSED) {
		mkfw_set_should_close(app.window, 1);
	}

	if(key == MKS_KEY_F11 && action == MKS_PRESSED) {
		app.fullscreen = !app.fullscreen;
		mkfw_fullscreen(app.window, app.fullscreen);
	}
}

// [=]===^=[ on_resize ]=========================================================================^===[=]
static void on_resize(struct mkfw_state *window, int32_t w, int32_t h, float aspect) {
	(void)window;
	(void)aspect;
	app.fb_width = w;
	app.fb_height = h;
	glViewport(0, 0, w, h);
}

// [=]===^=[ frame ]=============================================================================^===[=]
static void frame(struct mkfw_state *window) {
	app.time += 0.016f;

	glViewport(0, 0, app.fb_width, app.fb_height);

	float r = 0.1f + 0.05f * sinf(app.time * 1.1f);
	float g = 0.1f + 0.05f * sinf(app.time * 1.3f);
	float b = 0.2f + 0.1f * sinf(app.time * 0.7f);
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	mkfw_update_input_state(window);
}

// [=]===^=[ main ]==============================================================================^===[=]
int main(void) {
	memset(&app, 0, sizeof(app));

	app.window = mkfw_init(1280, 720);
	if(!app.window) {
		fprintf(stderr, "Failed to create window\n");
		return 1;
	}

	mkfw_gl_loader();
	mkfw_set_swapinterval(app.window, 1);
	mkfw_set_window_title(app.window, "mkfw_run example");
	mkfw_set_key_callback(app.window, on_key);
	mkfw_set_framebuffer_size_callback(app.window, on_resize);
	mkfw_show_window(app.window);

	mkfw_get_framebuffer_size(app.window, &app.fb_width, &app.fb_height);

	mkfw_run(app.window, frame);

	mkfw_cleanup(app.window);
	return 0;
}
