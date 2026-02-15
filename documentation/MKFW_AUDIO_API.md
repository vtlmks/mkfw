# MKFW Audio API Documentation

**Low-Latency Audio Subsystem for MKFW**

## Overview

The MKFW Audio API provides low-latency, real-time audio output for applications requiring precise audio timing. It uses platform-specific implementations with dedicated high-priority audio threads to minimize latency and prevent dropouts.

## Features

- Low-latency audio output (typically 10-20ms on modern systems)
- 48kHz sample rate, 16-bit stereo
- High-priority audio thread for glitch-free playback
- Simple callback-based interface
- Platform-optimized implementations (WASAPI/ALSA)
- Automatic recovery from audio device issues

## Platform Support

- Linux (ALSA, tries PipeWire first then falls back to default ALSA device)
- Windows (WASAPI event-driven)

---

## Enabling the Audio Subsystem

The audio subsystem is optional and must be enabled before including `mkfw.h`:

```c
#define MKFW_AUDIO
#include "mkfw.h"
```

This will include the platform-specific audio implementation:
- `mkfw_win32_audio.c` on Windows
- `mkfw_linux_audio.c` on Linux

---

## Audio Format

The audio subsystem uses a fixed format optimized for low latency:

- **Sample Rate:** 48,000 Hz
- **Channels:** 2 (stereo)
- **Bit Depth:** 16-bit signed integer
- **Frame Size:** 4 bytes (2 channels × 2 bytes)
- **Buffer Size:** Platform-dependent (256 frames on ALSA, 480 frames on WASAPI typical)

**Sample format:**
```c
int16_t audio_buffer[frames * 2];  // Interleaved stereo
// audio_buffer[0] = left channel, sample 0
// audio_buffer[1] = right channel, sample 0
// audio_buffer[2] = left channel, sample 1
// audio_buffer[3] = right channel, sample 1
// ...
```

---

## Core Concepts

### Callback-Based Audio

The audio system uses a **callback function** that you provide. This function is called automatically by the audio thread whenever the audio device needs more samples.

**Key principles:**
1. Your callback fills a buffer with audio samples
2. The audio thread calls your callback at regular intervals
3. You must fill the requested number of frames quickly
4. The callback runs on a high-priority thread - keep it fast!

### Audio Thread Priority

- Runs at realtime/high priority to prevent dropouts
- On Windows: Uses MMCSS "Pro Audio" scheduling class
- On Linux: Standard thread priority (relies on PipeWire/ALSA buffering)
- Your callback runs in this high-priority context - avoid blocking operations!

---

## Initialization and Shutdown

### `mkfw_audio_initialize`

```c
void mkfw_audio_initialize(void)
```

Initialize the audio subsystem and start playback.

**Notes:**
- Opens the default audio output device
- Creates a high-priority audio thread
- Starts audio playback immediately
- On Windows: Uses WASAPI in shared mode with event-driven callback
- On Linux: Tries "plug:pipewire" first, falls back to "default" ALSA device if PipeWire is not available
- Must set `mkfw_audio_callback` before calling, or audio will be silent

**Example:**
```c
mkfw_audio_callback = my_audio_callback;
mkfw_audio_initialize();
```

---

### `mkfw_audio_shutdown`

```c
void mkfw_audio_shutdown(void)
```

Stop playback and clean up audio resources.

**Notes:**
- Stops the audio thread gracefully
- On Windows: Fills remaining buffer with silence before stopping
- Closes audio device
- Frees allocated buffers
- Safe to call even if audio failed to initialize

**Example:**
```c
mkfw_audio_shutdown();
```

---

## Audio Callback

### `mkfw_audio_callback`

```c
void (*mkfw_audio_callback)(int16_t *audio_buffer, size_t frames);
```

Function pointer to your audio generation callback.

**Parameters:**
- `audio_buffer` - Buffer to fill with interleaved stereo samples
- `frames` - Number of frames to generate (NOT samples - multiply by 2 for sample count)

