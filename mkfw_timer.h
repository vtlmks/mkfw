// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT
//
// mkfw timer subsystem.  Include this header in addition to mkfw.h
// to use the high-precision sleep-and-spin timer.  The implementation
// is unity-built alongside the application; no separate compilation
// required.
//
//   #include "mkfw.h"
//   #include "mkfw_timer.h"

#pragma once

#include <stdint.h>

#include "mkfw.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifdef _WIN32
#include "mkfw_win32_timer.c"
#elif defined(__linux__)
#include "mkfw_linux_timer.c"
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
