// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

// MKFW Immediate Mode UI Library
// Simple, lightweight UI system inspired by Dear ImGui

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// EMBEDDED BITMAP FONT DATA (8x8 ASCII font)
// ============================================================================

static uint8_t mkui_font_bitmap[128][8] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 1
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 2
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 3
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 4
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 5
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 6
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 7
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 8
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 9
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 10
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 11
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 12
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 13
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 14
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 15
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 16
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 17
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 18
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 19
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 20
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 21
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 22
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 23
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 24
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 25
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 26
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 27
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 28
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 29
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 30
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 31
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 32 (space)
	{ 0x18, 0x3c, 0x3c, 0x18, 0x18, 0x00, 0x18, 0x00 }, // 33 !
	{ 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 34 "
	{ 0x36, 0x36, 0x7f, 0x36, 0x7f, 0x36, 0x36, 0x00 }, // 35 #
	{ 0x0c, 0x3e, 0x03, 0x1e, 0x30, 0x1f, 0x0c, 0x00 }, // 36 $
	{ 0x00, 0x63, 0x33, 0x18, 0x0c, 0x66, 0x63, 0x00 }, // 37 %
	{ 0x1c, 0x36, 0x1c, 0x6e, 0x3b, 0x33, 0x6e, 0x00 }, // 38 &
	{ 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 39 '
	{ 0x18, 0x0c, 0x06, 0x06, 0x06, 0x0c, 0x18, 0x00 }, // 40 (
	{ 0x06, 0x0c, 0x18, 0x18, 0x18, 0x0c, 0x06, 0x00 }, // 41 )
	{ 0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00 }, // 42 *
	{ 0x00, 0x0c, 0x0c, 0x3f, 0x0c, 0x0c, 0x00, 0x00 }, // 43 +
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x06 }, // 44 ,
	{ 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00 }, // 45 -
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x00 }, // 46 .
	{ 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x01, 0x00 }, // 47 /
	{ 0x3e, 0x63, 0x73, 0x7b, 0x6f, 0x67, 0x3e, 0x00 }, // 48 0
	{ 0x0c, 0x0e, 0x0c, 0x0c, 0x0c, 0x0c, 0x3f, 0x00 }, // 49 1
	{ 0x1e, 0x33, 0x30, 0x1c, 0x06, 0x33, 0x3f, 0x00 }, // 50 2
	{ 0x1e, 0x33, 0x30, 0x1c, 0x30, 0x33, 0x1e, 0x00 }, // 51 3
	{ 0x38, 0x3c, 0x36, 0x33, 0x7f, 0x30, 0x78, 0x00 }, // 52 4
	{ 0x3f, 0x03, 0x1f, 0x30, 0x30, 0x33, 0x1e, 0x00 }, // 53 5
	{ 0x1c, 0x06, 0x03, 0x1f, 0x33, 0x33, 0x1e, 0x00 }, // 54 6
	{ 0x3f, 0x33, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x00 }, // 55 7
	{ 0x1e, 0x33, 0x33, 0x1e, 0x33, 0x33, 0x1e, 0x00 }, // 56 8
	{ 0x1e, 0x33, 0x33, 0x3e, 0x30, 0x18, 0x0e, 0x00 }, // 57 9
	{ 0x00, 0x0c, 0x0c, 0x00, 0x00, 0x0c, 0x0c, 0x00 }, // 58 :
	{ 0x00, 0x0c, 0x0c, 0x00, 0x00, 0x0c, 0x0c, 0x06 }, // 59 ;
	{ 0x18, 0x0c, 0x06, 0x03, 0x06, 0x0c, 0x18, 0x00 }, // 60 <
	{ 0x00, 0x00, 0x3f, 0x00, 0x00, 0x3f, 0x00, 0x00 }, // 61 =
	{ 0x06, 0x0c, 0x18, 0x30, 0x18, 0x0c, 0x06, 0x00 }, // 62 >
	{ 0x1e, 0x33, 0x30, 0x18, 0x0c, 0x00, 0x0c, 0x00 }, // 63 ?
	{ 0x3e, 0x63, 0x7b, 0x7b, 0x7b, 0x03, 0x1e, 0x00 }, // 64 @
	{ 0x0c, 0x1e, 0x33, 0x33, 0x3f, 0x33, 0x33, 0x00 }, // 65 A
	{ 0x3f, 0x66, 0x66, 0x3e, 0x66, 0x66, 0x3f, 0x00 }, // 66 B
	{ 0x3c, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3c, 0x00 }, // 67 C
	{ 0x1f, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1f, 0x00 }, // 68 D
	{ 0x7f, 0x46, 0x16, 0x1e, 0x16, 0x46, 0x7f, 0x00 }, // 69 E
	{ 0x7f, 0x46, 0x16, 0x1e, 0x16, 0x06, 0x0f, 0x00 }, // 70 F
	{ 0x3c, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7c, 0x00 }, // 71 G
	{ 0x33, 0x33, 0x33, 0x3f, 0x33, 0x33, 0x33, 0x00 }, // 72 H
	{ 0x1e, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x1e, 0x00 }, // 73 I
	{ 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1e, 0x00 }, // 74 J
	{ 0x67, 0x66, 0x36, 0x1e, 0x36, 0x66, 0x67, 0x00 }, // 75 K
	{ 0x0f, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7f, 0x00 }, // 76 L
	{ 0x63, 0x77, 0x7f, 0x7f, 0x6b, 0x63, 0x63, 0x00 }, // 77 M
	{ 0x63, 0x67, 0x6f, 0x7b, 0x73, 0x63, 0x63, 0x00 }, // 78 N
	{ 0x1c, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1c, 0x00 }, // 79 O
	{ 0x3f, 0x66, 0x66, 0x3e, 0x06, 0x06, 0x0f, 0x00 }, // 80 P
	{ 0x1e, 0x33, 0x33, 0x33, 0x3b, 0x1e, 0x38, 0x00 }, // 81 Q
	{ 0x3f, 0x66, 0x66, 0x3e, 0x36, 0x66, 0x67, 0x00 }, // 82 R
	{ 0x1e, 0x33, 0x07, 0x0e, 0x38, 0x33, 0x1e, 0x00 }, // 83 S
	{ 0x3f, 0x2d, 0x0c, 0x0c, 0x0c, 0x0c, 0x1e, 0x00 }, // 84 T
	{ 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3f, 0x00 }, // 85 U
	{ 0x33, 0x33, 0x33, 0x33, 0x33, 0x1e, 0x0c, 0x00 }, // 86 V
	{ 0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00 }, // 87 W
	{ 0x63, 0x63, 0x36, 0x1c, 0x1c, 0x36, 0x63, 0x00 }, // 88 X
	{ 0x33, 0x33, 0x33, 0x1e, 0x0c, 0x0c, 0x1e, 0x00 }, // 89 Y
	{ 0x7f, 0x63, 0x31, 0x18, 0x4c, 0x66, 0x7f, 0x00 }, // 90 Z
	{ 0x1e, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1e, 0x00 }, // 91 [
	{ 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x40, 0x00 }, // 92 backslash
	{ 0x1e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1e, 0x00 }, // 93 ]
	{ 0x08, 0x1c, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00 }, // 94 ^
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff }, // 95 _
	{ 0x0c, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 96 `
	{ 0x00, 0x00, 0x1e, 0x30, 0x3e, 0x33, 0x6e, 0x00 }, // 97 a
	{ 0x07, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x3b, 0x00 }, // 98 b
	{ 0x00, 0x00, 0x1e, 0x33, 0x03, 0x33, 0x1e, 0x00 }, // 99 c
	{ 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6e, 0x00 }, // 100 d
	{ 0x00, 0x00, 0x1e, 0x33, 0x3f, 0x03, 0x1e, 0x00 }, // 101 e
	{ 0x1c, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0f, 0x00 }, // 102 f
	{ 0x00, 0x00, 0x6e, 0x33, 0x33, 0x3e, 0x30, 0x1f }, // 103 g
	{ 0x07, 0x06, 0x36, 0x6e, 0x66, 0x66, 0x67, 0x00 }, // 104 h
	{ 0x0c, 0x00, 0x0e, 0x0c, 0x0c, 0x0c, 0x1e, 0x00 }, // 105 i
	{ 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1e }, // 106 j
	{ 0x07, 0x06, 0x66, 0x36, 0x1e, 0x36, 0x67, 0x00 }, // 107 k
	{ 0x0e, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x1e, 0x00 }, // 108 l
	{ 0x00, 0x00, 0x33, 0x7f, 0x7f, 0x6b, 0x63, 0x00 }, // 109 m
	{ 0x00, 0x00, 0x1f, 0x33, 0x33, 0x33, 0x33, 0x00 }, // 110 n
	{ 0x00, 0x00, 0x1e, 0x33, 0x33, 0x33, 0x1e, 0x00 }, // 111 o
	{ 0x00, 0x00, 0x3b, 0x66, 0x66, 0x3e, 0x06, 0x0f }, // 112 p
	{ 0x00, 0x00, 0x6e, 0x33, 0x33, 0x3e, 0x30, 0x78 }, // 113 q
	{ 0x00, 0x00, 0x3b, 0x6e, 0x66, 0x06, 0x0f, 0x00 }, // 114 r
	{ 0x00, 0x00, 0x3e, 0x03, 0x1e, 0x30, 0x1f, 0x00 }, // 115 s
	{ 0x08, 0x0c, 0x3e, 0x0c, 0x0c, 0x2c, 0x18, 0x00 }, // 116 t
	{ 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6e, 0x00 }, // 117 u
	{ 0x00, 0x00, 0x33, 0x33, 0x33, 0x1e, 0x0c, 0x00 }, // 118 v
	{ 0x00, 0x00, 0x63, 0x6b, 0x7f, 0x7f, 0x36, 0x00 }, // 119 w
	{ 0x00, 0x00, 0x63, 0x36, 0x1c, 0x36, 0x63, 0x00 }, // 120 x
	{ 0x00, 0x00, 0x33, 0x33, 0x33, 0x3e, 0x30, 0x1f }, // 121 y
	{ 0x00, 0x00, 0x3f, 0x19, 0x0c, 0x26, 0x3f, 0x00 }, // 122 z
	{ 0x38, 0x0c, 0x0c, 0x07, 0x0c, 0x0c, 0x38, 0x00 }, // 123 {
	{ 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 }, // 124 |
	{ 0x07, 0x0c, 0x0c, 0x38, 0x0c, 0x0c, 0x07, 0x00 }, // 125 }
	{ 0x6e, 0x3b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 126 ~
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }  // 127
};

// ============================================================================
// COLOR DEFINITIONS
// ============================================================================

struct mkui_color {
	float r, g, b, a;
};

static inline struct mkui_color mkui_rgba(float r, float g, float b, float a) {
	struct mkui_color c = {r, g, b, a};
	return c;
}

static inline struct mkui_color mkui_rgb(float r, float g, float b) {
	return mkui_rgba(r, g, b, 1.0f);
}

// Default Dark Theme Colors (like ImGui)
struct mkui_style {
	struct mkui_color text;
	struct mkui_color text_disabled;
	struct mkui_color window_bg;
	struct mkui_color child_bg;
	struct mkui_color border;
	struct mkui_color frame_bg;
	struct mkui_color frame_bg_hovered;
	struct mkui_color frame_bg_active;
	struct mkui_color title_bg;
	struct mkui_color title_bg_active;
	struct mkui_color button;
	struct mkui_color button_hovered;
	struct mkui_color button_active;
	struct mkui_color checkmark;
	struct mkui_color slider_grab;
	struct mkui_color slider_grab_active;
};

static struct mkui_style mkui_default_style() {
	struct mkui_style s;
	s.text                  = mkui_rgb(1.00f, 1.00f, 1.00f);
	s.text_disabled         = mkui_rgb(0.50f, 0.50f, 0.50f);
	s.window_bg             = mkui_rgba(0.06f, 0.06f, 0.06f, 0.94f);
	s.child_bg              = mkui_rgba(0.00f, 0.00f, 0.00f, 0.00f);
	s.border                = mkui_rgba(0.43f, 0.43f, 0.50f, 0.50f);
	s.frame_bg              = mkui_rgba(0.16f, 0.29f, 0.48f, 0.54f);
	s.frame_bg_hovered      = mkui_rgba(0.26f, 0.59f, 0.98f, 0.40f);
	s.frame_bg_active       = mkui_rgba(0.26f, 0.59f, 0.98f, 0.67f);
	s.title_bg              = mkui_rgba(0.04f, 0.04f, 0.04f, 1.00f);
	s.title_bg_active       = mkui_rgba(0.16f, 0.29f, 0.48f, 1.00f);
	s.button                = mkui_rgba(0.26f, 0.59f, 0.98f, 0.40f);
	s.button_hovered        = mkui_rgba(0.26f, 0.59f, 0.98f, 1.00f);
	s.button_active         = mkui_rgba(0.06f, 0.53f, 0.98f, 1.00f);
	s.checkmark             = mkui_rgba(0.26f, 0.59f, 0.98f, 1.00f);
	s.slider_grab           = mkui_rgba(0.24f, 0.52f, 0.88f, 1.00f);
	s.slider_grab_active    = mkui_rgba(0.26f, 0.59f, 0.98f, 1.00f);
	return s;
}

// ============================================================================
// DRAWING PRIMITIVES
// ============================================================================

struct mkui_vec2 {
	float x, y;
};

struct mkui_rect {
	float x, y, w, h;
};

#define MKUI_MAX_VERTICES 65536
#define MKUI_MAX_DRAW_CMDS 256

struct mkui_vertex {
	float x, y;
	float u, v;
	struct mkui_color color;
};

struct mkui_draw_cmd {
	uint32_t vertex_offset;
	uint32_t vertex_count;
	int32_t scissor_x, scissor_y, scissor_w, scissor_h;
};

struct mkui_draw_list {
	struct mkui_vertex vertices[MKUI_MAX_VERTICES];
	uint32_t vertex_count;

	struct mkui_draw_cmd commands[MKUI_MAX_DRAW_CMDS];
	uint32_t cmd_count;

	GLuint vao;
	GLuint vbo;
	GLuint shader_program;
	GLint u_projection;
	GLint u_texture;
	GLuint font_texture;

	int32_t display_width;
	int32_t display_height;
};

// ============================================================================
// UI STATE
// ============================================================================

struct mkui_combo_popup {
	uint32_t active;
	uint32_t id;
	float x, y, w, item_h;
	char **items;
	int32_t items_count;
	int32_t *current_item;
	int32_t scroll_offset;
	int32_t changed;
};

struct mkui_scroll_region_state {
	float start_x, start_y;
	float w, h;
};

struct mkui_context {
	struct mkfw_state *mkfw;
	struct mkui_style style;
	struct mkui_draw_list draw_list;

	// Layout state
	float cursor_x, cursor_y;
	float item_spacing;
	float frame_padding;
	float window_padding;

	// Input state
	int32_t mouse_x, mouse_y;
	int32_t mouse_down[3];
	int32_t mouse_clicked[3];
	int32_t mouse_released[3];
	double scroll_y;

	// Active item tracking
	uint32_t hot_item;
	uint32_t active_item;

	// Text input state
	char text_input[256];
	int32_t text_input_len;

	// Scissor stack
	struct mkui_rect scissor_stack[8];
	uint32_t scissor_depth;

	// Combo popup overlay
	struct mkui_combo_popup combo_popup;

	// Scroll region stack
	struct mkui_scroll_region_state scroll_region_stack[8];
	uint32_t scroll_region_depth;

};

static struct mkui_context *mkui_ctx;

// ============================================================================
// SHADER CODE
// ============================================================================

static const char *mkui_vertex_shader =
	"#version 130\n"
	"uniform mat4 u_projection;\n"
	"in vec2 a_pos;\n"
	"in vec2 a_uv;\n"
	"in vec4 a_color;\n"
	"out vec2 v_uv;\n"
	"out vec4 v_color;\n"
	"void main() {\n"
	"    v_uv = a_uv;\n"
	"    v_color = a_color;\n"
	"    gl_Position = u_projection * vec4(a_pos, 0.0, 1.0);\n"
	"}\n";

static const char *mkui_fragment_shader =
	"#version 130\n"
	"uniform sampler2D u_texture;\n"
	"in vec2 v_uv;\n"
	"in vec4 v_color;\n"
	"out vec4 frag_color;\n"
	"void main() {\n"
	"    float alpha = texture(u_texture, v_uv).r;\n"
	"    frag_color = vec4(v_color.rgb, v_color.a * alpha);\n"
	"}\n";

static GLuint mkui_create_shader(const char *vertex_src, const char *fragment_src) {
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_src, 0);
	glCompileShader(vertex_shader);

	GLint success;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if(!success) {
		char info[512];
		glGetShaderInfoLog(vertex_shader, 512, 0, info);
		mkfw_error("vertex shader compilation failed: %s", info);
		return 0;
	}

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_src, 0);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if(!success) {
		char info[512];
		glGetShaderInfoLog(fragment_shader, 512, 0, info);
		mkfw_error("fragment shader compilation failed: %s", info);
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success) {
		char info[512];
		glGetProgramInfoLog(program, 512, 0, info);
		mkfw_error("shader linking failed: %s", info);
		return 0;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;
}

// ============================================================================
// FONT TEXTURE CREATION
// ============================================================================


static GLuint mkui_create_font_texture() {
	uint8_t pixels[256*256] = {0};

	// Put a white pixel at (0, 0) for solid rectangles
	pixels[0] = 255;

	for(int32_t i = 0; i < 128; i++) {
		int32_t cx = (i % 16) * 8;
		int32_t cy = (i / 16) * 8;

		for(int32_t y = 0; y < 8; y++) {
			uint8_t row = mkui_font_bitmap[i][y];
			for(int32_t x = 0; x < 8; x++) {
				if(row & (1 << x)) {  // Read bits from right to left
					pixels[(cy + y) * 256 + (cx + x)] = 255;
				}
			}
		}
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLint prev_unpack;
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &prev_unpack);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 256, 256, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, prev_unpack);

	return texture;
}

// ============================================================================
// DRAW LIST OPERATIONS
// ============================================================================

static void mkui_draw_list_init(struct mkui_draw_list *dl) {
	dl->vertex_count = 0;

	// Create VAO and VBO
	glGenVertexArrays(1, &dl->vao);
	glGenBuffers(1, &dl->vbo);

	glBindVertexArray(dl->vao);
	glBindBuffer(GL_ARRAY_BUFFER, dl->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(struct mkui_vertex) * MKUI_MAX_VERTICES, 0, GL_DYNAMIC_DRAW);

	// Create shader
	dl->shader_program = mkui_create_shader(mkui_vertex_shader, mkui_fragment_shader);
	dl->u_projection = glGetUniformLocation(dl->shader_program, "u_projection");
	dl->u_texture = glGetUniformLocation(dl->shader_program, "u_texture");

	// Set up vertex attributes
	GLint a_pos = glGetAttribLocation(dl->shader_program, "a_pos");
	GLint a_uv = glGetAttribLocation(dl->shader_program, "a_uv");
	GLint a_color = glGetAttribLocation(dl->shader_program, "a_color");

	glEnableVertexAttribArray(a_pos);
	glEnableVertexAttribArray(a_uv);
	glEnableVertexAttribArray(a_color);

	glVertexAttribPointer(a_pos, 2, GL_FLOAT, GL_FALSE, sizeof(struct mkui_vertex), (void*)0);
	glVertexAttribPointer(a_uv, 2, GL_FLOAT, GL_FALSE, sizeof(struct mkui_vertex), (void*)(2 * sizeof(float)));
	glVertexAttribPointer(a_color, 4, GL_FLOAT, GL_FALSE, sizeof(struct mkui_vertex), (void*)(4 * sizeof(float)));

	glBindVertexArray(0);

	// Create font texture
	dl->font_texture = mkui_create_font_texture();
}

static void mkui_draw_list_clear(struct mkui_draw_list *dl) {
	dl->vertex_count = 0;
	dl->cmd_count = 1;
	dl->commands[0].vertex_offset = 0;
	dl->commands[0].vertex_count = 0;
	dl->commands[0].scissor_x = 0;
	dl->commands[0].scissor_y = 0;
	dl->commands[0].scissor_w = 0;
	dl->commands[0].scissor_h = 0;
}

static void mkui_draw_list_add_vertex(struct mkui_draw_list *dl, float x, float y, float u, float v, struct mkui_color col) {
	if(dl->vertex_count >= MKUI_MAX_VERTICES) {
		return;
	}

	struct mkui_vertex *vtx = &dl->vertices[dl->vertex_count++];
	vtx->x = x;
	vtx->y = y;
	vtx->u = u;
	vtx->v = v;
	vtx->color = col;

	dl->commands[dl->cmd_count - 1].vertex_count++;
}

static void mkui_draw_rect_filled(struct mkui_draw_list *dl, float x, float y, float w, float h, struct mkui_color col) {
	// Use white pixel at (0,0) for solid color rectangles
	float uv = 0.5f / 256.0f; // Center of pixel (0,0)

	// Two triangles
	mkui_draw_list_add_vertex(dl, x, y, uv, uv, col);
	mkui_draw_list_add_vertex(dl, x + w, y, uv, uv, col);
	mkui_draw_list_add_vertex(dl, x + w, y + h, uv, uv, col);

	mkui_draw_list_add_vertex(dl, x, y, uv, uv, col);
	mkui_draw_list_add_vertex(dl, x + w, y + h, uv, uv, col);
	mkui_draw_list_add_vertex(dl, x, y + h, uv, uv, col);
}

static void mkui_draw_rect_outline(struct mkui_draw_list *dl, float x, float y, float w, float h, float thickness, struct mkui_color col) {
	mkui_draw_rect_filled(dl, x, y, w, thickness, col);																// Top
	mkui_draw_rect_filled(dl, x, y + h - thickness, w, thickness, col);											// Bottom
	mkui_draw_rect_filled(dl, x, y + thickness, thickness, h - 2 * thickness, col);							// Left
	mkui_draw_rect_filled(dl, x + w - thickness, y + thickness, thickness, h - 2 * thickness, col);		// Right
}

static void mkui_draw_text(struct mkui_draw_list *dl, float x, float y, const char *text, struct mkui_color col) {
	float cx = x;
	float cy = y;

	while(*text) {
		uint8_t c = (uint8_t)*text;
		if(c < 128) {
			// Calculate UV coordinates in the font atlas
			float char_x = (c % 16) * 8.0f;
			float char_y = (c / 16) * 8.0f;
			float u0 = char_x / 256.0f;
			float v0 = char_y / 256.0f;
			float u1 = (char_x + 8.0f) / 256.0f;
			float v1 = (char_y + 8.0f) / 256.0f;

			// Draw character quad
			mkui_draw_list_add_vertex(dl, cx, cy, u0, v0, col);
			mkui_draw_list_add_vertex(dl, cx + 8, cy, u1, v0, col);
			mkui_draw_list_add_vertex(dl, cx + 8, cy + 8, u1, v1, col);

			mkui_draw_list_add_vertex(dl, cx, cy, u0, v0, col);
			mkui_draw_list_add_vertex(dl, cx + 8, cy + 8, u1, v1, col);
			mkui_draw_list_add_vertex(dl, cx, cy + 8, u0, v1, col);

			cx += 8;
		}
		text++;
	}
}

static void mkui_push_scissor(float x, float y, float w, float h) {
	struct mkui_draw_list *dl = &mkui_ctx->draw_list;

	if(mkui_ctx->scissor_depth > 0) {
		struct mkui_rect *parent = &mkui_ctx->scissor_stack[mkui_ctx->scissor_depth - 1];
		float x2 = x + w;
		float y2 = y + h;
		float px2 = parent->x + parent->w;
		float py2 = parent->y + parent->h;
		if(x < parent->x) {
			x = parent->x;
		}
		if(y < parent->y) {
			y = parent->y;
		}
		if(x2 > px2) {
			x2 = px2;
		}
		if(y2 > py2) {
			y2 = py2;
		}
		w = x2 - x;
		h = y2 - y;
		if(w < 0) {
			w = 0;
		}
		if(h < 0) {
			h = 0;
		}
	}

	if(mkui_ctx->scissor_depth < 8) {
		struct mkui_rect *r = &mkui_ctx->scissor_stack[mkui_ctx->scissor_depth++];
		r->x = x;
		r->y = y;
		r->w = w;
		r->h = h;
	}

	if(dl->cmd_count < MKUI_MAX_DRAW_CMDS) {
		struct mkui_draw_cmd *cmd = &dl->commands[dl->cmd_count++];
		cmd->vertex_offset = dl->vertex_count;
		cmd->vertex_count = 0;
		cmd->scissor_x = (int32_t)x;
		cmd->scissor_y = (int32_t)y;
		cmd->scissor_w = (int32_t)w;
		cmd->scissor_h = (int32_t)h;
	}
}

static void mkui_pop_scissor() {
	struct mkui_draw_list *dl = &mkui_ctx->draw_list;

	if(mkui_ctx->scissor_depth > 0) {
		mkui_ctx->scissor_depth--;
	}

	if(dl->cmd_count < MKUI_MAX_DRAW_CMDS) {
		struct mkui_draw_cmd *cmd = &dl->commands[dl->cmd_count++];
		cmd->vertex_offset = dl->vertex_count;
		cmd->vertex_count = 0;

		if(mkui_ctx->scissor_depth > 0) {
			struct mkui_rect *r = &mkui_ctx->scissor_stack[mkui_ctx->scissor_depth - 1];
			cmd->scissor_x = (int32_t)r->x;
			cmd->scissor_y = (int32_t)r->y;
			cmd->scissor_w = (int32_t)r->w;
			cmd->scissor_h = (int32_t)r->h;
		} else {
			cmd->scissor_x = 0;
			cmd->scissor_y = 0;
			cmd->scissor_w = 0;
			cmd->scissor_h = 0;
		}
	}
}

static void mkui_draw_list_render(struct mkui_draw_list *dl) {
	if(dl->vertex_count == 0) {
		return;
	}

	// Save GL state
	GLint last_program, last_texture, last_vao, last_vbo;
	GLint last_viewport[4];
	GLint last_scissor[4];
	GLboolean last_blend, last_scissor_test, last_depth_test, last_cull_face;

	glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vao);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_vbo);
	glGetIntegerv(GL_VIEWPORT, last_viewport);
	glGetIntegerv(GL_SCISSOR_BOX, last_scissor);
	last_blend = glIsEnabled(GL_BLEND);
	last_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
	last_depth_test = glIsEnabled(GL_DEPTH_TEST);
	last_cull_face = glIsEnabled(GL_CULL_FACE);

	// Setup render state
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);

	// Setup viewport and projection
	glViewport(0, 0, dl->display_width, dl->display_height);

	float L = 0.0f;
	float R = (float)dl->display_width;
	float T = 0.0f;
	float B = (float)dl->display_height;
	float ortho_projection[4][4] = {
		{ 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
		{ 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
		{ 0.0f,         0.0f,        -1.0f,   0.0f },
		{ (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
	};

	glUseProgram(dl->shader_program);
	glUniform1i(dl->u_texture, 0);
	glUniformMatrix4fv(dl->u_projection, 1, GL_FALSE, &ortho_projection[0][0]);

	glBindVertexArray(dl->vao);
	glBindBuffer(GL_ARRAY_BUFFER, dl->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, dl->vertex_count * sizeof(struct mkui_vertex), dl->vertices);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, dl->font_texture);

	for(uint32_t i = 0; i < dl->cmd_count; ++i) {
		struct mkui_draw_cmd *cmd = &dl->commands[i];
		if(cmd->vertex_count == 0) {
			continue;
		}
		if(cmd->scissor_w > 0 && cmd->scissor_h > 0) {
			glEnable(GL_SCISSOR_TEST);
			glScissor(cmd->scissor_x, dl->display_height - cmd->scissor_y - cmd->scissor_h, cmd->scissor_w, cmd->scissor_h);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
		glDrawArrays(GL_TRIANGLES, cmd->vertex_offset, cmd->vertex_count);
	}
	glDisable(GL_SCISSOR_TEST);

	// Restore GL state
	glUseProgram(last_program);
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindVertexArray(last_vao);
	glBindBuffer(GL_ARRAY_BUFFER, last_vbo);
	glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
	glScissor(last_scissor[0], last_scissor[1], last_scissor[2], last_scissor[3]);
	if(last_blend) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
	if(last_scissor_test) {
		glEnable(GL_SCISSOR_TEST);
	} else {
		glDisable(GL_SCISSOR_TEST);
	}
	if(last_depth_test) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
	if(last_cull_face) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
}

// ============================================================================
// MKUI INITIALIZATION
// ============================================================================

static void mkui_char_callback(struct mkfw_state *state, uint32_t codepoint) {
	if(!mkui_ctx) {
		return;
	}

	if(mkui_ctx->active_item != 0) {
		if(codepoint == 8) {  // Backspace
			if(mkui_ctx->text_input_len > 0) {
				mkui_ctx->text_input_len--;
				mkui_ctx->text_input[mkui_ctx->text_input_len] = 0;
			}
		} else if(codepoint >= 32 && codepoint < 128 && mkui_ctx->text_input_len < 255) {
			mkui_ctx->text_input[mkui_ctx->text_input_len++] = (char)codepoint;
			mkui_ctx->text_input[mkui_ctx->text_input_len] = 0;
		}
	}
}

static void mkui_scroll_callback(struct mkfw_state *state, double xoffset, double yoffset) {
	if(!mkui_ctx) {
		return;
	}
	mkui_ctx->scroll_y += yoffset;
}

static void mkui_init(struct mkfw_state *mkfw) {
	if(mkui_ctx) return;

	mkui_ctx = (struct mkui_context *)calloc(1, sizeof(struct mkui_context));
	mkui_ctx->mkfw = mkfw;
	mkui_ctx->style = mkui_default_style();
	mkui_ctx->item_spacing = 4.0f;
	mkui_ctx->frame_padding = 4.0f;
	mkui_ctx->window_padding = 8.0f;

	mkui_draw_list_init(&mkui_ctx->draw_list);

	// Set up input callbacks
	mkfw_set_char_callback(mkfw, mkui_char_callback);
	mkfw_set_scroll_callback(mkfw, mkui_scroll_callback);
}

static void mkui_shutdown() {
	if(!mkui_ctx) {
		return;
	}

	glDeleteVertexArrays(1, &mkui_ctx->draw_list.vao);
	glDeleteBuffers(1, &mkui_ctx->draw_list.vbo);
	glDeleteProgram(mkui_ctx->draw_list.shader_program);
	glDeleteTextures(1, &mkui_ctx->draw_list.font_texture);

	free(mkui_ctx);
	mkui_ctx = 0;
}

static void mkui_new_frame(int32_t display_w, int32_t display_h) {
	if(!mkui_ctx) {
		return;
	}

	mkui_draw_list_clear(&mkui_ctx->draw_list);
	mkui_ctx->draw_list.display_width = display_w;
	mkui_ctx->draw_list.display_height = display_h;

	// Update input state
	mkui_ctx->mouse_x = mkui_ctx->mkfw->mouse_x;
	mkui_ctx->mouse_y = mkui_ctx->mkfw->mouse_y;

	for(int32_t i = 0; i < 3; i++) {
		int32_t was_down = mkui_ctx->mouse_down[i];
		mkui_ctx->mouse_down[i] = mkui_ctx->mkfw->mouse_buttons[i];
		mkui_ctx->mouse_clicked[i] = !was_down && mkui_ctx->mouse_down[i];
		mkui_ctx->mouse_released[i] = was_down && !mkui_ctx->mouse_down[i];
	}

	// Reset layout
	mkui_ctx->cursor_x = mkui_ctx->window_padding;
	mkui_ctx->cursor_y = mkui_ctx->window_padding;

	// Clear hot item if mouse not down
	if(!mkui_ctx->mouse_down[0]) {
		mkui_ctx->hot_item = 0;
	}
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static int32_t mkui_is_mouse_over(float x, float y, float w, float h) {
	return mkui_ctx->mouse_x >= x && mkui_ctx->mouse_x < x + w && mkui_ctx->mouse_y >= y && mkui_ctx->mouse_y < y + h;
}

static uint32_t mkui_gen_id(const char *label) {
	// FNV-1a hash (offset basis ensures empty string doesn't return 0)
	uint32_t hash = 2166136261u;
	while(*label) {
		hash ^= (uint8_t)*label;
		hash *= 16777619u;
		label++;
	}
	return hash;
}

static void mkui_set_cursor_pos(float x, float y) {
	mkui_ctx->cursor_x = x;
	mkui_ctx->cursor_y = y;
}

static void mkui_same_line() {
	// Move cursor back up by typical item height
	mkui_ctx->cursor_y -= (16 + mkui_ctx->item_spacing);
	mkui_ctx->cursor_x += 100; // Arbitrary offset for same line items
}

// [=]===^=[ mkui_draw_combo_popup ]========================================[=]
static void mkui_draw_combo_popup() {
	struct mkui_combo_popup *p = &mkui_ctx->combo_popup;
	if(!p->active) {
		return;
	}

	struct mkui_draw_list *dl = &mkui_ctx->draw_list;
	int32_t max_visible = 8;
	int32_t visible = p->items_count < max_visible ? p->items_count : max_visible;
	float popup_h = visible * p->item_h;

	mkui_draw_rect_filled(dl, p->x, p->y, p->w, popup_h, mkui_ctx->style.window_bg);
	mkui_draw_rect_outline(dl, p->x, p->y, p->w, popup_h, 1, mkui_ctx->style.border);

	mkui_push_scissor(p->x, p->y, p->w, popup_h);

	for(int32_t i = 0; i < visible; ++i) {
		int32_t idx = i + p->scroll_offset;
		if(idx >= p->items_count) {
			break;
		}
		float iy = p->y + i * p->item_h;
		int32_t is_selected = (idx == *p->current_item);
		int32_t item_hovered = mkui_is_mouse_over(p->x, iy, p->w, p->item_h);
		struct mkui_color item_bg = is_selected ? mkui_ctx->style.frame_bg_active : (item_hovered ? mkui_ctx->style.frame_bg_hovered : mkui_ctx->style.frame_bg);
		mkui_draw_rect_filled(dl, p->x, iy, p->w, p->item_h, item_bg);
		mkui_draw_text(dl, p->x + 4, iy + 6, p->items[idx], mkui_ctx->style.text);
	}

	mkui_pop_scissor();

	p->active = 0;
}

// [=]===^=[ mkui_render ]==================================================[=]
static void mkui_render() {
	if(!mkui_ctx) {
		return;
	}

	mkui_draw_combo_popup();
	mkui_draw_list_render(&mkui_ctx->draw_list);

	mkui_ctx->scroll_y = 0.0;
}

// ============================================================================
// WIDGETS
// ============================================================================

static void mkui_text(const char *text) {
	mkui_draw_text(&mkui_ctx->draw_list, mkui_ctx->cursor_x, mkui_ctx->cursor_y, text, mkui_ctx->style.text);
	mkui_ctx->cursor_y += 16 + mkui_ctx->item_spacing;
}

static void mkui_text_colored(const char *text, struct mkui_color color) {
	mkui_draw_text(&mkui_ctx->draw_list, mkui_ctx->cursor_x, mkui_ctx->cursor_y, text, color);
	mkui_ctx->cursor_y += 16 + mkui_ctx->item_spacing;
}

static int32_t mkui_button(const char *label) {
	uint32_t id = mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float w = (float)(strlen(label) * 8) + mkui_ctx->frame_padding * 2;
	float h = 10 + mkui_ctx->frame_padding * 2;

	int32_t hovered = mkui_is_mouse_over(x, y, w, h);
	int32_t clicked = 0;

	struct mkui_color bg_color = mkui_ctx->style.button;
	int32_t is_active = (mkui_ctx->active_item == id);

	// Check if mouse is being held down on this button
	if(is_active) {
		bg_color = mkui_ctx->style.button_active;
	} else if(hovered) {
		if(mkui_ctx->mouse_down[0]) {
			bg_color = mkui_ctx->style.button_active;
		} else {
			bg_color = mkui_ctx->style.button_hovered;
		}
		mkui_ctx->hot_item = id;
	}

	// Detect click: mouse was pressed and released while hovered
	if(hovered && mkui_ctx->mouse_clicked[0]) {
		clicked = 1;
	}

	// Draw button
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, w, h, bg_color);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, w, h, 1, mkui_ctx->style.border);
	mkui_draw_text(&mkui_ctx->draw_list, x + mkui_ctx->frame_padding, y + mkui_ctx->frame_padding, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += h + mkui_ctx->item_spacing;

	return clicked;
}

static int32_t mkui_checkbox(const char *label, int32_t *checked) {
	mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float box_size = 16;

	int32_t hovered = mkui_is_mouse_over(x, y, box_size, box_size);
	int32_t clicked = 0;

	if(hovered && mkui_ctx->mouse_clicked[0]) {
		*checked = !(*checked);
		clicked = 1;
	}

	// Draw checkbox box
	struct mkui_color bg = hovered ? mkui_ctx->style.frame_bg_hovered : mkui_ctx->style.frame_bg;
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, box_size, box_size, bg);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, box_size, box_size, 1, mkui_ctx->style.border);

	// Draw checkmark if checked
	if(*checked) {
		mkui_draw_rect_filled(&mkui_ctx->draw_list, x + 4, y + 4, 8, 8, mkui_ctx->style.checkmark);
	}

	// Draw label
	mkui_draw_text(&mkui_ctx->draw_list, x + box_size + 8, y + 4, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += box_size + mkui_ctx->item_spacing;

	return clicked;
}

static int32_t mkui_slider_float(const char *label, float *value, float min_val, float max_val) {
	uint32_t id = mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float w = 200;
	float h = 16;

	int32_t hovered = mkui_is_mouse_over(x, y, w, h);
	int32_t changed = 0;

	if(hovered && mkui_ctx->mouse_clicked[0]) {
		mkui_ctx->active_item = id;
	}

	if(mkui_ctx->active_item == id) {
		if(mkui_ctx->mouse_down[0]) {
			float t = (mkui_ctx->mouse_x - x) / w;
			if(t < 0.0f) {
				t = 0.0f;
			}

			if(t > 1.0f) {
				t = 1.0f;
			}

			float new_value = min_val + t * (max_val - min_val);

			if(new_value != *value) {
				*value = new_value;
				changed = 1;
			}

		} else {
			mkui_ctx->active_item = 0;
		}
	}

	// Mouse wheel support when hovered
	if(hovered && mkui_ctx->scroll_y != 0.0) {
		float range = max_val - min_val;
		float step = range * 0.05f;  // 5% of range per scroll tick
		float new_value = *value + mkui_ctx->scroll_y * step;
		if(new_value < min_val) {
			new_value = min_val;
		}

		if(new_value > max_val) {
			new_value = max_val;
		}

		if(new_value != *value) {
			*value = new_value;
			changed = 1;
		}
	}

	// Draw slider track
	struct mkui_color bg = mkui_ctx->style.frame_bg;
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, w, h, bg);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, w, h, 1, mkui_ctx->style.border);

	// Draw slider grab
	float t = (*value - min_val) / (max_val - min_val);
	float grab_x = x + t * (w - 8);
	struct mkui_color grab_col = (mkui_ctx->active_item == id) ? mkui_ctx->style.slider_grab_active : mkui_ctx->style.slider_grab;
	mkui_draw_rect_filled(&mkui_ctx->draw_list, grab_x, y, 8, h, grab_col);

	// Draw label
	mkui_draw_text(&mkui_ctx->draw_list, x + w + 8, y + 4, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += h + mkui_ctx->item_spacing;

	return changed;
}

static int32_t mkui_slider_int(const char *label, int32_t *value, int32_t min_val, int32_t max_val) {
	uint32_t id = mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float w = 200;
	float h = 16;

	int32_t hovered = mkui_is_mouse_over(x, y, w, h);
	int32_t changed = 0;

	if(hovered && mkui_ctx->mouse_clicked[0]) {
		mkui_ctx->active_item = id;
	}

	if(mkui_ctx->active_item == id) {
		if(mkui_ctx->mouse_down[0]) {
			float t = (mkui_ctx->mouse_x - x) / w;
			if(t < 0.0f) t = 0.0f;
			if(t > 1.0f) t = 1.0f;
			int32_t new_value = min_val + (int32_t)(t * (max_val - min_val));
			if(new_value != *value) {
				*value = new_value;
				changed = 1;
			}
		} else {
			mkui_ctx->active_item = 0;
		}
	}

	// Mouse wheel support when hovered
	if(hovered && mkui_ctx->scroll_y != 0.0) {
		int32_t range = max_val - min_val;
		int32_t step = (range / 20);  // ~5% of range per scroll tick
		if(step < 1) {
			step = 1;  // Minimum step of 1
		}
		int32_t new_value = *value + (int32_t)(mkui_ctx->scroll_y * step);

		if(new_value < min_val) {
			new_value = min_val;
		}

		if(new_value > max_val) {
			new_value = max_val;
		}

		if(new_value != *value) {
			*value = new_value;
			changed = 1;
		}
	}

	// Draw slider track
	struct mkui_color bg = mkui_ctx->style.frame_bg;
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, w, h, bg);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, w, h, 1, mkui_ctx->style.border);

	// Draw slider grab
	float t = (float)(*value - min_val) / (float)(max_val - min_val);
	float grab_x = x + t * (w - 8);
	struct mkui_color grab_col = (mkui_ctx->active_item == id) ? mkui_ctx->style.slider_grab_active : mkui_ctx->style.slider_grab;
	mkui_draw_rect_filled(&mkui_ctx->draw_list, grab_x, y, 8, h, grab_col);

	// Draw label
	mkui_draw_text(&mkui_ctx->draw_list, x + w + 8, y + 4, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += h + mkui_ctx->item_spacing;

	return changed;
}

static int32_t mkui_slider_int64(const char *label, int64_t *value, int64_t min_val, int64_t max_val) {
	uint32_t id = mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float w = 200;
	float h = 16;

	int32_t hovered = mkui_is_mouse_over(x, y, w, h);
	int32_t changed = 0;

	if(hovered && mkui_ctx->mouse_clicked[0]) {
		mkui_ctx->active_item = id;
	}

	if(mkui_ctx->active_item == id) {
		if(mkui_ctx->mouse_down[0]) {
			double t = (mkui_ctx->mouse_x - x) / w;
			if(t < 0.0) t = 0.0;
			if(t > 1.0) t = 1.0;
			int64_t new_value = min_val + (int64_t)(t * (double)(max_val - min_val));
			if(new_value != *value) {
				*value = new_value;
				changed = 1;
			}
		} else {
			mkui_ctx->active_item = 0;
		}
	}

	// Mouse wheel support when hovered
	if(hovered && mkui_ctx->scroll_y != 0.0) {
		int64_t range = max_val - min_val;
		int64_t step = (range / 20);  // ~5% of range per scroll tick
		if(step < 1) step = 1;  // Minimum step of 1
		int64_t new_value = *value + (int64_t)(mkui_ctx->scroll_y * step);
		if(new_value < min_val) new_value = min_val;
		if(new_value > max_val) new_value = max_val;
		if(new_value != *value) {
			*value = new_value;
			changed = 1;
		}
	}

	// Draw slider track
	struct mkui_color bg = mkui_ctx->style.frame_bg;
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, w, h, bg);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, w, h, 1, mkui_ctx->style.border);

	// Draw slider grab
	double t = (double)(*value - min_val) / (double)(max_val - min_val);
	float grab_x = x + (float)t * (w - 8);
	struct mkui_color grab_col = (mkui_ctx->active_item == id) ? mkui_ctx->style.slider_grab_active : mkui_ctx->style.slider_grab;
	mkui_draw_rect_filled(&mkui_ctx->draw_list, grab_x, y, 8, h, grab_col);

	// Draw label
	mkui_draw_text(&mkui_ctx->draw_list, x + w + 8, y + 4, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += h + mkui_ctx->item_spacing;

	return changed;
}

static int32_t mkui_radio_button(const char *label, int32_t *selected, int32_t value) {
	mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float circle_size = 16;

	int32_t hovered = mkui_is_mouse_over(x, y, circle_size, circle_size);
	int32_t clicked = 0;

	if(hovered && mkui_ctx->mouse_clicked[0]) {
		*selected = value;
		clicked = 1;
	}

	// Draw radio button circle (as a square for now with the simple font)
	struct mkui_color bg = hovered ? mkui_ctx->style.frame_bg_hovered : mkui_ctx->style.frame_bg;
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, circle_size, circle_size, bg);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, circle_size, circle_size, 1, mkui_ctx->style.border);

	// Draw dot if selected
	if(*selected == value) {
		mkui_draw_rect_filled(&mkui_ctx->draw_list, x + 4, y + 4, 8, 8, mkui_ctx->style.checkmark);
	}

	// Draw label
	mkui_draw_text(&mkui_ctx->draw_list, x + circle_size + 8, y + 4, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += circle_size + mkui_ctx->item_spacing;

	return clicked;
}

static int32_t mkui_collapsing_header(const char *label, int32_t *open) {
	mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float w = 200;
	float h = 20;

	int32_t hovered = mkui_is_mouse_over(x, y, w, h);

	if(hovered && mkui_ctx->mouse_clicked[0]) {
		*open = !(*open);
	}

	// Draw header background
	struct mkui_color bg = hovered ? mkui_ctx->style.frame_bg_hovered : mkui_ctx->style.frame_bg;
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, w, h, bg);

	// Draw arrow (> when closed, v when open)
	const char *arrow = *open ? "v" : ">";
	mkui_draw_text(&mkui_ctx->draw_list, x + 4, y + 6, arrow, mkui_ctx->style.text);

	// Draw label
	mkui_draw_text(&mkui_ctx->draw_list, x + 16, y + 6, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += h + mkui_ctx->item_spacing;

	return *open;
}

static int32_t mkui_text_input(const char *label, char *buffer, int32_t buffer_size) {
	uint32_t id = mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float w = 200;
	float h = 20;

	int32_t hovered = mkui_is_mouse_over(x, y, w, h);
	int32_t changed = 0;

	int32_t is_active = (mkui_ctx->active_item == id);

	// Activate on click
	if(hovered && mkui_ctx->mouse_clicked[0]) {
		mkui_ctx->active_item = id;
		// Copy buffer to internal text input
		strncpy(mkui_ctx->text_input, buffer, sizeof(mkui_ctx->text_input) - 1);
		mkui_ctx->text_input_len = strlen(mkui_ctx->text_input);
	} else if(!hovered && mkui_ctx->mouse_clicked[0] && is_active) {
		// Deactivate when clicking outside
		mkui_ctx->active_item = 0;
	}

	// Handle text input when active
	if(is_active) {
		// Copy internal buffer back to user buffer
		strncpy(buffer, mkui_ctx->text_input, buffer_size - 1);
		buffer[buffer_size - 1] = '\0';
		changed = 1;
	}

	// Draw input box
	struct mkui_color bg = is_active ? mkui_ctx->style.frame_bg_active : (hovered ? mkui_ctx->style.frame_bg_hovered : mkui_ctx->style.frame_bg);
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, w, h, bg);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, w, h, 1, mkui_ctx->style.border);

	// Draw text
	mkui_draw_text(&mkui_ctx->draw_list, x + 4, y + 6, buffer, mkui_ctx->style.text);

	// Draw cursor if active
	if(is_active) {
		float cursor_x = x + 4 + strlen(buffer) * 8;
		mkui_draw_rect_filled(&mkui_ctx->draw_list, cursor_x, y + 4, 2, 12, mkui_ctx->style.text);
	}

	// Draw label
	mkui_draw_text(&mkui_ctx->draw_list, x + w + 8, y + 6, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += h + mkui_ctx->item_spacing;

	return changed;
}

static void mkui_image(GLuint texture_id, float width, float height) {
	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;

	// For now, just draw a placeholder rectangle
	// Full texture support would require modifying the shader or adding a second draw call
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, width, height, mkui_rgba(0.5f, 0.5f, 0.5f, 1.0f));
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, width, height, 1, mkui_ctx->style.border);

	mkui_ctx->cursor_y += height + mkui_ctx->item_spacing;
}

static void mkui_image_rgba(uint32_t *rgba_buffer, int32_t width, int32_t height) {
	// This would require creating a temporary texture from the RGBA buffer
	// For now, placeholder implementation
	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;

	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, (float)width, (float)height, mkui_rgba(0.3f, 0.3f, 0.3f, 1.0f));
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, (float)width, (float)height, 1, mkui_ctx->style.border);

	mkui_ctx->cursor_y += height + mkui_ctx->item_spacing;
}

static int32_t mkui_combo(const char *label, int32_t *current_item, char **items, int32_t items_count) {
	uint32_t id = mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float w = 200;
	float h = 20;
	float item_h = 20;
	int32_t max_visible = 8;

	int32_t hovered = mkui_is_mouse_over(x, y, w, h);
	int32_t changed = 0;

	static uint32_t open_combo_id = 0;
	int32_t is_open = (open_combo_id == id);

	if(is_open) {
		int32_t visible = items_count < max_visible ? items_count : max_visible;
		float popup_y = y + h;
		float popup_h = visible * item_h;
		int32_t popup_hovered = mkui_is_mouse_over(x, popup_y, w, popup_h);

		if(popup_hovered && mkui_ctx->scroll_y != 0.0) {
			mkui_ctx->combo_popup.scroll_offset -= (int32_t)mkui_ctx->scroll_y;
			int32_t max_scroll = items_count - visible;
			if(max_scroll < 0) {
				max_scroll = 0;
			}
			if(mkui_ctx->combo_popup.scroll_offset < 0) {
				mkui_ctx->combo_popup.scroll_offset = 0;
			}
			if(mkui_ctx->combo_popup.scroll_offset > max_scroll) {
				mkui_ctx->combo_popup.scroll_offset = max_scroll;
			}
			mkui_ctx->scroll_y = 0.0;
		}

		for(int32_t i = 0; i < visible; ++i) {
			int32_t idx = i + mkui_ctx->combo_popup.scroll_offset;
			if(idx >= items_count) {
				break;
			}
			float iy = popup_y + i * item_h;
			if(mkui_is_mouse_over(x, iy, w, item_h) && mkui_ctx->mouse_clicked[0]) {
				*current_item = idx;
				open_combo_id = 0;
				changed = 1;
			}
		}

		if(!changed && mkui_ctx->mouse_clicked[0]) {
			if(!hovered && !popup_hovered) {
				open_combo_id = 0;
			}
		}

		if(open_combo_id == id) {
			mkui_ctx->combo_popup.active = 1;
			mkui_ctx->combo_popup.id = id;
			mkui_ctx->combo_popup.x = x;
			mkui_ctx->combo_popup.y = popup_y;
			mkui_ctx->combo_popup.w = w;
			mkui_ctx->combo_popup.item_h = item_h;
			mkui_ctx->combo_popup.items = items;
			mkui_ctx->combo_popup.items_count = items_count;
			mkui_ctx->combo_popup.current_item = current_item;
			mkui_ctx->combo_popup.changed = changed;
		} else {
			mkui_ctx->combo_popup.active = 0;
		}

		mkui_ctx->mouse_clicked[0] = 0;

	} else if(hovered && mkui_ctx->mouse_clicked[0]) {
		open_combo_id = id;
		mkui_ctx->combo_popup.scroll_offset = 0;
	}

	// Draw combo box
	struct mkui_color bg = hovered ? mkui_ctx->style.frame_bg_hovered : mkui_ctx->style.frame_bg;
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, w, h, bg);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, w, h, 1, mkui_ctx->style.border);

	const char *current_text = (*current_item >= 0 && *current_item < items_count) ? items[*current_item] : "";
	mkui_draw_text(&mkui_ctx->draw_list, x + 4, y + 6, current_text, mkui_ctx->style.text);
	mkui_draw_text(&mkui_ctx->draw_list, x + w - 16, y + 6, is_open ? "^" : "v", mkui_ctx->style.text);
	mkui_draw_text(&mkui_ctx->draw_list, x + w + 8, y + 6, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += h + mkui_ctx->item_spacing;

	return changed;
}

static int32_t mkui_listbox(const char *label, int32_t *current_item, char **items, int32_t items_count, int32_t visible_items) {
	uint32_t id = mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float w = 200;
	float item_h = 20;
	float h = item_h * visible_items;

	int32_t changed = 0;

	// Track scroll offset per listbox using static array (limited to 16 listboxes)
	static uint32_t listbox_ids[16] = {0};
	static int32_t listbox_scrolls[16] = {0};
	int32_t scroll_index = -1;

	// Find or allocate scroll state for this listbox
	for(int32_t i = 0; i < 16; i++) {
		if(listbox_ids[i] == id || listbox_ids[i] == 0) {
			listbox_ids[i] = id;
			scroll_index = i;
			break;
		}
	}

	int32_t *scroll_offset = (scroll_index >= 0) ? &listbox_scrolls[scroll_index] : 0;
	if(!scroll_offset) {
		scroll_offset = &listbox_scrolls[0]; // Fallback
	}

	// Handle mouse wheel scrolling when hovering over listbox
	int32_t listbox_hovered = mkui_is_mouse_over(x, y, w, h);
	if(listbox_hovered && mkui_ctx->scroll_y != 0.0) {
		*scroll_offset -= (int)mkui_ctx->scroll_y;
		if(*scroll_offset < 0) *scroll_offset = 0;
		if(*scroll_offset > items_count - visible_items) {
			*scroll_offset = items_count - visible_items;
		}
		if(*scroll_offset < 0) *scroll_offset = 0;
	}

	// Draw listbox background
	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, w, h, mkui_ctx->style.frame_bg);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, w, h, 1, mkui_ctx->style.border);

	// Draw visible items
	int32_t end_item = *scroll_offset + visible_items;
	if(end_item > items_count) {
		end_item = items_count;
	}

	for(int32_t i = *scroll_offset; i < end_item; i++) {
		float item_y = y + (i - *scroll_offset) * item_h;
		int32_t item_hovered = mkui_is_mouse_over(x, item_y, w, item_h);

		if(item_hovered && mkui_ctx->mouse_clicked[0]) {
			*current_item = i;
			changed = 1;
		}

		// Draw item
		struct mkui_color item_bg = (i == *current_item) ? mkui_ctx->style.frame_bg_active : (item_hovered ? mkui_ctx->style.frame_bg_hovered : mkui_ctx->style.frame_bg);
		mkui_draw_rect_filled(&mkui_ctx->draw_list, x, item_y, w, item_h, item_bg);
		mkui_draw_text(&mkui_ctx->draw_list, x + 4, item_y + 6, items[i], mkui_ctx->style.text);
	}

	// Draw label
	mkui_draw_text(&mkui_ctx->draw_list, x + w + 8, y + 4, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += h + mkui_ctx->item_spacing;

	return changed;
}

// [=]===^=[ mkui_begin_scroll_region ]=====================================[=]
static void mkui_begin_scroll_region(const char *label, float w, float h) {
	uint32_t id = mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;

	static uint32_t scroll_ids[16] = {0};
	static float scroll_offsets[16] = {0};
	int32_t scroll_index = -1;

	for(uint32_t i = 0; i < 16; ++i) {
		if(scroll_ids[i] == id || scroll_ids[i] == 0) {
			scroll_ids[i] = id;
			scroll_index = i;
			break;
		}
	}

	float *scroll_offset = (scroll_index >= 0) ? &scroll_offsets[scroll_index] : &scroll_offsets[0];

	int32_t region_hovered = mkui_is_mouse_over(x, y, w, h);
	if(region_hovered && mkui_ctx->scroll_y != 0.0) {
		*scroll_offset -= (float)(mkui_ctx->scroll_y * 20.0);
		if(*scroll_offset < 0) {
			*scroll_offset = 0;
		}
		mkui_ctx->scroll_y = 0.0;
	}

	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, w, h, mkui_ctx->style.child_bg);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, x, y, w, h, 1, mkui_ctx->style.border);

	if(mkui_ctx->scroll_region_depth < 8) {
		struct mkui_scroll_region_state *state = &mkui_ctx->scroll_region_stack[mkui_ctx->scroll_region_depth++];
		state->start_x = x;
		state->start_y = y;
		state->w = w;
		state->h = h;
	}

	mkui_push_scissor(x, y, w, h);
	mkui_ctx->cursor_y = y - *scroll_offset;
}

// [=]===^=[ mkui_end_scroll_region ]=======================================[=]
static void mkui_end_scroll_region() {
	mkui_pop_scissor();

	if(mkui_ctx->scroll_region_depth > 0) {
		struct mkui_scroll_region_state *state = &mkui_ctx->scroll_region_stack[--mkui_ctx->scroll_region_depth];
		mkui_ctx->cursor_x = state->start_x;
		mkui_ctx->cursor_y = state->start_y + state->h + mkui_ctx->item_spacing;
	}
}

// [=]===^=[ mkui_table ]===================================================[=]
static int32_t mkui_table(const char *label, int32_t col_count, float *col_widths, char **headers, char **cell_text, int32_t row_count, int32_t visible_rows, int32_t *selected_row) {
	uint32_t id = mkui_gen_id(label);

	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y;
	float row_h = 20;
	float header_h = 22;

	float total_w = 0;
	for(int32_t c = 0; c < col_count; ++c) {
		total_w += col_widths[c];
	}

	float body_h = row_h * visible_rows;
	float total_h = header_h + body_h;
	int32_t changed = 0;

	static uint32_t table_ids[16] = {0};
	static int32_t table_scrolls[16] = {0};
	int32_t scroll_index = -1;

	for(uint32_t i = 0; i < 16; ++i) {
		if(table_ids[i] == id || table_ids[i] == 0) {
			table_ids[i] = id;
			scroll_index = i;
			break;
		}
	}

	int32_t *scroll_offset = (scroll_index >= 0) ? &table_scrolls[scroll_index] : &table_scrolls[0];

	float body_y = y + header_h;
	int32_t body_hovered = mkui_is_mouse_over(x, body_y, total_w, body_h);
	if(body_hovered && mkui_ctx->scroll_y != 0.0) {
		*scroll_offset -= (int32_t)mkui_ctx->scroll_y;
		int32_t max_scroll = row_count - visible_rows;
		if(max_scroll < 0) {
			max_scroll = 0;
		}
		if(*scroll_offset < 0) {
			*scroll_offset = 0;
		}
		if(*scroll_offset > max_scroll) {
			*scroll_offset = max_scroll;
		}
	}

	struct mkui_draw_list *dl = &mkui_ctx->draw_list;

	mkui_draw_rect_filled(dl, x, y, total_w, total_h, mkui_ctx->style.frame_bg);
	mkui_draw_rect_outline(dl, x, y, total_w, total_h, 1, mkui_ctx->style.border);

	float cx = x;
	for(int32_t c = 0; c < col_count; ++c) {
		mkui_draw_rect_filled(dl, cx, y, col_widths[c], header_h, mkui_ctx->style.title_bg);
		if(headers) {
			mkui_draw_text(dl, cx + 4, y + 7, headers[c], mkui_ctx->style.text);
		}
		if(c > 0) {
			mkui_draw_rect_filled(dl, cx, y, 1, total_h, mkui_ctx->style.border);
		}
		cx += col_widths[c];
	}
	mkui_draw_rect_filled(dl, x, y + header_h - 1, total_w, 1, mkui_ctx->style.border);

	mkui_push_scissor(x, body_y, total_w, body_h);

	int32_t end_row = *scroll_offset + visible_rows;
	if(end_row > row_count) {
		end_row = row_count;
	}

	for(int32_t r = *scroll_offset; r < end_row; ++r) {
		float ry = body_y + (r - *scroll_offset) * row_h;
		int32_t row_hovered = mkui_is_mouse_over(x, ry, total_w, row_h);

		if(row_hovered && mkui_ctx->mouse_clicked[0]) {
			*selected_row = r;
			changed = 1;
		}

		struct mkui_color row_bg;
		if(r == *selected_row) {
			row_bg = mkui_ctx->style.frame_bg_active;
		} else if(row_hovered) {
			row_bg = mkui_ctx->style.frame_bg_hovered;
		} else if(r % 2 == 1) {
			row_bg = mkui_rgba(0.0f, 0.0f, 0.0f, 0.1f);
		} else {
			row_bg = mkui_rgba(0.0f, 0.0f, 0.0f, 0.0f);
		}

		if(row_bg.a > 0.0f) {
			mkui_draw_rect_filled(dl, x, ry, total_w, row_h, row_bg);
		}

		cx = x;
		for(int32_t c = 0; c < col_count; ++c) {
			const char *text = cell_text[r * col_count + c];
			if(text) {
				mkui_draw_text(dl, cx + 4, ry + 6, text, mkui_ctx->style.text);
			}
			cx += col_widths[c];
		}
	}

	mkui_pop_scissor();

	mkui_draw_text(dl, x + total_w + 8, y + 4, label, mkui_ctx->style.text);

	mkui_ctx->cursor_y += total_h + mkui_ctx->item_spacing;

	return changed;
}

// [=]===^=[ mkui_separator ]===============================================[=]
static void mkui_separator() {
	float x = mkui_ctx->cursor_x;
	float y = mkui_ctx->cursor_y + 4;
	float w = 200;

	mkui_draw_rect_filled(&mkui_ctx->draw_list, x, y, w, 1, mkui_ctx->style.border);

	mkui_ctx->cursor_y += 8 + mkui_ctx->item_spacing;
}

static void mkui_begin_window(const char *title, float *x, float *y, float w, float h) {
	uint32_t id = mkui_gen_id(title);

	// Check if title bar is being dragged
	int32_t title_bar_hovered = mkui_is_mouse_over(*x, *y, w, 24);

	if(title_bar_hovered && mkui_ctx->mouse_clicked[0]) {
		mkui_ctx->active_item = id;
	}

	if(mkui_ctx->active_item == id) {
		if(mkui_ctx->mouse_down[0]) {
			// Dragging - update window position based on mouse movement
			static int32_t last_mouse_x = 0;
			static int32_t last_mouse_y = 0;

			if(mkui_ctx->mouse_clicked[0]) {
				// Just started dragging - store initial position
				last_mouse_x = mkui_ctx->mouse_x;
				last_mouse_y = mkui_ctx->mouse_y;
			} else {
				// Continue dragging - apply delta
				int32_t dx = mkui_ctx->mouse_x - last_mouse_x;
				int32_t dy = mkui_ctx->mouse_y - last_mouse_y;
				*x += dx;
				*y += dy;
				last_mouse_x = mkui_ctx->mouse_x;
				last_mouse_y = mkui_ctx->mouse_y;
			}
		} else {
			// Mouse released, stop dragging
			mkui_ctx->active_item = 0;
		}
	}

	// Draw window background
	mkui_draw_rect_filled(&mkui_ctx->draw_list, *x, *y, w, h, mkui_ctx->style.window_bg);
	mkui_draw_rect_outline(&mkui_ctx->draw_list, *x, *y, w, h, 1, mkui_ctx->style.border);

	// Draw title bar
	mkui_draw_rect_filled(&mkui_ctx->draw_list, *x, *y, w, 24, mkui_ctx->style.title_bg_active);
	mkui_draw_text(&mkui_ctx->draw_list, *x + 8, *y + 8, title, mkui_ctx->style.text);

	// Set cursor to content area
	mkui_ctx->cursor_x = *x + mkui_ctx->window_padding;
	mkui_ctx->cursor_y = *y + 24 + mkui_ctx->window_padding;
}

static void mkui_end_window() {
	// Nothing needed for now
}

// ============================================================================
// STYLE CUSTOMIZATION
// ============================================================================

static void mkui_style_set_color(int32_t color_id, float r, float g, float b, float a) {
	struct mkui_color col = mkui_rgba(r, g, b, a);

	// Simple color ID system (extend as needed)
	switch(color_id) {
		case 0: mkui_ctx->style.text = col; break;
		case 1: mkui_ctx->style.window_bg = col; break;
		case 2: mkui_ctx->style.button = col; break;
		case 3: mkui_ctx->style.button_hovered = col; break;
		case 4: mkui_ctx->style.button_active = col; break;
		// Add more as needed
	}
}

static struct mkui_style *mkui_get_style() {
	return &mkui_ctx->style;
}
