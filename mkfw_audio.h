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
// See the Linking section in README.md for the canonical list of
// required libraries on each platform (Audio adds ole32, avrt,
// and uuid on Windows; nothing extra on Linux).
//
// Format contract.  The audio callback always receives interleaved
// 32-bit IEEE float samples in the [-1.0, 1.0] range.  Sample rate
// and channel count are negotiated at mkfw_audio_init() time; the
// negotiated values (along with buffer size and estimated latency)
// are retrievable via mkfw_audio_info().  mkfw does not resample;
// the OS audio layer (WASAPI shared / PipeWire / PulseAudio / ALSA
// plug) is trusted to convert the negotiated stream to whatever the
// physical device wants.

#pragma once

#include <stdint.h>
#include <stddef.h>

#include "mkfw.h"

/* Callback invoked from the audio thread to fill the next period.
 * The buffer holds (frames * channels) interleaved float samples. */
typedef void (*mkfw_audio_callback_t)(void *userdata, float *buffer, uint32_t frames);

/* Callback fired once each time the audio device transitions from
 * playing to lost.  mkfw will continue trying to reopen the default
 * device in the background; this hook only exists so the
 * application can react (pause music, dim a UI indicator, ...). */
typedef void (*mkfw_audio_device_lost_callback_t)(void *userdata);

/* Callback fired once each time the audio device transitions from
 * lost back to playing (the silent retry loop in the audio thread
 * successfully re-opened the default device).  Does NOT fire for
 * the initial open at mkfw_audio_init time; install the callback
 * before init and use the init return value for that case.
 *
 * The negotiated rate / channels / buffer size may differ from the
 * previous device.  Re-read mkfw_audio_info() before relying on the
 * old values. */
typedef void (*mkfw_audio_device_acquired_callback_t)(void *userdata);

/* Audio init options.  Pass 0 to use defaults for every field.
 * The version field must be 0 for now; future revisions may add
 * fields and bump the version. */
struct mkfw_audio_options {
	uint32_t version;                 // 0 = current
	uint32_t preferred_sample_rate;   // 0 = 48000
	uint32_t preferred_channels;      // 0 = 2 (stereo)
	uint32_t preferred_buffer_frames; // 0 = backend default
	uint32_t realtime_priority;       // 0 = normal, non-zero = try RT scheduling
};

/* Audio device info.  Filled by mkfw_audio_info(). */
struct mkfw_audio_info {
	uint32_t sample_rate;        // negotiated, in Hz
	uint32_t channels;           // negotiated
	uint32_t frames_per_buffer;  // device buffer in frames
	uint32_t period_frames;      // callback granularity in frames
	uint64_t latency_ns;         // best estimate, total
};

MKFW_API uint32_t mkfw_audio_init(struct mkfw_audio_options *opts);
MKFW_API void     mkfw_audio_shutdown(void);
MKFW_API void     mkfw_audio_set_callback(mkfw_audio_callback_t cb, void *userdata);
MKFW_API void     mkfw_audio_set_device_lost_callback(mkfw_audio_device_lost_callback_t cb, void *userdata);
MKFW_API void     mkfw_audio_set_device_acquired_callback(mkfw_audio_device_acquired_callback_t cb, void *userdata);
MKFW_API void     mkfw_audio_info(struct mkfw_audio_info *out);

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
