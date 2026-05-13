// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

// In unity mode this file is #included from mkfw_audio.h.  In library
// mode it is compiled standalone; the include below pulls in the
// public audio types and MKFW_API macro.
#include "mkfw_audio.h"

#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <sched.h>

typedef int (*PFN_snd_pcm_open)(snd_pcm_t **, const char *, snd_pcm_stream_t, int);
typedef int (*PFN_snd_pcm_close)(snd_pcm_t *);
typedef int (*PFN_snd_pcm_hw_params_any)(snd_pcm_t *, snd_pcm_hw_params_t *);
typedef int (*PFN_snd_pcm_hw_params_set_access)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_access_t);
typedef int (*PFN_snd_pcm_hw_params_set_format)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_format_t);
typedef int (*PFN_snd_pcm_hw_params_set_channels_near)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int *);
typedef int (*PFN_snd_pcm_hw_params_set_rate_near)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int *, int *);
typedef int (*PFN_snd_pcm_hw_params_set_period_size_near)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_uframes_t *, int *);
typedef int (*PFN_snd_pcm_hw_params_set_buffer_size_near)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_uframes_t *);
typedef int (*PFN_snd_pcm_hw_params)(snd_pcm_t *, snd_pcm_hw_params_t *);
typedef int (*PFN_snd_pcm_hw_params_get_period_size)(const snd_pcm_hw_params_t *, snd_pcm_uframes_t *, int *);
typedef int (*PFN_snd_pcm_hw_params_get_buffer_size)(const snd_pcm_hw_params_t *, snd_pcm_uframes_t *);
typedef int (*PFN_snd_pcm_hw_params_get_rate)(const snd_pcm_hw_params_t *, unsigned int *, int *);
typedef int (*PFN_snd_pcm_hw_params_get_channels)(const snd_pcm_hw_params_t *, unsigned int *);
typedef size_t (*PFN_snd_pcm_hw_params_sizeof)(void);
typedef int (*PFN_snd_pcm_start)(snd_pcm_t *);
typedef int (*PFN_snd_pcm_wait)(snd_pcm_t *, int);
typedef snd_pcm_sframes_t (*PFN_snd_pcm_writei)(snd_pcm_t *, const void *, snd_pcm_uframes_t);
typedef int (*PFN_snd_pcm_drop)(snd_pcm_t *);
typedef int (*PFN_snd_pcm_recover)(snd_pcm_t *, int, int);

static PFN_snd_pcm_open mkfw_snd_pcm_open;
static PFN_snd_pcm_close mkfw_snd_pcm_close;
static PFN_snd_pcm_hw_params_any mkfw_snd_pcm_hw_params_any;
static PFN_snd_pcm_hw_params_set_access mkfw_snd_pcm_hw_params_set_access;
static PFN_snd_pcm_hw_params_set_format mkfw_snd_pcm_hw_params_set_format;
static PFN_snd_pcm_hw_params_set_channels_near mkfw_snd_pcm_hw_params_set_channels_near;
static PFN_snd_pcm_hw_params_set_rate_near mkfw_snd_pcm_hw_params_set_rate_near;
static PFN_snd_pcm_hw_params_set_period_size_near mkfw_snd_pcm_hw_params_set_period_size_near;
static PFN_snd_pcm_hw_params_set_buffer_size_near mkfw_snd_pcm_hw_params_set_buffer_size_near;
static PFN_snd_pcm_hw_params mkfw_snd_pcm_hw_params;
static PFN_snd_pcm_hw_params_get_period_size mkfw_snd_pcm_hw_params_get_period_size;
static PFN_snd_pcm_hw_params_get_buffer_size mkfw_snd_pcm_hw_params_get_buffer_size;
static PFN_snd_pcm_hw_params_get_rate mkfw_snd_pcm_hw_params_get_rate;
static PFN_snd_pcm_hw_params_get_channels mkfw_snd_pcm_hw_params_get_channels;
static PFN_snd_pcm_hw_params_sizeof mkfw_snd_pcm_hw_params_sizeof;
static PFN_snd_pcm_start mkfw_snd_pcm_start;
static PFN_snd_pcm_wait mkfw_snd_pcm_wait;
static PFN_snd_pcm_writei mkfw_snd_pcm_writei;
static PFN_snd_pcm_drop mkfw_snd_pcm_drop;
static PFN_snd_pcm_recover mkfw_snd_pcm_recover;

