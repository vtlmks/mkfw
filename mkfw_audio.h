// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT
//
// mkfw audio subsystem.  Include this header in addition to mkfw.h
// to use the callback-based audio output.  The implementation is
// unity-built alongside the application; no separate compilation
// required.
//
//   #include "mkfw.h"
//   #include "mkfw_audio.h"
//
// On Linux, link with -lm -ldl.  Platform libraries (libasound) are
// loaded at runtime via dlopen.
//
// On Windows, link with ole32.lib, avrt.lib, uuid.lib in addition to
// the base mkfw libraries.

#pragma once

#include <stdint.h>
#include <stddef.h>

#include "mkfw.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#if !defined(MKFW_BUILD_LIBRARY) && !defined(MKFW_USE_SHARED)
#ifdef _WIN32
#include "mkfw_win32_audio.c"
#elif defined(__linux__)
#include "mkfw_linux_audio.c"
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