**Callback responsibilities:**
1. Fill `frames * 2` samples in the buffer (interleaved L/R)
2. Return quickly (this runs on realtime thread)
3. Generate audio without blocking or allocating memory

**Notes:**
- Buffer is pre-zeroed before your callback is called
- If callback is NULL, audio plays silence
- Runs on high-priority audio thread - keep processing minimal
- Typical frame counts: 256 frames on ALSA (5.3ms), 480 frames on WASAPI (10ms) at 48kHz

**Example:**
```c
void my_audio_callback(int16_t *audio_buffer, size_t frames) {
    for (size_t i = 0; i < frames; i++) {
        // Generate left and right samples
        int16_t left = generate_sample();
        int16_t right = generate_sample();

        // Interleaved stereo
        audio_buffer[i * 2 + 0] = left;
        audio_buffer[i * 2 + 1] = right;
    }
}

// Set callback
mkfw_audio_callback = my_audio_callback;
```

---

## Typical Usage Patterns

### Simple Tone Generation

```c
#define MKFW_AUDIO
#include "mkfw.h"

float phase = 0.0f;

void audio_callback(int16_t *buffer, size_t frames) {
    float frequency = 440.0f;  // A4
    float sample_rate = 48000.0f;
    float phase_increment = frequency / sample_rate;

    for (size_t i = 0; i < frames; i++) {
        // Generate sine wave
        int16_t sample = (int16_t)(sin(phase * 2.0f * M_PI) * 16000.0f);

        // Stereo - same on both channels
        buffer[i * 2 + 0] = sample;
        buffer[i * 2 + 1] = sample;

        // Advance phase
        phase += phase_increment;
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

int main(void) {
    mkfw_audio_callback = audio_callback;
    mkfw_audio_initialize();

    // Audio plays in background
    while (running) {
        // Do other work
    }

    mkfw_audio_shutdown();
    return 0;
}
```

---

### Music Player with External Library

```c
#define MKFW_AUDIO
#include "mkfw.h"
#include "micromod.h"  // Example: tracker music player

void audio_callback(int16_t *buffer, size_t frames) {
    // Use external library to generate audio
    micromod_get_audio(buffer, frames);
}

int main(void) {
    // Initialize music player
    micromod_initialise(mod_data, sample_rate);

    // Start audio
    mkfw_audio_callback = audio_callback;
    mkfw_audio_initialize();

    // Run application
    while (running) {
        // ...
    }

    mkfw_audio_shutdown();
    return 0;
}
```

---

### Mixer with Multiple Sources

```c
#define MKFW_AUDIO
#include "mkfw.h"

typedef struct {
    int16_t *samples;
    size_t length;
    size_t position;
    float volume;
    int playing;
} sound_source_t;

#define MAX_SOURCES 16
sound_source_t sources[MAX_SOURCES];

void audio_callback(int16_t *buffer, size_t frames) {
    // Buffer is pre-zeroed

    // Mix all active sources
    for (int s = 0; s < MAX_SOURCES; s++) {
        if (!sources[s].playing) continue;

        sound_source_t *src = &sources[s];

        for (size_t i = 0; i < frames && src->position < src->length; i++) {
            // Read stereo sample from source
            int16_t left = src->samples[src->position * 2 + 0];
            int16_t right = src->samples[src->position * 2 + 1];

            // Apply volume and mix (with clamping)
            int32_t mixed_left = buffer[i * 2 + 0] + (int32_t)(left * src->volume);
            int32_t mixed_right = buffer[i * 2 + 1] + (int32_t)(right * src->volume);

            // Clamp to int16 range
            if (mixed_left > 32767) mixed_left = 32767;
            if (mixed_left < -32768) mixed_left = -32768;
            if (mixed_right > 32767) mixed_right = 32767;
            if (mixed_right < -32768) mixed_right = -32768;

            buffer[i * 2 + 0] = (int16_t)mixed_left;
            buffer[i * 2 + 1] = (int16_t)mixed_right;

            src->position++;
        }

        // Stop if finished
        if (src->position >= src->length) {
            src->playing = 0;
        }
    }
}
```

---

