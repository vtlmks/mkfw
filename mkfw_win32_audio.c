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
#ifdef __cplusplus
#define MKFW_GUID_DEF(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	static const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#else
#define MKFW_GUID_DEF(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	static const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#endif

MKFW_GUID_DEF(mkfw_CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);
MKFW_GUID_DEF(mkfw_IID_IMMDeviceEnumerator,   0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
MKFW_GUID_DEF(mkfw_IID_IAudioClient,          0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
MKFW_GUID_DEF(mkfw_IID_IAudioRenderClient,    0xf294acfc, 0x3146, 0x4483, 0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2);

#undef MKFW_GUID_DEF

#define MKFW_SAMPLE_RATE     48000
#define MKFW_NUM_CHANNELS    2
#define MKFW_BITS_PER_SAMPLE 16
#define MKFW_FRAME_SIZE      (MKFW_NUM_CHANNELS * (MKFW_BITS_PER_SAMPLE / 8))

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

static IMMDeviceEnumerator *mkfw_enumerator;
static IMMDevice *mkfw_device_out;
static IAudioClient *mkfw_audio_client_out;
static IAudioRenderClient *mkfw_render_client;
static HANDLE mkfw_audio_event;
static mkfw_thread mkfw_audio_thread;
static int mkfw_audio_running;

// [=]===^=[ mkfw_audio_open_device_win32 ]=======================================================[=]
static int32_t mkfw_audio_open_device_win32(void) {
	WAVEFORMATEX wf;
	REFERENCE_TIME dur_out;

	if(FAILED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(mkfw_enumerator, eRender, eConsole, &mkfw_device_out))) {
		return -1;
	}
	if(FAILED(IMMDevice_Activate(mkfw_device_out, &mkfw_IID_IAudioClient, CLSCTX_ALL, 0, (void**)&mkfw_audio_client_out))) {
		IMMDevice_Release(mkfw_device_out);
		mkfw_device_out = 0;
		return -1;
	}

	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = MKFW_NUM_CHANNELS;
	wf.nSamplesPerSec = MKFW_SAMPLE_RATE;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = (wf.nChannels * wf.wBitsPerSample) / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	wf.cbSize = 0;

	if(FAILED(IAudioClient_GetDevicePeriod(mkfw_audio_client_out, &dur_out, 0))) {
		goto fail;
	}
	if(FAILED(IAudioClient_Initialize(mkfw_audio_client_out, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, dur_out, 0, &wf, 0))) {
		goto fail;
	}
	if(FAILED(IAudioClient_SetEventHandle(mkfw_audio_client_out, mkfw_audio_event))) {
		goto fail;
	}
	if(FAILED(IAudioClient_GetService(mkfw_audio_client_out, &mkfw_IID_IAudioRenderClient, (void**)&mkfw_render_client))) {
		goto fail;
	}
	if(FAILED(IAudioClient_Start(mkfw_audio_client_out))) {
		IAudioRenderClient_Release(mkfw_render_client);
		mkfw_render_client = 0;
		goto fail;
	}
	return 0;

fail:
	IAudioClient_Release(mkfw_audio_client_out);
	mkfw_audio_client_out = 0;
	IMMDevice_Release(mkfw_device_out);
	mkfw_device_out = 0;
	return -1;
}

// [=]===^=[ mkfw_audio_close_device_win32 ]======================================================[=]
static void mkfw_audio_close_device_win32(void) {
	if(mkfw_audio_client_out) {
		IAudioClient_Stop(mkfw_audio_client_out);
		IAudioClient_Reset(mkfw_audio_client_out);
	}
	if(mkfw_render_client) {
		IAudioRenderClient_Release(mkfw_render_client);
		mkfw_render_client = 0;
	}
	if(mkfw_audio_client_out) {
		IAudioClient_Release(mkfw_audio_client_out);
		mkfw_audio_client_out = 0;
	}
	if(mkfw_device_out) {
		IMMDevice_Release(mkfw_device_out);
		mkfw_device_out = 0;
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

	avrt_handle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &task_index);

	while(__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) {
		if(!mkfw_audio_client_out) {
			if(mkfw_audio_open_device_win32() < 0) {
				Sleep(100);
				continue;
			}
			IAudioClient_GetBufferSize(mkfw_audio_client_out, &buffer_size);
		}

		WaitForSingleObject(mkfw_audio_event, 500);
		if(!__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) {
			break;
		}

		if(FAILED(IAudioClient_GetCurrentPadding(mkfw_audio_client_out, &padding))) {
			mkfw_audio_close_device_win32();
			continue;
		}
		available = buffer_size - padding;

		while(available) {
			if(FAILED(IAudioRenderClient_GetBuffer(mkfw_render_client, available, &data))) {
				mkfw_audio_close_device_win32();
				break;
			}
			mkfw_audio_callback_thread((int16_t*)data, available);
			IAudioRenderClient_ReleaseBuffer(mkfw_render_client, available, 0);

			if(FAILED(IAudioClient_GetCurrentPadding(mkfw_audio_client_out, &padding))) {
				mkfw_audio_close_device_win32();
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

// [=]===^=[ mkfw_audio_initialize ]==============================================================[=]
static void mkfw_audio_initialize(void) {
	if(FAILED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
		return;
	}
	if(FAILED(CoCreateInstance(&mkfw_CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, &mkfw_IID_IMMDeviceEnumerator, (void**)&mkfw_enumerator))) {
		CoUninitialize();
		return;
	}
	mkfw_audio_event = CreateEvent(0, 0, 0, 0);
	if(!mkfw_audio_event) {
		IMMDeviceEnumerator_Release(mkfw_enumerator);
		mkfw_enumerator = 0;
		CoUninitialize();
		return;
	}
	__atomic_store_n(&mkfw_audio_running, 1, __ATOMIC_RELEASE);
	mkfw_audio_thread = mkfw_thread_create(mkfw_audio_thread_proc, 0);
	if(!mkfw_audio_thread) {
		__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
		mkfw_audio_close_device_win32();
		CloseHandle(mkfw_audio_event);
		mkfw_audio_event = 0;
		IMMDeviceEnumerator_Release(mkfw_enumerator);
		mkfw_enumerator = 0;
		CoUninitialize();
	}
}

// [=]===^=[ mkfw_audio_shutdown ]================================================================[=]
static void mkfw_audio_shutdown(void) {
	__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
	if(mkfw_audio_thread) {
		SetEvent(mkfw_audio_event);
		mkfw_thread_join(mkfw_audio_thread);
	}
	mkfw_audio_close_device_win32();
	if(mkfw_audio_event) { CloseHandle(mkfw_audio_event); }
	if(mkfw_enumerator) { IMMDeviceEnumerator_Release(mkfw_enumerator); }
	CoUninitialize();
}
