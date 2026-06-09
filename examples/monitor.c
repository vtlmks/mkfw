// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

// Monitor enumeration example for MKFW (terminal output)
//
// Initializes the library, queries all connected monitors via the context
// (no window required), and prints their properties to stdout.

#include <stdio.h>
#include <stdint.h>

#include "../mkfw.h"

// [=]===^=[ main ]===============================================================================[=]
int main(void) {
	struct mkfw_context *ctx = mkfw_init(0);
	if(!ctx) {
		fprintf(stderr, "Failed to initialize mkfw\n");
		return 1;
	}

	struct mkfw_monitor monitors[MKFW_MAX_MONITORS];
	int32_t count = mkfw_get_monitors(ctx, monitors, MKFW_MAX_MONITORS);

	printf("Detected %d monitor(s):\n\n", count);

	for(int32_t i = 0; i < count; ++i) {
		struct mkfw_monitor *m = &monitors[i];
		printf("  [%d] %s%s\n", i, m->name, m->primary ? " (primary)" : "");
		printf("      Resolution:   %dx%d\n", m->width, m->height);
		printf("      Position:     (%d, %d)\n", m->x, m->y);
		printf("      Refresh rate: %d Hz\n", m->refresh_rate);
		printf("      Physical:     %dx%d mm\n", m->width_mm, m->height_mm);
		printf("      Content scale: %.2f\n", (double)m->content_scale);
		printf("      Work area:    (%d, %d) %dx%d\n", m->work_x, m->work_y, m->work_width, m->work_height);

		struct mkfw_video_mode modes[256];
		int32_t mode_count = mkfw_get_video_modes(ctx, i, modes, 256);
		printf("      Video modes:  %d\n", mode_count);
		for(int32_t k = 0; k < mode_count; ++k) {
			printf("        %dx%d @ %d Hz\n", modes[k].width, modes[k].height, modes[k].refresh_rate);
		}
		printf("\n");
	}

	mkfw_shutdown(ctx);
	return 0;
}
