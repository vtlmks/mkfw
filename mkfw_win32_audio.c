// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#define COBJMACROS
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#include <timeapi.h>

// WASAPI GUIDs -- defined explicitly so we don't depend on uuid.lib (MSVC/clang-cl)
// or initguid.h ordering. Works on MinGW, MSVC, and clang-cl.
#define MKFW_GUID_DEF(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	static const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

MKFW_GUID_DEF(mkfw_CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);
MKFW_GUID_DEF(mkfw_IID_IMMDeviceEnumerator,   0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
MKFW_GUID_DEF(mkfw_IID_IAudioClient,          0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
MKFW_GUID_DEF(mkfw_IID_IAudioRenderClient,    0xf294acfc, 0x3146, 0x4483, 0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2);

#undef MKFW_GUID_DEF

#define MKFW_AUDIO_DEFAULT_RATE     48000
#define MKFW_AUDIO_DEFAULT_CHANNELS 2

static IMMDeviceEnumerator *mkfw_audio_enumerator;
static IMMDevice *mkfw_audio_device;
static IAudioClient *mkfw_audio_client;
static IAudioRenderClient *mkfw_audio_render_client;
static HANDLE mkfw_audio_event;
static mkfw_thread mkfw_audio_thread;
static uint32_t mkfw_audio_running;
static uint32_t mkfw_audio_alive;
static struct mkfw_audio_options mkfw_audio_opts;
static struct mkfw_audio_info    mkfw_audio_negotiated;
static mkfw_audio_callback_t mkfw_audio_user_cb;
static void *mkfw_audio_user_data;
static mkfw_audio_device_lost_callback_t mkfw_audio_lost_cb;
static void *mkfw_audio_lost_data;

// [=]===^=[ mkfw_audio_open_device_win32 ]=======================================================[=]
static int32_t mkfw_audio_open_device_win32(void) {
	WAVEFORMATEX wf;
	REFERENCE_TIME device_period;
	uint32_t buffer_frames;

	if(FAILED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(mkfw_audio_enumerator, eRender, eConsole, &mkfw_audio_device))) {
		return -1;
	}
	if(FAILED(IMMDevice_Activate(mkfw_audio_device, &mkfw_IID_IAudioClient, CLSCTX_ALL, 0, (void**)&mkfw_audio_client))) {
		IMMDevice_Release(mkfw_audio_device);
		mkfw_audio_device = 0;
		return -1;
	}

	uint32_t rate = mkfw_audio_opts.preferred_sample_rate ? mkfw_audio_opts.preferred_sample_rate : MKFW_AUDIO_DEFAULT_RATE;
	uint32_t channels = mkfw_audio_opts.preferred_channels ? mkfw_audio_opts.preferred_channels : MKFW_AUDIO_DEFAULT_CHANNELS;

	wf.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	wf.nChannels = (WORD)channels;
	wf.nSamplesPerSec = rate;
	wf.wBitsPerSample = 32;
	wf.nBlockAlign = (wf.nChannels * wf.wBitsPerSample) / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	wf.cbSize = 0;

	if(FAILED(IAudioClient_GetDevicePeriod(mkfw_audio_client, &device_period, 0))) {
		goto fail;
	}

	REFERENCE_TIME requested_duration = device_period;
	if(mkfw_audio_opts.preferred_buffer_frames) {
		// 100ns ticks: frames * 10^7 / rate
		requested_duration = ((REFERENCE_TIME)mkfw_audio_opts.preferred_buffer_frames * 10000000LL) / rate;
	}

	if(FAILED(IAudioClient_Initialize(mkfw_audio_client, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, requested_duration, 0, &wf, 0))) {
		goto fail;
	}
	if(FAILED(IAudioClient_SetEventHandle(mkfw_audio_client, mkfw_audio_event))) {
		goto fail;
	}
	if(FAILED(IAudioClient_GetService(mkfw_audio_client, &mkfw_IID_IAudioRenderClient, (void**)&mkfw_audio_render_client))) {
		goto fail;
	}
	if(FAILED(IAudioClient_GetBufferSize(mkfw_audio_client, &buffer_frames))) {
		goto fail;
	}
	if(FAILED(IAudioClient_Start(mkfw_audio_client))) {
		IAudioRenderClient_Release(mkfw_audio_render_client);
		mkfw_audio_render_client = 0;
		goto fail;
	}

	mkfw_audio_negotiated.sample_rate       = rate;
	mkfw_audio_negotiated.channels          = channels;
	mkfw_audio_negotiated.frames_per_buffer = buffer_frames;
	// device_period is in 100ns ticks; convert to frames at the negotiated rate.
	mkfw_audio_negotiated.period_frames     = (uint32_t)(((uint64_t)device_period * rate) / 10000000ULL);
	mkfw_audio_negotiated.latency_ns        = ((uint64_t)buffer_frames * 1000000000ULL) / rate;
	return 0;

fail:
	IAudioClient_Release(mkfw_audio_client);
	mkfw_audio_client = 0;
	IMMDevice_Release(mkfw_audio_device);
	mkfw_audio_device = 0;
	return -1;
}

// [=]===^=[ mkfw_audio_close_device_win32 ]======================================================[=]
static void mkfw_audio_close_device_win32(void) {
	if(mkfw_audio_client) {
		IAudioClient_Stop(mkfw_audio_client);
		IAudioClient_Reset(mkfw_audio_client);
	}
	if(mkfw_audio_render_client) {
		IAudioRenderClient_Release(mkfw_audio_render_client);
		mkfw_audio_render_client = 0;
	}
	if(mkfw_audio_client) {
		IAudioClient_Release(mkfw_audio_client);
		mkfw_audio_client = 0;
	}
	if(mkfw_audio_device) {
		IMMDevice_Release(mkfw_audio_device);
		mkfw_audio_device = 0;
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

// [=]===^=[ mkfw_audio_thread_proc ]=============================================================[=]
static DWORD WINAPI mkfw_audio_thread_proc(void *arg) {
	(void)arg;
	uint32_t buffer_size = 0;
	uint32_t padding;
	uint32_t available;
	uint8_t *data;
	HANDLE avrt_handle = 0;
	DWORD task_index = 0;

	if(mkfw_audio_opts.realtime_priority) {
		avrt_handle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &task_index);
		if(!avrt_handle) {
			mkfw_error("audio: MMCSS Pro Audio class not granted; running at normal priority");
		}
	}

	while(__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) {
		if(!mkfw_audio_client) {
			if(mkfw_audio_open_device_win32() < 0) {
				Sleep(100);
				continue;
			}
			buffer_size = mkfw_audio_negotiated.frames_per_buffer;
			__atomic_store_n(&mkfw_audio_alive, 1, __ATOMIC_RELEASE);
		}

		WaitForSingleObject(mkfw_audio_event, 500);
		if(!__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) {
			break;
		}

		if(FAILED(IAudioClient_GetCurrentPadding(mkfw_audio_client, &padding))) {
			mkfw_audio_close_device_win32();
			mkfw_audio_notify_lost();
			continue;
		}
		available = buffer_size - padding;

		while(available) {
			if(FAILED(IAudioRenderClient_GetBuffer(mkfw_audio_render_client, available, &data))) {
				mkfw_audio_close_device_win32();
				mkfw_audio_notify_lost();
				break;
			}
			mkfw_audio_dispatch((float *)data, available);
			IAudioRenderClient_ReleaseBuffer(mkfw_audio_render_client, available, 0);

			if(FAILED(IAudioClient_GetCurrentPadding(mkfw_audio_client, &padding))) {
				mkfw_audio_close_device_win32();
				mkfw_audio_notify_lost();
				break;
			}
			available = buffer_size - padding;
		}
	}

	if(avrt_handle) {
		AvRevertMmThreadCharacteristics(avrt_handle);
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

	if(FAILED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
		mkfw_error("audio: CoInitializeEx failed");
		return 0;
	}
	if(FAILED(CoCreateInstance(&mkfw_CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, &mkfw_IID_IMMDeviceEnumerator, (void**)&mkfw_audio_enumerator))) {
		CoUninitialize();
		mkfw_error("audio: MMDeviceEnumerator unavailable");
		return 0;
	}
	mkfw_audio_event = CreateEvent(0, 0, 0, 0);
	if(!mkfw_audio_event) {
		IMMDeviceEnumerator_Release(mkfw_audio_enumerator);
		mkfw_audio_enumerator = 0;
		CoUninitialize();
		mkfw_error("audio: CreateEvent failed");
		return 0;
	}
	__atomic_store_n(&mkfw_audio_running, 1, __ATOMIC_RELEASE);
	mkfw_audio_thread = mkfw_thread_create(mkfw_audio_thread_proc, 0);
	if(!mkfw_audio_thread) {
		__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
		CloseHandle(mkfw_audio_event);
		mkfw_audio_event = 0;
		IMMDeviceEnumerator_Release(mkfw_audio_enumerator);
		mkfw_audio_enumerator = 0;
		CoUninitialize();
		mkfw_error("audio: failed to spawn audio thread");
		return 0;
	}
	return 1;
}

// [=]===^=[ mkfw_audio_shutdown ]================================================================[=]
MKFW_API void mkfw_audio_shutdown(void) {
	__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
	if(mkfw_audio_thread) {
		SetEvent(mkfw_audio_event);
		mkfw_thread_join(mkfw_audio_thread);
		mkfw_audio_thread = 0;
	}
	mkfw_audio_close_device_win32();
	if(mkfw_audio_event) {
		CloseHandle(mkfw_audio_event);
		mkfw_audio_event = 0;
	}
	if(mkfw_audio_enumerator) {
		IMMDeviceEnumerator_Release(mkfw_audio_enumerator);
		mkfw_audio_enumerator = 0;
	}
	CoUninitialize();
	__atomic_store_n(&mkfw_audio_alive, 0, __ATOMIC_RELEASE);
}
