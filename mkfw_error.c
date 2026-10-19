// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#include "mkfw_error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(MKFW_BUILD_LIBRARY) || defined(MKFW_BUILD_SHARED)
mkfw_error_callback_t mkfw_error_callback;
#endif

struct mkfw_error_state {
	char message[512];
	int64_t code;
	uint8_t valid;
};

#ifdef _WIN32
#include <windows.h>

static volatile LONG mkfw_win32_tls_index = -1;

// [=]===^=[ mkfw_win32_error_state ]============================================================[=]
// NOTE(peter): Native Win32 TLS avoids MinGW's libwinpthread emutls dependency.
static struct mkfw_error_state *mkfw_win32_error_state(uint32_t create) {
	DWORD slot = InterlockedCompareExchange(&mkfw_win32_tls_index, -1, -1);

	if(slot == 0xffffffffu) {
		if(!create) {
			return 0;

		}

		DWORD fresh = TlsAlloc();

		if(fresh == 0xffffffffu) {
			return 0;

		}

		LONG previous = InterlockedCompareExchange(&mkfw_win32_tls_index, fresh, -1);

		if(previous != -1) {
			TlsFree(fresh);
			slot = previous;

		} else {
			slot = fresh;

		}
	}

	struct mkfw_error_state *state = TlsGetValue(slot);

	if(!state && create) {
		state = calloc(1, sizeof(*state));

		if(state) {
			TlsSetValue(slot, state);

		}
	}

	return state;

}
#else
static __thread struct mkfw_error_state mkfw_linux_error_state;
#endif

// [=]===^=[ mkfw_record_error ]=================================================================[=]
MKFW_ERROR_API void mkfw_record_error(char *message, int64_t code) {
#ifdef _WIN32
	struct mkfw_error_state *state = mkfw_win32_error_state(1);

	if(!state) {
		return;

	}
#else
	struct mkfw_error_state *state = &mkfw_linux_error_state;
#endif

	size_t length = strlen(message);

	if(length >= sizeof(state->message)) {
		length = sizeof(state->message) - 1;

	}

	memcpy(state->message, message, length);
	state->message[length] = 0;
	state->code = code;
	state->valid = 1;

	if(mkfw_error_callback) {
		mkfw_error_callback(state->message);

	}
}

// [=]===^=[ mkfw_error ]========================================================================[=]
MKFW_ERROR_API void mkfw_error(const char *fmt, ...) {
#ifdef _WIN32
	struct mkfw_error_state *state = mkfw_win32_error_state(1);

	if(!state) {
		return;

	}
#else
	struct mkfw_error_state *state = &mkfw_linux_error_state;
#endif

	va_list args;

	va_start(args, fmt);
	vsnprintf(state->message, sizeof(state->message), fmt, args);
	va_end(args);
	state->code = 0;
	state->valid = 1;

	if(mkfw_error_callback) {
		mkfw_error_callback(state->message);

	}
}

// [=]===^=[ mkfw_get_last_error ]===============================================================[=]
MKFW_ERROR_API const char *mkfw_get_last_error(void) {
#ifdef _WIN32
	struct mkfw_error_state *state = mkfw_win32_error_state(0);
#else
	struct mkfw_error_state *state = &mkfw_linux_error_state;
#endif

	return state && state->valid ? state->message : 0;

}

// [=]===^=[ mkfw_get_last_error_code ]===========================================================[=]
MKFW_ERROR_API int64_t mkfw_get_last_error_code(void) {
#ifdef _WIN32
	struct mkfw_error_state *state = mkfw_win32_error_state(0);
#else
	struct mkfw_error_state *state = &mkfw_linux_error_state;
#endif

	return state && state->valid ? state->code : 0;

}

// [=]===^=[ mkfw_clear_last_error ]===============================================================[=]
MKFW_ERROR_API void mkfw_clear_last_error(void) {
#ifdef _WIN32
	struct mkfw_error_state *state = mkfw_win32_error_state(0);

	if(state) {
		memset(state, 0, sizeof(*state));

	}
#else
	memset(&mkfw_linux_error_state, 0, sizeof(mkfw_linux_error_state));
#endif
}
