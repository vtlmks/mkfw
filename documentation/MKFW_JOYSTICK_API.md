# MKFW Joystick API Documentation

**Gamepad/Joystick Input Subsystem for MKFW**

## Overview

The MKFW Joystick API provides polling-based gamepad and joystick input for up to 4 controllers simultaneously. It uses platform-specific implementations (XInput on Windows, evdev on Linux) with automatic hotplug detection.

An optional mapping layer (`MKFW_JOYSTICK_GAMEDB`) provides standardized gamepad button/axis names using the SDL GameController DB format, allowing you to query `MKFW_GAMEPAD_A` instead of raw button indices.

## Features

- Up to 4 simultaneous controllers
- Polling-based API with per-frame updates
- Button edge detection (pressed/released this frame)
- Normalized axes (-1.0 to 1.0)
- D-pad as discrete hat values
- Hotplug detection (connect/disconnect callbacks)
- Optional SDL GameController DB mapping layer
- No external library dependencies (Linux)

## Platform Support

| Platform | Backend | Notes |
|----------|---------|-------|
| Windows | XInput | Up to 4 Xbox-compatible controllers, standardized layout |
| Linux | evdev | Any gamepad via `/dev/input/event*`, inotify hotplug |

---

## Enabling the Joystick Subsystem

The joystick subsystem is optional and must be enabled before including `mkfw.h`:

```c
#define MKFW_JOYSTICK
#include "mkfw.h"
```

For standardized gamepad button names (SDL GameController DB mapping):

```c
#define MKFW_JOYSTICK
#define MKFW_JOYSTICK_GAMEDB
#include "mkfw.h"
```

`MKFW_JOYSTICK_GAMEDB` requires `MKFW_JOYSTICK` to also be defined.

---

## Core Concepts

### Polling Model

The joystick API is polling-based. Each frame you:

1. Call `mkfw_joystick_update()` to read all connected devices
2. Query button/axis/hat state using the accessor functions
3. Edge detection (pressed/released) is handled automatically via double-buffered state

### Global State

Joystick state is **global**, not per-window. Controllers are system-level resources. All functions take a `pad_index` (0-3) rather than a window state pointer.

### Raw vs Mapped API

- **Raw API** (`MKFW_JOYSTICK`): Query buttons by index, axes by index. The button/axis layout depends on the controller and platform.
- **Mapped API** (`MKFW_JOYSTICK_GAMEDB`): Query by standardized names like `MKFW_GAMEPAD_A`, `MKFW_GAMEPAD_AXIS_LEFT_X`. The mapping database translates these to the correct raw indices for each controller model.

On Windows with XInput, the raw layout is already standardized (Xbox layout). The mapping layer is primarily valuable on Linux where different controllers report buttons/axes in different orders.

### D-pad

The D-pad is reliably readable on all platforms without any mapping database:

- **Windows**: D-pad state comes from XInput button flags
- **Linux**: D-pad is reported as `ABS_HAT0X`/`ABS_HAT0Y` axes (or as buttons on some controllers)

Hat values are -1, 0, or 1 using screen coordinates (Y-down: up = -1, down = 1).

---

## Initialization and Shutdown

### `mkfw_joystick_init`

```c
void mkfw_joystick_init(void)
```

Initialize the joystick subsystem.

**Notes:**
- On Linux: Sets up inotify for hotplug detection and scans `/dev/input/` for existing gamepads
- On Windows: Zeroes state (XInput requires no explicit initialization)
- Call before the main loop
- Safe to call multiple times (idempotent)

---

### `mkfw_joystick_shutdown`

```c
void mkfw_joystick_shutdown(void)
```

Clean up joystick resources.

**Notes:**
- On Linux: Closes all open device file descriptors and inotify watches
- On Windows: Zeroes state
- Safe to call even if no joysticks were connected

---

## Per-Frame Update

### `mkfw_joystick_update`

```c
void mkfw_joystick_update(void)
```

Poll all connected controllers and update state.

**Notes:**
- Must be called once per frame, before querying state
- Copies current button state to previous (for edge detection)
- On Linux: Checks inotify for hotplug events, reads pending evdev events
- On Windows: Polls XInputGetState for each controller slot
- Fires connect/disconnect callbacks on state transitions

**Example:**
```c
while (running) {
    mkfw_pump_messages(window);
    mkfw_joystick_update();

    // Query state here...

    mkfw_update_input_state(window);
}
```

---

## Query API

### `mkfw_joystick_connected`

