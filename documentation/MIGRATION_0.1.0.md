# Migrating from mkfw v0.1.0

This guide covers the breaking changes between the [`v0.1.0`](../../../releases/tag/v0.1.0)
tag and current master.  It is a one-way reference: there is no
automated path, every call site needs to be updated by hand.

## Contents

- [Why the API changed](#why-the-api-changed)
- [The mental model](#the-mental-model)
- [Minimal program, before and after](#minimal-program-before-and-after)
- [Initialization and shutdown](#initialization-and-shutdown)
- [Window options replace global setters](#window-options-replace-global-setters)
- [Callbacks](#callbacks)
- [Window operations](#window-operations)
- [Input queries](#input-queries)
- [Event pump and time](#event-pump-and-time)
- [Joystick](#joystick)
- [Audio](#audio)
- [Timer](#timer)
- [Renamed key and modifier constants](#renamed-key-and-modifier-constants)
- [Removed entirely](#removed-entirely)
- [Behavioral changes worth knowing](#behavioral-changes-worth-knowing)
- [What else is new](#what-else-is-new)

---

## Why the API changed

mkfw started as a single-window helper and grew the rest of its
surface, lego-style, on top of that initial shape.  The original
`mkfw_init()` both initialised the library *and* opened a window,
returning a single `struct mkfw_state` that was simultaneously
"the library" and "this window".  Once multi-window came up and
game-development use cases drove cleaner subsystem boundaries
(audio, timer, joystick as companion headers; scancode tracking
for WASD-on-AZERTY; native-handle escape for Vulkan/Direct2D),
that shape stopped fitting.

The refactor split the responsibilities and renamed everything
along the way to be self-describing.  The result is more in line
with what consumers of GLFW-style libraries expect, and the
ergonomics are better for code that opens more than one window
or wants to use mkfw without GL at all.

## The mental model

v0.1.0:

```
mkfw_init(w, h)  -->  struct mkfw_state *      (library + window in one)
                          |
                          +-- input state
                          +-- callbacks
                          +-- platform handle
```

master:

```
mkfw_init(opts)               -->  struct mkfw_context *   (library)
                                       |
                                       +-- monitors, display, etc.
                                       |
mkfw_window_create(ctx, opts) -->  struct mkfw_window *    (0..N per context)
                                       |
                                       +-- input state
                                       +-- callbacks
                                       +-- platform handle
```

A single `mkfw_poll_events(ctx)` drains events for every window
the context owns.  Per-window state (keyboard, mouse buttons,
focus, fullscreen, callbacks) lives on `struct mkfw_window`.

## Minimal program, before and after

v0.1.0:

```c
#include "mkfw_gl_loader.h"
#include "mkfw.h"

int main(void) {
    struct mkfw_state *win = mkfw_init(1280, 720);
    if(!win) {
        return 1;
    }
    mkfw_show_window(win);
    mkfw_gl_loader();

    while(!mkfw_should_close(win)) {
        mkfw_pump_messages(win);

        if(win->keyboard_state[MKS_KEY_ESCAPE]) {
            mkfw_set_should_close(win, 1);
        }

        // render ...

        mkfw_swap_buffers(win);
        mkfw_update_input_state(win);
    }

    mkfw_cleanup(win);
    return 0;
}
```

master:

```c
#include "mkfw_gl_loader.h"
#include "mkfw.h"

int main(void) {
    struct mkfw_context *ctx = mkfw_init(0);
    if(!ctx) {
        return 1;
    }
    struct mkfw_window_options opts = {
        .width = 1280, .height = 720, .title = "Hello mkfw",
    };
    struct mkfw_window *win = mkfw_window_create(ctx, &opts);
    if(!win) {
        mkfw_shutdown(ctx);
        return 1;
    }
    mkfw_gl_loader();

    while(!mkfw_window_should_close(win)) {
        mkfw_poll_events(ctx);

        if(win->keyboard_state[MKFW_KEY_ESCAPE]) {
            mkfw_window_set_should_close(win, 1);
        }

        // render ...

        mkfw_window_swap_buffers(win);
        mkfw_window_update_input_state(win);
    }

    mkfw_window_destroy(win);
    mkfw_shutdown(ctx);
    return 0;
}
```

The rest of this document is the symbol-level mapping for the
remaining surface area.

## Initialization and shutdown

| v0.1.0 | master |
|---|---|
| `struct mkfw_state *mkfw_init(int w, int h)` | `struct mkfw_context *mkfw_init(struct mkfw_options *opts)` (pass `0` for defaults) |
| (window was created by `mkfw_init`) | `struct mkfw_window *mkfw_window_create(ctx, struct mkfw_window_options *opts)` |
| `mkfw_cleanup(state)` | `mkfw_window_destroy(win)` + `mkfw_shutdown(ctx)` |
| `mkfw_show_window(state)` (required after init) | implicit on `mkfw_window_create`; pass `MKFW_WIN_HIDDEN` in `opts->flags` to opt out |
| `mkfw_hide_window(state)` | `mkfw_window_hide(win)` |
| `int mkfw_query_max_gl_version(int *, int *)` | `uint32_t mkfw_query_max_gl_version(int32_t *, int32_t *)` |

## Window options replace global setters

v0.1.0 configured the library through file-scope setters that
had to run before `mkfw_init`:

```c
mkfw_set_gl_version(4, 6);
mkfw_set_transparent(1);
struct mkfw_state *win = mkfw_init(1280, 720);
```

master moves those into `struct mkfw_window_options` so each
window can request its own configuration:

```c
struct mkfw_window_options opts = {
    .width = 1280, .height = 720, .title = "Hello mkfw",
    .gl_major = 4, .gl_minor = 6,
    .flags = MKFW_WIN_TRANSPARENT,
};
struct mkfw_window *win = mkfw_window_create(ctx, &opts);
```

| v0.1.0 setter | master field |
|---|---|
| `mkfw_set_gl_version(major, minor)` | `opts.gl_major`, `opts.gl_minor` |
| `mkfw_set_transparent(1)` | `MKFW_WIN_TRANSPARENT` bit in `opts.flags` |
| (no equivalent, init always showed) | `MKFW_WIN_HIDDEN` bit in `opts.flags` |
| (always GL) | `opts.graphics_api = MKFW_GFX_NONE` for Vulkan / 2D users |
| (always Compatibility Profile, 3.1) | `opts.gl_profile = MKFW_GL_PROFILE_COMPAT` (see below) |

The default GL version and profile both changed.  See
[GL default changed to latest + Core](#gl-default-changed-to-latest--core)
under behavioral changes for the consequences.

## Callbacks

Every callback typedef gained an `mkfw_` prefix and its first
parameter changed from `struct mkfw_state *` to
`struct mkfw_window *`.  `mouse_button_callback_t::action`
widened from `int` to `uint32_t`.  Setters gained an
`mkfw_window_` prefix; the library-scoped `mkfw_set_error_callback`
kept its name.

| v0.1.0 setter | master setter |
|---|---|
| `mkfw_set_key_callback(state, cb)` | `mkfw_window_set_key_callback(win, cb)` |
| `mkfw_set_char_callback(state, cb)` | `mkfw_window_set_char_callback(win, cb)` |
| `mkfw_set_scroll_callback(state, cb)` | `mkfw_window_set_scroll_callback(win, cb)` |
| `mkfw_set_mouse_move_delta_callback(state, cb)` | `mkfw_window_set_mouse_move_delta_callback(win, cb)` |
| `mkfw_set_mouse_button_callback(state, cb)` | `mkfw_window_set_mouse_button_callback(win, cb)` |
| `mkfw_set_framebuffer_size_callback(state, cb)` | `mkfw_window_set_framebuffer_size_callback(win, cb)` |
| `mkfw_set_focus_callback(state, cb)` | `mkfw_window_set_focus_callback(win, cb)` |
| `mkfw_set_drop_callback(state, cb)` | `mkfw_window_set_drop_callback(win, cb)` |
| `mkfw_set_window_state_callback(state, cb)` | `mkfw_window_set_state_callback(win, cb)` (shorter name) |
| `mkfw_set_user_data(state, p)` / `mkfw_get_user_data(state)` | `mkfw_window_set_user_data(win, p)` / `mkfw_window_get_user_data(win)` |
| `mkfw_set_error_callback(cb)` | unchanged |

The drop callback's own signature is unchanged in either
version (`void (*)(uint32_t count, const char **paths)`); it
never took a state pointer.  A new helper
`mkfw_drop_paths_free(count, paths)` codifies the cleanup
contract.

## Window operations

All window ops moved under the `mkfw_window_` prefix and take
`struct mkfw_window *` as their first argument.  Several were
also renamed or split.

| v0.1.0 | master |
|---|---|
| `mkfw_set_window_title(s, t)` | `mkfw_window_set_title(w, t)` |
| `mkfw_set_window_size(s, w, h)` | `mkfw_window_set_size(w, w, h)` |
| `mkfw_get_framebuffer_size(s, *w, *h)` | `mkfw_window_get_framebuffer_size(w, *w, *h)` |
| `mkfw_set_window_position(s, x, y)` | `mkfw_window_set_position(w, x, y)` |
| `mkfw_get_window_position(s, *x, *y)` | `mkfw_window_get_position(w, *x, *y)` |
| `mkfw_set_window_min_size_and_aspect(s, min_w, min_h, max_w, max_h, num, den)` | split: `mkfw_window_set_size_limits(w, min_w, min_h, max_w, max_h)` + `mkfw_window_set_aspect_ratio(w, num, den)` |
| `mkfw_set_window_resizable(s, r)` | `mkfw_window_set_resizable(w, r)` |
| `mkfw_set_window_decorated(s, d)` | `mkfw_window_set_decorated(w, d)` |
| `mkfw_set_window_opacity(s, o)` | `mkfw_window_set_opacity(w, o)` |
| `mkfw_set_window_icon(s, w, h, rgba)` | `mkfw_window_set_icon(w, w, h, rgba)` |
| `mkfw_fullscreen(s, e)` | `mkfw_window_set_fullscreen(w, e)` |
| `mkfw_maximize_window(s)` / `mkfw_minimize_window(s)` / `mkfw_restore_window(s)` | `mkfw_window_maximize(w)` / `mkfw_window_minimize(w)` / `mkfw_window_restore(w)` |
| `mkfw_is_minimized(s)` / `mkfw_is_maximized(s)` | `mkfw_window_is_minimized(w)` / `mkfw_window_is_maximized(w)` |
| `mkfw_should_close(s)` / `mkfw_set_should_close(s, v)` | `mkfw_window_should_close(w)` / `mkfw_window_set_should_close(w, v)` |
| `mkfw_get_content_scale(s)` | `mkfw_window_get_content_scale(w)` |
| `mkfw_request_attention(s)` | `mkfw_window_request_attention(w)` |
| `mkfw_get_monitors(s, out, max)` | `mkfw_get_monitors(ctx, out, max)` (moved to context) |
| `mkfw_attach_context(s)` / `mkfw_detach_context(s)` | `mkfw_window_attach_context(w)` / `mkfw_window_detach_context(w)` |
| `mkfw_swap_buffers(s)` | `mkfw_window_swap_buffers(w)` |
| `mkfw_set_swapinterval(s, i)` / `mkfw_get_swapinterval(s)` | `mkfw_window_set_swap_interval(w, i)` / `mkfw_window_get_swap_interval(w)` (underscore added) |
| `mkfw_set_clipboard_text(s, t)` | `mkfw_window_set_clipboard_text(w, t)` |
| `mkfw_get_clipboard_text(s)` returns `const char *` | `mkfw_window_get_clipboard_text(w)` returns `char *` (caller owns) |
| (implicit via drop callback setter) | `mkfw_window_enable_drop(w, enable)` is now also callable directly |
| (no escape hatch) | `mkfw_window_get_native_handles(w, struct mkfw_native_handles *out)` |

## Input queries

`struct mkfw_state` fields are now `struct mkfw_window` fields.
Most direct accesses (`win->keyboard_state[...]`, `win->mouse_x`,
etc.) translate one-for-one, except the modifier array, which is
gone.

| v0.1.0 | master |
|---|---|
| `state->keyboard_state[MKS_KEY_*]` | `win->keyboard_state[MKFW_KEY_*]` |
| `state->prev_keyboard_state[]` | `win->prev_keyboard_state[]` |
| `state->modifier_state[MKS_MODIFIER_*]` | gone; use `mkfw_window_get_modifiers(w)` returning OR'd `MKFW_MOD_*` bits |
| `state->mouse_buttons[5]` / `state->previous_mouse_buttons[5]` | unchanged on `win` |
| `state->mouse_x`, `state->mouse_y` | unchanged on `win` |
| `mkfw_is_key_pressed(s, k)` | `mkfw_window_is_key_pressed(w, k)` |
| `mkfw_was_key_released(s, k)` | `mkfw_window_was_key_released(w, k)` |
| `mkfw_is_button_pressed(s, b)` | `mkfw_window_is_button_pressed(w, b)` |
| `mkfw_was_button_released(s, b)` | `mkfw_window_was_button_released(w, b)` |
| `mkfw_update_input_state(s)` | `mkfw_window_update_input_state(w)` |
| `mkfw_set_mouse_cursor(s, v)` | `mkfw_window_set_cursor_visible(w, v)` |
| `mkfw_constrain_mouse(s, c)` | `mkfw_window_set_cursor_locked(w, c)` |
| `mkfw_set_cursor_shape(s, c)` | `mkfw_window_set_cursor_shape(w, c)` |
| `mkfw_get_cursor_position(s, *x, *y)` | `mkfw_window_get_cursor_position(w, *x, *y)` |
| `mkfw_set_cursor_position(s, x, y)` | `mkfw_window_set_cursor_position(w, x, y)` |
| `mkfw_set_mouse_sensitivity(s, sens)` | `mkfw_window_set_mouse_sensitivity(w, sens)` |
| `mkfw_get_and_clear_mouse_delta(s, *dx, *dy)` | `mkfw_window_get_and_clear_mouse_delta(w, *dx, *dy)` |
| `mkfw_get_key_name(key)` | unchanged |

## Event pump and time

| v0.1.0 | master |
|---|---|
| `mkfw_pump_messages(state)` | `mkfw_poll_events(ctx)` (renamed, moved to context, dispatches to all windows) |
| `mkfw_wait_events(state)` | `mkfw_wait_events(ctx)` |
| `mkfw_wait_events_timeout(state, ns)` | `mkfw_wait_events_timeout(ctx, ns)` |
| `mkfw_run(state, frame_cb)` | removed (see [Removed entirely](#removed-entirely)) |
| `mkfw_gettime(state)` | `mkfw_get_time(void)` |
| `mkfw_sleep(ns)` | unchanged |

## Joystick

Move from `#define MKFW_JOYSTICK` + including `mkfw.h` to
including the companion header `mkfw_joystick.h`.  All
integer parameters widened from `int` to `uint32_t`.  Query
helpers picked up `get_`, `is_`, `was_` prefixes for
consistency.

| v0.1.0 | master |
|---|---|
| `mkfw_joystick_init/shutdown/update()` | unchanged |
| `mkfw_joystick_connected(idx)` | `mkfw_joystick_is_connected(idx)` |
| `mkfw_joystick_name(idx)` | `mkfw_joystick_get_name(idx)` |
| `mkfw_joystick_button(idx, b)` | `mkfw_joystick_get_button(idx, b)` |
| `mkfw_joystick_button_pressed(idx, b)` | `mkfw_joystick_is_button_pressed(idx, b)` |
| `mkfw_joystick_button_released(idx, b)` | `mkfw_joystick_was_button_released(idx, b)` |
| `mkfw_joystick_axis(idx, a)` | `mkfw_joystick_get_axis(idx, a)` |
| `mkfw_joystick_hat_x(idx)` / `mkfw_joystick_hat_y(idx)` | `mkfw_joystick_get_hat_x(idx)` / `mkfw_joystick_get_hat_y(idx)` |
| `mkfw_joystick_button_count(idx)` / `mkfw_joystick_axis_count(idx)` | `mkfw_joystick_get_button_count(idx)` / `mkfw_joystick_get_axis_count(idx)` |
| `mkfw_joystick_rumble(idx, low, high, ms)` | unchanged signature (just `uint32_t` types) |
| (callback) `void (*)(int idx, int connected)` | `void (*)(uint32_t idx, uint32_t connected)` |
| `mkfw_gamepad_button(idx, btn)` | `mkfw_gamepad_get_button(idx, btn)` |
| `mkfw_gamepad_button_pressed(idx, btn)` | `mkfw_gamepad_is_button_pressed(idx, btn)` |
| `mkfw_gamepad_axis(idx, axis)` | `mkfw_gamepad_get_axis(idx, axis)` |
| (no equivalent) | `mkfw_joystick_rumble_set(idx, low, high)` (continuous rumble) |
| (no equivalent) | `mkfw_joystick_get_type(idx)` returning `MKFW_JOYSTICK_TYPE_{GENERIC,XBOX,PLAYSTATION,SWITCH}` |
| (no equivalent) | `mkfw_joystick_get_battery(idx)` |

## Audio

v0.1.0's audio surface was three functions activated by
`#define MKFW_AUDIO` and shipped `int16_t` samples to the
callback.  master moves audio into its own `mkfw_audio.h`
companion header, switches to interleaved 32-bit float
samples in `[-1.0, 1.0]`, and adds device-loss / device-
acquired notifications plus an options struct.

| v0.1.0 | master |
|---|---|
| `mkfw_audio_initialize(void)` | `uint32_t mkfw_audio_init(struct mkfw_audio_options *opts)` |
| `mkfw_audio_shutdown(void)` | unchanged |
| `mkfw_set_audio_callback(void (*)(int16_t *, size_t))` | `mkfw_audio_set_callback(mkfw_audio_callback_t cb, void *userdata)` where the callback is `void (*)(void *userdata, float *buffer, uint32_t frames)` |
| (no equivalent) | `mkfw_audio_set_device_lost_callback(...)`, `mkfw_audio_set_device_acquired_callback(...)`, `mkfw_audio_info(struct mkfw_audio_info *out)` |

## Timer

v0.1.0's timer subsystem (`#define MKFW_TIMER`) is mostly
preserved with one rename and one addition; the companion
header is `mkfw_timer.h`.

| v0.1.0 | master |
|---|---|
| `mkfw_timer_init/shutdown()` | unchanged |
| `mkfw_timer_new(interval_ns)` | `mkfw_timer_create(interval_ns)` |
| `mkfw_timer_destroy(t)`, `mkfw_timer_wait(t)`, `mkfw_timer_set_interval(t, ns)` | unchanged |
| (no equivalent) | `mkfw_timer_set_spin(t, enabled)` |

## Renamed key and modifier constants

The `MKS_` prefix is gone.  Everywhere in the keys header,
`MKS_*` became `MKFW_*` with numeric values unchanged.

- `MKS_KEY_*` becomes `MKFW_KEY_*` (every member, including
  `MKS_KEY_LAST`).
- `MKS_RELEASED` / `MKS_PRESSED` become `MKFW_RELEASED` /
  `MKFW_PRESSED`.
- `MKS_MOD_*` becomes `MKFW_MOD_*` (all eight bit flags plus
  the four combined macros, values unchanged).
- `MOUSE_BUTTON_LEFT/MIDDLE/RIGHT/EXTRA1/EXTRA2` becomes
  `MKFW_MOUSE_LEFT/MIDDLE/RIGHT/EXTRA1/EXTRA2`.
- `MKFW_CURSOR_*` is unchanged.
- `MKS_MODIFIER_SHIFT/CTRL/ALT/LAST` (the indices into the
  per-state modifier array) are gone, the array with them.
  Use `mkfw_window_get_modifiers(w)` and test the
  `MKFW_MOD_*` bits.

## Removed entirely

- **Emscripten port.**  Every `#ifdef __EMSCRIPTEN__` branch,
  the Emscripten audio/joystick implementations, and the
  `mkfw_emscripten*.c` files were removed.  If you need a
  web build, the v0.1.0 tag still has it.
- **`mkfw_run(state, frame_cb)`.**  The callback-driven main
  loop was kept for parity with the Emscripten port; with
  that port gone the desktop pattern is just a plain
  `while(!mkfw_window_should_close(w)) { ... }` loop.  The
  separate `MKFW_RUN_API.md` document is gone too.
- **`mkfw_set_gl_version(major, minor)`** (file-scope, called
  before init).  Use `opts.gl_major`, `opts.gl_minor` on the
  per-window options struct.
- **`mkfw_set_transparent(enable)`** (file-scope).  Use
  `opts.flags |= MKFW_WIN_TRANSPARENT`.
- **`state->modifier_state[]`** and the `MKS_MODIFIER_*` enum
  used to index it.  Modifiers are derived on demand via
  `mkfw_window_get_modifiers(w)`.

## Behavioral changes worth knowing

### GL default changed to latest + Core

v0.1.0 always created an OpenGL 3.1 *Compatibility Profile*
context, which keeps the fixed-function pipeline (glBegin/glEnd,
glColor, the matrix stack, GL_QUADS, etc.) available.

master changes both halves of that default.  When `gl_major` is
0 in `struct mkfw_window_options`, `mkfw_window_create` calls
`mkfw_query_max_gl_version` internally and asks for that version
in the Core (non-backwards-compatible) profile.  Any draw code
that uses the fixed-function pipeline will fail to link or fail
at runtime against a Core context.

To keep v0.1.0's behavior, opt back in explicitly:

```c
struct mkfw_window_options opts = {
    .width = 1280, .height = 720,
    .gl_major = 3, .gl_minor = 1,
    .gl_profile = MKFW_GL_PROFILE_COMPAT,
};
```

If `gl_major` is set and the driver cannot provide that exact
version (or that profile), window creation fails and the error
includes the driver's maximum.  There is no silent fallback to
a lower version; the caller decides whether to retry.

### Other behavioral changes

- **Two-step init.**  `mkfw_init()` no longer opens a window.
  Code that called `mkfw_init(w, h)` followed by
  `mkfw_show_window` has to be rewritten as `mkfw_init(0)`
  followed by `mkfw_window_create(ctx, &opts)`.
- **Windows are shown by default on create.**  v0.1.0
  required an explicit `mkfw_show_window` after init.
  master shows the window unless `MKFW_WIN_HIDDEN` is set in
  `opts->flags`.
- **One event pump for all windows.**  v0.1.0's
  `mkfw_pump_messages(state)` was per-window even though
  events came from a shared display connection.  master's
  `mkfw_poll_events(ctx)` is per-context and dispatches to
  every window the context owns; you no longer pump each
  window individually.
- **Time is not per-window.**  `mkfw_gettime(state)` was
  always state-independent in practice; the signature is
  now `mkfw_get_time(void)`.
- **Error reporting always records.**  v0.1.0's `mkfw_error`
  was a no-op if no callback was set.  master always writes
  to a thread-local `mkfw_last_error_buf[512]` and *also*
  fires the callback if installed.  Two new accessors,
  `mkfw_get_last_error()` and `mkfw_clear_last_error()`, let
  callers poll instead of registering a callback.
- **Clipboard ownership changed.**  v0.1.0's
  `mkfw_get_clipboard_text(state)` returned `const char *`
  pointing at library-owned storage.  master returns
  `char *` and the caller owns the buffer.
- **Linkage modes.**  v0.1.0 was unity-only and every public
  function was `static inline`.  master introduces the
  `MKFW_API` / `MKFW_VAR` macros that switch the public
  surface between `static`, `extern`, and DLL import/export
  based on `MKFW_BUILD_LIBRARY`, `MKFW_BUILD_SHARED`, and
  `MKFW_USE_SHARED`.  The default (header-only, unity-build)
  mode is unchanged in behavior, but the `mkfw.h` header now
  contains explicit prototypes at the bottom.

## What else is new

These do not require migration work, but they are worth
knowing about while you are already updating call sites:

- **Scancode tracking** alongside the layout-dependent
  `keyboard_state`.  `win->scancode_state[256]` /
  `prev_scancode_state[256]` hold USB HID scancodes so WASD
  works on AZERTY.  Helpers:
  `mkfw_window_is_scancode_down/pressed`,
  `mkfw_window_was_scancode_released`.
- **Custom RGBA cursors.**  `mkfw_cursor_create_rgba(ctx, w,
  h, rgba, hx, hy)` + `mkfw_window_set_custom_cursor(w,
  cursor)`.
- **Native handle escape.**
  `mkfw_window_get_native_handles(w, &out)` exposes the
  underlying `Display *` / `Window` / `GLXContext` (or HWND
  / HDC / HGLRC on Windows) for engines that need to layer
  Vulkan or a 2D renderer over an mkfw window.  Combine with
  `opts.graphics_api = MKFW_GFX_NONE` to skip GL context
  creation.
- **Joystick gamedb tooling.**  `tools/gen_joystick_gamedb.py`
  regenerates `mkfw_joystick_gamedb.h` from the upstream SDL
  game-controller DB.  A monthly GitHub Action refreshes it
  automatically.
- **Audio device-loss / device-acquired callbacks.**  Wired
  up for hot-swapping output devices without restarting
  audio.
- **`mkfw_drop_paths_free(count, paths)`** helper.
- **`mkfw_timer_set_spin(t, enabled)`** to toggle the spin
  half of the sleep+spin strategy per timer.
