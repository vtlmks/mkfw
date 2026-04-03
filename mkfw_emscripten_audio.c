// Copyright (c) 2026 Peter Fors
// SPDX-License-Identifier: MIT

#include <emscripten/webaudio.h>

#define MKFW_SAMPLE_RATE     48000
#define MKFW_NUM_CHANNELS    2
#define MKFW_BITS_PER_SAMPLE 16
#define MKFW_FRAME_SIZE      (MKFW_NUM_CHANNELS * (MKFW_BITS_PER_SAMPLE / 8))
#define MKFW_AUDIO_QUANTUM_MAX 256

void (*mkfw_audio_callback)(int16_t *audio_buffer, size_t frames);
static EMSCRIPTEN_WEBAUDIO_T mkfw_audio_context;
static uint8_t mkfw_audio_worklet_stack[4096];
static int32_t mkfw_audio_initialized;
static int16_t mkfw_audio_scratch[MKFW_AUDIO_QUANTUM_MAX * MKFW_NUM_CHANNELS];

// [=]===^=[ mkfw_set_audio_callback ]============================================================^===[=]
static void mkfw_set_audio_callback(void (*cb)(int16_t *, size_t)) {
	mkfw_audio_callback = cb;
}

// [=]===^=[ mkfw_audio_process ]=================================================================^===[=]
static EM_BOOL mkfw_audio_process(int32_t num_inputs, const AudioSampleFrame *inputs, int32_t num_outputs, AudioSampleFrame *outputs, int32_t num_params, const AudioParamFrame *params, void *user_data) {
	(void)num_inputs;
	(void)inputs;
	(void)num_params;
	(void)params;
	(void)user_data;

	for(int32_t o = 0; o < num_outputs; ++o) {
		int32_t frames = outputs[o].samplesPerChannel;
		int32_t channels = outputs[o].numberOfChannels;
		float *out = outputs[o].data;

		if(frames > MKFW_AUDIO_QUANTUM_MAX) {
			frames = MKFW_AUDIO_QUANTUM_MAX;
		}

		memset(mkfw_audio_scratch, 0, (size_t)(frames * MKFW_NUM_CHANNELS) * sizeof(int16_t));
		if(mkfw_audio_callback) {
			mkfw_audio_callback(mkfw_audio_scratch, (size_t)frames);
		}

#ifdef MKFW_AUDIO_POST_PROCESS
		MKFW_AUDIO_POST_PROCESS(mkfw_audio_scratch, (size_t)frames);
#endif

		// Convert interleaved int16 to planar float
		if(channels >= 2) {
			for(int32_t i = 0; i < frames; ++i) {
				out[i]          = (float)mkfw_audio_scratch[i * 2]     / 32768.0f;
				out[frames + i] = (float)mkfw_audio_scratch[i * 2 + 1] / 32768.0f;
			}
			for(int32_t ch = 2; ch < channels; ++ch) {
				memset(out + ch * frames, 0, (size_t)frames * sizeof(float));
			}
		} else if(channels == 1) {
			for(int32_t i = 0; i < frames; ++i) {
				float l = (float)mkfw_audio_scratch[i * 2]     / 32768.0f;
				float r = (float)mkfw_audio_scratch[i * 2 + 1] / 32768.0f;
				out[i] = (l + r) * 0.5f;
			}
		}
	}

	return EM_TRUE;
}

// [=]===^=[ mkfw_audio_processor_created ]=======================================================^===[=]
static void mkfw_audio_processor_created(EMSCRIPTEN_WEBAUDIO_T audio_context, EM_BOOL success, void *user_data) {
	(void)user_data;
	if(!success) {
		return;
	}

	int32_t output_channel_counts[1] = { MKFW_NUM_CHANNELS };

	EmscriptenAudioWorkletNodeCreateOptions options = {
		.numberOfInputs = 0,
		.numberOfOutputs = 1,
		.outputChannelCounts = output_channel_counts,
	};

	EMSCRIPTEN_AUDIO_WORKLET_NODE_T node = emscripten_create_wasm_audio_worklet_node(audio_context, "mkfw-audio", &options, &mkfw_audio_process, 0);
	emscripten_audio_node_connect(node, audio_context, 0, 0);
}

// [=]===^=[ mkfw_audio_worklet_initialized ]=====================================================^===[=]
static void mkfw_audio_worklet_initialized(EMSCRIPTEN_WEBAUDIO_T audio_context, EM_BOOL success, void *user_data) {
	(void)user_data;
	if(!success) {
		return;
	}

	WebAudioWorkletProcessorCreateOptions opts = {
		.name = "mkfw-audio",
	};
	emscripten_create_wasm_audio_worklet_processor_async(audio_context, &opts, mkfw_audio_processor_created, 0);
}

// [=]===^=[ mkfw_audio_initialize ]=============================================================^===[=]
static void mkfw_audio_initialize(void) {
	if(mkfw_audio_initialized) {
		return;
	}
	mkfw_audio_initialized = 1;

	EmscriptenWebAudioCreateAttributes attrs = {
		.latencyHint = "interactive",
		.sampleRate = MKFW_SAMPLE_RATE,
	};

	mkfw_audio_context = emscripten_create_audio_context(&attrs);
	emscripten_start_wasm_audio_worklet_thread_async(mkfw_audio_context, mkfw_audio_worklet_stack, sizeof(mkfw_audio_worklet_stack), mkfw_audio_worklet_initialized, 0);
}

// [=]===^=[ mkfw_em_resume_audio_ctx ]==========================================================^===[=]
static void mkfw_em_resume_audio_ctx(void) {
	if(mkfw_audio_context) {
		emscripten_resume_audio_context_sync(mkfw_audio_context);
	}
}

// [=]===^=[ mkfw_audio_shutdown ]===============================================================^===[=]
static void mkfw_audio_shutdown(void) {
	if(!mkfw_audio_initialized) {
		return;
	}
	mkfw_audio_initialized = 0;
	emscripten_destroy_audio_context(mkfw_audio_context);
	mkfw_audio_context = 0;
}
