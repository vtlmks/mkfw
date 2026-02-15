// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#ifdef _WIN32
typedef __int64 GLintptr;
#else
typedef intptr_t GLintptr;
#endif
typedef void GLvoid;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;
typedef char GLchar;

typedef int GLint;
typedef int GLsizei;

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef unsigned int GLbitfield;

typedef float GLfloat;
typedef double GLdouble;

typedef unsigned long long GLsizeiptr;

// --- Boolean ---
#define GL_FALSE 0
#define GL_TRUE  1

// --- Error ---
#define GL_NO_ERROR 0

// --- Data types ---
#define GL_UNSIGNED_BYTE      0x1401
#define GL_UNSIGNED_SHORT     0x1403
#define GL_UNSIGNED_INT       0x1405
#define GL_FLOAT              0x1406
#define GL_UNSIGNED_INT_8_8_8_8 0x8035

// --- Primitives ---
#define GL_LINES          0x0001
#define GL_TRIANGLES      0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_QUADS          0x0007

// --- Blend ---
#define GL_ZERO                0x0000
#define GL_ONE                 0x0001
#define GL_SRC_ALPHA           0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_BLEND               0x0be2
#define GL_FUNC_ADD            0x8006

// --- Pixel formats ---
#define GL_RED          0x1903
#define GL_RGB          0x1907
#define GL_ALPHA        0x1906
#define GL_RGBA         0x1908
#define GL_RGB8         0x8051
#define GL_RGBA8        0x8058
#define GL_SRGB8_ALPHA8 0x8c43
#define GL_SRGB_ALPHA   0x8c43
#define GL_RGBA16F      0x881a
#define GL_RGBA32F      0x8814

// --- Texture ---
#define GL_TEXTURE_2D           0x0de1
#define GL_TEXTURE_MIN_FILTER   0x2801
#define GL_TEXTURE_MAG_FILTER   0x2800
#define GL_TEXTURE_WRAP_S       0x2802
#define GL_TEXTURE_WRAP_T       0x2803
#define GL_TEXTURE_COORD_ARRAY  0x8078
#define GL_TEXTURE_BINDING_2D   0x8069
#define GL_TEXTURE_BORDER_COLOR 0x1004
#define GL_NEAREST              0x2600
#define GL_LINEAR               0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_REPEAT               0x2901
#define GL_CLAMP_TO_EDGE        0x812f
#define GL_CLAMP_TO_BORDER      0x812D
#define GL_TEXTURE_ENV          0x2300
#define GL_TEXTURE_ENV_MODE     0x2200
#define GL_MODULATE             0x2100

// --- Texture units ---
#define GL_TEXTURE0        0x84c0
#define GL_TEXTURE1        0x84c1
#define GL_TEXTURE2        0x84c2
#define GL_TEXTURE3        0x84c3
#define GL_ACTIVE_TEXTURE  0x84e0

// --- Pixel store ---
#define GL_UNPACK_ROW_LENGTH  0x0cf2
#define GL_UNPACK_SKIP_ROWS   0x0cf3
#define GL_UNPACK_SKIP_PIXELS 0x0cf4

// --- Buffers ---
#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_PIXEL_UNPACK_BUFFER  0x88ec
#define GL_STATIC_DRAW          0x88e4
#define GL_DYNAMIC_DRAW         0x88e8
#define GL_STREAM_DRAW          0x88e0
#define GL_WRITE_ONLY           0x88b9

// --- Framebuffer ---
#define GL_FRAMEBUFFER          0x8d40
#define GL_FRAMEBUFFER_COMPLETE 0x8cd5
#define GL_FRAMEBUFFER_SRGB     0x8db9
#define GL_FRAMEBUFFER_WIDTH    0x9310
#define GL_FRAMEBUFFER_HEIGHT   0x9311
#define GL_COLOR_ATTACHMENT0    0x8ce0
#define GL_COLOR_BUFFER_BIT     0x4000

// --- Shaders ---
#define GL_VERTEX_SHADER   0x8b31
#define GL_FRAGMENT_SHADER 0x8b30
#define GL_COMPILE_STATUS  0x8b81
#define GL_LINK_STATUS     0x8b82
#define GL_INFO_LOG_LENGTH 0x8b84

