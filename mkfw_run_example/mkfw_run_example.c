// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

// mkfw_run() example -- cross-platform frame callback
//
// Two spinning cubes orbiting a center point. Demonstrates mkfw_run() which
// works on Linux, Windows, and Emscripten without any #ifdefs.
//
// On Linux:      single-thread while loop, vsync paces the frame
// On Windows:    render thread calls frame(), main thread pumps messages
// On Emscripten: requestAnimationFrame calls frame() at display refresh rate
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
	float time;
	int32_t fullscreen;
	int32_t fb_width;
	int32_t fb_height;
	uint32_t vao;
	uint32_t vbo;
	uint32_t ibo;
	uint32_t program;
	int32_t u_mvp;
};

static struct app_state app;

// ============================================================
// Math helpers
// ============================================================

// [=]===^=[ mat4_identity ]=====================================================================^===[=]
static void mat4_identity(float *m) {
	memset(m, 0, 16 * sizeof(float));
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

// [=]===^=[ mat4_multiply ]=====================================================================^===[=]
static void mat4_multiply(float *out, float *a, float *b) {
	float tmp[16];
	for(uint32_t c = 0; c < 4; ++c) {
		for(uint32_t r = 0; r < 4; ++r) {
			tmp[c * 4 + r] = 0.0f;
			for(uint32_t k = 0; k < 4; ++k) {
				tmp[c * 4 + r] += a[k * 4 + r] * b[c * 4 + k];
			}
		}
	}
	memcpy(out, tmp, 16 * sizeof(float));
}

// [=]===^=[ mat4_perspective ]==================================================================^===[=]
static void mat4_perspective(float *m, float fov_rad, float aspect, float znear, float zfar) {
	float f = 1.0f / tanf(fov_rad * 0.5f);
	memset(m, 0, 16 * sizeof(float));
	m[0] = f / aspect;
	m[5] = f;
	m[10] = (zfar + znear) / (znear - zfar);
	m[11] = -1.0f;
	m[14] = (2.0f * zfar * znear) / (znear - zfar);
}

// [=]===^=[ mat4_translate ]====================================================================^===[=]
static void mat4_translate(float *m, float x, float y, float z) {
	mat4_identity(m);
	m[12] = x;
	m[13] = y;
	m[14] = z;
}

// [=]===^=[ mat4_rotate_x ]====================================================================^===[=]
static void mat4_rotate_x(float *m, float rad) {
	float c = cosf(rad);
	float s = sinf(rad);
	mat4_identity(m);
	m[5] = c;
	m[6] = -s;
	m[9] = s;
	m[10] = c;
}

// [=]===^=[ mat4_rotate_y ]====================================================================^===[=]
static void mat4_rotate_y(float *m, float rad) {
	float c = cosf(rad);
	float s = sinf(rad);
	mat4_identity(m);
	m[0] = c;
	m[2] = s;
	m[8] = -s;
	m[10] = c;
}

// ============================================================
// Cube geometry
// ============================================================

static float cube_vertices[] = {
	// position            // color
	-0.5f, -0.5f, -0.5f,  0.8f, 0.2f, 0.2f,
	 0.5f, -0.5f, -0.5f,  0.2f, 0.8f, 0.2f,
	 0.5f,  0.5f, -0.5f,  0.2f, 0.2f, 0.8f,
	-0.5f,  0.5f, -0.5f,  0.8f, 0.8f, 0.2f,
	-0.5f, -0.5f,  0.5f,  0.8f, 0.2f, 0.8f,
	 0.5f, -0.5f,  0.5f,  0.2f, 0.8f, 0.8f,
	 0.5f,  0.5f,  0.5f,  0.9f, 0.9f, 0.9f,
	-0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
};

static uint16_t cube_indices[] = {
	0, 1, 2,  2, 3, 0,
	4, 5, 6,  6, 7, 4,
	0, 4, 7,  7, 3, 0,
	1, 5, 6,  6, 2, 1,
	3, 2, 6,  6, 7, 3,
	0, 1, 5,  5, 4, 0,
};

// ============================================================
// Shaders
// ============================================================

#ifdef __EMSCRIPTEN__
#define SHADER_VERSION "#version 300 es\nprecision mediump float;\n"
#else
#define SHADER_VERSION "#version 140\n"
#endif

static const char *vert_src =
	SHADER_VERSION
	"in vec3 a_position;\n"
	"in vec3 a_color;\n"
	"out vec3 v_color;\n"
	"uniform mat4 u_mvp;\n"
	"void main() {\n"
	"    v_color = a_color;\n"
	"    gl_Position = u_mvp * vec4(a_position, 1.0);\n"
	"}\n";

static const char *frag_src =
	SHADER_VERSION
	"in vec3 v_color;\n"
	"out vec4 frag_color;\n"
	"void main() {\n"
	"    frag_color = vec4(v_color, 1.0);\n"
	"}\n";

// [=]===^=[ compile_shader ]====================================================================^===[=]
static uint32_t compile_shader(uint32_t type, const char *src) {
	uint32_t s = glCreateShader(type);
	glShaderSource(s, 1, &src, 0);
	glCompileShader(s);
	int32_t ok;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if(!ok) {
		char log[512];
		glGetShaderInfoLog(s, 512, 0, log);
		fprintf(stderr, "Shader error: %s\n", log);
	}
	return s;
}

// [=]===^=[ setup_gl ]==========================================================================^===[=]
static void setup_gl(struct app_state *state) {
	uint32_t vs = compile_shader(GL_VERTEX_SHADER, vert_src);
	uint32_t fs = compile_shader(GL_FRAGMENT_SHADER, frag_src);
	state->program = glCreateProgram();
	glAttachShader(state->program, vs);
	glAttachShader(state->program, fs);
	glBindAttribLocation(state->program, 0, "a_position");
	glBindAttribLocation(state->program, 1, "a_color");
	glLinkProgram(state->program);
	glDeleteShader(vs);
	glDeleteShader(fs);
	state->u_mvp = glGetUniformLocation(state->program, "u_mvp");

	glGenVertexArrays(1, &state->vao);
	glBindVertexArray(state->vao);

	glGenBuffers(1, &state->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &state->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));

	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
}

