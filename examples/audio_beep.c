// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT
//
// Plays a 440 Hz sine wave for one second through mkfw_audio.

#include <math.h>
#include <stdio.h>
#include <time.h>

#include "../mkfw.h"
#include "../mkfw_audio.h"

struct beep_state {
	double phase;
	double phase_step;
};

// [=]===^=[ on_audio ]==========================================================================^===[=]
static void on_audio(void *userdata, float *buffer, uint32_t frames) {
	struct beep_state *st = (struct beep_state *)userdata;
	struct mkfw_audio_info info;
	mkfw_audio_info(&info);

	for(uint32_t f = 0; f < frames; ++f) {
		float sample = (float)(sin(st->phase) * 0.2);
		st->phase += st->phase_step;
		if(st->phase > 6.283185307179586) {
			st->phase -= 6.283185307179586;
		}
		for(uint32_t c = 0; c < info.channels; ++c) {
			buffer[f * info.channels + c] = sample;
		}
	}
}

// [=]===^=[ on_device_lost ]====================================================================^===[=]
static void on_device_lost(void *userdata) {
	(void)userdata;
	fprintf(stderr, "audio device lost; mkfw will keep retrying\n");
}

// [=]===^=[ on_error ]==========================================================================^===[=]
static void on_error(const char *message) {
	fprintf(stderr, "mkfw: %s\n", message);
}

// [=]===^=[ main ]==============================================================================^===[=]
int main(void) {
	mkfw_set_error_callback(on_error);

	struct mkfw_audio_options opts = {0};
	if(!mkfw_audio_init(&opts)) {
		return 1;
	}

	struct mkfw_audio_info info;
	mkfw_audio_info(&info);
	printf("audio: %u Hz, %u channel(s), %u frames/buffer, %u frames/period, %.2f ms latency\n",
		info.sample_rate, info.channels,
		info.frames_per_buffer, info.period_frames,
		(double)info.latency_ns / 1.0e6);

	struct beep_state st = {0};
	st.phase_step = 2.0 * 3.141592653589793 * 440.0 / (double)info.sample_rate;

	mkfw_audio_set_device_lost_callback(on_device_lost, 0);
	mkfw_audio_set_callback(on_audio, &st);

	struct timespec ts = {1, 0};
	nanosleep(&ts, 0);

	mkfw_audio_set_callback(0, 0);
	mkfw_audio_shutdown();
	return 0;
}
