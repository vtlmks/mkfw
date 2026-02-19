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

static snd_pcm_t *mkfw_pcm;
static mkfw_thread mkfw_audio_thread;
static int16_t *mkfw_audio_buffer;
static snd_pcm_uframes_t mkfw_frames_per_period;
static int mkfw_audio_running;

static MKFW_THREAD_FUNC(mkfw_audio_thread_func, arg) {
	while(__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) {
		int32_t err = snd_pcm_wait(mkfw_pcm, 100);
		if(err < 0) {
			if(!__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) break;
			snd_pcm_recover(mkfw_pcm, err, 0);
			continue;
		}
		if(err == 0) continue; // timeout, re-check flag

		mkfw_audio_callback_thread(mkfw_audio_buffer, mkfw_frames_per_period);
		err = snd_pcm_writei(mkfw_pcm, mkfw_audio_buffer, mkfw_frames_per_period);
		if(err < 0) {
			snd_pcm_recover(mkfw_pcm, err, 0);
		}
	}
	return 0;
}

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

static void mkfw_audio_initialize(void) {
	if(snd_pcm_open(&mkfw_pcm, "plug:pipewire", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		if(snd_pcm_open(&mkfw_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
			return;
		}
	}
	if(mkfw_set_hw_params(mkfw_pcm) < 0) {
		return;
	}

	mkfw_audio_buffer = calloc(mkfw_frames_per_period * MKFW_NUM_CHANNELS, sizeof(int16_t));

	if(snd_pcm_start(mkfw_pcm) < 0) {
		return;
	}
	__atomic_store_n(&mkfw_audio_running, 1, __ATOMIC_RELEASE);
	mkfw_audio_thread = mkfw_thread_create(mkfw_audio_thread_func, 0);
}

static void mkfw_audio_shutdown(void) {
	__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
	mkfw_thread_join(mkfw_audio_thread);
	snd_pcm_drop(mkfw_pcm);
	snd_pcm_close(mkfw_pcm);
	free(mkfw_audio_buffer);
}

