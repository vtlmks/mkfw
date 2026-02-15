# MKFW Timer API Documentation

**High-Precision Timer Subsystem for MKFW**

## Overview

The MKFW Timer API provides high-precision, low-jitter timing for applications requiring accurate frame pacing. It uses platform-specific implementations with sleep+spin strategies to achieve sub-millisecond accuracy while minimizing CPU usage.

## Features

- High-precision timing with nanosecond resolution
- Hybrid sleep+spin approach for low jitter
- Platform-optimized implementations (Win32/Linux)
- Realtime priority support for minimal latency
- Configurable spin threshold
- Multiple independent timer instances

## Platform Support

- Linux (using futex, CLOCK_MONOTONIC_RAW, and nanosleep)
- Windows (using QueryPerformanceCounter and NtDelayExecution)

---

## Enabling the Timer Subsystem

The timer subsystem is optional and must be enabled before including `mkfw.h`:

```c
#define MKFW_TIMER
#include "mkfw.h"
```

This will include the platform-specific timer implementation:
- `mkfw_win32_timer.c` on Windows
- `mkfw_linux_timer.c` on Linux

---

## Core Concepts

The timer subsystem uses a **sleep+spin strategy** for accurate timing:

1. **Sleep phase** - For large time intervals, uses platform sleep functions to yield CPU
2. **Spin phase** - For the final microseconds before deadline, busy-waits for precision
3. **Deadline tracking** - Maintains absolute deadlines to prevent drift over time

This approach balances CPU efficiency (sleep) with timing accuracy (spin).

### Spin Thresholds

- **Windows:** 1000µs (1ms) - Higher threshold due to timer resolution limitations
- **Linux:** 500µs - Lower threshold thanks to more accurate nanosleep

---

## Initialization

### `mkfw_timer_init`

```c
void mkfw_timer_init(void)
```

Initialize the timer subsystem. Must be called before creating any timers.

**Notes:**
- On Windows: Sets timer resolution to 1ms via `timeBeginPeriod()` and requests 0.5ms via `NtSetTimerResolution()`
- On Windows: Caches QueryPerformanceCounter frequency for efficient time queries
- On Windows: Loads `NtDelayExecution` from ntdll.dll for high-precision sleep
- On Linux: No-op (stub function for API compatibility)
- Must be paired with `mkfw_timer_shutdown()` before program exit

**Example:**
```c
mkfw_timer_init();
```

---

### `mkfw_timer_shutdown`

```c
void mkfw_timer_shutdown(void)
```

Clean up timer subsystem resources.

**Notes:**
- On Windows: Restores system timer resolution via `timeEndPeriod()`
- On Linux: No-op (stub function for API compatibility)
- Call after destroying all timer instances

**Example:**
```c
mkfw_timer_shutdown();
```

---

## Timer Handle Management

### `mkfw_timer_new`

```c
struct mkfw_timer_handle *mkfw_timer_new(uint64_t interval_ns)
```

Create a new timer instance with the specified interval.

**Parameters:**
- `interval_ns` - Timer interval in nanoseconds

**Returns:**
- Pointer to timer handle

**Notes:**
- Creates a dedicated high-priority background thread
- Thread runs at realtime priority for minimal jitter
- On Windows: Uses MMCSS "Pro Audio" class or falls back to REALTIME_PRIORITY_CLASS
- On Windows: Pins thread to CPU core 0 for consistency
- On Linux: Uses futex for efficient wait/wake
- Maintains absolute deadlines to prevent cumulative drift
- Each timer is completely independent

**Example:**
```c
// 60 FPS timer (16.666... ms)
#define FRAME_TIME_NS 16666666
struct mkfw_timer_handle *timer = mkfw_timer_new(FRAME_TIME_NS);
```

**Common intervals:**
```c
// 60 FPS
#define FPS_60  16666666ULL

// 50 FPS (PAL video timing)
#define FPS_50  20000000ULL

// 30 FPS
#define FPS_30  33333333ULL

// 120 FPS
#define FPS_120 8333333ULL

struct mkfw_timer_handle *timer = mkfw_timer_new(FPS_60);
```

---

### `mkfw_timer_wait`

```c
uint32_t mkfw_timer_wait(struct mkfw_timer_handle *t)
```

Wait for the next timer tick.

**Parameters:**
- `t` - Timer handle

**Returns:**
- Always returns 1

