// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

// Monitor enumeration example for MKFW (terminal output)
//
// Creates a hidden window (required for platform init), queries all
// connected monitors, and prints their properties to stdout.
//
// Compile with: ./build_monitor.sh

#include <stdio.h>
#include <stdint.h>

#include "../mkfw.h"

// [=]===^=[ main ]===============================================================================[=]
int main(void) {
	struct mkfw_state *window = mkfw_init(1, 1);
	if(!window) {
		fprintf(stderr, "Failed to initialize mkfw\n");
		return 1;
	}

	struct mkfw_monitor monitors[MKFW_MAX_MONITORS];
	int32_t count = mkfw_get_monitors(window, monitors, MKFW_MAX_MONITORS);

	printf("Detected %d monitor(s):\n\n", count);

	for(int32_t i = 0; i < count; ++i) {
		struct mkfw_monitor *m = &monitors[i];
		printf("  [%d] %s%s\n", i, m->name, m->primary ? " (primary)" : "");
		printf("      Resolution:   %dx%d\n", m->width, m->height);
		printf("      Position:     (%d, %d)\n", m->x, m->y);
		printf("      Refresh rate: %d Hz\n", m->refresh_rate);
		printf("\n");
	}

	mkfw_cleanup(window);
	return 0;
}
