# mkfw

A minimal windowing and input library for OpenGL applications on Linux (X11) and Windows (Win32).

mkfw is what I use in my own projects.  It covers similar ground
to GLFW — window creation, input handling, OpenGL context
management — and ships with a small smoke test suite that
exercises every subsystem.  It's still a solo effort, so the
ecosystem (third-party bindings, integrations, distro packages)
isn't there yet; evaluate accordingly if those matter to you.

## Updating from 0.1.0

The API was refactored after the `0.1.0` tag and the change is
not source-compatible.  Previously `mkfw_init()` initialised
the library and created a window in a single call, returning a
per-window context.  The current API splits that responsibility:
`mkfw_init()` creates one library-wide context, and windows are
created on top of it with `mkfw_window_create()`.  A single
context can own multiple windows that share one event pump.

There is no automatic migration path; call sites have to be
updated by hand.  A symbol-level mapping with before/after
snippets is in [MIGRATION_0.1.0.md](documentation/MIGRATION_0.1.0.md).
For the full reference, see [MKFW_API.md](documentation/MKFW_API.md).
If you need the previous API, check out the
[`v0.1.0`](../../releases/tag/v0.1.0) tag.  Future breaking
changes will ship with a proper migration note.

## Features

**Core:**
- Window creation and management (resize, fullscreen, aspect ratio, min size)
- Monitor enumeration with physical size, content scale, and work area; video-mode listing; per-monitor fullscreen with optional exclusive mode; hotplug notification
- Window event callbacks (move, refresh, content-scale, cursor enter/leave, absolute cursor position, close) plus always-on-top / maximized / no-focus creation hints and force-focus
- OpenGL Compatibility Profile context creation (configurable version, default 3.1)
- Framebuffer format hints (depth, stencil, MSAA samples, sRGB)
- Debug / forward-compatible context flags and context sharing between windows
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

macOS is out of scope. Apple deprecated OpenGL, and a Cocoa backend is
a permanent maintenance cost that pulls against the library's focus;
mkfw targets Linux/X11 and Windows/Win32 only.

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

### Building

See [Linking](#linking) below for the single, canonical table of
required libraries on each platform.  In short:

```sh
# Linux
gcc main.c -lm -ldl -lpthread -o app

# Windows, MinGW
gcc main.c -lopengl32 -lgdi32 -lwinmm -o app.exe

# Windows, clang-cl
clang-cl main.c opengl32.lib gdi32.lib winmm.lib user32.lib shell32.lib
```

Audio adds `ole32 avrt uuid` on Windows; everything else is
already covered.  No flag changes are required to enable joystick
or timer.

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

By default mkfw creates an OpenGL context at the highest version
the driver reports, using the Core (non-backwards-compatible)
profile.  Code that needs the fixed-function pipeline, immediate
mode, or any other deprecated GL must opt into the Compatibility
profile explicitly:

```c
// Defaults: highest available version, Core profile
struct mkfw_window_options wopts = {
    .width = 1280, .height = 720,
};
struct mkfw_window *window = mkfw_window_create(ctx, &wopts);

// Pin to a specific version; window creation fails (with an
// error reporting the driver's maximum) if it is not available
struct mkfw_window_options pinned = {
    .gl_major = 4, .gl_minor = 6,
};

// Immediate mode / fixed function (opt in)
struct mkfw_window_options legacy = {
    .gl_profile = MKFW_GL_PROFILE_COMPAT,
};
```

`mkfw_query_max_gl_version(&major, &minor)` is available if you
want to inspect the driver maximum yourself.  See
[MKFW_API.md](documentation/MKFW_API.md#opengl-version-configuration)
for details.

## Examples

Working examples are included in the repository:

- [examples/joystick.c](examples/joystick.c) - gamepad input with terminal visualization
- [examples/threaded.c](examples/threaded.c) - render loop on a separate thread, decoupled from the OS message pump
- [examples/monitor.c](examples/monitor.c) - monitor enumeration
- [examples/transparency.c](examples/transparency.c) - per-pixel transparency
- [examples/audio_beep.c](examples/audio_beep.c) - 440 Hz sine for one second
- [examples/multi_window.c](examples/multi_window.c) - two windows on one context, one event pump

Build all examples:

```sh
cd examples && bash build_examples.sh
```

## Tests

A small smoke-test suite lives under [tests/](tests/) and exercises
every subsystem (window, monitor query, audio, timer, joystick,
multi-window event dispatch).  Build and run:

```sh
cd tests && bash build_tests.sh
./smoke
./multi_window
```

Each test exits `0` on success.

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

## Joystick gamedb

When `MKFW_JOYSTICK_GAMEDB` is defined before including
`mkfw_joystick.h`, mkfw resolves a connected controller's
vendor/product ID against a snapshot of the SDL
[`SDL_GameControllerDB`](https://github.com/mdqinc/SDL_GameControllerDB)
mappings shipped in [mkfw_joystick_gamedb.h](mkfw_joystick_gamedb.h).
This is what gives `mkfw_gamepad_get_button(pad, MKFW_GAMEPAD_A)`
its layout-agnostic meaning.

The data block in that header is regenerated from upstream by
[tools/gen_joystick_gamedb.py](tools/gen_joystick_gamedb.py).  A
GitHub Action ([gamedb-refresh.yml](.github/workflows/gamedb-refresh.yml))
runs it on the first of each month, commits the result if anything
changed, and tags the commit as `gamedb-YYYY-MM`.

To refresh manually:

```sh
python3 tools/gen_joystick_gamedb.py             # fetch and rewrite
python3 tools/gen_joystick_gamedb.py --dry-run   # preview only
python3 tools/gen_joystick_gamedb.py --from-file gamecontrollerdb.txt
```

The script only touches the text between the `BEGIN GENERATED
gamedb` / `END GENERATED gamedb` markers inside
`mkfw_joystick_gamedb.h`; the button/axis enums and file header
are preserved.  Entries are filtered to `platform:Linux` and
`platform:Windows` (mkfw's targets), deduplicated by GUID +
platform, and emitted as one big C string literal.

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

## Linking

All optional subsystems link into the same translation unit as the
core.  The table below is the **single, canonical** reference;
the per-subsystem docs do not repeat it.

| Platform / build | Core | Joystick | Timer | Audio |
|------------------|------|----------|-------|-------|
| Linux, any compiler | `-lm -ldl -lpthread` | (no extra) | (no extra) | (no extra) |
| Windows, MinGW | `-lopengl32 -lgdi32 -lwinmm` | `-lxinput` | (no extra) | `-lole32 -lavrt -luuid` |
| Windows, clang-cl | `opengl32.lib gdi32.lib winmm.lib user32.lib shell32.lib` | `xinput.lib` | (no extra) | `ole32.lib avrt.lib uuid.lib` |
| Windows, MSVC | same as clang-cl | `xinput.lib` | (no extra) | `ole32.lib avrt.lib uuid.lib` |

Notes:

- **Linux platform libraries** (X11, GL, Xi, Xrandr, libasound, libXcursor)
  are loaded at runtime via `dlopen`, never with `-l<name>`.  Their
  development headers are still needed at compile time.  libXcursor
  is optional at runtime: missing it disables `mkfw_cursor_create_rgba`
  but leaves the rest of mkfw working.
- **clang-cl** requires explicit linking of `user32` and `shell32`;
  MinGW links them implicitly.

## License

[MIT](LICENSE)
