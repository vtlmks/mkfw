// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#pragma once

#include <dlfcn.h>

typedef Status (*PFN_XIQueryVersion)(Display *, int *, int *);
typedef int (*PFN_XISelectEvents)(Display *, Window, XIEventMask *, int);

static PFN_XIQueryVersion mkfw_XIQueryVersion;
static PFN_XISelectEvents mkfw_XISelectEvents;

#define XIQueryVersion mkfw_XIQueryVersion
#define XISelectEvents mkfw_XISelectEvents

static void load_xinput2_functions(void) {
	static uint8_t loaded = 0;
	if(loaded) {
		return;
	}
	loaded = 1;

	void *lib = dlopen("libXi.so.6", RTLD_LAZY | RTLD_GLOBAL);
	if(!lib) {
		mkfw_error("failed to load libXi.so.6");
		exit(EXIT_FAILURE);
	}

	#define LOAD(name) *(void **)&mkfw_##name = dlsym(lib, #name)
	LOAD(XIQueryVersion);
	LOAD(XISelectEvents);
	#undef LOAD

	if(!mkfw_XIQueryVersion || !mkfw_XISelectEvents) {
		mkfw_error("failed to load required XInput2 functions");
		exit(EXIT_FAILURE);
	}
}
