// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

// Joystick/Gamepad test for MKFW (terminal output)
// Compile with: ./build_joystick.sh

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define MKFW_JOYSTICK
#define MKFW_JOYSTICK_GAMEDB
#include "../mkfw.h"

static volatile sig_atomic_t running = 1;

// [=]===^=[ on_signal ]=========================================================================[=]
static void on_signal(int sig) {
	(void)sig;
	running = 0;
}

// [=]===^=[ on_gamepad_connect ]=================================================================[=]
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

// [=]===^=[ sleep_ms ]==========================================================================[=]
static void sleep_ms(uint32_t ms) {
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

// [=]===^=[ main ]==============================================================================[=]
int main(void) {
	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);

	mkfw_joystick_init();
	mkfw_joystick_set_callback(on_gamepad_connect);

	printf("MKFW Joystick Test (Ctrl+C to exit)\n\n");

	/* Hide cursor, switch to alternate screen buffer */
	printf("\033[?1049h\033[?25l");
	fflush(stdout);

	while(running) {
		mkfw_joystick_update();

		/* Cursor home + clear screen */
		printf("\033[H\033[J");

		printf("\033[1mMKFW Joystick Test\033[0m (Ctrl+C to exit)\n\n");

		uint32_t any_connected = 0;

		for(uint32_t p = 0; p < MKFW_JOYSTICK_MAX_PADS; ++p) {
			if(!mkfw_joystick_connected(p)) {
				continue;
			}
			any_connected = 1;

			/* Pad header (cyan) */
			printf("\033[36m--- Pad %u: %s ---\033[0m\n", p, mkfw_joystick_name(p));

			/* Raw buttons */
			uint32_t bc = (uint32_t)mkfw_joystick_button_count(p);
			printf("  Buttons: ");
			uint32_t any_btn = 0;
			for(uint32_t b = 0; b < bc; ++b) {
				if(mkfw_joystick_button(p, b)) {
					printf("%u ", b);
					any_btn = 1;
				}
			}
			if(!any_btn) {
				printf("(none)");
			}
			printf("\n");

			/* Raw axes */
			uint32_t ac = (uint32_t)mkfw_joystick_axis_count(p);
			printf("  Axes:    ");
			for(uint32_t a = 0; a < ac; ++a) {
				float v = mkfw_joystick_axis(p, a);
				printf("%u:% .3f  ", a, (double)v);
			}
			printf("\n");

			/* Hat / D-pad */
			float hx = mkfw_joystick_hat_x(p);
			float hy = mkfw_joystick_hat_y(p);
			printf("  D-Pad:   x=% .0f  y=% .0f\n", (double)hx, (double)hy);

			/* Mapped gamepad (green header) */
			printf("\033[32m  -- Mapped Gamepad --\033[0m\n");

			struct { int32_t id; const char *name; } btn_names[] = {
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
			printf("    ");
			uint32_t any_mapped = 0;
			for(uint32_t b = 0; b < 15; ++b) {
				if(mkfw_gamepad_button(p, btn_names[b].id)) {
					printf("\033[1m[%s]\033[0m ", btn_names[b].name);
					any_mapped = 1;
				}
			}
			if(!any_mapped) {
				printf("(no buttons)");
			}
			printf("\n");

			float lx = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_LEFT_X);
			float ly = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_LEFT_Y);
			float rx = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_RIGHT_X);
			float ry = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_RIGHT_Y);
			float lt = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_LEFT_TRIGGER);
			float rt = mkfw_gamepad_axis(p, MKFW_GAMEPAD_AXIS_RIGHT_TRIGGER);

			printf("    LStick: (% .2f, % .2f)  RStick: (% .2f, % .2f)\n",
				(double)lx, (double)ly, (double)rx, (double)ry);
			printf("    LTrigger: % .2f  RTrigger: % .2f\n", (double)lt, (double)rt);

			printf("\n");
		}

		if(!any_connected) {
			printf("\033[33mNo controllers connected.\033[0m\n");
			printf("Plug in a gamepad to see its state here.\n\n");
			printf("Supported:\n");
			printf("  - Xbox 360/One/Series controllers\n");
			printf("  - PlayStation DualShock 4 / DualSense\n");
			printf("  - Nintendo Switch Pro Controller\n");
			printf("  - 8BitDo, Logitech, and more\n\n");
			printf("On Linux, you may need to be in the 'input' group:\n");
			printf("  sudo usermod -aG input $USER\n");
		}

		fflush(stdout);
		sleep_ms(16);
	}

	/* Restore alternate screen buffer and cursor */
	printf("\033[?25h\033[?1049l");
	fflush(stdout);

	mkfw_joystick_shutdown();

	printf("Done.\n");
	return 0;
}
