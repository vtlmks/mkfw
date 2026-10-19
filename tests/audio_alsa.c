// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#define _GNU_SOURCE
#include "../mkfw_audio.h"

struct test_write {
	snd_pcm_sframes_t frames;
	uint32_t stop;
};

struct test_case {
	char *name;
	struct test_write writes[4];
	uint32_t write_count;
	int32_t recovery_result;
	uint32_t recoveries;
	uint32_t closes;
	uint32_t frames;
};

static struct test_case cases[] = {
	{"partial writes", {{2, 0}, {1, 0}, {1, 1}}, 3, 0, 0, 0, 4},
	{"underrun recovery", {{-EPIPE, 0}, {4, 1}}, 2, 0, 1, 0, 4},
	{"partial write interrupted", {{2, 0}, {-EINTR, 0}, {2, 1}}, 3, 0, 1, 0, 4},
	{"shutdown during repeated recovery", {{-EPIPE, 0}, {-EPIPE, 1}}, 2, 0, 1, 0, 0},
	{"shutdown during successful writes", {{2, 1}, {2, 0}}, 2, 0, 0, 0, 4},
	{"unrecoverable write", {{-ENODEV, 0}}, 1, -ENODEV, 1, 1, 0},
	{"zero write", {{0, 0}}, 1, 0, 0, 1, 0}
};

static struct test_case *current;
static PFN_snd_pcm_open alsa_open;
static uint32_t failures;
static uint32_t checks;
static uint32_t write_position;
static uint32_t frame_position;
static uint32_t callback_count;
static uint32_t recovery_count;
static uint32_t close_count;
static uint32_t lost_count;
static uint32_t wait_count;

// [=]===^=[ test_check ]========================================================================[=]
static void test_check(uint32_t condition, char *message) {
	++checks;

	if(!condition) {
		fprintf(stderr, "FAIL: %s: %s\n", current ? current->name : "startup", message);
		++failures;

	}
}

// [=]===^=[ test_open ]=========================================================================[=]
static int test_open(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode) {
	test_check(strcmp(name, "default") == 0, "opens default device");
	return alsa_open(pcm, "null", stream, mode);

}

// [=]===^=[ test_avail_failure ]================================================================[=]
static int test_avail_failure(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t frames) {
	test_check(pcm == mkfw_audio_pcm && params && frames == mkfw_audio_negotiated.period_frames, "software parameter failure arguments");
	return -EIO;

}

// [=]===^=[ test_wait ]=========================================================================[=]
static int test_wait(snd_pcm_t *pcm, int timeout) {
	test_check(pcm == mkfw_audio_pcm && timeout == 100, "wait arguments");
	++wait_count;

	if(wait_count > 1) {
		__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
		return -EINTR;

	}

	return 1;

}

// [=]===^=[ test_callback ]=====================================================================[=]
static void test_callback(void *userdata, float *buffer, uint32_t frames) {
	test_check(userdata == current && frames == 4, "callback arguments");
	++callback_count;

	for(uint32_t i = 0; i < frames * 2; ++i) {
		buffer[i] = i + 1;

	}
}

// [=]===^=[ test_write ]========================================================================[=]
static snd_pcm_sframes_t test_write(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t frames) {
	test_check(pcm == mkfw_audio_pcm, "write device");
	test_check(buffer == mkfw_audio_buffer + frame_position * 2, "write resumes at unwritten samples");
	test_check(frames == 4 - frame_position, "write requests remaining frames");

	if(write_position >= current->write_count) {
		test_check(0, "unexpected extra write");
		__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
		return -EIO;

	}

	struct test_write *step = &current->writes[write_position++];

	if(step->frames > 0) {
		float *samples = mkfw_audio_buffer + frame_position * 2;

		for(snd_pcm_sframes_t i = 0; i < step->frames * 2; ++i) {
			test_check(samples[i] == frame_position * 2 + i + 1, "sample contents preserved");

		}

		frame_position += step->frames;

	}

	if(step->stop) {
		__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);

	}

	return step->frames;

}

// [=]===^=[ test_recover ]======================================================================[=]
static int test_recover(snd_pcm_t *pcm, int error, int silent) {
	test_check(pcm == mkfw_audio_pcm && error == current->writes[write_position - 1].frames && silent == 0, "recovery arguments");
	++recovery_count;
	return current->recovery_result;

}

// [=]===^=[ test_drop ]=========================================================================[=]
static int test_drop(snd_pcm_t *pcm) {
	test_check(pcm == mkfw_audio_pcm, "drop device");
	return 0;

}