// [=]===^=[ draw_cube ]=========================================================================^===[=]
static void draw_cube(struct app_state *state, float orbit_angle, float spin_angle, float orbit_radius) {
	float proj[16], view[16], model[16], rx[16], ry[16], tr[16], orbit[16], tmp[16], mvp[16];

	float aspect = (float)state->fb_width / (float)(state->fb_height > 0 ? state->fb_height : 1);
	mat4_perspective(proj, 1.0f, aspect, 0.1f, 100.0f);
	mat4_translate(view, 0.0f, 0.0f, -5.0f);

	mat4_rotate_y(orbit, orbit_angle);
	mat4_translate(tr, orbit_radius, 0.0f, 0.0f);
	mat4_multiply(model, orbit, tr);

	mat4_rotate_y(ry, spin_angle);
	mat4_rotate_x(rx, spin_angle * 0.7f);
	mat4_multiply(tmp, model, ry);
	mat4_multiply(model, tmp, rx);

	mat4_multiply(tmp, view, model);
	mat4_multiply(mvp, proj, tmp);

	glUniformMatrix4fv(state->u_mvp, 1, GL_FALSE, mvp);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
}

// ============================================================
// Callbacks
// ============================================================

// [=]===^=[ on_key ]============================================================================^===[=]
static void on_key(struct mkfw_state *window, uint32_t key, uint32_t action, uint32_t mods) {
	(void)mods;
	struct app_state *state = (struct app_state *)mkfw_get_user_data(window);

	if(key == MKS_KEY_ESCAPE && action == MKS_PRESSED) {
		mkfw_set_should_close(window, 1);
	}

	if(key == MKS_KEY_F11 && action == MKS_PRESSED) {
		state->fullscreen = !state->fullscreen;
		mkfw_fullscreen(window, state->fullscreen);
	}
}

// [=]===^=[ on_resize ]=========================================================================^===[=]
static void on_resize(struct mkfw_state *window, int32_t w, int32_t h, float aspect) {
	(void)aspect;
	struct app_state *state = (struct app_state *)mkfw_get_user_data(window);
	state->fb_width = w;
	state->fb_height = h;
}

// [=]===^=[ frame ]=============================================================================^===[=]
static void frame(struct mkfw_state *window) {
	struct app_state *state = (struct app_state *)mkfw_get_user_data(window);

	state->time += 0.016f;

	glViewport(0, 0, state->fb_width, state->fb_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(state->program);
	glBindVertexArray(state->vao);

	draw_cube(state, state->time * 0.8f, state->time * 2.0f, 1.5f);
	draw_cube(state, state->time * 0.8f + 3.14159f, state->time * 1.5f, 1.5f);

	glBindVertexArray(0);

	mkfw_update_input_state(window);
}

// [=]===^=[ main ]==============================================================================^===[=]
int main(void) {
	struct mkfw_state *window = mkfw_init(1280, 720);
	if(!window) {
		fprintf(stderr, "Failed to create window\n");
		return 1;
	}

	mkfw_gl_loader();
	mkfw_set_swapinterval(window, 1);
	mkfw_set_window_title(window, "mkfw_run example - spinning cubes");
	mkfw_set_user_data(window, &app);
	mkfw_set_key_callback(window, on_key);
	mkfw_set_framebuffer_size_callback(window, on_resize);
	mkfw_show_window(window);

	mkfw_get_framebuffer_size(window, &app.fb_width, &app.fb_height);

	setup_gl(&app);

	mkfw_run(window, frame);

	mkfw_cleanup(window);
	return 0;
}
