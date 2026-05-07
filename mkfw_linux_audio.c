// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT


#define MKFW_SAMPLE_RATE     48000
#define MKFW_NUM_CHANNELS    2
#define MKFW_BITS_PER_SAMPLE 16
#define MKFW_FRAME_SIZE      (MKFW_NUM_CHANNELS * (MKFW_BITS_PER_SAMPLE / 8))
#define MKFW_PREFERRED_FRAMES_PER_BUFFER 256
#define MKFW_BUFFER_SIZE     (MKFW_PREFERRED_FRAMES_PER_BUFFER * MKFW_FRAME_SIZE)
#define MKFW_BUFFER_COUNT    2

void (*mkfw_audio_callback)(int16_t *audio_buffer, size_t frames);

// [=]===^=[ mkfw_set_audio_callback ]============================================================[=]
static void mkfw_set_audio_callback(void (*cb)(int16_t *, size_t)) {
	__atomic_store(&mkfw_audio_callback, &cb, __ATOMIC_RELEASE);
}

// [=]===^=[ mkfw_audio_callback_thread ]=========================================================[=]
static void mkfw_audio_callback_thread(int16_t *audio_buffer, size_t frames) {
	memset(audio_buffer, 0, frames * MKFW_NUM_CHANNELS * 2);
	void (*cb)(int16_t *, size_t);
	__atomic_load(&mkfw_audio_callback, &cb, __ATOMIC_ACQUIRE);
	if(cb) {
		cb(audio_buffer, frames);
	}
#ifdef MKFW_AUDIO_POST_PROCESS
	MKFW_AUDIO_POST_PROCESS(audio_buffer, frames);
#endif
}

#include <alsa/asoundlib.h>
#include <dlfcn.h>

typedef int (*PFN_snd_pcm_open)(snd_pcm_t **, const char *, snd_pcm_stream_t, int);
typedef int (*PFN_snd_pcm_close)(snd_pcm_t *);
typedef int (*PFN_snd_pcm_hw_params_any)(snd_pcm_t *, snd_pcm_hw_params_t *);
typedef int (*PFN_snd_pcm_hw_params_set_access)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_access_t);
typedef int (*PFN_snd_pcm_hw_params_set_format)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_format_t);
typedef int (*PFN_snd_pcm_hw_params_set_channels)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int);
typedef int (*PFN_snd_pcm_hw_params_set_rate_near)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int *, int *);
typedef int (*PFN_snd_pcm_hw_params_set_period_size_near)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_uframes_t *, int *);
typedef int (*PFN_snd_pcm_hw_params_set_buffer_size_near)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_uframes_t *);
typedef int (*PFN_snd_pcm_hw_params)(snd_pcm_t *, snd_pcm_hw_params_t *);
typedef int (*PFN_snd_pcm_hw_params_get_period_size)(const snd_pcm_hw_params_t *, snd_pcm_uframes_t *, int *);
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
static PFN_snd_pcm_hw_params_set_channels mkfw_snd_pcm_hw_params_set_channels;
static PFN_snd_pcm_hw_params_set_rate_near mkfw_snd_pcm_hw_params_set_rate_near;
static PFN_snd_pcm_hw_params_set_period_size_near mkfw_snd_pcm_hw_params_set_period_size_near;
static PFN_snd_pcm_hw_params_set_buffer_size_near mkfw_snd_pcm_hw_params_set_buffer_size_near;
static PFN_snd_pcm_hw_params mkfw_snd_pcm_hw_params;
static PFN_snd_pcm_hw_params_get_period_size mkfw_snd_pcm_hw_params_get_period_size;
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
#define snd_pcm_hw_params_set_channels mkfw_snd_pcm_hw_params_set_channels
#define snd_pcm_hw_params_set_rate_near mkfw_snd_pcm_hw_params_set_rate_near
#define snd_pcm_hw_params_set_period_size_near mkfw_snd_pcm_hw_params_set_period_size_near
#define snd_pcm_hw_params_set_buffer_size_near mkfw_snd_pcm_hw_params_set_buffer_size_near
#define snd_pcm_hw_params mkfw_snd_pcm_hw_params
#define snd_pcm_hw_params_get_period_size mkfw_snd_pcm_hw_params_get_period_size
#define snd_pcm_hw_params_sizeof mkfw_snd_pcm_hw_params_sizeof
#define snd_pcm_start mkfw_snd_pcm_start
#define snd_pcm_wait mkfw_snd_pcm_wait
#define snd_pcm_writei mkfw_snd_pcm_writei
#define snd_pcm_drop mkfw_snd_pcm_drop
#define snd_pcm_recover mkfw_snd_pcm_recover

static int32_t load_alsa_functions(void) {
	void *lib = dlopen("libasound.so.2", RTLD_LAZY | RTLD_GLOBAL);
	if(!lib) {
		return -1;
	}

	#define LOAD(name) *(void **)&mkfw_##name = dlsym(lib, #name)
	LOAD(snd_pcm_open);
	LOAD(snd_pcm_close);
	LOAD(snd_pcm_hw_params_any);
	LOAD(snd_pcm_hw_params_set_access);
	LOAD(snd_pcm_hw_params_set_format);
	LOAD(snd_pcm_hw_params_set_channels);
	LOAD(snd_pcm_hw_params_set_rate_near);
	LOAD(snd_pcm_hw_params_set_period_size_near);
	LOAD(snd_pcm_hw_params_set_buffer_size_near);
	LOAD(snd_pcm_hw_params);
	LOAD(snd_pcm_hw_params_get_period_size);
	LOAD(snd_pcm_hw_params_sizeof);
	LOAD(snd_pcm_start);
	LOAD(snd_pcm_wait);
	LOAD(snd_pcm_writei);
	LOAD(snd_pcm_drop);
	LOAD(snd_pcm_recover);
	#undef LOAD

	if(!mkfw_snd_pcm_open || !mkfw_snd_pcm_close || !mkfw_snd_pcm_hw_params_sizeof) {
		return -1;
	}
	return 0;
}

