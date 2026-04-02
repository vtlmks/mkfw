// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#include <emscripten.h>

#define MKFW_SAMPLE_RATE     48000
#define MKFW_NUM_CHANNELS    2
#define MKFW_BITS_PER_SAMPLE 16
#define MKFW_FRAME_SIZE      (MKFW_NUM_CHANNELS * (MKFW_BITS_PER_SAMPLE / 8))
#define MKFW_PREFERRED_FRAMES_PER_BUFFER 256

void (*mkfw_audio_callback)(int16_t *audio_buffer, size_t frames);

// [=]===^=[ mkfw_set_audio_callback ]============================================================^===[=]
static void mkfw_set_audio_callback(void (*cb)(int16_t *, size_t)) {
	mkfw_audio_callback = cb;
}

// [=]===^=[ mkfw_audio_callback_thread ]=========================================================^===[=]
static void mkfw_audio_callback_thread(int16_t *audio_buffer, size_t frames) {
	memset(audio_buffer, 0, frames * MKFW_NUM_CHANNELS * 2);
	if(mkfw_audio_callback) {
		mkfw_audio_callback(audio_buffer, frames);
	}
#ifdef MKFW_AUDIO_POST_PROCESS
	MKFW_AUDIO_POST_PROCESS(audio_buffer, frames);
#endif
}

static int16_t mkfw_emscripten_audio_buffer[MKFW_PREFERRED_FRAMES_PER_BUFFER * MKFW_NUM_CHANNELS];
static int32_t mkfw_audio_initialized;

// Called from JavaScript to fill the audio buffer.
// Returns a pointer to the int16 buffer for JS to read.
EMSCRIPTEN_KEEPALIVE
int16_t *mkfw_emscripten_audio_fill(int32_t frames) {
	if(frames > MKFW_PREFERRED_FRAMES_PER_BUFFER) {
		frames = MKFW_PREFERRED_FRAMES_PER_BUFFER;
	}
	mkfw_audio_callback_thread(mkfw_emscripten_audio_buffer, (size_t)frames);
	return mkfw_emscripten_audio_buffer;
}

// [=]===^=[ mkfw_audio_initialize ]=============================================================^===[=]
static void mkfw_audio_initialize(void) {
	if(mkfw_audio_initialized) {
		return;
	}
	mkfw_audio_initialized = 1;

	EM_ASM({
		var bufferSize = $0;
		var sampleRate = $1;
		var channels = $2;

		// AudioContext must be resumed after a user gesture in most browsers.
		// We create it here and let the browser auto-resume on first interaction.
		var ctx = new (window.AudioContext || window.webkitAudioContext)({ sampleRate: sampleRate });
		Module._mkfw_audio_ctx = ctx;

		var node = ctx.createScriptProcessor(bufferSize, 0, channels);
		Module._mkfw_audio_node = node;

		node.onaudioprocess = function(e) {
			var outputBuffer = e.outputBuffer;
			var frames = outputBuffer.length;
			var ptr = Module._mkfw_emscripten_audio_fill(frames);
			var left = outputBuffer.getChannelData(0);
			var right = outputBuffer.getChannelData(1);

			// Convert int16 interleaved to float separate channels
			for(var i = 0; i < frames; i++) {
				left[i] = Module.HEAP16[(ptr >> 1) + i * 2] / 32768.0;
				right[i] = Module.HEAP16[(ptr >> 1) + i * 2 + 1] / 32768.0;
			}
		};

		node.connect(ctx.destination);

		// Try to resume AudioContext (browsers require user gesture)
		if(ctx.state === 'suspended') {
			var resume = function() {
				ctx.resume();
				document.removeEventListener('click', resume);
				document.removeEventListener('keydown', resume);
				document.removeEventListener('touchstart', resume);
			};
			document.addEventListener('click', resume);
			document.addEventListener('keydown', resume);
			document.addEventListener('touchstart', resume);
		}
	}, MKFW_PREFERRED_FRAMES_PER_BUFFER, MKFW_SAMPLE_RATE, MKFW_NUM_CHANNELS);
}

// [=]===^=[ mkfw_audio_shutdown ]===============================================================^===[=]
static void mkfw_audio_shutdown(void) {
	if(!mkfw_audio_initialized) {
		return;
	}
	mkfw_audio_initialized = 0;

	EM_ASM({
		if(Module._mkfw_audio_node) {
			Module._mkfw_audio_node.disconnect();
			Module._mkfw_audio_node = null;
		}
		if(Module._mkfw_audio_ctx) {
			Module._mkfw_audio_ctx.close();
			Module._mkfw_audio_ctx = null;
		}
	});
}
