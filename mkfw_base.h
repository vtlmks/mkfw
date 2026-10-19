// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#pragma once

/* Public-API linkage macros.
 *
 *   MKFW_API marks functions; MKFW_VAR marks the cross-TU variables
 *   that the inline helpers in this header (and in companion
 *   subsystem headers) reference.
 *
 *   Default (header-only / unity build):    MKFW_API == MKFW_VAR == static
 *     The platform .c is #included from this header; every public
 *     symbol is file-scope in the consumer's translation unit.
 *
 *   Static library build:                   MKFW_API == MKFW_VAR == extern
 *     Define MKFW_BUILD_LIBRARY when compiling the .c files into a
 *     static .a / .lib.  Consumers link against the archive and
 *     reference the public symbols via the header declarations.
 *     The library .c files each provide the storage for their
 *     MKFW_VAR variables in a gated block.
 *
 *   Shared library build (Windows DLL):     MKFW_API == __declspec(dllexport)
 *     Define MKFW_BUILD_SHARED when compiling the DLL.
 *
 *   Shared library consumer (Windows DLL):  MKFW_API == __declspec(dllimport)
 *     Define MKFW_USE_SHARED when including this header from a TU
 *     that links against the import library.
 *
 * Internal helpers (file-scope to one .c) stay marked `static`
 * regardless of mode.
 */
#if defined(MKFW_BUILD_SHARED) && defined(_WIN32)
	#define MKFW_API __declspec(dllexport)
	#define MKFW_VAR __declspec(dllexport) extern
#elif defined(MKFW_USE_SHARED) && defined(_WIN32)
	#define MKFW_API __declspec(dllimport)
	#define MKFW_VAR __declspec(dllimport) extern
#elif defined(MKFW_BUILD_LIBRARY)
	#define MKFW_API extern
	#define MKFW_VAR extern
#else
	#define MKFW_API static
	#define MKFW_VAR static
#endif
