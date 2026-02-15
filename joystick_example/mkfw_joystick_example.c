// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

// Joystick/Gamepad test for MKFW
// Compile with: ./build_joystick.sh

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define MKFW_UI
#define MKFW_JOYSTICK
#define MKFW_JOYSTICK_GAMEDB
#include "../mkfw_gl_loader.h"
#include "../mkfw.h"

static void on_gamepad_connect(int pad, int connected) {
	if(connected) {
		printf("Pad %d connected: %s (vendor:%04x product:%04x)\n",
			pad, mkfw_joystick_name(pad),
			mkfw_joystick_pads[pad].vendor_id,
			mkfw_joystick_pads[pad].product_id);
	} else {
		printf("Pad %d disconnected\n", pad);
	}
}

int main(void) {
	struct mkfw_state *mkfw = mkfw_init(800, 600);
	if(!mkfw) {
		printf("Failed to initialize MKFW\n");
		return 1;
	}

	mkfw_set_window_title(mkfw, "MKFW Joystick Test");
	mkfw_show_window(mkfw);

	mkfw_gl_loader();
	mkfw_set_swapinterval(mkfw, 1);

	mkui_init(mkfw);

	mkfw_joystick_init();
	mkfw_joystick_set_callback(on_gamepad_connect);

	printf("Joystick test running. Connect a controller...\n");
	printf("Press ESC to exit.\n\n");

	while(!mkfw_should_close(mkfw)) {
		mkfw_pump_messages(mkfw);
		mkfw_joystick_update();

		if(mkfw_is_key_pressed(mkfw, MKS_KEY_ESCAPE)) {
			mkfw_set_should_close(mkfw, 1);
		}

		int fw, fh;
		mkfw_get_framebuffer_size(mkfw, &fw, &fh);

		glViewport(0, 0, fw, fh);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		mkui_new_frame(fw, fh);

		/* Main window */
		static float wx = 10.0f, wy = 10.0f;
		mkui_begin_window("Joystick Test", &wx, &wy, 780, 580);

		int any_connected = 0;

		for(int p = 0; p < MKFW_JOYSTICK_MAX_PADS; p++) {
			if(!mkfw_joystick_connected(p)) continue;
			any_connected = 1;

			/* Pad header */
			char buf[512];
			snprintf(buf, sizeof(buf), "--- Pad %d: %s ---", p, mkfw_joystick_name(p));
			mkui_text_colored(buf, mkui_rgb(0.4f, 0.8f, 1.0f));

			/* Raw buttons */
			int bc = mkfw_joystick_button_count(p);
			char btn_str[256] = "Buttons: ";
			int pos = 9;
			for(int b = 0; b < bc && pos < 240; b++) {
				if(mkfw_joystick_button(p, b)) {
					pos += snprintf(btn_str + pos, sizeof(btn_str) - pos, "%d ", b);
				}
			}
			if(pos == 9) strcat(btn_str, "(none)");
			mkui_text(btn_str);

			/* Raw axes */
			int ac = mkfw_joystick_axis_count(p);
			for(int a = 0; a < ac; a++) {
				float v = mkfw_joystick_axis(p, a);
				snprintf(buf, sizeof(buf), "Axis %d: %.3f", a, v);
				mkui_text(buf);
			}

			/* Hat / D-pad */
			float hx = mkfw_joystick_hat_x(p);
			float hy = mkfw_joystick_hat_y(p);
			snprintf(buf, sizeof(buf), "D-Pad: x=%.0f y=%.0f", hx, hy);
			mkui_text(buf);

			/* Mapped gamepad (if DB has a mapping) */
			mkui_separator();
			mkui_text_colored("Mapped Gamepad:", mkui_rgb(0.4f, 1.0f, 0.4f));

			char mapped[256] = "  ";
			pos = 2;
			struct { int id; const char *name; } btn_names[] = {
				{ MKFW_GAMEPAD_A, "A" },
				{ MKFW_GAMEPAD_B, "B" },
				{ MKFW_GAMEPAD_X, "X" },
				{ MKFW_GAMEPAD_Y, "Y" },
				{ MKFW_GAMEPAD_LEFT_BUMPER, "LB" },
				{ MKFW_GAMEPAD_RIGHT_BUMPER, "RB" },
				{ MKFW_GAMEPAD_BACK, "Back" },
				{ MKFW_GAMEPAD_START, "Start" },
				{ MKFW_GAMEPAD_GUIDE, "Guide" },
				{ MKFW_GAMEPAD_LEFT_THUMB, "LS" },
				{ MKFW_GAMEPAD_RIGHT_THUMB, "RS" },
				{ MKFW_GAMEPAD_DPAD_UP, "Up" },
				{ MKFW_GAMEPAD_DPAD_DOWN, "Down" },
				{ MKFW_GAMEPAD_DPAD_LEFT, "Left" },
				{ MKFW_GAMEPAD_DPAD_RIGHT, "Right" },
			};
			for(int b = 0; b < 15 && pos < 240; b++) {
				if(mkfw_gamepad_button(p, btn_names[b].id)) {
					pos += snprintf(mapped + pos, sizeof(mapped) - pos, "[%s] ", btn_names[b].name);
				}
			}
			if(pos == 2) strcat(mapped, "(no buttons)");
			mkui_text(mapped);

			float lx = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_LEFT_X);
			float ly = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_LEFT_Y);
			float rx = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_RIGHT_X);
			float ry = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_RIGHT_Y);
			float lt = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_LEFT_TRIGGER);
			float rt = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_RIGHT_TRIGGER);

			snprintf(buf, sizeof(buf), "  LStick: (%.2f, %.2f)  RStick: (%.2f, %.2f)", lx, ly, rx, ry);
			mkui_text(buf);
			snprintf(buf, sizeof(buf), "  LTrigger: %.2f  RTrigger: %.2f", lt, rt);
			mkui_text(buf);

			mkui_separator();
		}

		if(!any_connected) {
			mkui_text_colored("No controllers connected.", mkui_rgb(1.0f, 0.6f, 0.2f));
			mkui_text("Plug in a gamepad to see its state here.");
			mkui_text("");
			mkui_text("Supported:");
			mkui_text("  - Xbox 360/One/Series controllers");
			mkui_text("  - PlayStation DualShock 4 / DualSense");
			mkui_text("  - Nintendo Switch Pro Controller");
			mkui_text("  - 8BitDo, Logitech, and more");
			mkui_text("");
			mkui_text("On Linux, you may need to be in the 'input' group:");
			mkui_text("  sudo usermod -aG input $USER");
		}

		mkui_end_window();

		mkui_render();
		mkfw_swap_buffers(mkfw);
		mkfw_update_input_state(mkfw);
	}

	mkfw_joystick_shutdown();
	mkui_shutdown();
	mkfw_cleanup(mkfw);

	printf("Done.\n");
	return 0;
}
