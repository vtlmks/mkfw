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

**Optional subsystems** (enabled via compile-time defines):

| Define | Subsystem | Description |
|--------|-----------|-------------|
| `MKFW_JOYSTICK` | Joystick | Up to 4 gamepads, hotplug, analog axes, buttons, d-pad |
| `MKFW_JOYSTICK_GAMEDB` | GameDB | SDL GameController DB mappings for standardized button names |
| `MKFW_AUDIO` | Audio | Low-latency callback-based audio output (WASAPI / ALSA) |
| `MKFW_TIMER` | Timer | High-precision timing with sleep+spin strategy |
| `MKFW_UI` | UI | Immediate-mode debug UI with built-in font, single draw call |

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
gcc main.c -lX11 -lXi -lGL -lm -o app
```

## Optional subsystems

Enable subsystems by defining the corresponding flag before including the header:

```c
#define MKFW_AUDIO
#define MKFW_TIMER
#define MKFW_JOYSTICK
#define MKFW_JOYSTICK_GAMEDB
#define MKFW_UI
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

- [ui_example/](ui_example/) — demonstrates all UI widgets (buttons, sliders, text input, combo boxes, etc.)
- [joystick_example/](joystick_example/) — gamepad input with real-time visualization

Build an example:

```sh
cd ui_example && sh build_ui.sh
cd joystick_example && sh build_joystick.sh
```

## Documentation

Detailed API documentation for each subsystem is in the [documentation/](documentation/) directory:

- [MKFW_API.md](documentation/MKFW_API.md) — core window, input, and context management
- [MKFW_AUDIO_API.md](documentation/MKFW_AUDIO_API.md) — audio output
- [MKFW_TIMER_API.md](documentation/MKFW_TIMER_API.md) — high-precision timing
- [MKFW_JOYSTICK_API.md](documentation/MKFW_JOYSTICK_API.md) — gamepad input
- [MKFW_UI_API.md](documentation/MKFW_UI_API.md) — immediate-mode UI
- [MKFW_UI_WIDGET_REFERENCE.md](documentation/MKFW_UI_WIDGET_REFERENCE.md) — UI widget quick reference

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

If you're using `MKFW_UI`, include the loader *before* `mkfw.h` so the UI subsystem can see the GL types.

## Dependencies

None beyond platform libraries:

- **Linux:** X11, Xi, GL, m (all standard)
- **Windows:** Windows SDK (standard)

## License

[MIT](LICENSE)
