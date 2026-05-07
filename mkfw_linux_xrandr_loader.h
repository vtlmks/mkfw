// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#pragma once

#include <dlfcn.h>

typedef XRRScreenResources *(*PFN_XRRGetScreenResourcesCurrent)(Display *, Window);
typedef RROutput (*PFN_XRRGetOutputPrimary)(Display *, Window);
typedef XRROutputInfo *(*PFN_XRRGetOutputInfo)(Display *, XRRScreenResources *, RROutput);
typedef void (*PFN_XRRFreeOutputInfo)(XRROutputInfo *);
typedef XRRCrtcInfo *(*PFN_XRRGetCrtcInfo)(Display *, XRRScreenResources *, RRCrtc);
typedef void (*PFN_XRRFreeCrtcInfo)(XRRCrtcInfo *);
typedef void (*PFN_XRRFreeScreenResources)(XRRScreenResources *);

static PFN_XRRGetScreenResourcesCurrent mkfw_XRRGetScreenResourcesCurrent;
static PFN_XRRGetOutputPrimary mkfw_XRRGetOutputPrimary;
static PFN_XRRGetOutputInfo mkfw_XRRGetOutputInfo;
static PFN_XRRFreeOutputInfo mkfw_XRRFreeOutputInfo;
static PFN_XRRGetCrtcInfo mkfw_XRRGetCrtcInfo;
static PFN_XRRFreeCrtcInfo mkfw_XRRFreeCrtcInfo;
static PFN_XRRFreeScreenResources mkfw_XRRFreeScreenResources;

#define XRRGetScreenResourcesCurrent mkfw_XRRGetScreenResourcesCurrent
#define XRRGetOutputPrimary mkfw_XRRGetOutputPrimary
#define XRRGetOutputInfo mkfw_XRRGetOutputInfo
#define XRRFreeOutputInfo mkfw_XRRFreeOutputInfo
#define XRRGetCrtcInfo mkfw_XRRGetCrtcInfo
#define XRRFreeCrtcInfo mkfw_XRRFreeCrtcInfo
#define XRRFreeScreenResources mkfw_XRRFreeScreenResources

static void load_xrandr_functions(void) {
	static uint8_t loaded = 0;
	if(loaded) {
		return;
	}
	loaded = 1;

	void *lib = dlopen("libXrandr.so.2", RTLD_LAZY | RTLD_GLOBAL);
	if(!lib) {
		mkfw_error("failed to load libXrandr.so.2");
		exit(EXIT_FAILURE);
	}

	#define LOAD(name) *(void **)&mkfw_##name = dlsym(lib, #name)
	LOAD(XRRGetScreenResourcesCurrent);
	LOAD(XRRGetOutputPrimary);
	LOAD(XRRGetOutputInfo);
	LOAD(XRRFreeOutputInfo);
	LOAD(XRRGetCrtcInfo);
	LOAD(XRRFreeCrtcInfo);
	LOAD(XRRFreeScreenResources);
	#undef LOAD

	if(!mkfw_XRRGetScreenResourcesCurrent || !mkfw_XRRGetCrtcInfo) {
		mkfw_error("failed to load required XRandR functions");
		exit(EXIT_FAILURE);
	}
}