### Integration with Platform Template (from platform.c)

```c
#define MKFW_TIMER
#define MKFW_AUDIO
#include "mkfw.h"

// Your audio callback
void my_audio_callback(int16_t *buffer, size_t frames) {
    // Generate audio
}

int main(int argc, char **argv) {
    // Initialize audio first
    mkfw_audio_callback = my_audio_callback;
    mkfw_audio_initialize();

    // Initialize timer
    mkfw_timer_init();

    // Create window and run application
    // ...

    // Cleanup in reverse order
    mkfw_timer_shutdown();
    mkfw_audio_shutdown();

    return 0;
}
```

---

## Best Practices

### DO:

✓ Keep callback processing fast and deterministic
✓ Pre-allocate all resources before audio starts
✓ Use lock-free data structures for communication with audio thread
✓ Handle the exact number of frames requested
✓ Fill buffer completely (or leave zeros for silence)
✓ Use atomic operations or lock-free queues for inter-thread communication

### DON'T:

✗ Allocate memory in the callback
✗ Use mutexes or blocking operations
✗ Perform file I/O
✗ Call system functions that might block
✗ Access graphics resources
✗ Assume a specific callback frequency

---

## Platform-Specific Implementation Details

### Windows Implementation

**Files:** `mkfw_win32_audio.c`

**Key features:**
- Uses WASAPI (Windows Audio Session API) in shared mode
- Event-driven callback (no polling)
- MMCSS "Pro Audio" thread priority
- Automatic format conversion (via `AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM`)
- Sample rate conversion (via `AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY`)
- Graceful shutdown with buffer flushing

**Dependencies:**
- audioclient.h, mmdeviceapi.h, avrt.h, timeapi.h
- Links: `-lole32 -lavrt -lwinmm`

**Buffer size:**
- Determined by device default period in shared mode (typically 10ms = 480 frames at 48kHz)
- System manages buffering automatically

---

### Linux Implementation

**Files:** `mkfw_linux_audio.c`

**Key features:**
- Uses ALSA (Advanced Linux Sound Architecture)
- Tries "plug:pipewire" first, falls back to "default" device (compatible with both PipeWire and ALSA-only systems)
- Double buffering for low latency (2 × 256 frames = ~10.7ms latency)
- Automatic recovery from buffer underruns
- Blocking I/O with `snd_pcm_wait()`

**Dependencies:**
- alsa/asoundlib.h, pthread.h
- Links: `-lasound -lpthread`

**Buffer configuration:**
- Period size: 256 frames (5.3ms at 48kHz)
- Buffer size: 512 frames (10.7ms at 48kHz)
- 2 periods for low-latency playback

---

## Constants and Configuration

### Audio Format Constants

```c
#define MKFW_SAMPLE_RATE     48000
#define MKFW_NUM_CHANNELS    2
#define MKFW_BITS_PER_SAMPLE 16
#define MKFW_FRAME_SIZE      4  // 2 channels × 2 bytes
```

### Linux-Specific Constants

```c
#define MKFW_PREFERRED_FRAMES_PER_BUFFER 256
#define MKFW_BUFFER_SIZE     1024  // 256 × 4 bytes
#define MKFW_BUFFER_COUNT    2     // Double buffering
```

---

## Latency Characteristics

### Expected Latency

| Platform | Typical Latency | Notes |
|----------|----------------|-------|
| Windows (WASAPI) | 10-20ms | Depends on device period |
| Linux (ALSA/PipeWire) | 10-15ms | 2× buffering, low latency |

### Reducing Latency

**Windows:**
- Latency is determined by audio device and system settings
- Exclusive mode could reduce latency (not currently supported)

**Linux:**
- Already optimized for low latency (10-15ms)
- Can reduce `MKFW_PREFERRED_FRAMES_PER_BUFFER` to 128 for ultra-low latency (~5ms)
- May cause underruns on slower systems or under high load
- Use real-time kernel for even better scheduling

---

## Error Handling

The audio system handles errors gracefully:

