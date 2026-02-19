// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#include <timeapi.h>

#define MKFW_SAMPLE_RATE     48000
#define MKFW_NUM_CHANNELS    2
#define MKFW_BITS_PER_SAMPLE 16
#define MKFW_FRAME_SIZE      (MKFW_NUM_CHANNELS * (MKFW_BITS_PER_SAMPLE / 8))

void (*mkfw_audio_callback)(int16_t *audio_buffer, size_t frames);

static void mkfw_set_audio_callback(void (*cb)(int16_t *, size_t)) {
	__atomic_store(&mkfw_audio_callback, &cb, __ATOMIC_RELEASE);
}

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

static DWORD WINAPI mkfw_audio_thread_proc(void *arg) {
	uint32_t buffer_size;
	uint32_t padding;
	uint32_t available;
	uint8_t *data;
	HANDLE avrt_handle = 0;
	DWORD task_index = 0;

	avrt_handle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &task_index);
	IAudioClient_GetBufferSize(mkfw_audio_client_out, &buffer_size);

	for(;;) {
		DWORD r = WaitForSingleObject(mkfw_audio_event, INFINITE);
		if(!__atomic_load_n(&mkfw_audio_running, __ATOMIC_ACQUIRE)) {
			break;
		}

		IAudioClient_GetCurrentPadding(mkfw_audio_client_out, &padding);
		available = buffer_size - padding;

		while(available) {
			if(SUCCEEDED(IAudioRenderClient_GetBuffer(mkfw_render_client, available, &data))) {
				mkfw_audio_callback_thread((int16_t*)data, available);
				IAudioRenderClient_ReleaseBuffer(mkfw_render_client, available, 0);
			} else {
				__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);
				break;
			}

			IAudioClient_GetCurrentPadding(mkfw_audio_client_out, &padding);
			available = buffer_size - padding;
		}
	}

	if(avrt_handle) {
		AvRevertMmThreadCharacteristics(avrt_handle);
	}
	return 0;
}

static void mkfw_audio_initialize(void) {
	WAVEFORMATEX wf;
	REFERENCE_TIME dur_out;
	int com_initialized = 0;

	if(SUCCEEDED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
		com_initialized = 1;
	}

	if(com_initialized && SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&mkfw_enumerator))) {
		if(SUCCEEDED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(mkfw_enumerator, eRender, eConsole, &mkfw_device_out))) {
			if(SUCCEEDED(IMMDevice_Activate(mkfw_device_out, &IID_IAudioClient, CLSCTX_ALL, 0, (void**)&mkfw_audio_client_out))) {
				wf.wFormatTag = WAVE_FORMAT_PCM;
				wf.nChannels = MKFW_NUM_CHANNELS;
				wf.nSamplesPerSec = MKFW_SAMPLE_RATE;
				wf.wBitsPerSample = 16;
				wf.nBlockAlign = (wf.nChannels * wf.wBitsPerSample) / 8;
				wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
				wf.cbSize = 0;
				if(SUCCEEDED(IAudioClient_GetDevicePeriod(mkfw_audio_client_out, &dur_out, 0))) {
					if(SUCCEEDED(IAudioClient_Initialize( mkfw_audio_client_out, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK|AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM|AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, dur_out, 0, &wf, 0))) {
						mkfw_audio_event = CreateEvent(0, 0, 0, 0);
						if(mkfw_audio_event) {
							if(SUCCEEDED(IAudioClient_SetEventHandle(mkfw_audio_client_out, mkfw_audio_event))) {
								if(SUCCEEDED(IAudioClient_GetService(mkfw_audio_client_out, &IID_IAudioRenderClient, (void**)&mkfw_render_client))) {
									if(SUCCEEDED(IAudioClient_Start(mkfw_audio_client_out))) {
										__atomic_store_n(&mkfw_audio_running, 1, __ATOMIC_RELEASE);
										mkfw_audio_thread = mkfw_thread_create(mkfw_audio_thread_proc, 0);
										if(mkfw_audio_thread) {
											return;

										} else { mkfw_error("mkfw_thread_create failed"); }
										IAudioClient_Stop(mkfw_audio_client_out);

									} else { mkfw_error("IAudioClient_Start failed"); }
									IAudioRenderClient_Release(mkfw_render_client);

								} else { mkfw_error("IAudioClient_GetService(IAudioRenderClient) failed"); }

							} else { mkfw_error("IAudioClient_SetEventHandle failed"); }
							CloseHandle(mkfw_audio_event);

						} else { mkfw_error("CreateEvent failed"); }

					} else { mkfw_error("IAudioClient_Initialize failed"); }

				} else { mkfw_error("IAudioClient_GetDevicePeriod failed"); }
				IAudioClient_Release(mkfw_audio_client_out);

			} else { mkfw_error("IMMDevice_Activate failed"); }
			IMMDevice_Release(mkfw_device_out);

		} else { mkfw_error("IMMDeviceEnumerator_GetDefaultAudioEndpoint failed"); }
		IMMDeviceEnumerator_Release(mkfw_enumerator);

	} else { mkfw_error("CoCreateInstance(CLSID_MMDeviceEnumerator) failed"); }

	mkfw_enumerator = 0;
	mkfw_device_out = 0;
	mkfw_audio_client_out = 0;
	mkfw_render_client = 0;
	mkfw_audio_event = 0;

	if(com_initialized) {
		CoUninitialize();
	}
}

static void mkfw_audio_shutdown(void) {
	uint32_t frames;
	uint32_t padding;

	__atomic_store_n(&mkfw_audio_running, 0, __ATOMIC_RELEASE);

	if(mkfw_audio_thread) {
		SetEvent(mkfw_audio_event);
		mkfw_thread_join(mkfw_audio_thread);
	}

	if(mkfw_audio_client_out && mkfw_render_client) {
		if(SUCCEEDED(IAudioClient_GetBufferSize(mkfw_audio_client_out, &frames))) {
			for(;;) {
				if(FAILED(IAudioClient_GetCurrentPadding(mkfw_audio_client_out, &padding))) { break; }
				if(padding >= frames) { break; }

				uint32_t to_write = frames - padding;
				uint8_t *p;
				if(FAILED(IAudioRenderClient_GetBuffer(mkfw_render_client, to_write, &p))) { break; }
				IAudioRenderClient_ReleaseBuffer(mkfw_render_client, to_write, AUDCLNT_BUFFERFLAGS_SILENT);
			}
		}
	}

	if(mkfw_audio_client_out) {
		IAudioClient_Stop(mkfw_audio_client_out);
		IAudioClient_Reset(mkfw_audio_client_out);
		IAudioClient_Release(mkfw_audio_client_out);
	}

	if(mkfw_render_client) { IAudioRenderClient_Release(mkfw_render_client); }
	if(mkfw_audio_event) { CloseHandle(mkfw_audio_event); }
	if(mkfw_device_out) { IMMDevice_Release(mkfw_device_out); }
	if(mkfw_enumerator) { IMMDeviceEnumerator_Release(mkfw_enumerator); }

	CoUninitialize();
}