// [=]===^=[ test_close ]========================================================================[=]
static int test_close(snd_pcm_t *pcm) {
	test_check(pcm == mkfw_audio_pcm, "close device");
	++close_count;
	__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
	return 0;

}

// [=]===^=[ test_lost ]=========================================================================[=]
static void test_lost(void *userdata) {
	test_check(userdata == current, "device lost userdata");
	++lost_count;

}

// [=]===^=[ main ]==============================================================================[=]
int main(void) {
	if(load_alsa_functions()) {
		alsa_open = mkfw_snd_pcm_open;
		mkfw_snd_pcm_open = test_open;

		if(mkfw_audio_open_device() == 0) {
			snd_pcm_t *pcm = mkfw_audio_pcm;
			PFN_snd_pcm_close alsa_close = mkfw_snd_pcm_close;
			snd_pcm_sw_params_t *params;
			snd_pcm_uframes_t avail_min = 0;
			snd_pcm_uframes_t start_threshold = 0;

			snd_pcm_sw_params_alloca(&params);
			test_check(snd_pcm_sw_params_current(pcm, params) == 0, "read negotiated software parameters");
			test_check(snd_pcm_sw_params_get_avail_min(params, &avail_min) == 0 && avail_min == mkfw_audio_negotiated.period_frames, "one period readiness threshold");
			test_check(snd_pcm_sw_params_get_start_threshold(params, &start_threshold) == 0 && start_threshold == mkfw_audio_negotiated.frames_per_buffer, "full buffer startup threshold");
			test_check(snd_pcm_state(pcm) == SND_PCM_STATE_PREPARED, "playback waits for priming");
			PFN_snd_pcm_sw_params_set_avail_min alsa_set_avail_min = mkfw_snd_pcm_sw_params_set_avail_min;
			mkfw_snd_pcm_sw_params_set_avail_min = test_avail_failure;
			mkfw_audio_init_phase = 1;
			test_check(mkfw_audio_set_hw_params(pcm) == -EIO, "software parameter failure returned");
			test_check(mkfw_get_last_error() && strcmp(mkfw_get_last_error(), "ALSA: snd_pcm_sw_params_set_avail_min failed") == 0 && mkfw_get_last_error_code() == -EIO, "fixed diagnostic retains native error code");
			mkfw_snd_pcm_sw_params_set_avail_min = alsa_set_avail_min;
			mkfw_audio_init_phase = 0;
			mkfw_clear_last_error();
			test_check(mkfw_audio_set_hw_params(pcm) == 0, "software parameters recover after failure");

			mkfw_snd_pcm_wait = test_wait;
			mkfw_snd_pcm_writei = test_write;
			mkfw_snd_pcm_recover = test_recover;
			mkfw_snd_pcm_drop = test_drop;
			mkfw_snd_pcm_close = test_close;
			mkfw_audio_negotiated.period_frames = 4;
			mkfw_audio_negotiated.channels = 2;

			for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
				current = &cases[i];
				write_position = 0;
				frame_position = 0;
				callback_count = 0;
				recovery_count = 0;
				close_count = 0;
				lost_count = 0;
				wait_count = 0;
				mkfw_audio_pcm = pcm;
				mkfw_audio_set_callback(test_callback, current);
				mkfw_audio_set_device_lost_callback(test_lost, current);
				__atomic_store_n(&mkfw_audio_running, 1, __ATOMIC_RELEASE);
				__atomic_store_n(&mkfw_audio_alive, 1, __ATOMIC_RELEASE);
				mkfw_audio_thread_func(0);
				test_check(write_position == current->write_count, "expected write count");
				test_check(frame_position == current->frames, "expected submitted frames");
				test_check(callback_count == 1 && wait_count == 1, "period generated once");
				test_check(recovery_count == current->recoveries, "expected recovery count");
				test_check(close_count == current->closes && lost_count == current->closes, "device lost only for failed writes");

			}

			alsa_close(pcm);
			mkfw_audio_pcm = 0;
			free(mkfw_audio_buffer);
			mkfw_audio_buffer = 0;
			printf("ALSA audio: %u checks, %u failures\n", checks, failures);
			return failures ? 1 : 0;

		} else {
			fprintf(stderr, "Cannot open ALSA null device\n");

		}

	} else {
		fprintf(stderr, "Cannot load ALSA\n");

	}

	return 1;

}