// --- Capabilities ---
#define GL_DEPTH_TEST   0x0b71
#define GL_CULL_FACE    0x0b44
#define GL_SCISSOR_TEST 0x0c11
#define GL_MULTISAMPLE  0x809d
#define GL_LIGHTING     0x0b50

// --- State queries ---
#define GL_VIEWPORT                      0x0ba2
#define GL_SCISSOR_BOX                   0x0c10
#define GL_CURRENT_PROGRAM               0x8b8d
#define GL_VERTEX_ARRAY_BINDING          0x85b5
#define GL_ARRAY_BUFFER_BINDING          0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING  0x8895
#define GL_PIXEL_UNPACK_BUFFER_BINDING   0x88ef
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED   0x8646
#define GL_RENDERER                      0x1F01

// --- Fixed-function (legacy) ---
#define GL_MODELVIEW         0x1700
#define GL_PROJECTION        0x1701
#define GL_MODELVIEW_MATRIX  0x0ba6
#define GL_PROJECTION_MATRIX 0x0ba7

// --- Attrib stack (legacy) ---
#define GL_VIEWPORT_BIT  0x00000800
#define GL_TRANSFORM_BIT 0x00001000
#define GL_ENABLE_BIT    0x00002000

void glTexEnvi(GLenum target, GLenum pname, GLint param);
void glMatrixMode(GLenum mode);
void glPushMatrix(void);
void glPopMatrix(void);
void glLoadIdentity(void);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
void glBegin(GLenum mode);
void glEnd(void);
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glPushAttrib(GLbitfield mask);
void glPopAttrib(void);

#define DECLARE_GL_FUNCTION(Name, ReturnType, ...) typedef ReturnType (*type_##Name)(__VA_ARGS__);
#define DECLARE_GLOBAL_FUNCTION(Name, ...) type_##Name Name;

