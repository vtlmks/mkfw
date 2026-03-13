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
	struct mkfw_state *window;
	int32_t running;
	float time;
	int32_t fb_width;
	int32_t fb_height;
};

// [=]===^=[ on_key ]=============================================================================[=]
static void on_key(struct mkfw_state *window, uint32_t key, uint32_t action, uint32_t mods) {
	(void)mods;
	struct app_state *app = (struct app_state *)mkfw_get_user_data(window);

	if(key == MKS_KEY_ESCAPE && action == MKS_PRESSED) {
		app->running = 0;
	}
}

// [=]===^=[ on_resize ]==========================================================================[=]
static void on_resize(struct mkfw_state *window, int32_t w, int32_t h, float aspect) {
	(void)aspect;
	struct app_state *app = (struct app_state *)mkfw_get_user_data(window);
	app->fb_width = w;
	app->fb_height = h;
	glViewport(0, 0, w, h);
}

// [=]===^=[ main ]===============================================================================[=]
int main(void) {
	struct app_state app = {0};
	app.running = 1;

	mkfw_set_transparent(1);
	app.window = mkfw_init(800, 600);
	if(!app.window) {
		fprintf(stderr, "Failed to create window\n");
		return 1;
	}

	mkfw_set_window_title(app.window, "MKFW Transparency");
	mkfw_set_user_data(app.window, &app);
	mkfw_set_key_callback(app.window, on_key);
	mkfw_set_framebuffer_size_callback(app.window, on_resize);
	mkfw_show_window(app.window);

	mkfw_gl_loader();
	mkfw_set_swapinterval(app.window, 1);

	mkfw_get_framebuffer_size(app.window, &app.fb_width, &app.fb_height);
	glViewport(0, 0, app.fb_width, app.fb_height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while(app.running && !mkfw_should_close(app.window)) {
		mkfw_pump_messages(app.window);
		mkfw_update_input_state(app.window);

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

		mkfw_swap_buffers(app.window);
	}

	mkfw_cleanup(app.window);
	return 0;
}
