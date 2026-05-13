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

struct mkfw_timer_handle;

MKFW_API void                       mkfw_timer_init(void);
MKFW_API void                       mkfw_timer_shutdown(void);
MKFW_API struct mkfw_timer_handle  *mkfw_timer_create(uint64_t interval_ns);
MKFW_API void                       mkfw_timer_destroy(struct mkfw_timer_handle *t);
MKFW_API uint32_t                   mkfw_timer_wait(struct mkfw_timer_handle *t);
MKFW_API void                       mkfw_timer_set_interval(struct mkfw_timer_handle *t, uint64_t interval_ns);
MKFW_API void                       mkfw_timer_set_spin(struct mkfw_timer_handle *t, uint32_t enabled);

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#if !defined(MKFW_BUILD_LIBRARY) && !defined(MKFW_USE_SHARED)
#ifdef _WIN32
#include "mkfw_win32_timer.c"
#elif defined(__linux__)
#include "mkfw_linux_timer.c"
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
