# mkfw joystick API

Gamepad input subsystem: tracks up to 4 controllers, exposes raw
buttons / axes / hat (D-pad) state, and optionally maps them to
standardised gamepad layout names via the SDL_GameControllerDB
mappings shipped in [`mkfw_joystick_gamedb.h`](../mkfw_joystick_gamedb.h).

## Enabling

```c
#include "mkfw.h"
#include "mkfw_joystick.h"
```

To also pull in the SDL gamepad mapping database (so you can
write `mkfw_gamepad_get_button(pad, MKFW_GAMEPAD_A)` instead of
hardcoding raw button indices), define `MKFW_JOYSTICK_GAMEDB`
before including:

```c
#define MKFW_JOYSTICK_GAMEDB
#include "mkfw_joystick.h"
```

For required linker flags see the **Linking** section in the
project README (joystick adds nothing beyond the core libs).

## Contents

- [Overview](#overview)
- [Constants](#constants)
- [Pad state](#pad-state)
- [Lifecycle](#lifecycle)
- [Connection state](#connection-state)
- [Raw button / axis / hat queries](#raw-button--axis--hat-queries)
- [Connect / disconnect callback](#connect--disconnect-callback)
- [Rumble](#rumble)
- [Standardised gamepad layout (gamedb)](#standardised-gamepad-layout-gamedb)
- [Threading](#threading)
- [Platform details](#platform-details)
- [Refreshing the gamepad mapping database](#refreshing-the-gamepad-mapping-database)

---

## Overview

The joystick subsystem is **global** (not per-window).
Controllers are system-level resources; mkfw exposes them as
indices `0..MKFW_JOYSTICK_MAX_PADS-1`.

```c
mkfw_joystick_init();

while(running) {
    mkfw_joystick_update();   // pull new state from the OS

    for(uint32_t p = 0; p < MKFW_JOYSTICK_MAX_PADS; ++p) {
        if(!mkfw_joystick_is_connected(p)) {
            continue;
        }
        // raw read:
        if(mkfw_joystick_is_button_pressed(p, 0)) {
            printf("Pad %u button 0 pressed\n", p);
        }
        // gamedb-mapped read (requires MKFW_JOYSTICK_GAMEDB):
        if(mkfw_gamepad_is_button_pressed(p, MKFW_GAMEPAD_A)) {
            printf("Pad %u: A pressed\n", p);
        }
    }
}

mkfw_joystick_shutdown();
```

`mkfw_joystick_update()` is the polling driver: call it once per
frame (typically right after `mkfw_poll_events`).  Connect /
disconnect transitions also fire inside `update`.

---

## Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `MKFW_JOYSTICK_MAX_PADS`    | 4   | Maximum concurrently-tracked pads |
| `MKFW_JOYSTICK_MAX_BUTTONS` | 32  | Max raw buttons per pad |
| `MKFW_JOYSTICK_MAX_AXES`    | 8   | Max raw axes per pad |
| `MKFW_JOYSTICK_NAME_LEN`    | 256 | Bytes in `pad->name[]` |

Gamedb (when `MKFW_JOYSTICK_GAMEDB` is defined):

```c
enum {                                  enum {
    MKFW_GAMEPAD_A = 0,                     MKFW_GAMEPAD_AXIS_LEFT_X = 0,
    MKFW_GAMEPAD_B,                         MKFW_GAMEPAD_AXIS_LEFT_Y,
    MKFW_GAMEPAD_X,                         MKFW_GAMEPAD_AXIS_RIGHT_X,
    MKFW_GAMEPAD_Y,                         MKFW_GAMEPAD_AXIS_RIGHT_Y,
    MKFW_GAMEPAD_LEFT_BUMPER,               MKFW_GAMEPAD_AXIS_LEFT_TRIGGER,
    MKFW_GAMEPAD_RIGHT_BUMPER,              MKFW_GAMEPAD_AXIS_RIGHT_TRIGGER,
    MKFW_GAMEPAD_BACK,                      MKFW_GAMEPAD_AXIS_LAST,
    MKFW_GAMEPAD_START,                 };
    MKFW_GAMEPAD_GUIDE,
    MKFW_GAMEPAD_LEFT_THUMB,
    MKFW_GAMEPAD_RIGHT_THUMB,
    MKFW_GAMEPAD_DPAD_UP,
    MKFW_GAMEPAD_DPAD_DOWN,
    MKFW_GAMEPAD_DPAD_LEFT,
    MKFW_GAMEPAD_DPAD_RIGHT,
    MKFW_GAMEPAD_BUTTON_LAST,
};
```

---

## Pad state

The library exposes the array
`mkfw_joystick_pads[MKFW_JOYSTICK_MAX_PADS]` directly; callers
may read it freely (alongside the accessor helpers below):

```c
struct mkfw_joystick_pad {
    uint8_t  connected;                          // 1 if present
    uint8_t  was_connected;                      // previous-frame snapshot
    char     name[MKFW_JOYSTICK_NAME_LEN];       // device name (UTF-8)
    uint16_t vendor_id;                          // USB VID
    uint16_t product_id;                         // USB PID
    int      button_count;                       // populated by update()
    int      axis_count;                         // populated by update()
    uint8_t  buttons[MKFW_JOYSTICK_MAX_BUTTONS]; // 1 = held
    uint8_t  prev_buttons[MKFW_JOYSTICK_MAX_BUTTONS];
    float    axes[MKFW_JOYSTICK_MAX_AXES];       // -1.0 .. 1.0
    float    hat_x;                              // -1, 0, 1
    float    hat_y;                              // -1, 0, 1
};
```

`vendor_id` and `product_id` are what the gamedb keys on; they
are also useful for hand-rolled mappings of obscure controllers.

---

## Lifecycle

### `mkfw_joystick_init`

```c
void mkfw_joystick_init(void);
```

Initialize the subsystem.  Linux scans `/dev/input/event*`; Win32
queries XInput.  Safe to call before any windows exist.

### `mkfw_joystick_shutdown`

```c
void mkfw_joystick_shutdown(void);
```

Release device handles, internal state, and (Linux) close
`/dev/input` file descriptors.

### `mkfw_joystick_update`

```c
void mkfw_joystick_update(void);
```

Drain pending device events, snapshot the previous-frame button
state for edge detection, refresh connect / disconnect status,
and fire the connect callback for any transitions.

Call once per frame, typically right after `mkfw_poll_events`.

---

## Connection state

### `mkfw_joystick_is_connected`

```c
uint32_t mkfw_joystick_is_connected(uint32_t pad_index);
```

Return non-zero if the slot is currently populated.  Pads can
connect / disconnect at any time (hotplug supported on both
platforms); always test before reading other state.

### `mkfw_joystick_get_name`

```c
const char *mkfw_joystick_get_name(uint32_t pad_index);
```

Return the device's human-readable name (UTF-8).  The pointer is
borrowed; valid only until the pad disconnects.

### `mkfw_joystick_get_button_count` / `_get_axis_count`

```c
uint32_t mkfw_joystick_get_button_count(uint32_t pad_index);
uint32_t mkfw_joystick_get_axis_count  (uint32_t pad_index);
```

Number of buttons / axes the device exposes.  Bounded by
`MKFW_JOYSTICK_MAX_BUTTONS` / `MKFW_JOYSTICK_MAX_AXES`.

---

## Raw button / axis / hat queries

These read from the raw arrays without any gamedb-style
normalisation.  Use them for non-gamepad controllers (joysticks,
flight sticks, racing wheels) where the layout isn't a standard
A/B/X/Y dual-stick gamepad.

### `mkfw_joystick_get_button`

```c
uint32_t mkfw_joystick_get_button(uint32_t pad_index, uint32_t button_index);
```

Non-zero while the raw button is held.

### `mkfw_joystick_is_button_pressed` / `_was_button_released`

```c
uint32_t mkfw_joystick_is_button_pressed   (uint32_t pad_index, uint32_t button_index);
uint32_t mkfw_joystick_was_button_released (uint32_t pad_index, uint32_t button_index);
```

Edge-detected button queries (compared against
`prev_buttons[]`, refreshed by `mkfw_joystick_update`).

### `mkfw_joystick_get_axis`

```c
float mkfw_joystick_get_axis(uint32_t pad_index, uint32_t axis_index);
```

Current axis position in `[-1.0, 1.0]` (with deadzone applied).
Triggers report `[0.0, 1.0]`.

### `mkfw_joystick_get_hat_x` / `_get_hat_y`

```c
float mkfw_joystick_get_hat_x(uint32_t pad_index);
float mkfw_joystick_get_hat_y(uint32_t pad_index);
```

D-pad / hat position decomposed into X / Y axes (`-1.0`, `0.0`,
or `1.0`).  Used for D-pad input on controllers that report the
D-pad as a single hat instead of four buttons.

---

## Connect / disconnect callback

```c
typedef void (*mkfw_joystick_callback_t)(int pad_index, int connected);

void mkfw_joystick_set_callback(mkfw_joystick_callback_t callback);
```

Install a callback fired by `mkfw_joystick_update` whenever a
pad's connection state transitions.  `connected` is `1` on
plug-in, `0` on unplug.  Pass `0` to remove the callback.

```c
static void on_pad(int pad, int connected) {
    if(connected) {
        printf("Pad %d connected: %s\n", pad, mkfw_joystick_get_name(pad));
    } else {
        printf("Pad %d disconnected\n", pad);
    }
}

mkfw_joystick_set_callback(on_pad);
```

The callback fires from `mkfw_joystick_update`'s thread (the
same one that calls it; typically the main thread).

---

## Rumble

### `mkfw_joystick_rumble`

```c
void mkfw_joystick_rumble(uint32_t pad_index,
                          float low_freq, float high_freq,
                          uint32_t duration_ms);
```

Trigger force-feedback rumble for `duration_ms` milliseconds.
`low_freq` / `high_freq` are the magnitudes of the two rumble
motors in `[0.0, 1.0]` (mapped from "low-frequency" /
"high-frequency" actuator semantics).

A duration of `0` and both magnitudes `0` stop any active rumble.

Supported on Linux via the kernel evdev force-feedback interface
and on Windows via `XInputSetState`.  Unsupported controllers
silently ignore the request.

---

## Standardised gamepad layout (gamedb)

When `MKFW_JOYSTICK_GAMEDB` is defined before including
`mkfw_joystick.h`, mkfw matches each connected pad against
[`mkfw_joystick_gamedb.h`](../mkfw_joystick_gamedb.h) (a snapshot
of the SDL_GameControllerDB project) by USB VID/PID and
populates an internal mapping.  Three accessor helpers then
expose pads in the standardised gamepad layout:

### `mkfw_gamepad_get_button`

```c
int mkfw_gamepad_get_button(int pad_index, int gamepad_button);
```

Non-zero while the mapped button (`MKFW_GAMEPAD_A`,
`_DPAD_UP`, ...) is held.

### `mkfw_gamepad_is_button_pressed`

```c
int mkfw_gamepad_is_button_pressed(int pad_index, int gamepad_button);
```

Edge-detected rising-edge query.

### `mkfw_gamepad_get_axis`

```c
float mkfw_gamepad_get_axis(int pad_index, int gamepad_axis);
```

Mapped axis value (`MKFW_GAMEPAD_AXIS_LEFT_X`, `_RIGHT_X`,
`_LEFT_TRIGGER`, ...) in `[-1.0, 1.0]` (sticks) or `[0.0, 1.0]`
(triggers).

If `MKFW_JOYSTICK_GAMEDB` is **not** defined, these three
functions are still declared but expand to constant zero
returns, so conditional code using them stays compilable.

```c
#define MKFW_JOYSTICK_GAMEDB
#include "mkfw_joystick.h"

float lx = mkfw_gamepad_get_axis(0, MKFW_GAMEPAD_AXIS_LEFT_X);
float ly = mkfw_gamepad_get_axis(0, MKFW_GAMEPAD_AXIS_LEFT_Y);
if(mkfw_gamepad_is_button_pressed(0, MKFW_GAMEPAD_A)) {
    fire();
}
```

A pad that the database doesn't recognise will report zeros for
every mapped query but still works through the raw API.
XInput-class controllers on Windows are always supported because
mkfw applies a fixed 1:1 mapping there.

---

## Threading

`mkfw_joystick_init`, `_shutdown`, `_update`, and the connect
callback all run on the application's owning thread (the one
calling `mkfw_joystick_update`).  Reads from
`mkfw_joystick_pads[]` and the accessor helpers are safe from
the same thread.

Reads from other threads see eventually-consistent values; the
per-byte fields (buttons, hat) are atomic on x86-64, but the
multi-byte `axes[]` fields can be torn if the reader and the
update thread race.  If you need to read pad state from a
non-main thread, snapshot the pad inside
`mkfw_joystick_update`'s thread first.

---

## Platform details

### Linux

- Uses `/dev/input/event*` directly (libudev *not* required).
  Hotplug is detected by re-scanning the directory.
- Force-feedback rumble via the kernel evdev FF protocol.
- The user typically needs to be in the `input` group:

  ```sh
  sudo usermod -aG input $USER
  ```

  (re-login after, or use `newgrp input`).

### Windows

- Uses XInput.  Up to 4 pads, always recognised, fixed gamedb
  mapping.
- `mkfw_joystick_rumble` calls `XInputSetState`.

---

## Refreshing the gamepad mapping database

`mkfw_joystick_gamedb.h` is a snapshot of upstream
SDL_GameControllerDB.  Refresh it by running
[`tools/gen_joystick_gamedb.py`](../tools/gen_joystick_gamedb.py):

```sh
python3 tools/gen_joystick_gamedb.py             # fetch and rewrite
python3 tools/gen_joystick_gamedb.py --dry-run   # preview only
python3 tools/gen_joystick_gamedb.py --from-file gamecontrollerdb.txt
```

The script replaces only the text between the
`BEGIN GENERATED gamedb` / `END GENERATED gamedb` marker
comments in the header.  Entries are filtered to
`platform:Linux` / `platform:Windows`, deduplicated by GUID +
platform, and emitted as one big C string literal.

A monthly GitHub Action
([`.github/workflows/gamedb-refresh.yml`](../.github/workflows/gamedb-refresh.yml))
runs the script on the first of each month, commits the result
if anything changed, and tags the commit `gamedb-YYYY-MM`.
