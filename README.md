# mkfw

A minimal windowing and input library for OpenGL applications on Linux (X11) and Windows (Win32).

mkfw is what I use in my own projects. It covers similar ground to GLFW — window creation, input handling, OpenGL context management — but it's a solo effort without the test coverage or ecosystem that GLFW has. It works well for me, and you're welcome to use it, but I'd suggest evaluating it with that in mind.

## Features

**Core:**
- Window creation and management (resize, fullscreen, aspect ratio, min size)
- OpenGL Compatibility Profile context creation (configurable version, default 3.1)
- Keyboard input with key press/release edge detection
- Mouse input with raw motion deltas and configurable sensitivity
- Unicode text input via character callbacks
- VSync control
- Multiple windows
- Cross-platform threading primitives

**Optional subsystems** (enabled by including the matching companion header):

| Header | Subsystem | Description |
|--------|-----------|-------------|
| `mkfw_joystick.h` | Joystick | Up to 4 gamepads, hotplug, analog axes, buttons, d-pad. Define `MKFW_JOYSTICK_GAMEDB` before including to also pull in the SDL GameController DB mappings. |
| `mkfw_audio.h` | Audio | Low-latency callback-based audio output (WASAPI / ALSA) |
| `mkfw_timer.h` | Timer | High-precision timing with sleep+spin strategy |

## Platforms

| Platform | Window System | GL Context | Input |
|----------|---------------|------------|-------|
| Linux | X11 | GLX | XInput2 (mouse), evdev (joystick) |
| Windows | Win32 | WGL | Raw Input (mouse), XInput (joystick) |

## Quick start

mkfw is a set of headers and source files you include directly into your project — no build system or external dependencies required.

```c
#include "mkfw_gl_loader.h"
#include "mkfw.h"

int main(void) {
    struct mkfw_context *ctx = mkfw_init(0);
    struct mkfw_window_options wopts = {
        .width = 1280, .height = 720, .title = "Hello mkfw",
    };
    struct mkfw_window *window = mkfw_window_create(ctx, &wopts);

    mkfw_gl_loader();

    while (!mkfw_window_should_close(window)) {
        mkfw_poll_events(ctx);

        if (window->keyboard_state[MKFW_KEY_ESCAPE])
            mkfw_window_set_should_close(window, 1);

        // render ...

        mkfw_window_swap_buffers(window);
    }

    mkfw_window_destroy(window);
    mkfw_shutdown(ctx);
    return 0;
}
```

Compile on Linux:

```sh
gcc main.c -lm -ldl -o app
```

Compile on Windows (MinGW):

```sh
gcc main.c -lopengl32 -lgdi32 -lwinmm -o app.exe
```

Compile on Windows (clang-cl):

```sh
clang-cl main.c opengl32.lib gdi32.lib winmm.lib user32.lib shell32.lib
```

MinGW implicitly links `user32` and `shell32`. clang-cl requires them explicitly. When using optional subsystems, additional libraries are needed -- see the subsystem documentation for details.

## Optional subsystems

Each subsystem has its own companion header.  Include it in addition to `mkfw.h`:

```c
#include "mkfw.h"
#include "mkfw_audio.h"
#include "mkfw_timer.h"

#define MKFW_JOYSTICK_GAMEDB   // optional: also pull in SDL gamedb
#include "mkfw_joystick.h"
```

The subsystem headers are unity-build companion files (they `#include` the platform `.c` source directly).  No separate compilation step is required.

## OpenGL version

By default mkfw creates an OpenGL 3.1 Compatibility Profile context. You can request a higher version for features like compute shaders (4.3+) while keeping immediate mode available:

```c
// Query what the driver supports
int32_t major, minor;
mkfw_query_max_gl_version(&major, &minor);

// Request a specific version via the window options struct
struct mkfw_context *ctx = mkfw_init(0);
struct mkfw_window_options wopts = {
    .width = 1280, .height = 720,
    .gl_major = 4, .gl_minor = 6,
};
struct mkfw_window *window = mkfw_window_create(ctx, &wopts);
```

See [MKFW_API.md](documentation/MKFW_API.md#opengl-version-configuration) for details.

## Examples

Working examples are included in the repository:

- [examples/joystick.c](examples/joystick.c) — gamepad input with terminal visualization
- [examples/threaded.c](examples/threaded.c) — render loop on a separate thread, decoupled from the OS message pump
- [examples/monitor.c](examples/monitor.c) — monitor enumeration
- [examples/transparency.c](examples/transparency.c) — per-pixel transparency

Build all examples:

```sh
cd examples && bash build_examples.sh
```

### Threaded rendering

On Windows, dragging or resizing a window causes `DispatchMessage()` to enter a modal loop inside the OS. It doesn't return until the user releases the mouse. If your rendering happens on the same thread, frames freeze for the entire duration of the drag or resize.

The fix is to run rendering on a separate thread:

```
Main thread:     create window → detach GL context → pump messages in a loop
Render thread:   attach GL context → render loop (runs independently)
```

The key steps:

1. Create the window and set up callbacks on the main thread as usual
2. Call `mkfw_window_detach_context()` to release the GL context (a context can only be current on one thread at a time)
3. Spawn a render thread that calls `mkfw_window_attach_context()`, then runs your normal render loop
4. The main thread loops on `mkfw_poll_events()` + `mkfw_sleep()` — nothing else
5. On shutdown, set a shared flag, join the render thread, then clean up

This pattern is not specific to mkfw — the same approach works with GLFW, SDL, or raw Win32. [examples/threaded.c](examples/threaded.c) shows a complete, minimal implementation.

## Documentation

Detailed API documentation for each subsystem is in the [documentation/](documentation/) directory:

- [MKFW_API.md](documentation/MKFW_API.md) — core window, input, and context management
- [MKFW_AUDIO_API.md](documentation/MKFW_AUDIO_API.md) — audio output
- [MKFW_TIMER_API.md](documentation/MKFW_TIMER_API.md) — high-precision timing
- [MKFW_JOYSTICK_API.md](documentation/MKFW_JOYSTICK_API.md) — gamepad input

## OpenGL function loader

`mkfw_gl_loader.h` is an optional, freestanding OpenGL function loader included in the repository. It has no dependency on mkfw — you can use it in any project, or swap it for your own loader (or the full `GL/gl.h`).

It provides all GL types, constants, and function declarations up to OpenGL 4.6, version-gated at compile time. By default it includes everything up to GL 3.1. To select a different version, define `MKFW_GL_VERSION` before including:

```c
#define MKFW_GL_VERSION 46   // GL 4.6 (major*10 + minor)
#include "mkfw_gl_loader.h"
#include "mkfw.h"

// after mkfw_init + mkfw_window_show:
mkfw_gl_loader();
```

## Dependencies

None beyond platform libraries:

- **Linux:** m, dl (all standard). Platform libraries (X11, GL, Xi, Xrandr, ALSA) are loaded at runtime via dlopen, so they are not link-time dependencies. X11/GL/ALSA development headers are still needed at compile time.
- **Windows:** opengl32, gdi32, winmm (all standard). clang-cl additionally requires explicit linking of user32 and shell32, which MinGW links implicitly. Audio subsystem adds ole32, avrt, and uuid.

## License

[MIT](LICENSE)
