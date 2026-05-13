# mkfw timer API

High-precision interval timing for frame pacing and fixed-step
simulation loops.  Uses a sleep + spin strategy: the timer
thread sleeps until close to the deadline, then signals the
waiter via a futex / event so the caller wakes on the precise
deadline tick rather than the OS scheduler's coarse granularity.

## Enabling

```c
#include "mkfw.h"
#include "mkfw_timer.h"
```

For required linker flags see the **Linking** section in the
project README (the timer adds nothing beyond the core libs).

## Contents

- [Overview](#overview)
- [Lifecycle: init and shutdown](#lifecycle-init-and-shutdown)
- [Timer handles](#timer-handles)
- [Waiting on a deadline](#waiting-on-a-deadline)
- [Adjusting an active timer](#adjusting-an-active-timer)
- [Threading](#threading)
- [Platform details](#platform-details)
- [Patterns](#patterns)

---

## Overview

Each timer owns a worker thread that tracks a monotonic deadline
and wakes the application precisely at every tick.

```c
mkfw_timer_init();   // Win32 needs this; Linux no-op
struct mkfw_timer_handle *t = mkfw_timer_create(16666666ULL);   // ~60 Hz

while(running) {
    mkfw_timer_wait(t);
    update();
    render();
}

mkfw_timer_destroy(t);
mkfw_timer_shutdown();
```

The interval is in **nanoseconds**.  Common cadences:

| Rate    | Interval (ns) |
|---------|---------------|
| 30 Hz   | 33333333      |
| 60 Hz   | 16666666      |
| 120 Hz  | 8333333       |
| NTSC field (60.0988 Hz) | 16639090 |
| PAL field (50 Hz)       | 20000000 |
| 240 Hz  | 4166666       |

---

## Lifecycle: init and shutdown

### `mkfw_timer_init`

```c
void mkfw_timer_init(void);
```

Initialize the timer subsystem.  Must be called before
`mkfw_timer_create`.

- **Linux**: no-op stub.  Safe to call (and required for API
  symmetry).
- **Windows**: raises the global timer resolution to 1 ms via
  `timeBeginPeriod(1)` and requests 0.5 ms via
  `NtSetTimerResolution`; caches `QueryPerformanceFrequency`;
  loads `NtDelayExecution` from ntdll.dll.  On Windows 10
  version 2004 and newer `timeBeginPeriod` is **per-process for
  foreground apps** (older Windows applies it system-wide), so
  the call is well-behaved on modern systems.

### `mkfw_timer_shutdown`

```c
void mkfw_timer_shutdown(void);
```

Tear down what `mkfw_timer_init` set up.

- **Linux**: no-op.
- **Windows**: restores the global timer resolution and releases
  the cached function pointers.

Call after every `mkfw_timer_destroy` you intend to make.

---

## Timer handles

### `mkfw_timer_create`

```c
struct mkfw_timer_handle *mkfw_timer_create(uint64_t interval_ns);
```

Spawn a new timer ticking every `interval_ns` nanoseconds.  The
internal worker thread starts immediately; the first
`mkfw_timer_wait` blocks until the first deadline.

Spin-correction is enabled by default
(`mkfw_timer_set_spin(t, 1)`).  Turn off if you want pure sleep
behaviour (cheaper, but coarser).

Returns the new timer handle.  The caller owns it and must
release it with `mkfw_timer_destroy`.

### `mkfw_timer_destroy`

```c
void mkfw_timer_destroy(struct mkfw_timer_handle *t);
```

Stop the worker thread, free internal storage.  Passing `0` is a
no-op.  Safe to call from any thread.

---

## Waiting on a deadline

### `mkfw_timer_wait`

```c
uint32_t mkfw_timer_wait(struct mkfw_timer_handle *t);
```

Block until the timer's next deadline.  Returns non-zero on a
normal tick, zero only if `t` is `0`.

Each call returns once per deadline.  If the application falls
behind (handlers take longer than the interval), missed ticks
accumulate at the deadline boundary; subsequent calls will
return immediately until the application catches up.  Plan for
either:

- catching up by running the simulation at fixed-step regardless
  of how many ticks fired, or
- skipping render frames when too far behind.

---

## Adjusting an active timer

### `mkfw_timer_set_interval`

```c
void mkfw_timer_set_interval(struct mkfw_timer_handle *t, uint64_t interval_ns);
```

Change the tick interval.  Takes effect on the next deadline
after the current one completes; the active deadline is not
disturbed.  Safe to call from any thread.

Useful for variable-rate pacing (e.g. nudging frame time to
match audio buffer fill level).

### `mkfw_timer_set_spin`

```c
void mkfw_timer_set_spin(struct mkfw_timer_handle *t, uint32_t enabled);
```

Toggle the spin-correction phase.

- `enabled != 0` (default): the worker sleeps until close to the
  deadline then **spins** until the precise deadline.  Lower
  jitter, higher CPU.
- `enabled == 0`: pure sleep.  Higher jitter (typically OS
  scheduler quantum), lower CPU.

Safe to flip at runtime; the next tick observes the new value.

---

## Threading

The internal worker thread is invisible to the application.
`mkfw_timer_wait` is called from whichever thread the
application chooses to run the wait loop on.  All of
`mkfw_timer_set_interval`, `_set_spin`, `_destroy` are safe to
call from any thread; their effects are visible to the worker
via acquire/release atomics.

Do **not** mix:

- Driving the wait loop on one thread and calling
  `mkfw_timer_destroy` on the same handle from another without
  first making sure the wait loop is no longer running, or
- Sharing a single timer handle between multiple waiting
  threads (only one of them will be woken per tick; behaviour
  is otherwise undefined).

If two threads need independent cadences, create two timers.

---

## Platform details

### Linux

- The worker uses `clock_gettime(CLOCK_MONOTONIC_RAW)` for
  deadlines, `nanosleep` for the sleep phase, and a custom
  futex (`SYS_futex` with `FUTEX_WAIT` / `FUTEX_WAKE`) for
  cross-thread wake-up.
- No system-wide configuration change.

### Windows

- The worker uses `QueryPerformanceCounter` for deadlines.  The
  sleep phase uses `NtDelayExecution` with the cached 0.5 ms
  resolution (via `timeBeginPeriod` + `NtSetTimerResolution`
  in `mkfw_timer_init`).
- Wake-up uses `WaitForSingleObject` on a manual-reset Event.
- The 1 ms timer resolution is held only for the lifetime of
  the subsystem (between `mkfw_timer_init` and
  `mkfw_timer_shutdown`).

---

## Patterns

### Fixed-step game loop

```c
mkfw_timer_init();
struct mkfw_timer_handle *t = mkfw_timer_create(16666666ULL);  // 60 Hz

while(!mkfw_window_should_close(win)) {
    mkfw_poll_events(ctx);

    // Fixed-step simulation
    update_simulation();

    // Render
    render_frame();
    mkfw_window_swap_buffers(win);
    mkfw_window_update_input_state(win);

    mkfw_timer_wait(t);  // sleep until next tick
}

mkfw_timer_destroy(t);
mkfw_timer_shutdown();
```

### NTSC frame pacing (emulator)

The NTSC field rate is 60.0988 Hz, not 60 Hz exactly:

```c
#define NES_NTSC_FRAME_NS 16639090   /* 60.0988 Hz field rate */

struct mkfw_timer_handle *t = mkfw_timer_create(NES_NTSC_FRAME_NS);
```

Nudge the interval at runtime to keep the audio buffer at a
healthy level:

```c
if(audio_buffer_low()) {
    mkfw_timer_set_interval(t, NES_NTSC_FRAME_NS - 50000);
} else if(audio_buffer_high()) {
    mkfw_timer_set_interval(t, NES_NTSC_FRAME_NS + 50000);
}
```

### Render thread (separate from input pump)

The threaded-rendering pattern (`examples/threaded.c`) pairs the
timer with a dedicated render thread:

```c
static MKFW_THREAD_FUNC(render_thread, arg) {
    struct app *app = arg;
    mkfw_window_attach_context(app->win);
    mkfw_timer_init();
    struct mkfw_timer_handle *t = mkfw_timer_create(16666666ULL);
    while(app->running) {
        render(app);
        mkfw_window_swap_buffers(app->win);
        mkfw_timer_wait(t);
    }
    mkfw_timer_destroy(t);
    mkfw_timer_shutdown();
    return 0;
}
```

The main thread loops on `mkfw_poll_events` + a coarse
`mkfw_sleep`; the render thread owns the GL context and the
timer.
