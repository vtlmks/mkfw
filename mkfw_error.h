// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#pragma once

#include "mkfw_base.h"
#include <stdint.h>

#if defined(MKFW_BUILD_LIBRARY) || defined(MKFW_BUILD_SHARED) || defined(MKFW_USE_SHARED)
#define MKFW_ERROR_API MKFW_API
#else
#define MKFW_ERROR_API static inline
#endif

/* Errors are recorded on the calling thread. Fixed-message reports retain
 * their backend code; formatted reports use code zero. No output is written.
 * The callback receives the recorded message synchronously.
 */
typedef void (*mkfw_error_callback_t)(const char *message);
MKFW_VAR mkfw_error_callback_t mkfw_error_callback;

__attribute__((format(printf, 1, 2)))
MKFW_ERROR_API void mkfw_error(const char *fmt, ...);
MKFW_ERROR_API void mkfw_record_error(char *message, int64_t code);
MKFW_ERROR_API const char *mkfw_get_last_error(void);
MKFW_ERROR_API int64_t mkfw_get_last_error_code(void);
MKFW_ERROR_API void mkfw_clear_last_error(void);

// [=]===^=[ mkfw_set_error_callback ]===========================================================[=]
static inline void mkfw_set_error_callback(mkfw_error_callback_t callback) {
	mkfw_error_callback = callback;

}

#if !defined(MKFW_BUILD_LIBRARY) && !defined(MKFW_USE_SHARED)
#include "mkfw_error.c"
#endif