#define snd_pcm_open mkfw_snd_pcm_open
#define snd_pcm_close mkfw_snd_pcm_close
#define snd_pcm_hw_params_any mkfw_snd_pcm_hw_params_any
#define snd_pcm_hw_params_set_access mkfw_snd_pcm_hw_params_set_access
#define snd_pcm_hw_params_set_format mkfw_snd_pcm_hw_params_set_format
#define snd_pcm_hw_params_set_channels_near mkfw_snd_pcm_hw_params_set_channels_near
#define snd_pcm_hw_params_set_rate_near mkfw_snd_pcm_hw_params_set_rate_near
#define snd_pcm_hw_params_set_period_size_near mkfw_snd_pcm_hw_params_set_period_size_near
#define snd_pcm_hw_params_set_buffer_size_near mkfw_snd_pcm_hw_params_set_buffer_size_near
#define snd_pcm_hw_params mkfw_snd_pcm_hw_params
#define snd_pcm_hw_params_get_period_size mkfw_snd_pcm_hw_params_get_period_size
#define snd_pcm_hw_params_get_buffer_size mkfw_snd_pcm_hw_params_get_buffer_size
#define snd_pcm_hw_params_get_rate mkfw_snd_pcm_hw_params_get_rate
#define snd_pcm_hw_params_get_channels mkfw_snd_pcm_hw_params_get_channels
#define snd_pcm_hw_params_sizeof mkfw_snd_pcm_hw_params_sizeof
#define snd_pcm_start mkfw_snd_pcm_start
#define snd_pcm_wait mkfw_snd_pcm_wait
#define snd_pcm_writei mkfw_snd_pcm_writei
#define snd_pcm_drop mkfw_snd_pcm_drop
#define snd_pcm_recover mkfw_snd_pcm_recover

#define MKFW_AUDIO_DEFAULT_RATE     48000
#define MKFW_AUDIO_DEFAULT_CHANNELS 2
#define MKFW_AUDIO_DEFAULT_PERIOD   256
#define MKFW_AUDIO_BUFFER_PERIODS   2

static snd_pcm_t *mkfw_audio_pcm;
static mkfw_thread mkfw_audio_thread;
static float *mkfw_audio_buffer;
static uint32_t mkfw_audio_running;
static uint32_t mkfw_audio_alive;
static struct mkfw_audio_options mkfw_audio_opts;
static struct mkfw_audio_info    mkfw_audio_negotiated;
static mkfw_audio_callback_t mkfw_audio_user_cb;
static void *mkfw_audio_user_data;
static mkfw_audio_device_lost_callback_t mkfw_audio_lost_cb;
static void *mkfw_audio_lost_data;

// Set to 1 while mkfw_audio_init's synchronous open is in flight.
// Lets open_device / set_hw_params fire descriptive mkfw_error()
// calls on the first attempt without spamming the audio thread's
// background retry loop with duplicate messages.
static uint8_t mkfw_audio_init_phase;

