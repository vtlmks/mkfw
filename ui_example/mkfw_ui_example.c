// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

// Example usage of MKFW UI System
// Compile with: -DMKFW_UI

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MKFW_UI
#include "../mkfw_gl_loader.h"
#include "../mkfw.h"

static void on_drop(uint32_t count, const char **paths) {
	for(uint32_t i = 0; i < count; ++i) {
		printf("Dropped: %s\n", paths[i]);
	}
}

int main() {
	// Initialize MKFW window
	struct mkfw_state *mkfw = mkfw_init(1280, 720);
	if(!mkfw) {
		printf("Failed to initialize MKFW\n");
		return 1;
	}

	mkfw_set_window_title(mkfw, "MKFW UI Example");
	mkfw_set_drop_callback(mkfw, on_drop);
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

	// New widget state
	float window3_x = 50.f;
	float window3_y = 500.f;

	int32_t big_combo_selection = 0;
	char *big_combo_items[] = {
		"NROM", "SxROM/MMC1", "UxROM", "CNROM", "TxROM/MMC3",
		"ExROM/MMC5", "AxROM", "BxROM", "GxROM", "ColorDreams",
		"CPROM", "BNROM", "NINA-001", "RAMBO-1", "Jaleco SS8806",
		"Namco 129/163", "VRC4a", "VRC2a", "VRC6a", "VRC4c"
	};

	int32_t table_selection = 0;
	float col_widths[] = {80, 120, 60, 80};
	char *col_headers[] = {"CRC32", "Mapper", "Region", "Mirror"};
	char *table_data[] = {
		"A1B2C3D4", "NROM",       "US",  "Horz",
		"E5F60718", "MMC1",       "JP",  "Vert",
		"92A3B4C5", "MMC3",       "EU",  "Horz",
		"D6E7F809", "UxROM",      "US",  "Vert",
		"1A2B3C4D", "CNROM",      "JP",  "Horz",
		"5E6F7081", "MMC5",       "US",  "4-Scr",
		"92031415", "AxROM",      "EU",  "1-Scr",
		"26374859", "MMC3",       "JP",  "Vert",
		"6A7B8C9D", "MMC1",       "US",  "Horz",
		"AE0F1021", "ColorDrmz",  "JP",  "Vert",
		"32435465", "VRC6",       "JP",  "Vert",
		"76879809", "Namco163",   "JP",  "Horz",
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

		// New widgets window
		mkui_begin_window("New Widgets", &window3_x, &window3_y, 500, 450);

		mkui_text("Combo with 20 entries (scrollable popup):");
		if(mkui_combo("Mapper", &big_combo_selection, big_combo_items, 20)) {
			printf("Mapper: %s\n", big_combo_items[big_combo_selection]);
		}
		snprintf(buf, sizeof(buf), "Selected mapper: %s", big_combo_items[big_combo_selection]);
		mkui_text(buf);

		mkui_separator();

		mkui_text("Table/grid widget:");
		if(mkui_table("##db", 4, col_widths, col_headers, table_data, 12, 6, &table_selection)) {
			printf("Table row: %d\n", table_selection);
		}
		snprintf(buf, sizeof(buf), "Selected row: %d  CRC: %s", table_selection, table_data[table_selection * 4]);
		mkui_text(buf);

		mkui_separator();

		mkui_text("Scroll region:");
		mkui_begin_scroll_region("##scroll", 300, 80);
		for(int32_t i = 0; i < 20; ++i) {
			snprintf(buf, sizeof(buf), "Scrollable item %d", i);
			mkui_text(buf);
		}
		mkui_end_scroll_region();

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