static snd_pcm_t *mkfw_pcm;
static mkfw_thread mkfw_audio_thread;
static int16_t *mkfw_audio_buffer;
static snd_pcm_uframes_t mkfw_frames_per_period;
static int mkfw_audio_running;

// [=]===^=[ mkfw_set_hw_params ]=================================================================[=]
static int32_t mkfw_set_hw_params(snd_pcm_t *handle) {
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t period = MKFW_PREFERRED_FRAMES_PER_BUFFER;
	snd_pcm_uframes_t buffer = period * MKFW_BUFFER_COUNT;
	uint32_t rate = MKFW_SAMPLE_RATE;
	int32_t err;
	int32_t dir;

	snd_pcm_hw_params_alloca(&params);
	if((err = snd_pcm_hw_params_any(handle, params)) < 0) {
		return err;
	}
	if((err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		return err;
	}
	if((err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE)) < 0) {
		return err;
	}
	if((err = snd_pcm_hw_params_set_channels(handle, params, MKFW_NUM_CHANNELS)) < 0) {
		return err;
	}
	if((err = snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0)) < 0) {
		return err;
	}
	if((err = snd_pcm_hw_params_set_period_size_near(handle, params, &period, 0)) < 0) {
		return err;
	}
	if((err = snd_pcm_hw_params_set_buffer_size_near(handle, params, &buffer)) < 0) {
		return err;
	}
	if((err = snd_pcm_hw_params(handle, params)) < 0) {
		return err;
	}

	snd_pcm_hw_params_get_period_size(params, &mkfw_frames_per_period, &dir);

	return 0;
}

// [=]===^=[ mkfw_audio_open_device ]=============================================================[=]
static int32_t mkfw_audio_open_device(void) {
	if(snd_pcm_open(&mkfw_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		mkfw_pcm = 0;
		return -1;
	}
	if(mkfw_set_hw_params(mkfw_pcm) < 0) {
		snd_pcm_close(mkfw_pcm);
		mkfw_pcm = 0;
		return -1;
	}
	free(mkfw_audio_buffer);
	mkfw_audio_buffer = calloc(mkfw_frames_per_period * MKFW_NUM_CHANNELS, sizeof(int16_t));
	if(snd_pcm_start(mkfw_pcm) < 0) {
		snd_pcm_close(mkfw_pcm);
		mkfw_pcm = 0;
		return -1;
	}
	return 0;
}

// [=]===^=[ mkfw_audio_close_device ]============================================================[=]
static void mkfw_audio_close_device(void) {
	if(mkfw_pcm) {
		snd_pcm_drop(mkfw_pcm);
		snd_pcm_close(mkfw_pcm);
		mkfw_pcm = 0;
	}
}

// [=]===^=[ mkfw_audio_thread_func ]=============================================================[=]
static MKFW_THREAD_FUNC(mkfw_audio_thread_func, arg) {
	(void)arg;
	while(__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) {
		if(!mkfw_pcm) {
			if(mkfw_audio_open_device() < 0) {
				struct timespec ts = {0, 100000000};
				nanosleep(&ts, 0);
				continue;
			}
		}

		int32_t err = snd_pcm_wait(mkfw_pcm, 100);
		if(err < 0) {
			if(!__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) {
				break;
			}
			if(snd_pcm_recover(mkfw_pcm, err, 0) < 0) {
				mkfw_audio_close_device();
				continue;
			}
			continue;
		}
		if(err == 0) {
			continue;
		}

		mkfw_audio_callback_thread(mkfw_audio_buffer, mkfw_frames_per_period);
		err = snd_pcm_writei(mkfw_pcm, mkfw_audio_buffer, mkfw_frames_per_period);
		if(err < 0) {
			if(snd_pcm_recover(mkfw_pcm, err, 0) < 0) {
				mkfw_audio_close_device();
			}
		}
	}
	return 0;
}

// [=]===^=[ mkfw_audio_initialize ]==============================================================[=]
static void mkfw_audio_initialize(void) {
	if(load_alsa_functions() < 0) {
		mkfw_error("ALSA not available");
		return;
	}
	mkfw_audio_open_device();
	__atomic_store_n(&mkfw_audio_running, 1, __ATOMIC_RELEASE);
	mkfw_audio_thread = mkfw_thread_create(mkfw_audio_thread_func, 0);
}

// [=]===^=[ mkfw_audio_shutdown ]================================================================[=]
static void mkfw_audio_shutdown(void) {
	__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
	if(mkfw_audio_thread) {
		mkfw_thread_join(mkfw_audio_thread);
	}
	mkfw_audio_close_device();
	free(mkfw_audio_buffer);
	mkfw_audio_buffer = 0;
}