**Notes:**
- Blocks until the next deadline is reached
- Uses sleep+spin strategy for accurate timing
- Automatically advances deadline for next tick
- Safe to call from any thread (typically the render thread)
- Will never drift - uses absolute deadlines internally

**Example:**
```c
while (running) {
    // Do frame work
    render_scene();
    swap_buffers();

    // Wait for next frame
    mkfw_timer_wait(timer);
}
```

---

### `mkfw_timer_destroy`

```c
void mkfw_timer_destroy(struct mkfw_timer_handle *t)
```

Destroy a timer instance and clean up resources.

**Parameters:**
- `t` - Timer handle

**Notes:**
- Stops the background timer thread
- Waits for thread to exit cleanly
- Frees all associated resources
- On Windows: Reverts MMCSS thread priority if it was set
- Safe to call even if timer is currently waiting

**Example:**
```c
mkfw_timer_destroy(timer);
```

---

## Typical Usage Pattern

### Fixed Frame Rate Application

```c
#define MKFW_TIMER
#include "mkfw.h"

#define FRAME_TIME_NS 16666666  // 60 FPS

int main(void) {
    // Initialize timer subsystem
    mkfw_timer_init();

    // Create 60 FPS timer
    struct mkfw_timer_handle *timer = mkfw_timer_new(FRAME_TIME_NS);

    // Main loop
    while (running) {
        // Update game state
        update_game();

        // Render frame
        render_scene();
        swap_buffers();

        // Wait for next frame (accurate timing)
        mkfw_timer_wait(timer);
    }

    // Cleanup
    mkfw_timer_destroy(timer);
    mkfw_timer_shutdown();

    return 0;
}
```

---

### Multiple Independent Timers

```c
// Different update rates for different systems
struct mkfw_timer_handle *render_timer = mkfw_timer_new(16666666);  // 60 FPS
struct mkfw_timer_handle *physics_timer = mkfw_timer_new(10000000); // 100 Hz
struct mkfw_timer_handle *network_timer = mkfw_timer_new(50000000); // 20 Hz

// Each timer runs independently with its own thread
```

---

### Render Thread Pattern (from platform.c)

```c
#define MKFW_TIMER
#include "mkfw.h"

void *render_thread_func(void *arg) {
    struct mkfw_timer_handle *timer = mkfw_timer_new(FRAME_TIME_NS);

    while (state.running) {
        // Handle input
        if (mkfw_is_key_pressed(window, MKS_KEY_ESCAPE)) {
            state.running = 0;
        }

        // Update state
        update_keyboard_state(window);
        update_mouse_state(window);

        // Render
        render_frame();
        mkfw_swap_buffers(window);

        // Precise timing
        mkfw_timer_wait(timer);
    }

    mkfw_timer_destroy(timer);
    return 0;
}

int main(void) {
    mkfw_timer_init();

    // Create window, set up context
    // ...

    // Detach context from main thread
    mkfw_detach_context(window);

    // Start render thread
    pthread_create(&thread, NULL, render_thread_func, NULL);

    // Main thread handles events
    while (state.running && !mkfw_should_close(window)) {
        mkfw_pump_messages(window);
        mkfw_sleep(5000000);  // 5ms
    }

    pthread_join(thread, NULL);
    mkfw_timer_shutdown();

    return 0;
}
```

---

## Debug Mode

Define `MKFW_TIMER_DEBUG` before including to enable debug output:

```c
#define MKFW_TIMER_DEBUG
#define MKFW_TIMER
#include "mkfw.h"
```

**Debug output includes:**
- Time remaining after sleep phase
- Overshoot amount (how late past the deadline)
- Helps diagnose timing issues

**Example output:**
```
[DEBUG] Woke up with 450123 ns left. Overshoot:    24 ns
[DEBUG] Woke up with 498765 ns left. Overshoot:    19 ns
```

---

## Platform-Specific Implementation Details

### Windows Implementation

**Files:** `mkfw_win32_timer.c`

**Key features:**
- Uses `QueryPerformanceCounter` for time measurement
- Uses `NtDelayExecution` for high-precision sleep
- Sets thread to MMCSS "Pro Audio" class for realtime priority
- Falls back to `REALTIME_PRIORITY_CLASS` + `THREAD_PRIORITY_TIME_CRITICAL`
- Pins timer thread to CPU core 0 for consistency
- Spin threshold: 1000µs (1ms)

