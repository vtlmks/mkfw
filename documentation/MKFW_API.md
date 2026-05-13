# mkfw core API

`mkfw.h` is the core of the library: window creation, input,
OpenGL context management, monitor enumeration, and error
reporting.  Optional subsystems (audio, timer, joystick) live in
companion headers documented separately.

## Contents

- [Overview](#overview)
- [Building](#building)
- [Core types](#core-types)
- [Initialization and shutdown](#initialization-and-shutdown)
- [Window creation and lifecycle](#window-creation-and-lifecycle)
- [Window attributes](#window-attributes)
- [Window state queries](#window-state-queries)
- [Rendering and OpenGL](#rendering-and-opengl)
- [Event pumping](#event-pumping)
- [Keyboard input](#keyboard-input)
- [Scancodes](#scancodes)
- [Mouse input](#mouse-input)
- [Cursor](#cursor)
- [Callbacks](#callbacks)
- [Clipboard](#clipboard)
- [File drop](#file-drop)
- [Monitors](#monitors)
- [Native handle escape](#native-handle-escape)
- [Time](#time)
- [Error reporting](#error-reporting)
- [Threading model](#threading-model)
- [Memory ownership](#memory-ownership)
- [HiDPI contract](#hidpi-contract)
- [Thread primitives](#thread-primitives)

---

## Overview

mkfw splits library state from window state.  `mkfw_init()`
returns a `struct mkfw_context *` that owns the platform display
connection, loaded function pointers, the monitor cache, and any
windows opened against it.  `mkfw_window_create()` returns a
`struct mkfw_window *` whose lifetime is bounded by the context.

```c
#include "mkfw_gl_loader.h"
#include "mkfw.h"

int main(void) {
    struct mkfw_context *ctx = mkfw_init(0);

    struct mkfw_window_options opts = {
        .width = 1280, .height = 720, .title = "Hello mkfw",
    };
    struct mkfw_window *win = mkfw_window_create(ctx, &opts);

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

Multiple windows on one context are supported; a single
`mkfw_poll_events(ctx)` dispatches to all of them.

---

## Building

mkfw is header-only by default; a unity-build pattern pulls the
platform `.c` file into your compilation unit when you include
`mkfw.h`.  No separate library to link against, no build system
required.  See the **Linking** section in the project README for
the canonical list of platform libraries required on each target.

The `MKFW_API` macro controls the linkage of every public function:

- **Default** (`MKFW_API` == `static`): header-only / unity build.
  Functions are file-scope to your TU.
- `MKFW_BUILD_LIBRARY`: compile the platform `.c` files as a
  static library; `MKFW_API` becomes `extern` and consumers link
  the archive.
- `MKFW_BUILD_SHARED` + `_WIN32`: build a Windows DLL;
  `MKFW_API` becomes `__declspec(dllexport)`.
- `MKFW_USE_SHARED` + `_WIN32`: consume the DLL;
  `MKFW_API` becomes `__declspec(dllimport)`.

---

## Core types

### `struct mkfw_context`

Opaque library handle.  Created by `mkfw_init`, destroyed by
`mkfw_shutdown`.  Owns the display connection, loaded function
pointers, monitor cache, and the list of windows.

Public field used by callers:

| Field | Type | Purpose |
|-------|------|---------|
| `windows[MKFW_MAX_WINDOWS]` | `struct mkfw_window *` | array of currently-live windows |
| `window_count` | `uint32_t` | number of entries in `windows[]` |
| `monitors[MKFW_MAX_MONITORS]` | `struct mkfw_monitor` | cached monitor list |
| `monitor_count` | `uint32_t` | number of entries in `monitors[]` |

### `struct mkfw_window`

Per-window state.  Created by `mkfw_window_create`, destroyed by
`mkfw_window_destroy`.  Fields the application is expected to
read directly:

| Field | Type | Purpose |
|-------|------|---------|
| `context` | `struct mkfw_context *` | the owning context |
| `keyboard_state[MKFW_KEY_LAST]` | `uint8_t` | 1 while key held, 0 otherwise (indexed by `MKFW_KEY_*`) |
| `prev_keyboard_state[MKFW_KEY_LAST]` | `uint8_t` | snapshot from the previous `mkfw_window_update_input_state` call |
| `scancode_state[256]` | `uint8_t` | 1 while key held, 0 otherwise (indexed by `MKFW_SCANCODE_*`) |
| `prev_scancode_state[256]` | `uint8_t` | snapshot from the previous update |
| `mouse_buttons[5]` | `uint8_t` | 1 while button held, 0 otherwise (indexed by `MKFW_MOUSE_*`) |
| `previous_mouse_buttons[5]` | `uint8_t` | snapshot from the previous update |
| `mouse_x`, `mouse_y` | `int32_t` | last absolute cursor position in physical pixels |
| `is_fullscreen` | `uint8_t` | set by `mkfw_window_set_fullscreen` |
| `has_focus` | `uint8_t` | set by the focus tracker |
| `mouse_in_window` | `uint8_t` | mouse is inside the client area |

Treating any of those as writable from the application is
undefined.  `user_data` is the documented application slot; use
`mkfw_window_set_user_data` / `mkfw_window_get_user_data`.

### `struct mkfw_monitor`

```c
struct mkfw_monitor {
    char name[128];
    int32_t x, y;             // top-left of the work area, in screen coords
    int32_t width, height;    // resolution in physical pixels
    int32_t refresh_rate;     // Hz
    uint8_t primary;          // 1 = primary monitor
};
```

### `struct mkfw_options`

Library init options.  Pass `0` to use defaults.

```c
struct mkfw_options {
    uint32_t version;   // 0 = current
};
```

Reserved for forward compatibility; today there are no fields to
set.

### `struct mkfw_window_options`

Window creation options.  Pass `0` to use defaults for every
field; pass a zero-initialized struct (or `{0}`) and fill only
the fields you care about.

```c
struct mkfw_window_options {
    uint32_t version;        // 0 = current
    int32_t  width;          // 0 = 1280
    int32_t  height;         // 0 = 720
    const char *title;       // 0 = "mkfw"
    int32_t  gl_major;       // 0 = 3 (GL Compatibility Profile)
    int32_t  gl_minor;       // 0 = 1
    uint32_t flags;          // MKFW_WIN_TRANSPARENT | MKFW_WIN_HIDDEN
    uint32_t graphics_api;   // MKFW_GFX_*; 0 = MKFW_GFX_GL
};
```

Flags:

| Flag | Effect |
|------|--------|
| `MKFW_WIN_TRANSPARENT` | request a 32-bit ARGB visual (Linux: GLX_ALPHA_SIZE > 0; Win32: DwmExtendFrameIntoClientArea) |
| `MKFW_WIN_HIDDEN` | do not map / show the window at creation; caller must call `mkfw_window_show` when ready |

Graphics-API selection:

```c
enum mkfw_graphics_api {
    MKFW_GFX_GL = 0,    // default; GLX / WGL Compatibility Profile
    MKFW_GFX_GLES,      // reserved; window creation fails today
    MKFW_GFX_VULKAN,    // reserved; window creation fails today
    MKFW_GFX_NONE,      // no rendering surface; caller manages it
};
```

`MKFW_GFX_NONE` is the escape hatch for Vulkan, Direct2D, or any
other API mkfw does not own.  Combine with
`mkfw_window_get_native_handles` to retrieve the platform window
handles and create your own surface.

### `struct mkfw_native_handles`

Platform handles for callers using `MKFW_GFX_NONE` or otherwise
integrating with an API mkfw does not know about.

```c
struct mkfw_native_handles {
    void     *display;     // Linux: Display *      Win32: HINSTANCE
    uintptr_t window;      // Linux: Window (XID)   Win32: HWND
    void     *gl_context;  // GLXContext / HGLRC; 0 if graphics_api != MKFW_GFX_GL
};
```

mkfw deliberately does not include `<X11/Xlib.h>` or
`<windows.h>` from `mkfw.h`.  Cast the `void *` / `uintptr_t`
slots to the appropriate platform type at the use site.

### Callback typedefs

```c
typedef void (*mkfw_key_callback_t)(struct mkfw_window *, uint32_t key, uint32_t action, uint32_t modifier_bits);
typedef void (*mkfw_char_callback_t)(struct mkfw_window *, uint32_t codepoint);
typedef void (*mkfw_scroll_callback_t)(struct mkfw_window *, double xoffset, double yoffset);
typedef void (*mkfw_mouse_move_delta_callback_t)(struct mkfw_window *, int32_t dx, int32_t dy);
typedef void (*mkfw_mouse_button_callback_t)(struct mkfw_window *, uint8_t button, int action);
typedef void (*mkfw_framebuffer_callback_t)(struct mkfw_window *, int32_t width, int32_t height, float aspect);
typedef void (*mkfw_focus_callback_t)(struct mkfw_window *, uint8_t focused);
typedef void (*mkfw_drop_callback_t)(uint32_t count, const char **paths);
typedef void (*mkfw_window_state_callback_t)(struct mkfw_window *, uint8_t maximized, uint8_t minimized);
typedef void (*mkfw_error_callback_t)(const char *message);
```

### Constants

| Constant | Meaning |
|----------|---------|
| `MKFW_MAX_MONITORS` | size of `mkfw_context.monitors[]` (16) |
| `MKFW_MAX_WINDOWS`  | maximum windows per context (16) |
| `MKFW_RELEASED`     | key/button action: release (passed to callbacks) |
| `MKFW_PRESSED`      | key/button action: press |

Key codes are listed in [`mkfw_keys.h`](../mkfw_keys.h):
`MKFW_KEY_*`, `MKFW_SCANCODE_*`, `MKFW_MOUSE_*`,
`MKFW_CURSOR_*`, `MKFW_MOD_*`.

---

## Initialization and shutdown

### `mkfw_init`

```c
struct mkfw_context *mkfw_init(struct mkfw_options *opts);
```

Open the display, load platform function pointers, query the
monitor list, install the X11/Win32 plumbing the rest of the API
depends on.  Pass `0` for `opts` to use defaults.

Returns the new context, or `0` on failure (call
`mkfw_get_last_error()` for the reason).  The caller owns the
returned pointer and must release it with `mkfw_shutdown`.

The thread that calls `mkfw_init` is the "owning thread" for the
context and every window created against it; see
[Threading model](#threading-model).

### `mkfw_shutdown`

```c
void mkfw_shutdown(struct mkfw_context *ctx);
```

Destroy every window still attached to the context, close the
platform display connection, and release the context itself.
Passing `0` is a no-op.

### `mkfw_query_max_gl_version`

```c
uint32_t mkfw_query_max_gl_version(int32_t *major, int32_t *minor);
```

Probe the driver for the highest OpenGL Compatibility Profile
version it can provide.  Writes the result into `*major` and
`*minor` and returns non-zero on success.

Useful for choosing an appropriate `gl_major` / `gl_minor` for
`mkfw_window_options`.  Safe to call before `mkfw_init` on Win32
and after on Linux (creates a throwaway window internally).

```c
int32_t maj = 0, min = 0;
if(mkfw_query_max_gl_version(&maj, &min)) {
    printf("driver supports up to GL %d.%d\n", maj, min);
}
```

---

## Window creation and lifecycle

### `mkfw_window_create`

```c
struct mkfw_window *mkfw_window_create(struct mkfw_context *ctx,
                                       struct mkfw_window_options *opts);
```

Create a window on `ctx`.  Pass `0` for `opts` to use defaults.

By default the window is mapped and visible immediately.  Pass
`MKFW_WIN_HIDDEN` to defer that to a later `mkfw_window_show`.

Returns the new window, or `0` on failure (consult
`mkfw_get_last_error`).

### `mkfw_window_destroy`

```c
void mkfw_window_destroy(struct mkfw_window *state);
```

Release the GL context (if any), destroy the OS window, and
unlink from the owning context's window list.  Passing `0` is
a no-op.

### `mkfw_window_show`

```c
void mkfw_window_show(struct mkfw_window *state);
```

Map / show the window.  Pair with `MKFW_WIN_HIDDEN` for the
common "create hidden, configure, then show" pattern.

### `mkfw_window_hide`

```c
void mkfw_window_hide(struct mkfw_window *state);
```

Unmap / hide the window without destroying it.

### `mkfw_window_should_close`

```c
uint32_t mkfw_window_should_close(struct mkfw_window *state);
```

Non-zero once the user has requested close (WM close button,
Alt-F4, etc.).  Reset with `mkfw_window_set_should_close`.

### `mkfw_window_set_should_close`

```c
void mkfw_window_set_should_close(struct mkfw_window *state, int32_t value);
```

Override the close flag.  Pass non-zero to request close from the
application, or zero to cancel a pending close (e.g. after
prompting the user to save).

---

## Window attributes

### `mkfw_window_set_title`

```c
void mkfw_window_set_title(struct mkfw_window *state, const char *title);
```

Replace the window title (UTF-8).

### `mkfw_window_set_size`

```c
void mkfw_window_set_size(struct mkfw_window *state, int32_t width, int32_t height);
```

Resize the client area to `width` x `height` physical pixels.

### `mkfw_window_get_framebuffer_size`

```c
void mkfw_window_get_framebuffer_size(struct mkfw_window *state, int32_t *width, int32_t *height);
```

Return the current client-area size in physical pixels.  Either
out pointer may be `0`.

### `mkfw_window_set_position`

```c
void mkfw_window_set_position(struct mkfw_window *state, int32_t x, int32_t y);
```

Move the window's top-left corner to the given screen
coordinates.

### `mkfw_window_get_position`

```c
void mkfw_window_get_position(struct mkfw_window *state, int32_t *x, int32_t *y);
```

Read the window's current top-left position.  Either out pointer
may be `0`.

### `mkfw_window_set_size_limits`

```c
void mkfw_window_set_size_limits(struct mkfw_window *state,
                                  int32_t min_w, int32_t min_h,
                                  int32_t max_w, int32_t max_h);
```

Constrain the user-resizable range.  Pass `0` for any field to
leave that bound unset (no minimum / no maximum).

### `mkfw_window_set_aspect_ratio`

```c
void mkfw_window_set_aspect_ratio(struct mkfw_window *state, int32_t num, int32_t den);
```

Lock the aspect ratio to `num:den`.  Pass `(0, 0)` to clear the
constraint.

### `mkfw_window_set_resizable`

```c
void mkfw_window_set_resizable(struct mkfw_window *state, int32_t resizable);
```

Allow (`!= 0`) or disallow (`0`) the user resizing the window
interactively.  When set non-resizable on Linux, the WM hints lock
to the current size; when set resizable, any previously-set
size-limit / aspect-ratio constraints are reapplied.

### `mkfw_window_set_decorated`

```c
void mkfw_window_set_decorated(struct mkfw_window *state, int32_t decorated);
```

Show (`!= 0`) or hide (`0`) the OS title bar and borders.
Implemented via `_MOTIF_WM_HINTS` on Linux and
`SetWindowLong(GWL_STYLE)` on Win32.

### `mkfw_window_set_opacity`

```c
void mkfw_window_set_opacity(struct mkfw_window *state, float opacity);
```

Set window opacity from `0.0` (fully transparent) to `1.0` (fully
opaque).  Distinct from `MKFW_WIN_TRANSPARENT`, which enables
per-pixel alpha blending of the framebuffer; this controls the
whole window's blend factor.

### `mkfw_window_set_icon`

```c
void mkfw_window_set_icon(struct mkfw_window *state,
                          int32_t width, int32_t height,
                          const uint8_t *rgba);
```

Set the taskbar / window icon from a top-down 32-bit RGBA pixel
buffer.  The buffer is consumed during the call.

### `mkfw_window_set_fullscreen`

```c
void mkfw_window_set_fullscreen(struct mkfw_window *state, int32_t enable);
```

Toggle fullscreen.  On Linux uses `_NET_WM_STATE_FULLSCREEN`; on
Win32 swaps to `WS_POPUP` and resizes to the monitor work area.
Does not touch cursor visibility or lock; use
`mkfw_window_set_cursor_visible` / `_set_cursor_locked`
separately.

### `mkfw_window_minimize` / `_maximize` / `_restore`

```c
void mkfw_window_minimize(struct mkfw_window *state);
void mkfw_window_maximize(struct mkfw_window *state);
void mkfw_window_restore(struct mkfw_window *state);
```

Iconify, zoom, or return the window to its normal state.

### `mkfw_window_request_attention`

```c
void mkfw_window_request_attention(struct mkfw_window *state);
```

Flash the taskbar entry / decorate the window to alert the user.
Linux: `_NET_WM_STATE_DEMANDS_ATTENTION`.  Win32: `FlashWindowEx`.

### `mkfw_window_get_content_scale`

```c
float mkfw_window_get_content_scale(struct mkfw_window *state);
```

Return the OS-reported DPI scale (1.0 = 96 DPI, 2.0 = Retina /
200%).  See [HiDPI contract](#hidpi-contract) for what this does
*not* mean.

---

## Window state queries

### `mkfw_window_is_minimized` / `_is_maximized`

```c
uint32_t mkfw_window_is_minimized(struct mkfw_window *state);
uint32_t mkfw_window_is_maximized(struct mkfw_window *state);
```

Non-zero if the window is currently iconified / zoomed.

---

## Rendering and OpenGL

### `mkfw_window_attach_context` / `_detach_context`

```c
void mkfw_window_attach_context(struct mkfw_window *state);
void mkfw_window_detach_context(struct mkfw_window *state);
```

Bind / unbind the window's GL context on the calling thread.  A
GL context can be current on only one thread at a time; for
threaded rendering, call `_detach_context` on the main thread
then `_attach_context` on the render thread.  See
`examples/threaded.c`.

### `mkfw_window_swap_buffers`

```c
void mkfw_window_swap_buffers(struct mkfw_window *state);
```

Present the back buffer.  Call on whichever thread currently has
the context attached.

### `mkfw_window_set_swap_interval` / `_get_swap_interval`

```c
void    mkfw_window_set_swap_interval(struct mkfw_window *state, uint32_t interval);
int32_t mkfw_window_get_swap_interval(struct mkfw_window *state);
```

Set or read VSync.  `0` = uncapped, `1` = wait for one refresh,
`-1` = adaptive (where supported).  Uses
`glXSwapIntervalEXT` / `wglSwapIntervalEXT` under the hood.

---

## Event pumping

### `mkfw_poll_events`

```c
void mkfw_poll_events(struct mkfw_context *ctx);
```

Drain the event queue.  Dispatches every pending event to every
window on the context: key callbacks, mouse callbacks, resize,
focus, drag-and-drop, close requests, ...  Call once per frame
at the top of the render loop.

### `mkfw_wait_events`

```c
void mkfw_wait_events(struct mkfw_context *ctx);
```

Block until at least one event arrives, then dispatch it (and
any others that piled up while you were waiting).  Useful for
event-driven UI where you do not need a fixed framerate.

### `mkfw_wait_events_timeout`

```c
void mkfw_wait_events_timeout(struct mkfw_context *ctx, uint64_t nanoseconds);
```

Block up to `nanoseconds`, then dispatch.  A timeout of 0 is
equivalent to `mkfw_poll_events`.

---

## Keyboard input

mkfw maintains two parallel views of the keyboard:

- **`keyboard_state[MKFW_KEY_*]`**: text-layer keys (the letter
  the user thinks they pressed).  Affected by layout / locale.
- **`scancode_state[MKFW_SCANCODE_*]`**: physical key positions.
  Independent of layout.  See [Scancodes](#scancodes).

After each `mkfw_poll_events` call, both arrays reflect the
current state.  Call `mkfw_window_update_input_state` once per
frame to snapshot them into the `prev_*` arrays so the
edge-detection helpers work.

### `mkfw_window_update_input_state`

```c
void mkfw_window_update_input_state(struct mkfw_window *state);
```

Copy `keyboard_state` -> `prev_keyboard_state`, `scancode_state`
-> `prev_scancode_state`, and `mouse_buttons` ->
`previous_mouse_buttons`.  Call once per frame, **after**
handling input, before the next pump.

### Polling helpers

```c
uint32_t mkfw_window_is_key_pressed   (struct mkfw_window *, uint8_t key);
uint32_t mkfw_window_was_key_released (struct mkfw_window *, uint8_t key);
```

`_is_key_pressed` is the rising-edge query (held now, not held
last frame).  `_was_key_released` is the falling edge.  For
"is currently held," index `state->keyboard_state[key]` directly.

### `mkfw_window_get_modifiers`

```c
uint32_t mkfw_window_get_modifiers(struct mkfw_window *state);
```

Return the current modifier bitmask derived from `keyboard_state`:
OR'd `MKFW_MOD_LSHIFT`, `_RSHIFT`, `_SHIFT`, `_LCTRL`, `_RCTRL`,
`_CTRL`, `_LALT`, `_RALT`, `_ALT`, `_LSUPER`, `_RSUPER`, `_SUPER`.

### `mkfw_get_key_name`

```c
const char *mkfw_get_key_name(uint32_t key);
```

Return a short human-readable name for a `MKFW_KEY_*` code
("Space", "Escape", "F1", "Left Shift", ...).  Useful for rebind
UIs.  Returns `"Unknown"` for unrecognized values.

---

## Scancodes

Scancodes identify a key by its **physical location** on the
keyboard, independent of locale or layout.  Use them when binding
movement keys ("the key in the W position regardless of
QWERTY/AZERTY"); use `MKFW_KEY_*` keysyms for the text meaning of
input.

mkfw exposes `MKFW_SCANCODE_*` values matching USB HID Usage
Page 7 codes (e.g. `MKFW_SCANCODE_W == 0x1a`), and maintains
`state->scancode_state[256]` / `state->prev_scancode_state[256]`
parallel to `keyboard_state`.

```c
uint32_t mkfw_window_is_scancode_down      (struct mkfw_window *, uint8_t scancode);
uint32_t mkfw_window_is_scancode_pressed   (struct mkfw_window *, uint8_t scancode);
uint32_t mkfw_window_was_scancode_released (struct mkfw_window *, uint8_t scancode);
```

`_is_scancode_down` is the held query; `_is_scancode_pressed` /
`_was_scancode_released` are the rising / falling edge.

**Example (WASD on AZERTY):**

```c
if(mkfw_window_is_scancode_down(win, MKFW_SCANCODE_W)) { player.y += dt; }
if(mkfw_window_is_scancode_down(win, MKFW_SCANCODE_S)) { player.y -= dt; }
```

The same code runs on AZERTY (where the physical key labelled
"Z" is in the W position) and QWERTY (where it is labelled "W").

---

## Mouse input

### Direct state

```c
state->mouse_buttons[MKFW_MOUSE_*]      // 1 = held
state->previous_mouse_buttons[MKFW_MOUSE_*]
state->mouse_x, state->mouse_y          // last absolute position, physical pixels
state->mouse_in_window                  // 1 while cursor is over the client area
```

### Polling helpers

```c
uint32_t mkfw_window_is_button_pressed   (struct mkfw_window *, uint8_t button);
uint32_t mkfw_window_was_button_released (struct mkfw_window *, uint8_t button);
```

Edge-detected button queries.  `button` is a `MKFW_MOUSE_*` index
(0..4).

### `mkfw_window_set_mouse_sensitivity`

```c
void mkfw_window_set_mouse_sensitivity(struct mkfw_window *state, double sensitivity);
```

Multiplier applied to raw mouse deltas before they are
accumulated.  `1.0` is the platform default.

### `mkfw_window_get_and_clear_mouse_delta`

```c
void mkfw_window_get_and_clear_mouse_delta(struct mkfw_window *state, int32_t *dx, int32_t *dy);
```

Read and reset the accumulated relative-motion delta since the
last call.  Fractional sub-pixel motion is preserved across calls
for smooth integration.  Either out pointer may be `0`.

This is the polling alternative to the
`mkfw_mouse_move_delta_callback_t` callback; they're driven by
the same accumulator.

### `mkfw_window_get_cursor_position` / `_set_cursor_position`

```c
void mkfw_window_get_cursor_position(struct mkfw_window *state, int32_t *x, int32_t *y);
void mkfw_window_set_cursor_position(struct mkfw_window *state, int32_t x, int32_t y);
```

Read / write the cursor's absolute position in client-area
coordinates (physical pixels).  Setting the position warps the
pointer on the OS side.  Either out pointer in the getter may be
`0`.

---

## Cursor

mkfw separates **visibility** (whether the OS cursor is drawn)
from **lock** (whether the pointer is grabbed and confined to the
client area).  Both toggles are independent.

| visible | locked | typical use |
|---------|--------|-------------|
| 1 | 0 | default; UI, drawing tool not in capture mode |
| 0 | 1 | FPS-style mouse look (window-centred each frame) |
| 1 | 1 | drawing tool with snap-to-window, cursor visible |
| 0 | 0 | hide cursor while idle in a fullscreen presenter |

### `mkfw_window_set_cursor_visible` / `_is_cursor_visible`

```c
void     mkfw_window_set_cursor_visible(struct mkfw_window *state, uint32_t visible);
uint32_t mkfw_window_is_cursor_visible (struct mkfw_window *state);
```

Show or hide the OS cursor inside the window's client area.

### `mkfw_window_set_cursor_locked` / `_is_cursor_locked`

```c
void     mkfw_window_set_cursor_locked(struct mkfw_window *state, uint32_t locked);
uint32_t mkfw_window_is_cursor_locked (struct mkfw_window *state);
```

Grab the pointer and confine it to the client area.  When locked,
relative motion is reported through the raw-delta callback /
`mkfw_window_get_and_clear_mouse_delta`.  mkfw only re-centres
the cursor each frame when **locked AND hidden** (FPS-style use);
visible-locked leaves the cursor wherever the user is drawing.

### `mkfw_window_set_cursor_shape`

```c
void mkfw_window_set_cursor_shape(struct mkfw_window *state, uint32_t cursor);
```

Set the stock cursor shape.  `cursor` is one of:

`MKFW_CURSOR_ARROW`, `MKFW_CURSOR_TEXT_INPUT`,
`MKFW_CURSOR_RESIZE_ALL`, `MKFW_CURSOR_RESIZE_NS`,
`MKFW_CURSOR_RESIZE_EW`, `MKFW_CURSOR_RESIZE_NESW`,
`MKFW_CURSOR_RESIZE_NWSE`, `MKFW_CURSOR_HAND`,
`MKFW_CURSOR_NOT_ALLOWED`.

Out-of-range values are clamped to `MKFW_CURSOR_ARROW`.  Setting
a stock shape clears any custom cursor previously installed with
`mkfw_window_set_custom_cursor`.

### Custom RGBA cursors

```c
struct mkfw_cursor *mkfw_cursor_create_rgba(struct mkfw_context *ctx,
                                            uint32_t width, uint32_t height,
                                            uint8_t *rgba,
                                            int32_t hotspot_x, int32_t hotspot_y);
void mkfw_cursor_destroy(struct mkfw_context *ctx, struct mkfw_cursor *cursor);
void mkfw_window_set_custom_cursor(struct mkfw_window *state, struct mkfw_cursor *cursor);
```

Build a cursor from a top-down 32-bit RGBA image plus a
hotspot (the pixel offset, from the top-left, that the OS treats
as the click point).  Linux uses `libXcursor.so.1` (dlopen'd; if
missing, `mkfw_cursor_create_rgba` returns `0`); Win32 uses
`CreateIconIndirect` on a 32-bit BGRA DIB section.

Cursors are owned by the caller and outlive any window they are
attached to.  Pass `0` to `mkfw_window_set_custom_cursor` to
revert to the stock shape last set with
`mkfw_window_set_cursor_shape`.

---

## Callbacks

Each window has slots for the following callbacks.  The setters
are inline and lock-free; pass `0` to remove a callback.

### Setters

```c
void mkfw_window_set_user_data    (struct mkfw_window *, void *user_data);
void *mkfw_window_get_user_data   (struct mkfw_window *);
void mkfw_window_set_key_callback              (struct mkfw_window *, mkfw_key_callback_t);
void mkfw_window_set_char_callback             (struct mkfw_window *, mkfw_char_callback_t);
void mkfw_window_set_scroll_callback           (struct mkfw_window *, mkfw_scroll_callback_t);
void mkfw_window_set_mouse_move_delta_callback (struct mkfw_window *, mkfw_mouse_move_delta_callback_t);
void mkfw_window_set_mouse_button_callback     (struct mkfw_window *, mkfw_mouse_button_callback_t);
void mkfw_window_set_framebuffer_size_callback (struct mkfw_window *, mkfw_framebuffer_callback_t);
void mkfw_window_set_focus_callback            (struct mkfw_window *, mkfw_focus_callback_t);
void mkfw_window_set_drop_callback             (struct mkfw_window *, mkfw_drop_callback_t);
void mkfw_window_set_state_callback            (struct mkfw_window *, mkfw_window_state_callback_t);
```

### Callback shapes

| Callback | Signature | Fires when |
|----------|-----------|-----------|
| key | `(win, key, action, modifier_bits)` | a key transitions; `action` is `MKFW_PRESSED` or `MKFW_RELEASED`; `modifier_bits` is OR'd `MKFW_MOD_*` |
| char | `(win, codepoint)` | a text input event produces a printable Unicode codepoint |
| scroll | `(win, xoffset, yoffset)` | scroll wheel or touchpad scroll; offsets in scroll-tick units |
| mouse move delta | `(win, dx, dy)` | relative pointer motion; uses XInput2 raw motion on Linux, Raw Input on Win32 |
| mouse button | `(win, button, action)` | mouse button press / release |
| framebuffer size | `(win, width, height, aspect)` | the client-area pixel size changed |
| focus | `(win, focused)` | window gained or lost keyboard focus |
| drop | `(count, paths)` | file drag-and-drop; see [File drop](#file-drop) |
| window state | `(win, maximized, minimized)` | maximize/restore/minimize transitions |

`user_data` is unused by mkfw; install your application's
context pointer with `mkfw_window_set_user_data` and read it back
with `mkfw_window_get_user_data`.

---

## Clipboard

### `mkfw_window_set_clipboard_text`

```c
void mkfw_window_set_clipboard_text(struct mkfw_window *state, const char *text);
```

Replace the system clipboard with the given UTF-8 string.  Pass
`0` to clear.

### `mkfw_window_get_clipboard_text`

```c
char *mkfw_window_get_clipboard_text(struct mkfw_window *state);
```

Return the clipboard text as a malloc'd UTF-8 NUL-terminated
string.  **The caller must release it with `free()`**.  Returns
`0` if the clipboard is empty or unavailable.

Linux blocks up to ~500 ms waiting on the selection owner; Win32
uses `GetClipboardData(CF_UNICODETEXT)` + UTF-16 -> UTF-8.

---

## File drop

Drag-and-drop is opt-in: register a drop callback with
`mkfw_window_set_drop_callback`, which implicitly enables drop
acceptance.  To unregister, set the callback to `0`.

`mkfw_window_enable_drop` can be used separately to toggle
acceptance without changing the callback:

```c
void mkfw_window_enable_drop(struct mkfw_window *state, uint8_t enable);
```

### Drop-callback contract

The callback receives a malloc'd array of malloc'd UTF-8 path
strings.  **Ownership passes to the callback**; release with
`mkfw_drop_paths_free` (or free each path and the array
yourself):

```c
void mkfw_drop_paths_free(uint32_t count, const char **paths);

void on_drop(uint32_t count, const char **paths) {
    for(uint32_t i = 0; i < count; ++i) {
        printf("Dropped: %s\n", paths[i]);
    }
    mkfw_drop_paths_free(count, paths);
}
mkfw_window_set_drop_callback(win, on_drop);
```

Platform details: Linux implements XDND v5 accepting
`text/uri-list`; Win32 uses `DragAcceptFiles` + `WM_DROPFILES`.

---

## Monitors

The context caches the monitor list at init time.  Re-query at
any time:

### `mkfw_get_monitors`

```c
int32_t mkfw_get_monitors(struct mkfw_context *ctx,
                          struct mkfw_monitor *out, int32_t max);
```

Fill up to `max` entries into the caller-supplied `out` array
and return the number written.  Use `MKFW_MAX_MONITORS` as a safe
upper bound:

```c
struct mkfw_monitor mons[MKFW_MAX_MONITORS];
int32_t n = mkfw_get_monitors(ctx, mons, MKFW_MAX_MONITORS);
for(int32_t i = 0; i < n; ++i) {
    printf("%s %dx%d @ %dHz%s\n", mons[i].name,
           mons[i].width, mons[i].height,
           mons[i].refresh_rate,
           mons[i].primary ? " (primary)" : "");
}
```

The cached list in `ctx->monitors[]` is updated by the same code
path; the array is caller-owned and mkfw does not allocate.

---

## Native handle escape

### `mkfw_window_get_native_handles`

```c
void mkfw_window_get_native_handles(struct mkfw_window *state,
                                    struct mkfw_native_handles *out);
```

Fill `*out` with the underlying platform handles.  Use this to
integrate with APIs mkfw does not own:

- **Vulkan**: `vkCreateXlibSurfaceKHR(display, window, ...)` on
  Linux, `vkCreateWin32SurfaceKHR(hinstance, hwnd, ...)` on Win32.
- **EGL**: pair the Linux `Display *` with `eglGetDisplay`.
- **Direct2D / GDI**: the Win32 `HWND` is a valid render target
  parent.

The handles are valid until the window is destroyed.  See the
[Core types](#core-types) section for the cast types.

---

## Time

### `mkfw_get_time`

```c
uint64_t mkfw_get_time(void);
```

Monotonic time in nanoseconds since an arbitrary platform epoch.
Linux: `clock_gettime(CLOCK_MONOTONIC)`.  Win32:
`QueryPerformanceCounter` scaled by the cached frequency.

### `mkfw_sleep`

```c
void mkfw_sleep(uint64_t nanoseconds);
```

Sleep the calling thread for at least the given number of
nanoseconds.  Linux: `nanosleep`.  Win32: a one-shot
`CreateWaitableTimer`.

For tight frame-pacing loops use the optional timer subsystem
instead; see [MKFW_TIMER_API.md](MKFW_TIMER_API.md).

---

## Error reporting

mkfw never writes to `stdout` / `stderr`.  Failures surface
through two parallel channels:

1. A per-thread last-error buffer, readable with
   `mkfw_get_last_error()`.
2. An optional callback installed by
   `mkfw_set_error_callback`.

Both are populated together by every internal failure site.

### `mkfw_get_last_error`

```c
const char *mkfw_get_last_error(void);
```

Return the most recent error string on the calling thread, or
`0` if no error has been recorded (or after
`mkfw_clear_last_error`).  The buffer is thread-local and
mkfw-owned; do not free.

### `mkfw_clear_last_error`

```c
void mkfw_clear_last_error(void);
```

Reset the per-thread last-error to "no error".

### `mkfw_set_error_callback`

```c
void mkfw_set_error_callback(mkfw_error_callback_t callback);
```

Install a callback that fires each time mkfw records an error.
The callback receives the same string subsequently visible from
`mkfw_get_last_error`.  Pass `0` to remove.

---

## Threading model

`mkfw_init` is the **owning thread**.  All mkfw calls on the
context (`mkfw_poll_events`, `mkfw_get_monitors`, error callback
firing) and on its windows (`mkfw_window_set_*`,
`mkfw_window_show`, etc.) must run on that thread.  Calls onto
the context from other threads are undefined.

Exceptions:

- **GL rendering**: `mkfw_window_swap_buffers` runs on whichever
  thread currently holds the GL context current via
  `mkfw_window_attach_context`.  The threaded-rendering pattern
  in `examples/threaded.c` shows the supported way to put
  rendering on a second thread.
- **Audio callback**: fires on the audio thread.  Do not touch
  GL, X11, or Win32 windowing from the audio callback.  See
  [MKFW_AUDIO_API.md](MKFW_AUDIO_API.md).
- **Timer**: timer callbacks fire on the thread that calls
  `mkfw_timer_wait`.  See [MKFW_TIMER_API.md](MKFW_TIMER_API.md).
- **Joystick callbacks**: fire inside `mkfw_poll_events(ctx)`,
  so they share the owning thread.
- **Input-state reads**: reading
  `state->keyboard_state[k]`, `state->scancode_state[k]`, or
  `state->mouse_buttons[b]` from another thread is byte-atomic
  on x86-64 and eventually consistent with the most recent
  `mkfw_poll_events`.
- **Last-error**: `mkfw_get_last_error` / `_clear_last_error`
  are per-thread and safe to call from any thread.

---

## Memory ownership

> Any pointer returned from an mkfw function, or passed to a
> callback by mkfw, is owned by the **caller / callback** and
> must be released with `free()` (or the matching
> `mkfw_*_destroy` when one is documented).  mkfw never returns
> library-owned scratch memory.

Specific instances:

- `mkfw_window_get_clipboard_text` returns a malloc'd UTF-8
  string; caller frees with `free()`.
- Drop callback receives a malloc'd array of malloc'd UTF-8
  strings; caller releases with `mkfw_drop_paths_free`.
- `mkfw_cursor_create_rgba` returns a `struct mkfw_cursor *`;
  release with `mkfw_cursor_destroy`.

Two explicit **exceptions**:

- `mkfw_joystick_get_name(idx)` returns a borrowed pointer
  into library state, valid until that pad disconnects.
- `mkfw_get_monitors(ctx, out, max)` writes into a
  caller-supplied array and does not allocate.

**Windows static-CRT note**: link the consuming binary against
the same CRT as the mkfw build, or use mkfw header-only.  The
cross-CRT `free()` mismatch is the only configuration where the
rule above fails.

---

## HiDPI contract

mkfw is built for OpenGL/Vulkan applications, not UI toolkits.
The contract is "the framebuffer you asked for is the
framebuffer you get, in physical pixels."  Anything else is the
application's problem.

- `mkfw_window_options.width` / `.height` are **physical pixels**.
  A 1280 by 720 window is a 1280 by 720 framebuffer regardless
  of the display's DPI scaling.
- `mkfw_window_get_framebuffer_size` returns physical pixels.
  The framebuffer callback delivers physical pixels on resize.
- Cursor position, mouse deltas, scroll offsets, and every other
  integer pixel coordinate in the API live in the same coordinate
  system as the framebuffer (i.e. physical pixels).
- `mkfw_window_get_content_scale` returns the OS's reported
  scale factor as a hint.  Acting on it is the application's
  choice (scaling a UI overlay, font atlas, etc.).  mkfw does
  not scale window decorations, cursor sizes, or pixel content
  for you; the OS may still scale decorations on Win32 because
  `SetProcessDPIAware` is enabled at init time.

This is the GLFW model and is the right one for GL / Vulkan
apps.  Logical pixels are a UI-toolkit concept; mkfw is not a UI
toolkit.

---

## Thread primitives

`mkfw.h` exposes a thin cross-platform thread wrapper used by the
audio / timer subsystems.  Available to applications too.

```c
typedef HANDLE    mkfw_thread;   // Win32
typedef pthread_t mkfw_thread;   // Linux

#define MKFW_THREAD_FUNC(name, arg) DWORD WINAPI name(LPVOID arg)   // Win32
#define MKFW_THREAD_FUNC(name, arg) void *name(void *arg)            // Linux

mkfw_thread mkfw_thread_create(thread_func, void *arg);
void        mkfw_thread_join(mkfw_thread t);
```

`thread_func` matches the platform's native signature
(`LPTHREAD_START_ROUTINE` on Win32, `void *(*)(void *)` on
Linux).  Define the thread entry point with `MKFW_THREAD_FUNC`
so the signature is portable.