- **Device open failure:** Returns silently, no audio plays
- **Buffer underrun (Linux):** Automatically recovered via `snd_pcm_recover()`
- **Device disconnect:** Thread exits, may need restart
- **Initialization failure:** `mkfw_audio_shutdown()` is still safe to call

**Checking for success:**
```c
// The API doesn't return error codes
// If audio initializes, it plays
// If it fails, it silently disables audio

mkfw_audio_callback = my_callback;
mkfw_audio_initialize();

// Audio is either playing or silently disabled
```

---

## Thread Safety

### Safe Operations

- Setting `mkfw_audio_callback` before `mkfw_audio_initialize()`
- Calling `mkfw_audio_shutdown()` from any thread
- Reading audio state from callback (one-way communication)

### Unsafe Operations

- Modifying `mkfw_audio_callback` while audio is running
- Accessing audio thread internals from outside

### Inter-thread Communication

For communicating with the audio thread, use:

1. **Atomic variables** - For simple flags and counters
2. **Lock-free ring buffers** - For audio data streaming
3. **Lock-free queues** - For event passing

**Example with atomics:**
```c
#include <stdatomic.h>

atomic_int volume = ATOMIC_VAR_INIT(100);

void audio_callback(int16_t *buffer, size_t frames) {
    int v = atomic_load(&volume);

    for (size_t i = 0; i < frames * 2; i++) {
        buffer[i] = (buffer[i] * v) / 100;
    }
}

// From main thread:
atomic_store(&volume, 50);  // Safe!
```

---

## Troubleshooting

### No audio output

1. Check that callback is set before `mkfw_audio_initialize()`
2. Verify callback is generating non-zero samples
3. Check system audio settings and volume
4. Ensure audio device is not in use by another application

### Audio dropouts/glitches

1. Callback is taking too long - profile and optimize
2. System is too loaded - close background applications
3. Memory allocation in callback - pre-allocate everything
4. Blocking operations in callback - use lock-free data structures

### Audio delay/latency

1. Check platform latency characteristics (see table above)
2. On Linux: Reduce buffer count (may cause instability)
3. Consider platform limitations

### Crackling/distortion

1. Samples exceeding ±32767 range - clamp your output
2. Buffer is not fully filled - generate all requested frames
3. Mixing overflow - use 32-bit intermediate values and clamp

---

## Example: Complete Audio Player

```c
#define MKFW_AUDIO
#include "mkfw.h"
#include <math.h>

typedef struct {
    float frequency;
    float phase;
    float volume;
} oscillator_t;

oscillator_t osc = { .frequency = 440.0f, .phase = 0.0f, .volume = 0.3f };

void audio_callback(int16_t *buffer, size_t frames) {
    const float sample_rate = 48000.0f;
    const float phase_inc = osc.frequency / sample_rate;

    for (size_t i = 0; i < frames; i++) {
        // Generate sine wave
        float sample_f = sinf(osc.phase * 2.0f * M_PI) * 32000.0f * osc.volume;
        int16_t sample = (int16_t)sample_f;

        // Stereo
        buffer[i * 2 + 0] = sample;
        buffer[i * 2 + 1] = sample;

        // Advance phase
        osc.phase += phase_inc;
        if (osc.phase >= 1.0f) {
            osc.phase -= 1.0f;
        }
    }
}

int main(void) {
    printf("Starting audio test (440Hz sine wave)...\n");

    mkfw_audio_callback = audio_callback;
    mkfw_audio_initialize();

    // Play for 5 seconds
    for (int i = 0; i < 5; i++) {
        printf("Playing... %d\n", i + 1);
        sleep(1);
    }

    mkfw_audio_shutdown();
    printf("Audio stopped.\n");

    return 0;
}
```

---

## See Also

- `MKFW_API.md` - Main MKFW windowing API
- `MKFW_TIMER_API.md` - Timer subsystem documentation
- External audio libraries compatible with this API:
  - MicroMod - MOD/XM/S3M tracker playback
  - FutureComposer - Amiga FC music playback
  - Custom synthesizers and samplers

---

## License

MIT License - See source files for full license text.