// [=]===^=[ load_alsa_functions ]=================================================================[=]
static uint32_t load_alsa_functions(void) {
	void *lib = dlopen("libasound.so.2", RTLD_LAZY | RTLD_GLOBAL);
	if(!lib) {
		return 0;
	}

	#define LOAD(name) *(void **)&mkfw_##name = dlsym(lib, #name)
	LOAD(snd_pcm_open);
	LOAD(snd_pcm_close);
	LOAD(snd_pcm_hw_params_any);
	LOAD(snd_pcm_hw_params_set_access);
	LOAD(snd_pcm_hw_params_set_format);
	LOAD(snd_pcm_hw_params_set_channels_near);
	LOAD(snd_pcm_hw_params_set_rate_near);
	LOAD(snd_pcm_hw_params_set_period_size_near);
	LOAD(snd_pcm_hw_params_set_buffer_size_near);
	LOAD(snd_pcm_hw_params);
	LOAD(snd_pcm_hw_params_get_period_size);
	LOAD(snd_pcm_hw_params_get_buffer_size);
	LOAD(snd_pcm_hw_params_get_rate);
	LOAD(snd_pcm_hw_params_get_channels);
	LOAD(snd_pcm_hw_params_sizeof);
	LOAD(snd_pcm_start);
	LOAD(snd_pcm_wait);
	LOAD(snd_pcm_writei);
	LOAD(snd_pcm_drop);
	LOAD(snd_pcm_recover);
	#undef LOAD

	if(!mkfw_snd_pcm_open || !mkfw_snd_pcm_close || !mkfw_snd_pcm_hw_params_sizeof) {
		return 0;
	}
	return 1;
}

// [=]===^=[ mkfw_audio_set_hw_params ]===========================================================[=]
static int32_t mkfw_audio_set_hw_params(snd_pcm_t *handle) {
	snd_pcm_hw_params_t *params;
	uint32_t rate = mkfw_audio_opts.preferred_sample_rate ? mkfw_audio_opts.preferred_sample_rate : MKFW_AUDIO_DEFAULT_RATE;
	uint32_t channels = mkfw_audio_opts.preferred_channels ? mkfw_audio_opts.preferred_channels : MKFW_AUDIO_DEFAULT_CHANNELS;
	snd_pcm_uframes_t period = mkfw_audio_opts.preferred_buffer_frames ? mkfw_audio_opts.preferred_buffer_frames : MKFW_AUDIO_DEFAULT_PERIOD;
	snd_pcm_uframes_t buffer = period * MKFW_AUDIO_BUFFER_PERIODS;
	int32_t err;
	int32_t dir;

	snd_pcm_hw_params_alloca(&params);
	if((err = snd_pcm_hw_params_any(handle, params)) < 0) {
		if(mkfw_audio_init_phase) mkfw_error("ALSA: snd_pcm_hw_params_any failed (%d)", err);
		return err;
	}
	if((err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		if(mkfw_audio_init_phase) mkfw_error("ALSA: snd_pcm_hw_params_set_access(RW_INTERLEAVED) failed (%d)", err);
		return err;
	}
	if((err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT_LE)) < 0) {
		if(mkfw_audio_init_phase) mkfw_error("ALSA: snd_pcm_hw_params_set_format(FLOAT_LE) failed (%d)", err);
		return err;
	}
	if((err = snd_pcm_hw_params_set_channels_near(handle, params, &channels)) < 0) {
		if(mkfw_audio_init_phase) mkfw_error("ALSA: snd_pcm_hw_params_set_channels_near(%u) failed (%d)", channels, err);
		return err;
	}
	if((err = snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0)) < 0) {
		if(mkfw_audio_init_phase) mkfw_error("ALSA: snd_pcm_hw_params_set_rate_near(%u) failed (%d)", rate, err);
		return err;
	}
	if((err = snd_pcm_hw_params_set_period_size_near(handle, params, &period, 0)) < 0) {
		if(mkfw_audio_init_phase) mkfw_error("ALSA: snd_pcm_hw_params_set_period_size_near(%lu) failed (%d)", (unsigned long)period, err);
		return err;
	}
	if((err = snd_pcm_hw_params_set_buffer_size_near(handle, params, &buffer)) < 0) {
		if(mkfw_audio_init_phase) mkfw_error("ALSA: snd_pcm_hw_params_set_buffer_size_near(%lu) failed (%d)", (unsigned long)buffer, err);
		return err;
	}
	if((err = snd_pcm_hw_params(handle, params)) < 0) {
		if(mkfw_audio_init_phase) mkfw_error("ALSA: snd_pcm_hw_params (commit) failed (%d)", err);
		return err;
	}

	uint32_t actual_rate = 0, actual_channels = 0;
	snd_pcm_uframes_t actual_period = 0, actual_buffer = 0;
	snd_pcm_hw_params_get_rate(params, &actual_rate, &dir);
	snd_pcm_hw_params_get_channels(params, &actual_channels);
	snd_pcm_hw_params_get_period_size(params, &actual_period, &dir);
	snd_pcm_hw_params_get_buffer_size(params, &actual_buffer);

	mkfw_audio_negotiated.sample_rate       = actual_rate;
	mkfw_audio_negotiated.channels          = actual_channels;
	mkfw_audio_negotiated.period_frames     = (uint32_t)actual_period;
	mkfw_audio_negotiated.frames_per_buffer = (uint32_t)actual_buffer;
	mkfw_audio_negotiated.latency_ns        = actual_rate ? ((uint64_t)actual_buffer * 1000000000ULL) / actual_rate : 0;

	return 0;
}

