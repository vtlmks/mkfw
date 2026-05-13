// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

// Per-pixel transparency example for MKFW
//
// Draws a spinning triangle with soft edges on a fully transparent background.
// The window has no decoration fill -- you see the desktop through the clear
// areas.  Requires a compositor (X11: picom/kwin/mutter, Windows: DWM).
//
// Compile with: ./build_transparency.sh

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../mkfw_gl_loader.h"

#include "../mkfw.h"

struct app_state {
	struct mkfw_window *window;
	int32_t running;
	float time;
	int32_t fb_width;
	int32_t fb_height;
};

// [=]===^=[ on_key ]=============================================================================[=]
static void on_key(struct mkfw_window *window, uint32_t key, uint32_t action, uint32_t mods) {
	(void)mods;
	struct app_state *app = (struct app_state *)mkfw_window_get_user_data(window);

	if(key == MKFW_KEY_ESCAPE && action == MKFW_PRESSED) {
		app->running = 0;
	}
}

// [=]===^=[ on_resize ]==========================================================================[=]
static void on_resize(struct mkfw_window *window, int32_t w, int32_t h, float aspect) {
	(void)aspect;
	struct app_state *app = (struct app_state *)mkfw_window_get_user_data(window);
	app->fb_width = w;
	app->fb_height = h;
	glViewport(0, 0, w, h);
}

// [=]===^=[ main ]===============================================================================[=]
int main(void) {
	struct app_state app = {0};
	app.running = 1;

	struct mkfw_context *ctx = mkfw_init(0);
	if(!ctx) {
		fprintf(stderr, "Failed to initialize mkfw\n");
		return 1;
	}

	struct mkfw_window_options wopts = {
		.width = 800,
		.height = 600,
		.title = "MKFW Transparency",
		.flags = MKFW_WIN_TRANSPARENT,
	};
	app.window = mkfw_window_create(ctx, &wopts);
	if(!app.window) {
		fprintf(stderr, "Failed to create window\n");
		mkfw_shutdown(ctx);
		return 1;
	}

	mkfw_window_set_user_data(app.window, &app);
	mkfw_window_set_key_callback(app.window, on_key);
	mkfw_window_set_framebuffer_size_callback(app.window, on_resize);

	mkfw_gl_loader();
	mkfw_window_set_swap_interval(app.window, 1);

	mkfw_window_get_framebuffer_size(app.window, &app.fb_width, &app.fb_height);
	glViewport(0, 0, app.fb_width, app.fb_height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while(app.running && !mkfw_window_should_close(app.window)) {
		mkfw_poll_events(ctx);
		mkfw_window_update_input_state(app.window);

		app.time += 0.016f;

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		float aspect = (float)app.fb_width / (float)(app.fb_height > 0 ? app.fb_height : 1);
		glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		/* Spinning triangle with per-vertex alpha */
		float angle = app.time * 1.5f;
		float s = sinf(angle);
		float c = cosf(angle);
		float r = 0.6f;

		/* Pulsing alpha on outer vertices */
		float alpha = 0.5f + 0.4f * sinf(app.time * 2.0f);

		glBegin(GL_TRIANGLES);
		{
			float x0 = 0.0f * c - r * s;
			float y0 = 0.0f * s + r * c;
			glColor4f(1.0f, 0.2f, 0.2f, 1.0f);
			glVertex2f(x0, y0);

			float x1 = (-r * 0.866f) * c - (-r * 0.5f) * s;
			float y1 = (-r * 0.866f) * s + (-r * 0.5f) * c;
			glColor4f(0.2f, 1.0f, 0.2f, alpha);
			glVertex2f(x1, y1);

			float x2 = (r * 0.866f) * c - (-r * 0.5f) * s;
			float y2 = (r * 0.866f) * s + (-r * 0.5f) * c;
			glColor4f(0.2f, 0.2f, 1.0f, alpha);
			glVertex2f(x2, y2);
		}
		glEnd();

		/* Semi-transparent circle in the center */
		glBegin(GL_TRIANGLE_FAN);
		glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
		glVertex2f(0.0f, 0.0f);
		for(uint32_t i = 0; i <= 32; ++i) {
			float a = (float)i / 32.0f * 2.0f * 3.14159265f;
			glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
			glVertex2f(cosf(a) * 0.15f, sinf(a) * 0.15f);
		}
		glEnd();

		mkfw_window_swap_buffers(app.window);
	}

	mkfw_window_destroy(app.window);
	mkfw_shutdown(ctx);
	return 0;
}
