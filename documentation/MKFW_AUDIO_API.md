# MKFW Audio API

Low-latency, callback-based audio output for mkfw applications.

## Overview

mkfw_audio is an optional companion subsystem for mkfw. It opens
the default system output, negotiates a sample rate and channel
count, and calls your fill function from a dedicated audio thread.
mkfw does not resample, mix, decode, or otherwise process the
samples. The OS audio layer (WASAPI shared mode / PipeWire /
PulseAudio / ALSA `plug`) converts the negotiated stream to
whatever the physical device requires.

- **Format**: 32-bit IEEE float, interleaved, range `[-1.0, 1.0]`.
- **Default request**: 48 kHz, stereo, ~10 ms latency. Configurable.
- **Transport**: WASAPI shared mode on Windows, ALSA `default`
  device on Linux (which is usually PipeWire's pipewire-alsa shim
  or PulseAudio's alsa plugin).

## Enabling

mkfw_audio ships as a header next to `mkfw.h`. Include both:

```c
#include "mkfw.h"
#include "mkfw_audio.h"
```

Link:
- Linux: `-lpthread -ldl -lm`
- Windows: `ole32.lib avrt.lib uuid.lib` plus the base mkfw libs.

ALSA on Linux is loaded at runtime via `dlopen` from
`libasound.so.2`; do not link `-lasound`.

---

## Types

### `mkfw_audio_callback_t`

```c
typedef void (*mkfw_audio_callback_t)(void *userdata, float *buffer, uint32_t frames);
```

Called from the audio thread. Fill `buffer` with `frames *
channels` interleaved float samples. Channel count is fixed for
the device's lifetime and retrievable via `mkfw_audio_info()`.

The callback should be lock-free, allocation-free, and bounded in
runtime. Avoid GL, X11 / Win32 windowing, file I/O, or anything
that can block.

### `mkfw_audio_device_lost_callback_t`

```c
typedef void (*mkfw_audio_device_lost_callback_t)(void *userdata);
```

Fired once each time the audio device transitions from playing to
lost (USB unplug, driver crash, user changed default device).
mkfw will continue retrying the default endpoint in the
background; this hook only exists so the application can react.

### `struct mkfw_audio_options`

```c
struct mkfw_audio_options {
    uint32_t version;                 // 0 = current
    uint32_t preferred_sample_rate;   // 0 = 48000
    uint32_t preferred_channels;      // 0 = 2 (stereo)
    uint32_t preferred_buffer_frames; // 0 = backend default
    uint32_t realtime_priority;       // 0 = normal, non-zero = try RT scheduling
};
```

Hints, not guarantees. Whatever the OS layer negotiates is what
`mkfw_audio_info` reports. Pass `0` for every field to use
defaults.

`realtime_priority` is opt-in: when set, mkfw asks for
`SCHED_FIFO` priority 5 on Linux (typically requires `CAP_SYS_NICE`
or an `rtkit-daemon` policy) and the `Pro Audio` MMCSS class on
Windows. On either platform mkfw reports the failure through the
error callback / `mkfw_get_last_error()` and continues at normal
priority.

### `struct mkfw_audio_info`

```c
struct mkfw_audio_info {
    uint32_t sample_rate;        // negotiated, in Hz
    uint32_t channels;           // negotiated
    uint32_t frames_per_buffer;  // device buffer in frames
    uint32_t period_frames;      // callback granularity in frames
    uint64_t latency_ns;         // best estimate, total
};
```

---

## Functions

### `mkfw_audio_init`

```c
uint32_t mkfw_audio_init(struct mkfw_audio_options *opts);
```

Open the default output, negotiate format, spawn the audio
thread. Pass `0` for `opts` to use defaults.

Returns non-zero on success, `0` on failure. On failure,
`mkfw_get_last_error()` returns a description and the error
callback (if installed) has been fired.

### `mkfw_audio_shutdown`

```c
void mkfw_audio_shutdown(void);
```

Stop the audio thread, release the device, free internal
buffers.

### `mkfw_audio_set_callback`

```c
void mkfw_audio_set_callback(mkfw_audio_callback_t cb, void *userdata);
```

Install (or replace) the fill callback. Safe to call at any time
including while audio is running; the swap is atomic and the
audio thread sees a consistent (cb, userdata) pair. Pass `0` for
both to silence output.

### `mkfw_audio_set_device_lost_callback`

```c
void mkfw_audio_set_device_lost_callback(mkfw_audio_device_lost_callback_t cb, void *userdata);
```

Install (or replace) the device-lost hook. Same swap semantics
as `mkfw_audio_set_callback`.

### `mkfw_audio_info`

```c
void mkfw_audio_info(struct mkfw_audio_info *out);
```

Copy the negotiated device info into `*out`. Valid only after a
successful `mkfw_audio_init`. Read once at startup and cache;
the values do not change for the device's lifetime.

---

## Example

```c
#include <math.h>
#include "mkfw.h"
#include "mkfw_audio.h"

struct synth_state {
    double phase;
    double phase_step;
    uint32_t channels;
};

static void on_audio(void *userdata, float *buffer, uint32_t frames) {
    struct synth_state *s = (struct synth_state *)userdata;
    for(uint32_t f = 0; f < frames; ++f) {
        float sample = (float)(sin(s->phase) * 0.2);
        s->phase += s->phase_step;
        for(uint32_t c = 0; c < s->channels; ++c) {
            buffer[f * s->channels + c] = sample;
        }
    }
}

int main(void) {
    struct mkfw_audio_options opts = {0};
    if(!mkfw_audio_init(&opts)) {
        return 1;
    }

    struct mkfw_audio_info info;
    mkfw_audio_info(&info);

    struct synth_state s = {0};
    s.phase_step = 2.0 * 3.141592653589793 * 440.0 / info.sample_rate;
    s.channels = info.channels;

    mkfw_audio_set_callback(on_audio, &s);

    // ... run application ...

    mkfw_audio_set_callback(0, 0);
    mkfw_audio_shutdown();
    return 0;
}
```

---

## Threading

- The audio callback runs on the audio thread. Do not touch GL,
  X11/Win32 windowing, or any non-realtime resource from inside
  it. Communicate with the main thread via atomics or a
  lock-free queue.
- `mkfw_audio_set_callback`, `mkfw_audio_set_device_lost_callback`,
  and `mkfw_audio_info` are safe to call from any thread.
- `mkfw_audio_init` / `mkfw_audio_shutdown` should be called from
  the same thread (typically main).

## Memory ownership

mkfw_audio never returns library-owned memory and never asks the
application for an allocation. The callback `buffer` is owned by
mkfw and valid only for the duration of the call.
