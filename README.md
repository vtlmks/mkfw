# mkfw

A minimal windowing and input library for OpenGL applications on Linux (X11), Windows (Win32), and Emscripten (WebGL2).

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

**Optional subsystems** (enabled via compile-time defines):

| Define | Subsystem | Description |
|--------|-----------|-------------|
| `MKFW_JOYSTICK` | Joystick | Up to 4 gamepads, hotplug, analog axes, buttons, d-pad |
| `MKFW_JOYSTICK_GAMEDB` | GameDB | SDL GameController DB mappings for standardized button names |
| `MKFW_AUDIO` | Audio | Low-latency callback-based audio output (WASAPI / ALSA) |
| `MKFW_TIMER` | Timer | High-precision timing with sleep+spin strategy |

## Platforms

| Platform | Window System | GL Context | Input |
|----------|---------------|------------|-------|
| Linux | X11 | GLX | XInput2 (mouse), evdev (joystick) |
| Windows | Win32 | WGL | Raw Input (mouse), XInput (joystick) |
| Emscripten | Canvas/DOM | WebGL2 | DOM events (keyboard/mouse), Gamepad API |

## Quick start

mkfw is a set of headers and source files you include directly into your project — no build system or external dependencies required.

```c
#include "mkfw_gl_loader.h"
#include "mkfw.h"

int main() {
    struct mkfw_state *mkfw = mkfw_init(1280, 720);
    mkfw_set_window_title(mkfw, "Hello mkfw");
    mkfw_show_window(mkfw);

    mkfw_gl_loader();

    while (!mkfw_should_close(mkfw)) {
        mkfw_pump_messages(mkfw);

        if (mkfw->keyboard_state[MKS_KEY_ESCAPE])
            mkfw_set_should_close(mkfw, 1);

        // render ...

        mkfw_swap_buffers(mkfw);
    }

    mkfw_cleanup(mkfw);
    return 0;
}
```

Compile on Linux:

```sh
gcc main.c -lX11 -lXi -lXrandr -lGL -lm -o app
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

Enable subsystems by defining the corresponding flag before including the header:

```c
#define MKFW_AUDIO
#define MKFW_TIMER
#define MKFW_JOYSTICK
#define MKFW_JOYSTICK_GAMEDB
#include "mkfw.h"
```

## OpenGL version

By default mkfw creates an OpenGL 3.1 Compatibility Profile context. You can request a higher version for features like compute shaders (4.3+) while keeping immediate mode available:

```c
// Query what the driver supports
int major, minor;
mkfw_query_max_gl_version(&major, &minor);

// Request a specific version before mkfw_init
mkfw_set_gl_version(4, 6);
struct mkfw_state *mkfw = mkfw_init(1280, 720);
```

See [MKFW_API.md](documentation/MKFW_API.md#opengl-version-configuration) for details.

## Examples

Working examples are included in the repository:

- [joystick_example/](joystick_example/) — gamepad input with terminal visualization
- [threaded_example/](threaded_example/) — render loop on a separate thread, decoupled from the OS message pump
- [mkfw_run_example/](mkfw_run_example/) — cross-platform frame callback (works on Linux, Windows, and Emscripten)

Build an example:

```sh
cd joystick_example && sh build_joystick.sh
cd threaded_example && sh build_threaded.sh
cd mkfw_run_example && sh build_desktop.sh
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
2. Call `mkfw_detach_context()` to release the GL context (a context can only be current on one thread at a time)
3. Spawn a render thread that calls `mkfw_attach_context()`, then runs your normal render loop
4. The main thread loops on `mkfw_pump_messages()` + `mkfw_sleep()` — nothing else
5. On shutdown, set a shared flag, join the render thread, then clean up

This pattern is not specific to mkfw — the same approach works with GLFW, SDL, or raw Win32. The [threaded_example/](threaded_example/) shows a complete, minimal implementation.

### Emscripten / mkfw_run()

`mkfw_run()` is an opt-in frame callback that replaces the manual while loop. It exists for users who want their code to compile for Emscripten (WebAssembly) in addition to desktop platforms. The existing manual loop pattern is unchanged and remains the recommended approach for desktop-only applications.

```c
static void frame(struct mkfw_state *window) {
    struct game_state *game = (struct game_state *)mkfw_get_user_data(window);
    // game logic + render
    mkfw_update_input_state(window);
}

int main(void) {
    struct mkfw_state *window = mkfw_init(1280, 720);
    mkfw_set_user_data(window, &game);
    mkfw_show_window(window);
    mkfw_gl_loader();

    mkfw_run(window, frame);

    mkfw_cleanup(window);
}
```

On Linux this is a simple while loop. On Windows it automatically handles the threaded rendering pattern. On Emscripten it uses `requestAnimationFrame`.

Compile for Emscripten:

```sh
emcc main.c -sUSE_WEBGL2=1 -sFULL_ES3=1 -o app.html
```

See [MKFW_RUN_API.md](documentation/MKFW_RUN_API.md) for the full contract, constraints, and Emscripten-specific details.

## Documentation

Detailed API documentation for each subsystem is in the [documentation/](documentation/) directory:

- [MKFW_API.md](documentation/MKFW_API.md) — core window, input, and context management
- [MKFW_RUN_API.md](documentation/MKFW_RUN_API.md) — mkfw_run() and Emscripten support
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

// after mkfw_init + mkfw_show_window:
mkfw_gl_loader();
```

## Dependencies

None beyond platform libraries:

- **Linux:** X11, Xi, GL, m (all standard)
- **Windows:** opengl32, gdi32, winmm (all standard). clang-cl additionally requires explicit linking of user32 and shell32, which MinGW links implicitly. Audio subsystem adds ole32, avrt, and uuid.
- **Emscripten:** Emscripten SDK (provides WebGL2, audio, gamepad APIs)

## License

[MIT](LICENSE)