**Dependencies:**
- Windows.h, mmsystem.h, avrt.h, winternl.h
- Links: `-lwinmm -lavrt`

---

### Linux Implementation

**Files:** `mkfw_linux_timer.c`

**Key features:**
- Uses `CLOCK_MONOTONIC_RAW` for drift-free time measurement
- Uses `nanosleep()` for sleep phase
- Uses futex for efficient wait/wake between threads
- Spin threshold: 500µs

**Dependencies:**
- pthread.h, time.h, linux/futex.h, sys/syscall.h
- Links: `-lpthread`

---

## Performance Characteristics

### Accuracy

- **Typical jitter:** 50-200 nanoseconds (on modern hardware)
- **Maximum jitter:** Usually under 1 microsecond
- **Long-term drift:** Zero (uses absolute deadlines)

### CPU Usage

- **Sleep phase:** Near-zero CPU usage
- **Spin phase:** 100% on one core for the spin threshold duration
- **Overall:** Up to ~3% of one core on Linux, ~6% on Windows at 60 FPS (spin only runs during idle time — if your frame uses most of the budget, spin cost approaches zero)

### Priority

- Runs at realtime priority to minimize scheduler interference
- Should not starve other processes (spins for microseconds, not milliseconds)

---

## Important Notes

### Thread Safety

- Each timer handle has its own dedicated thread
- `mkfw_timer_wait()` is safe to call from any thread
- Multiple timers are completely independent

### Realtime Scheduling

- Timers use realtime priority for accuracy
- Spin phase is intentionally kept short to avoid system impact

### System Timer Resolution

- Windows: Timer resolution is set to 1ms system-wide during `mkfw_timer_init()`
- This affects the entire system, not just your application
- Resolution is restored on `mkfw_timer_shutdown()`

### Deadline Management

- Timers maintain absolute deadlines, not relative intervals
- This prevents cumulative drift over long run times
- If you miss a deadline (frame drop), the next deadline compensates automatically

---

## Common Patterns

### Variable Refresh Rate Adaptation

```c
// Start with 60 FPS
uint64_t interval = 16666666;
struct mkfw_timer_handle *timer = mkfw_timer_new(interval);

// Later, switch to 120 FPS
mkfw_timer_destroy(timer);
interval = 8333333;
timer = mkfw_timer_new(interval);
```

### Frame Time Measurement

```c
#include "mkfw.h"

uint64_t last_time = mkfw_gettime(window);

while (running) {
    uint64_t current_time = mkfw_gettime(window);
    uint64_t delta = current_time - last_time;
    last_time = current_time;

    printf("Frame time: %llu ns (%.2f FPS)\n",
           delta, 1000000000.0 / delta);

    render_frame();
    mkfw_timer_wait(timer);
}
```

---

## Comparison with Other Timing Methods

| Method | Accuracy | CPU Usage | Complexity |
|--------|----------|-----------|------------|
| `sleep()` / `Sleep()` | ~15ms jitter | Very low | Simple |
| Busy spin | Perfect | 100% one core | Simple |
| **MKFW Timer** | ~0.0001ms jitter | Very low | Simple API |
| Custom sleep+spin | ~0.001ms jitter | Very low | Complex |

---

## Troubleshooting

### High jitter on Windows

- Ensure `mkfw_timer_init()` is called to set timer resolution
- Check system load - other realtime processes may interfere
- Disable Windows power saving features

### Occasional jitter spikes on Linux

- The timer uses normal thread priority and achieves excellent results (typically <50ns jitter)
- Occasional spikes (~80µs) are usually caused by SMI (System Management Interrupts), not scheduling
- SMI cannot be eliminated without BIOS settings changes (disable C-states, Intel ME, etc.)
- Advanced: Boot with `isolcpus=` and `nohz_full=` kernel parameters to isolate timer to dedicated CPU
- Not necessary for most use cases - default behavior is already excellent

### Consistent overshoot

- System may be too loaded to meet deadline
- Reduce frame rate or optimize frame workload
- Check if other realtime processes are competing

---

## See Also

- `MKFW_API.md` - Main MKFW windowing API
- `MKFW_AUDIO_API.md` - Audio subsystem documentation
- `mkfw_gettime()` - High-resolution time query (in main MKFW API)
- `mkfw_sleep()` - Simple sleep function (in main MKFW API)

---

## License

MIT License - See source files for full license text.
