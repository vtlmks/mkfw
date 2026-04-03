// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#pragma once

/* Forward declarations for glXGetProcAddress */
extern void *glXGetProcAddress(const unsigned char *procName);

typedef XID GLXDrawable;
typedef XID GLXContext;
typedef XID GLXPixmap;
typedef XID GLXPbuffer;

#define GLX_X_RENDERABLE	0x8012
#define GLX_DRAWABLE_TYPE	0x8010
#define GLX_RENDER_TYPE		0x8011
#define GLX_X_VISUAL_TYPE	0x22
#define GLX_RED_SIZE			0x801C
#define GLX_GREEN_SIZE		0x801D
#define GLX_BLUE_SIZE		0x801E
#define GLX_ALPHA_SIZE		0x801F
#define GLX_DEPTH_SIZE		0x8021
#define GLX_STENCIL_SIZE	0x8022
#define GLX_DOUBLEBUFFER	0x5
#define GLX_WINDOW_BIT		0x00000001
#define GLX_RGBA_BIT			0x00000001
#define GLX_TRUE_COLOR		0x8002
#define GLX_VENDOR			0x1

#define GLX_CONTEXT_MAJOR_VERSION_ARB					0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB					0x2092
#define GLX_CONTEXT_PROFILE_MASK_ARB					0x9126
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB				0x00000001
#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB	0x00000002

typedef struct __GLXFBConfigRec *GLXFBConfig;

typedef XVisualInfo *(*PFNGLXGETVISUALFROMFBCONFIGPROC)(Display *, GLXFBConfig);
typedef GLXFBConfig *(*PFNGLXCHOOSEFBCONFIGPROC)(Display *, int, const int *, int *);
typedef GLXContext (*PFNGLXCREATECONTEXTATTRIBSARBPROC)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
typedef void (*PFNGLXDESTROYCONTEXTPROC)(Display *, GLXContext);
typedef Bool (*PFNGLXMAKECURRENTPROC)(Display *, GLXDrawable, GLXContext);
typedef void (*PFNGLXSWAPBUFFERSPROC)(Display *, GLXDrawable);
typedef const char *(*PFNGLXGETCLIENTSTRINGPROC)(Display *, int);
typedef int (*PFNGLXGETFBCONFIGATTRIBPROC)(Display *, GLXFBConfig, int, int *);
typedef GLXDrawable (*PFNGLXGETCURRENTDRAWABLEPROC)(void);

static PFNGLXGETCURRENTDRAWABLEPROC glXGetCurrentDrawable;
static PFNGLXGETVISUALFROMFBCONFIGPROC glXGetVisualFromFBConfig;
static PFNGLXCHOOSEFBCONFIGPROC glXChooseFBConfig;
static PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;
static PFNGLXMAKECURRENTPROC glXMakeCurrent;
static PFNGLXSWAPBUFFERSPROC glXSwapBuffers;
static PFNGLXDESTROYCONTEXTPROC glXDestroyContext;
static PFNGLXGETCLIENTSTRINGPROC glXGetClientString;
static PFNGLXGETFBCONFIGATTRIBPROC glXGetFBConfigAttrib;

static void load_glx_functions(Display *display __attribute__((unused))) {
	glXChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC)glXGetProcAddress((const unsigned char *)"glXChooseFBConfig");
	glXGetVisualFromFBConfig = (PFNGLXGETVISUALFROMFBCONFIGPROC)glXGetProcAddress((const unsigned char *)"glXGetVisualFromFBConfig");
	glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const unsigned char *)"glXCreateContextAttribsARB");
	glXMakeCurrent = (PFNGLXMAKECURRENTPROC)glXGetProcAddress((const unsigned char *)"glXMakeCurrent");
	glXSwapBuffers = (PFNGLXSWAPBUFFERSPROC)glXGetProcAddress((const unsigned char *)"glXSwapBuffers");
	glXDestroyContext = (PFNGLXDESTROYCONTEXTPROC)glXGetProcAddress((const unsigned char *)"glXDestroyContext");
	glXGetCurrentDrawable = (PFNGLXGETCURRENTDRAWABLEPROC)glXGetProcAddress((const unsigned char *)"glXGetCurrentDrawable");
	glXGetClientString = (PFNGLXGETCLIENTSTRINGPROC)glXGetProcAddress((const unsigned char *)"glXGetClientString"); /* Added */
	glXGetFBConfigAttrib = (PFNGLXGETFBCONFIGATTRIBPROC)glXGetProcAddress((const unsigned char *)"glXGetFBConfigAttrib"); /* Added */

	if(!glXChooseFBConfig || !glXGetVisualFromFBConfig || !glXCreateContextAttribsARB || !glXMakeCurrent || !glXSwapBuffers || !glXDestroyContext || !glXGetCurrentDrawable || !glXGetClientString || !glXGetFBConfigAttrib) {
		mkfw_error("failed to load GLX functions");
		exit(EXIT_FAILURE);
	}
}