#define GL_FUNCTIONS(X) \
	X(glActiveTexture, void, GLenum texture) \
	X(glAttachShader, void, GLuint program, GLuint shader) \
	X(glBindBuffer, void, GLenum target, GLuint buffer) \
	X(glBindTexture, void, GLenum target, GLuint texture) \
	X(glBufferData, void, GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) \
	X(glClear, void, GLbitfield mask) \
	X(glClearColor, void, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) \
	X(glCompileShader, void, GLuint shader) \
	X(glCreateProgram, GLuint) \
	X(glCreateShader, GLuint, GLenum type) \
	X(glDeleteShader, void, GLuint shader) \
	X(glDrawElements, void, GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) \
	X(glEnableVertexAttribArray, void, GLuint index) \
	X(glGenBuffers, void, GLsizei n, GLuint *buffers) \
	X(glGenTextures, void, GLsizei n, GLuint *textures) \
	X(glGetShaderInfoLog, void, GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog) \
	X(glGetShaderiv, void, GLuint shader, GLenum pname, GLint *params) \
	X(glGetUniformLocation, GLint, GLuint program, const GLchar *name) \
	X(glLinkProgram, void, GLuint program) \
	X(glShaderSource, void, GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length) \
	X(glTexImage2D, void, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) \
	X(glTexParameteri, void, GLenum target, GLenum pname, GLint param) \
	X(glTexSubImage2D, void, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) \
	X(glUniform1f, void, GLint location, GLfloat v0) \
	X(glUniform1i, void, GLint location, GLint v0) \
	X(glUniform2f, void, GLint location, GLfloat v0, GLfloat v1) \
	X(glUniform4f, void, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) \
	X(glUseProgram, void, GLuint program) \
	X(glVertexAttribPointer, void, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer) \
	X(glViewport, void, GLint x, GLint y, GLsizei width, GLsizei height) \
	X(glDeleteProgram, void, GLuint program) \
	X(glDeleteBuffers, void, GLsizei n, const GLuint *buffers) \
	X(glDeleteTextures, void, GLsizei n, const GLuint *textures) \
	X(glEnable, void, GLenum cap) \
	X(glGenerateMipmap, void, GLenum target) \
	X(glGetProgramiv, void, GLuint program, GLenum pname, GLint *params) \
	X(glGetAttribLocation, GLint, GLuint program, const GLchar *name) \
	X(glDetachShader, void, GLuint program, GLuint shader) \
	X(glUniformMatrix4fv, void, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
	X(glMapBuffer, void*, GLenum target, GLenum access) \
	X(glUnmapBuffer, GLboolean, GLenum target) \
	X(glBlendEquation, void, GLenum mode) \
	X(glBlendFunc, void, GLenum sfactor, GLenum dfactor) \
	X(glDisable, void, GLenum cap) \
	X(glScissor, void, GLint x, GLint y, GLsizei width, GLsizei height) \
	X(glTexCoord2f, void, GLfloat s, GLfloat t) \
	X(glVertex2f, void, GLfloat x, GLfloat y) \
	X(glGetError, GLenum) \
	X(glGetProgramInfoLog, void, GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog) \
	X(glGenVertexArrays, void, GLsizei n, GLuint *arrays) \
	X(glBindVertexArray, void, GLuint array) \
	X(glDeleteVertexArrays, void, GLsizei n, const GLuint *arrays) \
	X(glDrawArrays, void, GLenum mode, GLint first, GLsizei count) \
	X(glBufferSubData, void, GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data) \
	X(glDisableVertexAttribArray, void, GLuint index) \
	X(glGetIntegerv, void, GLenum pname, GLint *data) \
	X(glBindAttribLocation, void, GLuint program, GLuint index, const GLchar *name) \
	X(glGetUniformfv, void, GLuint program, GLint location, GLfloat *params) \
	X(glPixelStorei, void, GLenum pname, GLint param) \
	X(glGetVertexAttribiv, void, GLuint index, GLenum pname, GLint *params) \
	X(glFinish, void)	\
	X(glIsEnabled, GLboolean, GLenum cap) \
	X(glGenFramebuffers, void, GLsizei n, GLuint *framebuffers) \
   X(glBindFramebuffer, void, GLenum target, GLuint framebuffer) \
	X(glCheckFramebufferStatus, GLenum, GLenum target) \
	X(glDeleteFramebuffers, void, GLsizei n, const GLuint *framebuffers) \
	X(glFlush, void) \
	X(glTexParameterfv, void, GLenum target, GLenum pname, const GLfloat *params) \
	X(glColor4ub, void, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) \
   X(glFramebufferTexture2D, void, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) \
	X(glCopyTexSubImage2D, void, GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) \
	X(glGetStringi, const GLubyte*, GLenum name, GLuint index);

GL_FUNCTIONS(DECLARE_GL_FUNCTION)
GL_FUNCTIONS(DECLARE_GLOBAL_FUNCTION)


#if defined(_WIN32)
static void *get_any_gl_address(const char *name) {
	void *p = (void *)wglGetProcAddress(name);
	if(!p) {
		HMODULE module = LoadLibraryA("opengl32.dll");
		if(module) {
			p = (void *)GetProcAddress(module, name);
		}
	}
	return p;
}

#define GetOpenGLFunction(Name, ...) \
	*(void **)&Name = (void *)get_any_gl_address(#Name); \
	if(!Name) { \
		exit(EXIT_FAILURE); \
	}

#elif defined(__linux__)
#include <dlfcn.h>
static void *(*opengl_glx_proc_address)(const GLubyte *);

static void *get_any_gl_address(const char *name) {
	if(!opengl_glx_proc_address) {
		void *libGL = dlopen("libGL.so.1", RTLD_LAZY | RTLD_GLOBAL);
		if(!libGL) {
			mkfw_error("unable to load libGL.so.1");
			exit(EXIT_FAILURE);
		}

		opengl_glx_proc_address = (void *(*)(const GLubyte *))dlsym(libGL, "glXGetProcAddress");

		if(!opengl_glx_proc_address) {
			mkfw_error("unable to find glXGetProcAddress");
			exit(EXIT_FAILURE);
		}
	}

	return opengl_glx_proc_address((const GLubyte *)name);
}

// Satisfies the forward declaration in mkfw_glx_mini.h
static void *glXGetProcAddress(const GLubyte *procName) {
	return get_any_gl_address((const char *)procName);
}

#define GetOpenGLFunction(Name, ...) \
	*(void **)&Name = (void *)get_any_gl_address(#Name);	\
	if(!Name) {	\
		mkfw_error("failed to load OpenGL function: %s", #Name);	\
		exit(EXIT_FAILURE);	\
	}
#endif

static void opengl_function_loader() {
	GL_FUNCTIONS(GetOpenGLFunction);
}
