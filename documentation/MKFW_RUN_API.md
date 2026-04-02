# MKFW Run API Documentation

**Cross-Platform Frame Callback for Linux, Windows, and Emscripten**

## Overview

`mkfw_run` is an opt-in alternative to the manual while loop pattern. It exists for users who want their code to compile and run on Emscripten (WebAssembly/browser) in addition to Linux and Windows.

The existing manual loop pattern (threads, timers, full control) is unchanged and remains the recommended approach for desktop-only applications.

## Two Paths

### Path 1: Desktop only (existing pattern, no mkfw_run)

Full control over threads, timers, frame pacing, and the pump loop. Exactly like the threaded example and platform.c. Linux and Windows only.

```c
mkfw_thread render_thread = mkfw_thread_create(render_thread_func, &state);

while(running && !mkfw_should_close(window)) {
    mkfw_pump_messages(window);
    mkfw_sleep(5000000);
}

mkfw_thread_join(render_thread);
```

### Path 2: Cross-platform including Emscripten (mkfw_run)

User provides a frame callback. mkfw handles the platform-specific loop mechanics.

```c
static void frame(struct mkfw_state *window) {
    // game logic + GL draw calls
    mkfw_update_input_state(window);
}

mkfw_run(window, frame);
```

---

## `mkfw_run`

```c
void mkfw_run(struct mkfw_state *state, void (*frame)(struct mkfw_state *))
```

Run the application main loop. Calls the frame callback repeatedly until `mkfw_should_close` returns true.

**Parameters:**
- `state` - Window state pointer returned from `mkfw_init()`
- `frame` - Frame callback function called once per frame

**Notes:**
- mkfw_run handles `mkfw_pump_messages` and `mkfw_swap_buffers` internally. Do NOT call them in your frame callback.
- The user MUST call `mkfw_update_input_state(window)` inside the frame callback for edge detection to work.
- Access your application state via `mkfw_get_user_data(window)` inside the frame callback.

### Per-Platform Behavior

**Linux:** Single-thread while loop. mkfw_run calls pump, frame, swap in sequence. Vsync (set via `mkfw_set_swapinterval` before mkfw_run) paces the loop.

**Windows:** Two threads. mkfw_run spawns a render thread that calls frame() + swap_buffers in a loop. The main thread (where mkfw_run blocks) pumps messages independently. This prevents the window from freezing during resize/move/drag operations.

**Emscripten:** Calls `emscripten_set_main_loop` with `requestAnimationFrame`. The browser calls frame() at the display refresh rate (typically 60Hz). **mkfw_run never returns on Emscripten** -- code after mkfw_run in main() is unreachable (see Emscripten Cleanup below).

---

## Frame Callback Contract

1. Do your game logic and GL draw calls, then return.
2. Do NOT call `mkfw_swap_buffers` -- mkfw_run handles it.
3. Do NOT call `mkfw_pump_messages` -- mkfw_run handles it.
4. Do NOT call `mkfw_timer_wait` or any blocking function.
5. DO call `mkfw_update_input_state(window)` at the end of your callback.
6. Your frame callback may run on a **render thread** (Windows) or the **main thread** (Linux/Emscripten).
7. Window event callbacks (key, mouse, framebuffer_size, focus) always fire on the main thread.
8. **Shared state between callbacks and frame() needs atomics/volatiles** -- same as the threaded rendering pattern.
9. Pacing: vsync on desktop (set with `mkfw_set_swapinterval` before mkfw_run), `requestAnimationFrame` on Emscripten.

---

## MKFW_DESKTOP Define

```c
// Set automatically by mkfw.h
#if defined(_WIN32) || (defined(__linux__) && !defined(__EMSCRIPTEN__))
#define MKFW_DESKTOP
#endif
```

Use this to guard desktop-only code:

```c
#ifdef MKFW_DESKTOP
    struct mkfw_monitor monitors[MKFW_MAX_MONITORS];
    int32_t count = mkfw_get_monitors(window, monitors, MKFW_MAX_MONITORS);
    // pick resolution, etc.
#endif
```

Desktop-only functions compile as no-ops on Emscripten (return 0/NULL/empty), so forgetting an `#ifdef` never breaks the build.

---

## Emscripten Constraints

When using mkfw_run with Emscripten:

- **No custom frame rates.** The browser controls pacing via `requestAnimationFrame` (typically ~60Hz or display refresh rate). You cannot do 50Hz, 480Hz, or any arbitrary rate.
- **No blocking.** `mkfw_timer_wait`, `mkfw_sleep`, and any blocking calls return immediately or are no-ops on Emscripten.
- **No user-managed threads.** `mkfw_thread_create` is a no-op stub.
- **Frame rate cannot exceed display refresh.** You can skip frames for lower rates but not go higher.
- **OpenGL ES 3.0 only.** WebGL2 = GL ES 3.0. No Compatibility Profile, no immediate mode (glBegin/glEnd), no GL 4.x features.

---

## Compile Errors on Emscripten

These functions produce a compile error if called when targeting Emscripten, because they indicate a fundamental misunderstanding of the target platform:

- `mkfw_set_gl_version` -- WebGL2 is fixed at ES 3.0
- `mkfw_query_max_gl_version` -- same reason
- `mkfw_set_transparent` -- per-pixel alpha compositing doesn't exist on canvas
- `mkfw_attach_context` -- single-threaded, single context
- `mkfw_detach_context` -- same

---

## No-Op Functions on Emscripten

These compile and link on Emscripten but do nothing (or return sensible defaults):

| Function | Emscripten behavior |
|---|---|
| `mkfw_get_monitors` | Returns 1 monitor with canvas dimensions |
| `mkfw_set_window_position` / `get` | No-op / returns 0,0 |
| `mkfw_maximize/minimize/restore_window` | No-op |
| `mkfw_is_minimized` / `is_maximized` | Returns 0 |
| `mkfw_set_window_decorated` | No-op |
| `mkfw_set_window_opacity` | No-op |
| `mkfw_set_window_icon` | No-op |
| `mkfw_set_window_resizable` | No-op |
| `mkfw_set_cursor_position` | No-op (browsers block this) |
| `mkfw_hide_window` | No-op |
| `mkfw_set_clipboard_text` / `get` | No-op / returns "" |
| `mkfw_set_drop_callback` | No-op |
| `mkfw_sleep` | No-op |
| `mkfw_pump_messages` | No-op (browser dispatches events) |
| `mkfw_swap_buffers` | No-op (browser composits) |
| `mkfw_timer_init` / `shutdown` | No-op |
| `mkfw_timer_new` | Returns NULL |
| `mkfw_timer_wait` | Returns immediately |
| `mkfw_timer_set_interval` / `destroy` | No-op |
| `mkfw_fullscreen` | Works, but requires user gesture (keypress/click) |

---

## Emscripten Cleanup

`mkfw_run` never returns on Emscripten (`emscripten_set_main_loop` with `simulate_infinite_loop=1`). Code after `mkfw_run` in `main()` is unreachable. Use `atexit()` for cleanup:

```c
static struct mkfw_state *g_window;

static void cleanup(void) {
    mkfw_cleanup(g_window);
}

int main(void) {
    g_window = mkfw_init(1280, 720);
    atexit(cleanup);

    // ... setup ...

    mkfw_run(g_window, frame);

    mkfw_cleanup(g_window);  // reached on desktop, unreachable on Emscripten
    return 0;
}
```

---

## Example

See `mkfw_run_example/mkfw_run_example.c` for a complete working example.

```c
static void frame(struct mkfw_state *window) {
    struct app_state *app = (struct app_state *)mkfw_get_user_data(window);

    if(mkfw_is_key_pressed(window, MKS_KEY_ESCAPE)) {
        mkfw_set_should_close(window, 1);
    }
    if(mkfw_is_key_pressed(window, MKS_KEY_F11)) {
        app->fullscreen = !app->fullscreen;
        mkfw_fullscreen(window, app->fullscreen);
    }

    // render
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    mkfw_update_input_state(window);
}

int main(void) {
    struct mkfw_state *window = mkfw_init(1280, 720);
    mkfw_gl_loader();
    mkfw_set_swapinterval(window, 1);
    mkfw_show_window(window);

    mkfw_run(window, frame);

    mkfw_cleanup(window);
    return 0;
}
```

---

## Build

```bash
# Desktop (Linux)
gcc app.c -lX11 -lXi -lXrandr -lGL -lm -lpthread -o app

# Desktop (Windows)
gcc app.c -lopengl32 -lgdi32 -lwinmm -o app.exe

# Emscripten (code must use mkfw_run)
emcc app.c -sUSE_WEBGL2=1 -sFULL_ES3=1 -o app.html
```

---

## See Also

- `MKFW_API.md` -- Main MKFW windowing API (manual loop pattern)
- `MKFW_AUDIO_API.md` -- Audio subsystem documentation
- `MKFW_JOYSTICK_API.md` -- Joystick subsystem documentation
- `mkfw_run_example/` -- Working example with desktop and Emscripten build scripts

---

## License

MIT License - See source files for full license text.