```c
int mkfw_joystick_connected(int pad_index)
```

Check if a controller is connected.

**Returns:** 1 if connected, 0 if not.

---

### `mkfw_joystick_name`

```c
const char *mkfw_joystick_name(int pad_index)
```

Get the controller name string.

**Returns:** Device name (e.g. "Xbox 360 Controller", "PS5 Controller"). Empty string if not connected.

**Notes:**
- On Windows: Returns "XInput Controller N" (XInput doesn't provide device names)
- On Linux: Returns the kernel-reported device name

---

### `mkfw_joystick_button`

```c
int mkfw_joystick_button(int pad_index, int button_index)
```

Check if a button is currently held down.

**Returns:** 1 if pressed, 0 if not.

---

### `mkfw_joystick_button_pressed`

```c
int mkfw_joystick_button_pressed(int pad_index, int button_index)
```

Check if a button was pressed this frame (edge detection).

**Returns:** 1 if the button transitioned from released to pressed this frame.

---

### `mkfw_joystick_button_released`

```c
int mkfw_joystick_button_released(int pad_index, int button_index)
```

Check if a button was released this frame (edge detection).

**Returns:** 1 if the button transitioned from pressed to released this frame.

---

### `mkfw_joystick_axis`

```c
float mkfw_joystick_axis(int pad_index, int axis_index)
```

Get a normalized axis value.

**Returns:** Value from -1.0 to 1.0. Triggers return 0.0 to 1.0.

**Notes:**
- On Windows: Thumbstick deadzones are applied automatically (7849 for left, 8689 for right)
- On Windows: Trigger threshold of 30 is applied
- On Linux: Values are normalized using the axis range reported by the kernel

---

### `mkfw_joystick_hat_x` / `mkfw_joystick_hat_y`

```c
float mkfw_joystick_hat_x(int pad_index)
float mkfw_joystick_hat_y(int pad_index)
```

Get D-pad state.

**Returns:**
- `hat_x`: -1.0 (left), 0.0 (center), 1.0 (right)
- `hat_y`: -1.0 (up), 0.0 (center), 1.0 (down)

---

### `mkfw_joystick_button_count` / `mkfw_joystick_axis_count`

```c
int mkfw_joystick_button_count(int pad_index)
int mkfw_joystick_axis_count(int pad_index)
```

Get the number of buttons/axes on a controller.

**Notes:**
- On Windows (XInput): Always 14 buttons and 6 axes
- On Linux: Varies by controller

---

### `mkfw_joystick_set_callback`

```c
void mkfw_joystick_set_callback(mkfw_joystick_callback_t callback)
```

Set a callback for controller connect/disconnect events.

**Callback signature:**
```c
void callback(int pad_index, int connected)
```

- `pad_index`: Controller slot (0-3)
- `connected`: 1 = connected, 0 = disconnected

**Example:**
```c
void on_gamepad(int pad, int connected) {
    printf("Pad %d %s: %s\n", pad,
           connected ? "connected" : "disconnected",
           mkfw_joystick_name(pad));
}

mkfw_joystick_set_callback(on_gamepad);
```

---

## Raw Button Layout

### Windows (XInput)

All XInput controllers use this fixed layout:

| Index | Button |
|-------|--------|
| 0 | A |
| 1 | B |
| 2 | X |
| 3 | Y |
| 4 | Left Bumper (LB) |
| 5 | Right Bumper (RB) |
| 6 | Back / View |
| 7 | Start / Menu |
| 8 | Left Stick Click |
| 9 | Right Stick Click |
| 10 | D-Pad Up |
| 11 | D-Pad Down |
| 12 | D-Pad Left |
| 13 | D-Pad Right |

**Axis layout:**

| Index | Axis | Range |
|-------|------|-------|
| 0 | Left Stick X | -1.0 to 1.0 |
| 1 | Left Stick Y | -1.0 to 1.0 (up = positive) |
| 2 | Right Stick X | -1.0 to 1.0 |
| 3 | Right Stick Y | -1.0 to 1.0 (up = positive) |
| 4 | Left Trigger | 0.0 to 1.0 |
| 5 | Right Trigger | 0.0 to 1.0 |

### Linux (evdev)

Button and axis indices depend on the controller model. Common Xbox-compatible controllers typically follow a similar layout to XInput, but this is not guaranteed for all devices.

Use the mapped gamepad API (`MKFW_JOYSTICK_GAMEDB`) for consistent cross-controller access on Linux.

---

## Mapped Gamepad API (MKFW_JOYSTICK_GAMEDB)

When `MKFW_JOYSTICK_GAMEDB` is defined, additional functions and constants are available for standardized access.

### Button Constants

```c
MKFW_GAMEPAD_A              // Cross (PS) / A (Xbox) / B (Nintendo)
MKFW_GAMEPAD_B              // Circle (PS) / B (Xbox) / A (Nintendo)
MKFW_GAMEPAD_X              // Square (PS) / X (Xbox) / Y (Nintendo)
MKFW_GAMEPAD_Y              // Triangle (PS) / Y (Xbox) / X (Nintendo)
MKFW_GAMEPAD_LEFT_BUMPER    // L1 (PS) / LB (Xbox)
MKFW_GAMEPAD_RIGHT_BUMPER   // R1 (PS) / RB (Xbox)
MKFW_GAMEPAD_BACK           // Share/Select/View
MKFW_GAMEPAD_START          // Options/Start/Menu
MKFW_GAMEPAD_GUIDE          // PS/Xbox/Home button
MKFW_GAMEPAD_LEFT_THUMB     // L3 (left stick click)
MKFW_GAMEPAD_RIGHT_THUMB    // R3 (right stick click)
MKFW_GAMEPAD_DPAD_UP
MKFW_GAMEPAD_DPAD_DOWN
MKFW_GAMEPAD_DPAD_LEFT
MKFW_GAMEPAD_DPAD_RIGHT
```

### Axis Constants

```c
MKFW_GAMEPAD_AXIS_LEFT_X         // Left stick horizontal
MKFW_GAMEPAD_AXIS_LEFT_Y         // Left stick vertical
MKFW_GAMEPAD_AXIS_RIGHT_X        // Right stick horizontal
MKFW_GAMEPAD_AXIS_RIGHT_Y        // Right stick vertical
MKFW_GAMEPAD_AXIS_LEFT_TRIGGER   // L2 / LT
MKFW_GAMEPAD_AXIS_RIGHT_TRIGGER  // R2 / RT
```

### `mkfw_gamepad_button`

```c
int mkfw_gamepad_button(int pad_index, int gamepad_button)
```

Query a button by standardized name.

**Returns:** 1 if pressed, 0 if not. Returns 0 if controller has no mapping in the database.

---

### `mkfw_gamepad_button_pressed`

```c
int mkfw_gamepad_button_pressed(int pad_index, int gamepad_button)
```

Check if a mapped button was pressed this frame.

---

### `mkfw_gamepad_axis`

```c
float mkfw_gamepad_axis(int pad_index, int gamepad_axis)
```

Query an axis by standardized name.

**Returns:** Normalized value. Returns 0.0 if controller has no mapping.

---

### Mapping Lookup

Mapping lookup happens automatically (lazy) on first query for a connected controller. When a controller disconnects, its mapping is cleared and will be re-looked-up on reconnection.

On Windows, XInput provides a fixed standardized layout, so the mapping is always a 1:1 identity mapping regardless of the database contents.

---

## Typical Usage Patterns

### Basic Joystick Input

```c
#define MKFW_JOYSTICK
#include "mkfw.h"

int main(void) {
    struct mkfw_state *window = mkfw_init(1280, 720);
    mkfw_show_window(window);

    mkfw_joystick_init();

    while (!mkfw_should_close(window)) {
        mkfw_pump_messages(window);
        mkfw_joystick_update();

        for (int p = 0; p < 4; p++) {
            if (!mkfw_joystick_connected(p)) continue;

            float lx = mkfw_joystick_axis(p, 0);
            float ly = mkfw_joystick_axis(p, 1);

            if (mkfw_joystick_button_pressed(p, 0)) {
                printf("Pad %d: button A pressed!\n", p);
            }

            float dpad_x = mkfw_joystick_hat_x(p);
            float dpad_y = mkfw_joystick_hat_y(p);
        }

        mkfw_update_input_state(window);
    }

    mkfw_joystick_shutdown();
    mkfw_cleanup(window);
    return 0;
}
```

### Mapped Gamepad Input

```c
#define MKFW_JOYSTICK
#define MKFW_JOYSTICK_GAMEDB
#include "mkfw.h"

int main(void) {
    struct mkfw_state *window = mkfw_init(1280, 720);
    mkfw_show_window(window);

    mkfw_joystick_init();

    while (!mkfw_should_close(window)) {
        mkfw_pump_messages(window);
        mkfw_joystick_update();

        if (mkfw_joystick_connected(0)) {
            // Use standardized names - works across controller brands
            if (mkfw_gamepad_button_pressed(0, MKFW_GAMEPAD_A)) {
                printf("Jump!\n");
            }
            if (mkfw_gamepad_button(0, MKFW_GAMEPAD_RIGHT_BUMPER)) {
                printf("Shooting...\n");
            }

            float move_x = mkfw_gamepad_axis(0, MKFW_GAMEPAD_AXIS_LEFT_X);
            float move_y = mkfw_gamepad_axis(0, MKFW_GAMEPAD_AXIS_LEFT_Y);
            float aim_x  = mkfw_gamepad_axis(0, MKFW_GAMEPAD_AXIS_RIGHT_X);
            float aim_y  = mkfw_gamepad_axis(0, MKFW_GAMEPAD_AXIS_RIGHT_Y);
        }

        mkfw_update_input_state(window);
    }

    mkfw_joystick_shutdown();
    mkfw_cleanup(window);
    return 0;
}
```

### Hotplug Detection

```c
void on_gamepad(int pad, int connected) {
    if (connected) {
        printf("Controller %d connected: %s\n", pad, mkfw_joystick_name(pad));
    } else {
        printf("Controller %d disconnected\n", pad);
    }
}

mkfw_joystick_init();
mkfw_joystick_set_callback(on_gamepad);
```

---

## Platform-Specific Notes

### Linux (evdev)

- Requires read access to `/dev/input/event*` devices
- User typically needs to be in the `input` group: `sudo usermod -aG input $USER`
- Or create a udev rule for gamepad access
- Hotplug uses inotify on `/dev/input/`
- Device capabilities are detected via ioctl (EV_ABS + EV_KEY with BTN_GAMEPAD range)
- Axis ranges are queried via `EVIOCGABS` and used for normalization
- No additional linking required beyond standard libc

**Linking:**
```bash
gcc -o app main.c -lX11 -lGL -lXi -ldl -lpthread
```

### Windows (XInput)

- Uses XInput API (supports Xbox-compatible controllers only)
- Up to 4 controllers (XInput hardware limit)
- Fixed standardized button/axis layout
- No device names available (XInput limitation)
- No vendor/product IDs available (XInput limitation)
- Deadzones applied automatically using XInput recommended values
- Hotplug detected via polling return codes

**Linking:**
```bash
gcc -o app.exe main.c -lopengl32 -lgdi32 -lwinmm -lxinput
```

---

## Updating the Controller Database

The embedded database in `mkfw_joystick_gamedb.h` contains mappings for popular controllers. To support more controllers:

1. Download the latest `gamecontrollerdb.txt` from [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB)
2. Convert to a C string and replace the `mkfw_gamecontrollerdb_data` array
3. Only Linux entries are needed (Windows XInput is already standardized)

---

## Stub Macros

When `MKFW_JOYSTICK` is not defined, all joystick functions are replaced with no-op macros that return zero values. This allows code to compile without `#ifdef` guards.

When `MKFW_JOYSTICK_GAMEDB` is not defined, the `mkfw_gamepad_*` functions are similarly stubbed out.

---

## Troubleshooting

### No controllers detected (Linux)

1. Check permissions: `ls -la /dev/input/event*`
2. Add user to input group: `sudo usermod -aG input $USER` (requires logout/login)
3. Verify the device is recognized: `cat /proc/bus/input/devices`
4. Check if evdev module is loaded: `lsmod | grep evdev`

### No controllers detected (Windows)

1. Ensure controller is Xbox-compatible or using XInput driver
2. DirectInput-only controllers are not supported (use MKFW_JOYSTICK_GAMEDB won't help here - XInput is required)
3. Check Windows Game Controllers settings

### Mapped buttons return 0

1. Controller may not be in the embedded database
2. Check `mkfw_joystick_name()` to verify the controller is detected
3. Raw API (`mkfw_joystick_button()`) should still work
4. Update the database with the latest SDL_GameControllerDB entries

### Axis values seem wrong on Linux

1. Different controllers report axes in different orders
2. Use `MKFW_JOYSTICK_GAMEDB` for standardized axis access
3. Or enumerate axes manually to find the correct indices

---

## See Also

- `MKFW_API.md` - Main MKFW windowing API
- `MKFW_AUDIO_API.md` - Audio subsystem documentation
- `MKFW_TIMER_API.md` - Timer subsystem documentation
- [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB) - Community controller mapping database

---

## License

MIT License - See source files for full license text.