// [=]===^=[ mkfw_audio_open_device ]=============================================================[=]
static int32_t mkfw_audio_open_device(void) {
	int err = snd_pcm_open(&mkfw_audio_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if(err < 0) {
		if(mkfw_audio_init_phase) mkfw_error("ALSA: snd_pcm_open(\"default\") failed (%d)", err);
		mkfw_audio_pcm = 0;
		return -1;
	}
	if(mkfw_audio_set_hw_params(mkfw_audio_pcm) < 0) {
		// set_hw_params already fired a specific mkfw_error if init_phase
		snd_pcm_close(mkfw_audio_pcm);
		mkfw_audio_pcm = 0;
		return -1;
	}
	free(mkfw_audio_buffer);
	mkfw_audio_buffer = calloc((size_t)mkfw_audio_negotiated.period_frames * mkfw_audio_negotiated.channels, sizeof(float));
	if((err = snd_pcm_start(mkfw_audio_pcm)) < 0) {
		if(mkfw_audio_init_phase) mkfw_error("ALSA: snd_pcm_start failed (%d)", err);
		snd_pcm_close(mkfw_audio_pcm);
		mkfw_audio_pcm = 0;
		return -1;
	}
	return 0;
}

// [=]===^=[ mkfw_audio_close_device ]============================================================[=]
static void mkfw_audio_close_device(void) {
	if(mkfw_audio_pcm) {
		snd_pcm_drop(mkfw_audio_pcm);
		snd_pcm_close(mkfw_audio_pcm);
		mkfw_audio_pcm = 0;
	}
}

// [=]===^=[ mkfw_audio_notify_lost ]=============================================================[=]
static void mkfw_audio_notify_lost(void) {
	if(!__atomic_exchange_n(&mkfw_audio_alive, 0, __ATOMIC_ACQ_REL)) {
		return;
	}
	mkfw_audio_device_lost_callback_t cb;
	__atomic_load(&mkfw_audio_lost_cb, &cb, __ATOMIC_ACQUIRE);
	if(cb) {
		cb(mkfw_audio_lost_data);
	}
}

// [=]===^=[ mkfw_audio_dispatch ]================================================================[=]
static void mkfw_audio_dispatch(float *buffer, uint32_t frames) {
	memset(buffer, 0, (size_t)frames * mkfw_audio_negotiated.channels * sizeof(float));
	mkfw_audio_callback_t cb;
	__atomic_load(&mkfw_audio_user_cb, &cb, __ATOMIC_ACQUIRE);
	if(cb) {
		cb(mkfw_audio_user_data, buffer, frames);
	}
}

// [=]===^=[ mkfw_audio_apply_rt_priority ]=======================================================[=]
static void mkfw_audio_apply_rt_priority(void) {
	if(!mkfw_audio_opts.realtime_priority) {
		return;
	}
	struct sched_param sp = {0};
	sp.sched_priority = 5;
	if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) != 0) {
		mkfw_error("audio: SCHED_FIFO not granted (no CAP_SYS_NICE / rtkit?); running at normal priority");
	}
}

