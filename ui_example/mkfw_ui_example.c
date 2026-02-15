// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

// Example usage of MKFW UI System
// Compile with: -DMKFW_UI

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MKFW_UI
#include "../mkfw_gl_loader.c"
#include "../mkfw.h"

int main() {
	// Initialize MKFW window
	struct mkfw_state *mkfw = mkfw_init(1280, 720);
	if(!mkfw) {
		printf("Failed to initialize MKFW\n");
		return 1;
	}

	mkfw_set_window_title(mkfw, "MKFW UI Example");
	mkfw_show_window(mkfw);

	// Load OpenGL functions
	mkfw_gl_loader();

	mkfw_set_swapinterval(mkfw, 1);

	// Initialize UI system
	mkui_init(mkfw);

	// UI state variables
	int checkbox_value = 0;
	float slider_value = 0.5f;
	int button_click_count = 0;
	float window1_x = 50.f;
	float window1_y = 50.f;
	float window2_x = 500.f;
	float window2_y = 50.f;

	// New widget state variables
	int radio_selection = 0;
	int advanced_header_open = 1;
	char text_buffer[64] = "Edit me!";
	int int_slider_value = 50;
	int64_t int64_slider_value = 1000000;
	int combo_selection = 0;
	char *combo_items[] = {"Option 1", "Option 2", "Option 3", "Option 4"};
	int listbox_selection = 0;
	char *listbox_items[] = {
		"Item 1", "Item 2", "Item 3", "Item 4", "Item 5",
		"Item 6", "Item 7", "Item 8", "Item 9", "Item 10",
		"Item 11", "Item 12", "Item 13", "Item 14", "Item 15"
	};

	// Main loop
	while(!mkfw_should_close(mkfw)) {
		mkfw_pump_messages(mkfw);

		// Get framebuffer size
		int display_w, display_h;
		mkfw_get_framebuffer_size(mkfw, &display_w, &display_h);

		// Clear screen
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Start new UI frame
		mkui_new_frame(display_w, display_h);

		// Draw a window
		mkui_begin_window("Demo Window", &window1_x, &window1_y, 450, 850);

		// Text
		mkui_text("Welcome to MKFW UI!");
		mkui_text("This is a simple immediate-mode UI library.");

		mkui_separator();

		// Button
		if(mkui_button("Click Me!")) {
			button_click_count++;
			printf("Button clicked! Count: %d\n", button_click_count);
		}

		// Show click count
		char buf[64];
		snprintf(buf, sizeof(buf), "Button clicks: %d", button_click_count);
		mkui_text(buf);

		mkui_separator();

		// Checkbox
		mkui_checkbox("Enable feature", &checkbox_value);

		if(checkbox_value) {
			mkui_text_colored("Feature is enabled!", mkui_rgb(0.0f, 1.0f, 0.0f));
		}

		mkui_separator();

		// Slider
		if(mkui_slider_float("Value", &slider_value, 0.0f, 1.0f)) {
			printf("Slider value: %.2f\n", slider_value);
		}

		// Display slider value
		snprintf(buf, sizeof(buf), "Current value: %.2f", slider_value);
		mkui_text(buf);

		mkui_separator();

		// Integer sliders (with mousewheel support)
		mkui_slider_int("Integer", &int_slider_value, 0, 100);
		snprintf(buf, sizeof(buf), "Int value: %d", int_slider_value);
		mkui_text(buf);

		mkui_slider_int64("Large Number", &int64_slider_value, 0, 10000000);
		snprintf(buf, sizeof(buf), "Int64 value: %lld", (long long)int64_slider_value);
		mkui_text(buf);

		mkui_separator();

		// Radio buttons
		mkui_text("Select an option:");
		mkui_radio_button("Option A", &radio_selection, 0);
		mkui_radio_button("Option B", &radio_selection, 1);
		mkui_radio_button("Option C", &radio_selection, 2);
		snprintf(buf, sizeof(buf), "Selected: %d", radio_selection);
		mkui_text(buf);

		mkui_separator();

		// Text input
		mkui_text("Text input:");
		if(mkui_text_input("Name", text_buffer, sizeof(text_buffer))) {
			printf("Text changed: %s\n", text_buffer);
		}

		mkui_separator();

		// Combo box
		mkui_text("Dropdown menu:");
		if(mkui_combo("Choice", &combo_selection, combo_items, 4)) {
			printf("Selection changed to: %s\n", combo_items[combo_selection]);
		}

		mkui_separator();

		// Listbox (scrollable list)
		mkui_text("Scrollable list (use mouse wheel):");
		if(mkui_listbox("Items", &listbox_selection, listbox_items, 15, 5)) {
			printf("Listbox selection: %s\n", listbox_items[listbox_selection]);
		}
		snprintf(buf, sizeof(buf), "Selected: %s", listbox_items[listbox_selection]);
		mkui_text(buf);

		mkui_separator();

		// Collapsible header
		if(mkui_collapsing_header("Advanced Options", &advanced_header_open)) {
			mkui_text("These are advanced settings!");
			mkui_text("You can hide/show this section.");
			if(mkui_button("Reset All")) {
				slider_value = 0.5f;
				int_slider_value = 50;
				radio_selection = 0;
				combo_selection = 0;
				printf("Settings reset!\n");
			}
		}

		mkui_end_window();

		// Another window
		mkui_begin_window("Info", &window2_x, &window2_y, 300, 200);
		mkui_text("Framebuffer size:");
		snprintf(buf, sizeof(buf), "%d x %d", display_w, display_h);
		mkui_text(buf);

		mkui_separator();

		mkui_text("Mouse position:");
		snprintf(buf, sizeof(buf), "%d, %d", mkfw->mouse_x, mkfw->mouse_y);
		mkui_text(buf);

		mkui_end_window();

		// Render UI
		mkui_render();

		// Swap buffers
		mkfw_swap_buffers(mkfw);
	}

	// Cleanup
	mkui_shutdown();
	mkfw_cleanup(mkfw);

	return 0;
}