// [=]===^=[ mkfw_audio_thread_func ]=============================================================[=]
static MKFW_THREAD_FUNC(mkfw_audio_thread_func, arg) {
	(void)arg;
	mkfw_audio_apply_rt_priority();

	while(__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) {
		if(!mkfw_audio_pcm) {
			if(mkfw_audio_open_device() < 0) {
				struct timespec ts = {0, 100000000};
				nanosleep(&ts, 0);
				continue;
			}
			__atomic_store_n(&mkfw_audio_alive, 1, __ATOMIC_RELEASE);
		}

		int32_t err = snd_pcm_wait(mkfw_audio_pcm, 100);
		if(err < 0) {
			if(!__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) {
				break;
			}
			if(snd_pcm_recover(mkfw_audio_pcm, err, 0) < 0) {
				mkfw_audio_close_device();
				mkfw_audio_notify_lost();
			}
			continue;
		}
		if(err == 0) {
			continue;
		}

		mkfw_audio_dispatch(mkfw_audio_buffer, mkfw_audio_negotiated.period_frames);
		err = snd_pcm_writei(mkfw_audio_pcm, mkfw_audio_buffer, mkfw_audio_negotiated.period_frames);
		if(err < 0) {
			if(snd_pcm_recover(mkfw_audio_pcm, err, 0) < 0) {
				mkfw_audio_close_device();
				mkfw_audio_notify_lost();
			}
		}
	}
	return 0;
}

// [=]===^=[ mkfw_audio_set_callback ]============================================================[=]
MKFW_API void mkfw_audio_set_callback(mkfw_audio_callback_t cb, void *userdata) {
	mkfw_audio_user_data = userdata;
	__atomic_store(&mkfw_audio_user_cb, &cb, __ATOMIC_RELEASE);
}

// [=]===^=[ mkfw_audio_set_device_lost_callback ]================================================[=]
MKFW_API void mkfw_audio_set_device_lost_callback(mkfw_audio_device_lost_callback_t cb, void *userdata) {
	mkfw_audio_lost_data = userdata;
	__atomic_store(&mkfw_audio_lost_cb, &cb, __ATOMIC_RELEASE);
}

// [=]===^=[ mkfw_audio_info ]====================================================================[=]
MKFW_API void mkfw_audio_info(struct mkfw_audio_info *out) {
	*out = mkfw_audio_negotiated;
}

// [=]===^=[ mkfw_audio_init ]====================================================================[=]
MKFW_API uint32_t mkfw_audio_init(struct mkfw_audio_options *opts) {
	struct mkfw_audio_options defaults = {0};
	if(!opts) {
		opts = &defaults;
	}
	mkfw_audio_opts = *opts;

	if(!load_alsa_functions()) {
		mkfw_error("ALSA: libasound.so.2 not loadable, or missing required symbols");
		return 0;
	}

	mkfw_audio_init_phase = 1;
	if(mkfw_audio_open_device() < 0) {
		// open_device / set_hw_params have fired a specific mkfw_error
		mkfw_audio_init_phase = 0;
		return 0;
	}
	mkfw_audio_init_phase = 0;
	__atomic_store_n(&mkfw_audio_alive, 1, __ATOMIC_RELEASE);
	__atomic_store_n(&mkfw_audio_running, 1, __ATOMIC_RELEASE);
	mkfw_audio_thread = mkfw_thread_create(mkfw_audio_thread_func, 0);
	return 1;
}

// [=]===^=[ mkfw_audio_shutdown ]================================================================[=]
MKFW_API void mkfw_audio_shutdown(void) {
	__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
	if(mkfw_audio_thread) {
		mkfw_thread_join(mkfw_audio_thread);
		mkfw_audio_thread = 0;
	}
	mkfw_audio_close_device();
	free(mkfw_audio_buffer);
	mkfw_audio_buffer = 0;
	__atomic_store_n(&mkfw_audio_alive, 0, __ATOMIC_RELEASE);
}
