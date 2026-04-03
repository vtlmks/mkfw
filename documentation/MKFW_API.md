# MKFW API Documentation

**Lightweight GLFW Alternative**

## Overview

MKFW is a minimal, single-header windowing and input library for OpenGL applications. It provides a simpler alternative to GLFW with platform-specific implementations for X11/Linux and Windows. By default it creates an OpenGL 3.1 Compatibility Profile context, but the version is configurable.

## Features

- Multiple independent window support
- Window creation and management
- OpenGL context creation (Compatibility Profile, configurable version)
- Keyboard and mouse input handling (5 buttons, Unicode character input)
- Raw mouse motion support (XInput2 on Linux, Raw Input on Windows)
- Mouse cursor shape control (arrow, text beam, resize handles, hand, etc.)
- Fullscreen toggling
- Window aspect ratio enforcement
- Window position get/set
- Window minimize, maximize, and restore
- Window icon (RGBA pixel data)
- Window size set/get
- Window decoration toggle (borderless windowed)
- Window opacity control
- Window minimized/maximized state queries
- Monitor enumeration (name, resolution, position, refresh rate, primary flag)
- Per-pixel transparency (composited windows with alpha channel)
- VSync control and query
- Cursor position get/set
- Content scale / DPI query
- Event callbacks (key, char, scroll, mouse, focus, resize, file drop, window state)
- Blocking event wait (for tool/editor apps)
- Window attention request (taskbar flash)
- Clipboard access (get/set UTF-8 text)
- File drag-and-drop
- Key name lookup for rebindable controls
- Window focus and mouse hover tracking

## Platform Support

- Linux (X11 with GLX)
- Windows (Win32 with WGL)

---

## Core Concepts

MKFW uses explicit state passing for all operations. Each window is represented by a `struct mkfw_state *` pointer, which is returned from `mkfw_init()` and must be passed to all subsequent API calls.

This design allows:
- Multiple independent windows in a single application
- No global state conflicts
- Thread-safe operation (when using separate states)
- Clear ownership semantics

---

## OpenGL Version Configuration

By default, MKFW creates an OpenGL 3.1 Compatibility Profile context. You can request a different version before calling `mkfw_init()`, and optionally query the maximum version the driver supports.

### `mkfw_set_gl_version`

```c
void mkfw_set_gl_version(int major, int minor)
```

Set the OpenGL version to request when creating the context. Call before `mkfw_init()`.

**Parameters:**
- `major` - OpenGL major version (e.g. 4)
- `minor` - OpenGL minor version (e.g. 6)

**Notes:**
- Defaults to 3.1 if never called
- Always uses the Compatibility Profile, so immediate mode functions remain available at any version
- If the requested version is not available, `mkfw_init()` will return NULL and fire the error callback with a message including the maximum supported version
- Module-level setting — applies to all subsequent `mkfw_init()` calls

**Example:**
```c
mkfw_set_gl_version(4, 6);
struct mkfw_state *window = mkfw_init(1280, 720);
if (!window) {
    // 4.6 not available — try a lower version
    mkfw_set_gl_version(4, 3);
    window = mkfw_init(1280, 720);
}
```

---

### `mkfw_query_max_gl_version`

```c
int mkfw_query_max_gl_version(int *major, int *minor)
```

Query the maximum OpenGL Compatibility Profile version supported by the driver. This creates a temporary display connection and context internally, then cleans up.

**Parameters:**
- `major` - Pointer to receive the major version
- `minor` - Pointer to receive the minor version

**Returns:**
- `1` on success
- `0` on failure (no GL support available)

**Notes:**
- Can be called before `mkfw_init()` to determine what versions are available
- Creates and destroys a temporary window/context — small one-time cost at startup
- The returned version is the maximum the driver supports with the Compatibility Profile

**Example:**
```c
int max_major, max_minor;
if (mkfw_query_max_gl_version(&max_major, &max_minor)) {
    printf("Driver supports up to OpenGL %d.%d\n", max_major, max_minor);
    mkfw_set_gl_version(max_major, max_minor);
} else {
    // No OpenGL support — mkfw_init will also fail
}

struct mkfw_state *window = mkfw_init(1280, 720);
```

---

## Per-Pixel Transparency

### `mkfw_set_transparent`

```c
void mkfw_set_transparent(int enable)
```

Enable per-pixel transparency for the window. Call before `mkfw_init()`.

**Parameters:**
- `enable` - Non-zero to enable transparency

**Notes:**
- Requires a compositor (X11: picom/kwin/mutter, Windows: DWM)
- On Linux: Selects an ARGB visual with alpha channel support
- On Windows: Uses `DwmExtendFrameIntoClientArea` to enable glass (loaded dynamically, no dwmapi.h dependency)
- Clear with `glClearColor(0, 0, 0, 0)` and use `GL_BLEND` for transparent areas
- Module-level setting -- applies to all subsequent `mkfw_init()` calls

**Example:**
```c
mkfw_set_transparent(1);
struct mkfw_state *window = mkfw_init(800, 600);

// In render loop:
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Fully transparent background
glClear(GL_COLOR_BUFFER_BIT);
// Draw with alpha < 1.0 for semi-transparent areas
```

---

## Initialization

### `mkfw_init`

```c
struct mkfw_state *mkfw_init(int width, int height)
```

Initialize the windowing system and create a window.

**Parameters:**
- `width` - Initial window width in pixels
- `height` - Initial window height in pixels

**Returns:**
- Pointer to window state on success
- `NULL` on failure

**Notes:**
- Creates an OpenGL Compatibility Profile context at the version set by `mkfw_set_gl_version()` (default: 3.1)
- Sets up X11/Win32 event handling
- Enables XInput2 raw motion on Linux
- Returns NULL on failure instead of calling exit()
- If the requested OpenGL version is not available, returns NULL and fires the error callback with details
- Call `mkfw_show_window()` after init to display the window

**Example:**
```c
struct mkfw_state *window = mkfw_init(1280, 720);
if (!window) {
    fprintf(stderr, "Failed to create window\n");
    return -1;
}
```

---

### `mkfw_show_window`

```c
void mkfw_show_window(struct mkfw_state *state)
```

Show the window on screen. The window is created hidden by `mkfw_init()` and must be shown explicitly.

**Parameters:**
- `state` - Window state pointer returned from `mkfw_init()`

**Example:**
```c
struct mkfw_state *window = mkfw_init(1280, 720);
mkfw_show_window(window);
```

---

### `mkfw_cleanup`

```c
void mkfw_cleanup(struct mkfw_state *state)
```

Clean up resources and close the window.

**Parameters:**
- `state` - Window state pointer returned from `mkfw_init()`

**Notes:**
- Destroys the OpenGL context
- Closes the display connection
- Frees the state structure
- Should be called before program exit

**Example:**
```c
mkfw_cleanup(window);
```

---

## Window Management

### `mkfw_set_window_title`

```c
void mkfw_set_window_title(struct mkfw_state *state, const char *title)
```

Set the window title text.

**Parameters:**
- `state` - Window state pointer
- `title` - Null-terminated UTF-8 string

**Example:**
```c
mkfw_set_window_title(window, "My Game v1.0");
```

---

### `mkfw_set_window_min_size_and_aspect`

```c
void mkfw_set_window_min_size_and_aspect(struct mkfw_state *state,
                                          int min_width, int min_height,
                                          float aspect_width, float aspect_height)
```

Set minimum window size and enforce aspect ratio.

**Parameters:**
- `state` - Window state pointer
- `min_width` - Minimum window width in pixels
- `min_height` - Minimum window height in pixels
- `aspect_width` - Aspect ratio numerator
- `aspect_height` - Aspect ratio denominator

**Example:**
```c
// 4:3 aspect ratio, minimum 640x480
mkfw_set_window_min_size_and_aspect(window, 640, 480, 4.0f, 3.0f);
```

---

### `mkfw_set_window_resizable`

```c
void mkfw_set_window_resizable(struct mkfw_state *state, int resizable)
```

Enable or disable window resizing.

**Parameters:**
- `state` - Window state pointer
- `resizable` - Non-zero to make window resizable, 0 to lock current size

**Notes:**
- When set to non-resizable, the window size is locked to its current dimensions
- The window can still be moved by dragging the title bar
- On Windows: removes the resize border and maximize button
- On X11: sets min and max size hints to the current window size
- Can be called at any time to toggle resizability

**Example:**
```c
// Make window non-resizable
mkfw_set_window_resizable(window, 0);

// Later, make it resizable again
mkfw_set_window_resizable(window, 1);
```

---

### `mkfw_set_window_icon`

```c
void mkfw_set_window_icon(struct mkfw_state *state, int32_t width, int32_t height, const uint8_t *rgba)
```

Set the window icon from RGBA pixel data.

**Parameters:**
- `state` - Window state pointer
- `width` - Icon width in pixels
- `height` - Icon height in pixels
- `rgba` - Pointer to `width * height * 4` bytes of RGBA pixel data (8 bits per channel)

**Notes:**
- On Linux: Sets `_NET_WM_ICON` property (ARGB format, converted internally)
- On Windows: Creates an icon via `CreateIconIndirect` from a DIB section (RGBA to BGRA conversion)
- Common icon sizes: 32x32, 48x48, 64x64
- Pass `NULL` for `rgba` to reset to default icon

**Example:**
```c
// Load icon pixels from your image loader
uint8_t *icon_rgba = load_image("icon.png", &w, &h);
mkfw_set_window_icon(window, w, h, icon_rgba);
```

---

### `mkfw_set_window_position`

```c
void mkfw_set_window_position(struct mkfw_state *state, int32_t x, int32_t y)
```

Set the window position on screen.

**Parameters:**
- `state` - Window state pointer
- `x` - X position in pixels
- `y` - Y position in pixels

---

### `mkfw_get_window_position`

```c
void mkfw_get_window_position(struct mkfw_state *state, int32_t *x, int32_t *y)
```

Get the current window position.

**Parameters:**
- `state` - Window state pointer
- `x` - Pointer to receive X position
- `y` - Pointer to receive Y position

---

### `mkfw_maximize_window`

```c
void mkfw_maximize_window(struct mkfw_state *state)
```

Maximize the window.

**Notes:**
- On Linux: Sends `_NET_WM_STATE_MAXIMIZED_HORZ` and `_NET_WM_STATE_MAXIMIZED_VERT` via client message
- On Windows: Calls `ShowWindow(hwnd, SW_MAXIMIZE)`

---

### `mkfw_minimize_window`

```c
void mkfw_minimize_window(struct mkfw_state *state)
```

Minimize (iconify) the window.

---

### `mkfw_restore_window`

```c
void mkfw_restore_window(struct mkfw_state *state)
```

Restore the window from maximized or minimized state.

---

### `mkfw_get_framebuffer_size`

```c
void mkfw_get_framebuffer_size(struct mkfw_state *state, int *width, int *height)
```

Query the current framebuffer dimensions.

**Parameters:**
- `state` - Window state pointer
- `width` - Pointer to receive width
- `height` - Pointer to receive height

**Example:**
```c
int w, h;
mkfw_get_framebuffer_size(window, &w, &h);
```

---

### `mkfw_set_window_size`

```c
void mkfw_set_window_size(struct mkfw_state *state, int32_t width, int32_t height)
```

Resize the window's client area to the specified dimensions.

**Parameters:**
- `state` - Window state pointer
- `width` - New client area width in pixels
- `height` - New client area height in pixels

**Notes:**
- Sets the client area size (not including title bar and borders)
- On Windows: uses `AdjustWindowRectEx` to account for decorations
- On Linux: calls `XResizeWindow` directly (X11 window size is the client area)

**Example:**
```c
mkfw_set_window_size(window, 1920, 1080);
```

---

### `mkfw_hide_window`

```c
void mkfw_hide_window(struct mkfw_state *state)
```

Hide the window. The window remains valid and can be shown again with `mkfw_show_window()`.

**Parameters:**
- `state` - Window state pointer

**Notes:**
- On Linux: calls `XUnmapWindow`
- On Windows: calls `ShowWindow(hwnd, SW_HIDE)`
- Useful for creating windows hidden, configuring them, then showing

**Example:**
```c
mkfw_hide_window(window);
// ... reconfigure ...
mkfw_show_window(window);
```

---

### `mkfw_is_minimized`

```c
int32_t mkfw_is_minimized(struct mkfw_state *state)
```

Check if the window is currently minimized (iconified).

**Returns:**
- Non-zero if the window is minimized

**Notes:**
- On Windows: uses `IsIconic()`
- On Linux: reads the `WM_STATE` property (iconic state = 3)

---

### `mkfw_is_maximized`

```c
int32_t mkfw_is_maximized(struct mkfw_state *state)
```

Check if the window is currently maximized.

**Returns:**
- Non-zero if the window is maximized (both horizontally and vertically)

**Notes:**
- On Windows: uses `IsZoomed()`
- On Linux: reads `_NET_WM_STATE` for `_NET_WM_STATE_MAXIMIZED_HORZ` and `_NET_WM_STATE_MAXIMIZED_VERT`

---

### `mkfw_set_window_decorated`

```c
void mkfw_set_window_decorated(struct mkfw_state *state, int32_t decorated)
```

Enable or disable window decorations (title bar, borders) at runtime.

**Parameters:**
- `state` - Window state pointer
- `decorated` - Non-zero for decorated, 0 for borderless

**Notes:**
- On Windows: toggles between `WS_OVERLAPPEDWINDOW` and `WS_POPUP` styles, preserving client area size
- On Linux: sets `_MOTIF_WM_HINTS` to control decoration visibility
- Useful for borderless windowed mode

**Example:**
```c
mkfw_set_window_decorated(window, 0); // Borderless
mkfw_set_window_decorated(window, 1); // Restore decorations
```

---

### `mkfw_set_window_opacity`

```c
void mkfw_set_window_opacity(struct mkfw_state *state, float opacity)
```

Set whole-window opacity (not per-pixel transparency).

**Parameters:**
- `state` - Window state pointer
- `opacity` - Opacity value from 0.0 (fully transparent) to 1.0 (fully opaque)

**Notes:**
- Distinct from `mkfw_set_transparent()` which enables per-pixel alpha blending
- On Windows: uses `WS_EX_LAYERED` with `SetLayeredWindowAttributes`
- On Linux: sets `_NET_WM_WINDOW_OPACITY` property (requires compositor)
- Setting opacity to 1.0 removes the layered/opacity property entirely

**Example:**
```c
mkfw_set_window_opacity(window, 0.8f); // 80% opaque
```

---

### `mkfw_fullscreen`

```c
void mkfw_fullscreen(struct mkfw_state *state, int enable)
```

Toggle fullscreen mode.

**Parameters:**
- `state` - Window state pointer
- `enable` - 1 to enable fullscreen, 0 to restore windowed mode

**Notes:**
- Uses `_NET_WM_STATE_FULLSCREEN` on Linux
- Saves and restores window position and size
- Hides cursor when entering fullscreen

---

### `mkfw_get_monitors`

```c
int32_t mkfw_get_monitors(struct mkfw_state *state, struct mkfw_monitor *out, int32_t max)
```

Enumerate connected monitors.

**Parameters:**
- `state` - Window state pointer
- `out` - Array of `struct mkfw_monitor` to fill
- `max` - Maximum number of monitors to return (use `MKFW_MAX_MONITORS` for the full set)

**Returns:**
- Number of monitors found

**Monitor struct fields:**
- `name[128]` - Monitor name/identifier
- `x`, `y` - Position in virtual screen coordinates
- `width`, `height` - Resolution in pixels
- `refresh_rate` - Refresh rate in Hz
- `primary` - Non-zero if this is the primary monitor

**Notes:**
- On Linux: Uses Xrandr to query connected outputs with active CRTCs
- On Windows: Uses `EnumDisplayMonitors` and `EnumDisplaySettings`
- Returns only currently active monitors (not disconnected outputs)

**Example:**
```c
struct mkfw_monitor monitors[MKFW_MAX_MONITORS];
int32_t count = mkfw_get_monitors(window, monitors, MKFW_MAX_MONITORS);

for (int32_t i = 0; i < count; i++) {
    printf("%s: %dx%d @ %dHz%s\n",
        monitors[i].name,
        monitors[i].width, monitors[i].height,
        monitors[i].refresh_rate,
        monitors[i].primary ? " (primary)" : "");
}
```

---

### `mkfw_should_close`

```c
int mkfw_should_close(struct mkfw_state *state)
```

Check if the window should close.

**Parameters:**
- `state` - Window state pointer

**Returns:**
- Non-zero if user requested close (window close button)

**Example:**
```c
while (!mkfw_should_close(window)) {
    // main loop
}
```

---

### `mkfw_set_should_close`

```c
void mkfw_set_should_close(struct mkfw_state *state, int value)
```

Manually set the close flag.

**Parameters:**
- `state` - Window state pointer
- `value` - Non-zero to request close

---

### `mkfw_get_content_scale`

```c
float mkfw_get_content_scale(struct mkfw_state *state)
```

Query the DPI scale factor for the window's display.

**Parameters:**
- `state` - Window state pointer

**Returns:**
- Scale factor as a float (1.0 = standard 96 DPI, 2.0 = Retina / 200%)

**Notes:**
- Use this to scale UI elements, font sizes, and other resolution-dependent content
- On Linux, reads `Xft.dpi` from the X resource database, falls back to computing from physical display dimensions
- On Windows, uses `GetDpiForWindow` (Windows 10 1607+) with `GetDeviceCaps` fallback
- On Emscripten, returns `window.devicePixelRatio`
- Call after `mkfw_init()`, and again after receiving a framebuffer size callback if DPI may have changed (e.g. dragging window between monitors)

**Example:**
```c
float scale = mkfw_get_content_scale(window);
int font_size = (int)(14.0f * scale);
printf("DPI scale: %.2f, font size: %d\n", scale, font_size);
```

---

### `mkfw_request_attention`

```c
void mkfw_request_attention(struct mkfw_state *state)
```

Request user attention by flashing the taskbar/dock entry.

**Parameters:**
- `state` - Window state pointer

**Notes:**
- On Windows, uses `FlashWindowEx` with `FLASHW_ALL | FLASHW_TIMERNOFG` -- flashes 3 times, stops when window gains focus
- On Linux, sets `_NET_WM_STATE_DEMANDS_ATTENTION` -- supported by GNOME, KDE, and most X11 window managers
- On Emscripten, this is a no-op (no taskbar concept in browser)
- Safe to call when the window already has focus (no effect)

**Example:**
```c
// Notify the user that a background task completed
if(!mkfw_has_focus(window)) {
    mkfw_request_attention(window);
}
```

---

### `mkfw_set_window_state_callback`

```c
void mkfw_set_window_state_callback(struct mkfw_state *state, window_state_callback_t callback)
```

Register a callback that fires when the window transitions between normal, maximized, and minimized states.

**Parameters:**
- `state` - Window state pointer
- `callback` - Function pointer matching `void (*)(struct mkfw_state *state, uint8_t maximized, uint8_t minimized)`, or NULL to remove

**Callback parameters:**
- `maximized` - 1 if the window is now maximized, 0 otherwise
- `minimized` - 1 if the window is now minimized (iconified), 0 otherwise
- Both 0 means the window was restored to normal size

**Notes:**
- Only fires on state transitions, not every resize
- On Emscripten, fires when tab visibility changes (hidden = minimized, visible = restored)
- Useful for pausing audio/rendering when minimized, or adjusting viewport on maximize

**Example:**
```c
void on_window_state(struct mkfw_state *window, uint8_t maximized, uint8_t minimized) {
    if(minimized) {
        pause_audio();
    } else {
        resume_audio();
    }
}

mkfw_set_window_state_callback(window, on_window_state);
```

---

## Input Handling

### Keyboard

#### Keyboard State Access

Each window state contains a `keyboard_state` array tracking current key states. Access directly via `state->keyboard_state[MKS_KEY_*]`.

#### Key Codes

**ASCII Printable Characters:**
- `MKS_KEY_SPACE` (0x20) through `MKS_KEY_TILDE` (0x7E)
- Letters: `MKS_KEY_A` through `MKS_KEY_Z` (lowercase 'a'-'z')
- Numbers: `MKS_KEY_0` through `MKS_KEY_9`

**Special Keys:**
- `MKS_KEY_ESCAPE`, `MKS_KEY_RETURN`, `MKS_KEY_TAB`, `MKS_KEY_BACKSPACE`
- `MKS_KEY_LEFT`, `MKS_KEY_RIGHT`, `MKS_KEY_UP`, `MKS_KEY_DOWN`
- `MKS_KEY_INSERT`, `MKS_KEY_DELETE`, `MKS_KEY_HOME`, `MKS_KEY_END`
- `MKS_KEY_PAGEUP`, `MKS_KEY_PAGEDOWN`
- `MKS_KEY_CAPSLOCK`, `MKS_KEY_NUMLOCK`, `MKS_KEY_SCROLLLOCK`
- `MKS_KEY_PRINTSCREEN`, `MKS_KEY_PAUSE`, `MKS_KEY_MENU`

**Modifiers:**
- `MKS_KEY_SHIFT`, `MKS_KEY_LSHIFT`, `MKS_KEY_RSHIFT`
- `MKS_KEY_CTRL`, `MKS_KEY_LCTRL`, `MKS_KEY_RCTRL`
- `MKS_KEY_ALT`, `MKS_KEY_LALT`, `MKS_KEY_RALT`
- `MKS_KEY_LSUPER`, `MKS_KEY_RSUPER` (Windows/Super key)

**Function Keys:**
- `MKS_KEY_F1` through `MKS_KEY_F12`

**Numpad:**
- `MKS_KEY_NUMPAD_0` through `MKS_KEY_NUMPAD_9`
- `MKS_KEY_NUMPAD_DECIMAL`, `MKS_KEY_NUMPAD_DIVIDE`
- `MKS_KEY_NUMPAD_MULTIPLY`, `MKS_KEY_NUMPAD_SUBTRACT`
- `MKS_KEY_NUMPAD_ADD`, `MKS_KEY_NUMPAD_ENTER`

#### Key Actions

- `MKS_PRESSED` - Key was pressed
- `MKS_RELEASED` - Key was released

#### Modifier Bits

- `MKS_MOD_SHIFT` - Either shift key
- `MKS_MOD_CTRL` - Either control key
- `MKS_MOD_ALT` - Either alt key
- `MKS_MOD_SUPER` - Either super/Windows key
- Individual bits: `MKS_MOD_LSHIFT`, `MKS_MOD_RSHIFT`, `MKS_MOD_LCTRL`, `MKS_MOD_RCTRL`, `MKS_MOD_LALT`, `MKS_MOD_RALT`, `MKS_MOD_LSUPER`, `MKS_MOD_RSUPER`

---

### `mkfw_is_key_pressed`

```c
int mkfw_is_key_pressed(struct mkfw_state *state, uint8_t key)
```

Check if a key was just pressed (edge detection).

**Parameters:**
- `state` - Window state pointer
- `key` - Key code from `MKS_KEY_*` constants

**Returns:**
- Non-zero if key was pressed this frame

**Example:**
```c
if (mkfw_is_key_pressed(window, MKS_KEY_SPACE)) {
    player_jump();
}
```

---

### `mkfw_was_key_released`

```c
int mkfw_was_key_released(struct mkfw_state *state, uint8_t key)
```

Check if a key was just released (edge detection).

**Parameters:**
- `state` - Window state pointer
- `key` - Key code from `MKS_KEY_*` constants

**Returns:**
- Non-zero if key was released this frame

---

### `mkfw_update_input_state`

```c
void mkfw_update_input_state(struct mkfw_state *state)
```

Update previous keyboard and mouse button state. Call once per frame after processing input. Required for edge-detection functions (`mkfw_is_key_pressed`, `mkfw_was_key_released`, `mkfw_is_button_pressed`, `mkfw_was_button_released`).

**Parameters:**
- `state` - Window state pointer

**Example:**
```c
mkfw_pump_messages(window);
// ... handle input ...
mkfw_update_input_state(window);
```

---

### `mkfw_get_key_name`

```c
const char *mkfw_get_key_name(uint32_t key)
```

Get a human-readable name for a key code.

**Parameters:**
- `key` - Key code from `MKS_KEY_*` constants

**Returns:**
- Pointer to a string literal with the key name (e.g. "A", "Space", "Left Ctrl", "F1")
- Returns "Unknown" for unrecognized key codes

**Notes:**
- Platform-independent (implemented in mkfw.h)
- Letters are returned as uppercase ("A" through "Z")
- Useful for key binding UIs and debug output

**Example:**
```c
void on_key(struct mkfw_state *state, uint32_t key, uint32_t action, uint32_t mods) {
    if(action == MKS_PRESSED) {
        printf("Pressed: %s\n", mkfw_get_key_name(key));
    }
}
```

---

### `mkfw_set_key_callback`

```c
typedef void (*key_callback_t)(struct mkfw_state *state, uint32_t key, uint32_t action, uint32_t mods);
void mkfw_set_key_callback(struct mkfw_state *state, key_callback_t callback)
```

Register a callback for keyboard events.

**Parameters:**
- `state` - Window state pointer

**Callback Parameters:**
- `state` - Window state pointer (access window-specific data in callback)
- `key` - Key code (`MKS_KEY_*`)
- `action` - `MKS_PRESSED` or `MKS_RELEASED`
- `mods` - Bitfield of active modifiers (`MKS_MOD_*`)

**Example:**
```c
void on_key(struct mkfw_state *state, uint32_t key, uint32_t action, uint32_t mods) {
    if (key == MKS_KEY_W && action == MKS_PRESSED) {
        if (mods & MKS_MOD_CTRL) {
            close_file();
        }
    }

    if (key == MKS_KEY_ESCAPE && action == MKS_PRESSED) {
        mkfw_set_should_close(state, 1);
    }
}

mkfw_set_key_callback(window, on_key);
```

---

### Mouse

#### Mouse State Access

Each window state contains a mouse button state array.

#### Mouse Buttons

- `MOUSE_BUTTON_LEFT` (0)
- `MOUSE_BUTTON_MIDDLE` (1)
- `MOUSE_BUTTON_RIGHT` (2)
- `MOUSE_BUTTON_EXTRA1` (3) - Side button (back)
- `MOUSE_BUTTON_EXTRA2` (4) - Side button (forward)

#### Mouse Actions

- `MKS_PRESSED`
- `MKS_RELEASED`

---

### `mkfw_is_button_pressed`

```c
uint8_t mkfw_is_button_pressed(struct mkfw_state *state, uint8_t button)
```

Check if a mouse button was just pressed.

**Parameters:**
- `state` - Window state pointer
- `button` - Button index (0-4, `MOUSE_BUTTON_*`)

**Returns:**
- Non-zero if button was pressed this frame

---

### `mkfw_was_button_released`

```c
uint8_t mkfw_was_button_released(struct mkfw_state *state, uint8_t button)
```

Check if a mouse button was just released.

**Parameters:**
- `state` - Window state pointer
- `button` - Button index (0-4, `MOUSE_BUTTON_*`)

**Returns:**
- Non-zero if button was released this frame

---

### `mkfw_set_mouse_cursor`

```c
void mkfw_set_mouse_cursor(struct mkfw_state *state, int visible)
```

Show or hide the mouse cursor.

**Parameters:**
- `state` - Window state pointer
- `visible` - Non-zero to show cursor, 0 to hide

**Notes:**
- Hides cursor by creating invisible pixmap cursor
- Automatically constrains mouse when hidden
- Restores cursor position when shown again

---

### `mkfw_constrain_mouse`

```c
void mkfw_constrain_mouse(struct mkfw_state *state, int constrain)
```

Constrain mouse to window (grab pointer).

**Parameters:**
- `state` - Window state pointer
- `constrain` - Non-zero to constrain, 0 to release

**Notes:**
- Warps pointer to window center when constrained
- Used for FPS-style mouse look

---

### `mkfw_set_mouse_move_delta_callback`

```c
typedef void (*mouse_move_delta_callback_t)(struct mkfw_state *state, int32_t dx, int32_t dy);
void mkfw_set_mouse_move_delta_callback(struct mkfw_state *state, mouse_move_delta_callback_t callback)
```

Register a callback for raw mouse motion.

**Parameters:**
- `state` - Window state pointer

**Callback Parameters:**
- `state` - Window state pointer
- `dx` - Horizontal movement delta in pixels
- `dy` - Vertical movement delta in pixels

**Notes:**
- Uses XInput2 raw motion on Linux, Raw Input API on Windows
- Called for relative mouse movement

**Example:**
```c
void on_mouse_move(struct mkfw_state *state, int32_t dx, int32_t dy) {
    camera_rotate(dx * 0.1f, dy * 0.1f);
}

mkfw_set_mouse_move_delta_callback(window, on_mouse_move);
```

---

### `mkfw_set_mouse_button_callback`

```c
typedef void (*mouse_button_callback_t)(struct mkfw_state *state, uint8_t button, int action);
void mkfw_set_mouse_button_callback(struct mkfw_state *state, mouse_button_callback_t callback)
```

Register a callback for mouse button events.

**Parameters:**
- `state` - Window state pointer

**Callback Parameters:**
- `state` - Window state pointer
- `button` - Button index (`MOUSE_BUTTON_*`)
- `action` - `MKS_PRESSED` or `MKS_RELEASED`

**Example:**
```c
void on_mouse_button(struct mkfw_state *state, uint8_t button, int action) {
    if (button == MOUSE_BUTTON_LEFT && action == MKS_PRESSED) {
        handle_click();
    }
}

mkfw_set_mouse_button_callback(window, on_mouse_button);
```

---

### `mkfw_set_char_callback`

```c
typedef void (*char_callback_t)(struct mkfw_state *state, uint32_t codepoint);
void mkfw_set_char_callback(struct mkfw_state *state, char_callback_t callback)
```

Register a callback for character input events.

**Parameters:**
- `state` - Window state pointer

**Callback Parameters:**
- `state` - Window state pointer
- `codepoint` - Unicode codepoint (backspace=8, printable >= 32)

**Notes:**
- Used for text input (typing characters)
- Delivers full Unicode codepoints (not just ASCII)
- On Linux: uses XIM (X Input Method) via `Xutf8LookupString` for international input
- On Windows: handles UTF-16 surrogate pairs from `WM_CHAR`
- Filters out control characters except backspace
- Distinct from key callback (handles translated characters, not raw keys)

---

### `mkfw_set_scroll_callback`

```c
typedef void (*scroll_callback_t)(struct mkfw_state *state, double xoffset, double yoffset);
void mkfw_set_scroll_callback(struct mkfw_state *state, scroll_callback_t callback)
```

Register a callback for mouse scroll events.

**Parameters:**
- `state` - Window state pointer

**Callback Parameters:**
- `state` - Window state pointer
- `xoffset` - Horizontal scroll offset
- `yoffset` - Vertical scroll offset (+1.0 = scroll up, -1.0 = scroll down)

---

### `mkfw_set_mouse_sensitivity`

```c
void mkfw_set_mouse_sensitivity(struct mkfw_state *state, double sensitivity)
```

Set mouse sensitivity multiplier for the accumulated delta API.

**Parameters:**
- `state` - Window state pointer
- `sensitivity` - Sensitivity multiplier (default: 1.0)

---

### `mkfw_get_and_clear_mouse_delta`

```c
void mkfw_get_and_clear_mouse_delta(struct mkfw_state *state, int32_t *dx, int32_t *dy)
```

Get accumulated mouse delta since last call, then reset the accumulator. Keeps fractional remainder for sub-pixel precision over time.

**Parameters:**
- `state` - Window state pointer
- `dx` - Pointer to receive horizontal delta
- `dy` - Pointer to receive vertical delta

**Notes:**
- Alternative to the delta callback for polling-style mouse input
- Sensitivity scaling is applied before accumulation
- Fractional values are preserved between calls for smooth motion

---

## Cursor Shapes

### `mkfw_set_cursor_shape`

```c
void mkfw_set_cursor_shape(struct mkfw_state *state, uint32_t cursor)
```

Set the mouse cursor shape.

**Parameters:**
- `state` - Window state pointer
- `cursor` - Cursor type (`MKFW_CURSOR_*`)

**Cursor Types:**
- `MKFW_CURSOR_ARROW` - Default arrow cursor
- `MKFW_CURSOR_TEXT_INPUT` - I-beam for text fields
- `MKFW_CURSOR_RESIZE_ALL` - 4-way arrow (move)
- `MKFW_CURSOR_RESIZE_NS` - Vertical resize
- `MKFW_CURSOR_RESIZE_EW` - Horizontal resize
- `MKFW_CURSOR_RESIZE_NESW` - Diagonal resize (NE-SW)
- `MKFW_CURSOR_RESIZE_NWSE` - Diagonal resize (NW-SE)
- `MKFW_CURSOR_HAND` - Pointing hand (links)
- `MKFW_CURSOR_NOT_ALLOWED` - Not allowed / forbidden

**Notes:**
- Falls back to `MKFW_CURSOR_ARROW` if cursor >= `MKFW_CURSOR_LAST`
- On Linux: uses X11 font cursors
- On Windows: uses system cursors via `WM_SETCURSOR`

**Example:**
```c
mkfw_set_cursor_shape(window, MKFW_CURSOR_HAND);
```

---

## Cursor Position

### `mkfw_get_cursor_position`

```c
void mkfw_get_cursor_position(struct mkfw_state *state, int32_t *x, int32_t *y)
```

Get the cursor position relative to the window's client area.

**Parameters:**
- `state` - Window state pointer
- `x` - Pointer to receive X position
- `y` - Pointer to receive Y position

**Notes:**
- Coordinates are relative to the top-left corner of the client area
- On Windows: uses `GetCursorPos` + `ScreenToClient`
- On Linux: uses `XQueryPointer`

---

### `mkfw_set_cursor_position`

```c
void mkfw_set_cursor_position(struct mkfw_state *state, int32_t x, int32_t y)
```

Warp the cursor to a position relative to the window's client area.

**Parameters:**
- `state` - Window state pointer
- `x` - X position in client area pixels
- `y` - Y position in client area pixels

**Notes:**
- On Windows: uses `ClientToScreen` + `SetCursorPos`
- On Linux: uses `XWarpPointer`
- Useful for centering the cursor in FPS games or UI interactions

**Example:**
```c
int32_t cx, cy;
mkfw_get_cursor_position(window, &cx, &cy);
printf("Cursor at: %d, %d\n", cx, cy);

// Center cursor in window
int32_t w, h;
mkfw_get_framebuffer_size(window, &w, &h);
mkfw_set_cursor_position(window, w / 2, h / 2);
```

---

## Clipboard

### `mkfw_set_clipboard_text`

```c
void mkfw_set_clipboard_text(struct mkfw_state *state, const char *text)
```

Set the system clipboard to a UTF-8 string.

**Parameters:**
- `state` - Window state pointer
- `text` - Null-terminated UTF-8 string to copy to clipboard

**Notes:**
- On Linux: takes ownership of the X11 CLIPBOARD selection
- On Windows: uses `SetClipboardData(CF_UNICODETEXT)` with UTF-8 to UTF-16 conversion

---

### `mkfw_get_clipboard_text`

```c
const char *mkfw_get_clipboard_text(struct mkfw_state *state)
```

Get the current system clipboard text as UTF-8.

**Parameters:**
- `state` - Window state pointer

**Returns:**
- Pointer to a UTF-8 string owned by mkfw. Do not free. Valid until the next call to `mkfw_get_clipboard_text` or `mkfw_set_clipboard_text`.
- Returns `""` (empty string) if clipboard is empty or unavailable.

**Notes:**
- On Linux: requests the CLIPBOARD selection via `XConvertSelection`. Blocks briefly (up to ~500ms) waiting for the selection owner to respond.
- On Windows: uses `GetClipboardData(CF_UNICODETEXT)` with UTF-16 to UTF-8 conversion.

**Example:**
```c
mkfw_set_clipboard_text(window, "Hello clipboard");
const char *text = mkfw_get_clipboard_text(window);
printf("Clipboard: %s\n", text);
```

---

## Focus and Hover

### `mkfw_set_focus_callback`

```c
typedef void (*focus_callback_t)(struct mkfw_state *state, uint8_t focused);
void mkfw_set_focus_callback(struct mkfw_state *state, focus_callback_t callback)
```

Register a callback for window focus changes.

**Callback Parameters:**
- `state` - Window state pointer
- `focused` - 1 if window gained focus, 0 if lost

**Notes:**
- On Linux: handles `FocusIn`/`FocusOut` X11 events
- On Windows: handles `WM_SETFOCUS`/`WM_KILLFOCUS`

### Window State Fields

The `mkfw_state` struct exposes:
- `state->has_focus` - Non-zero if window currently has keyboard focus
- `state->mouse_in_window` - Non-zero if mouse cursor is inside the window client area

These are updated automatically by `mkfw_pump_messages()`.

---

## File Drop

### `mkfw_set_drop_callback`

```c
typedef void (*drop_callback_t)(uint32_t count, const char **paths);
void mkfw_set_drop_callback(struct mkfw_state *state, drop_callback_t callback)
```

Register a callback for file drag-and-drop events. Setting a callback enables drop acceptance on the window. Setting it to `NULL` disables drops.

**Callback Parameters:**
- `count` - Number of files dropped
- `paths` - Array of `count` null-terminated UTF-8 file paths

**Important:** mkfw owns the `paths` array and all strings in it. Both the array and every path string are freed immediately after the callback returns. If you need to keep any paths, you must copy them (e.g. with `strdup`) inside the callback.

**Platform details:**
- Linux: implements the XDND protocol (version 5), accepting `text/uri-list` drops. Paths are percent-decoded and `file://` prefixes are stripped.
- Windows: uses `DragAcceptFiles` / `WM_DROPFILES`. Wide paths are converted to UTF-8.

**Example:**
```c
void on_drop(uint32_t count, const char **paths) {
    for(uint32_t i = 0; i < count; ++i) {
        // paths[i] is only valid for the duration of this callback
        char *copy = strdup(paths[i]);
        printf("Dropped: %s\n", copy);
        // ... store copy somewhere, free it later ...
    }
}

mkfw_set_drop_callback(window, on_drop);
```

---

## Rendering

### `mkfw_attach_context`

```c
void mkfw_attach_context(struct mkfw_state *state)
```

Make the OpenGL context current on the calling thread.

**Parameters:**
- `state` - Window state pointer

**Notes:**
- Required before making OpenGL calls
- Used when transferring context between threads

---

### `mkfw_detach_context`

```c
void mkfw_detach_context(struct mkfw_state *state)
```

Release the OpenGL context from the calling thread.

**Parameters:**
- `state` - Window state pointer

---

### `mkfw_swap_buffers`

```c
void mkfw_swap_buffers(struct mkfw_state *state)
```

Swap front and back buffers (present frame).

**Parameters:**
- `state` - Window state pointer

**Example:**
```c
// Render scene
glClear(GL_COLOR_BUFFER_BIT);
// ... draw calls ...
mkfw_swap_buffers(window);
```

---

### `mkfw_set_swapinterval`

```c
void mkfw_set_swapinterval(struct mkfw_state *state, uint32_t interval)
```

Control VSync behavior.

**Parameters:**
- `state` - Window state pointer
- `interval` - 0 for immediate updates (no VSync)
- 1 for synchronized to vertical refresh
- N for updates every N frames

**Example:**
```c
mkfw_set_swapinterval(window, 0); // Disable VSync
```

---

### `mkfw_get_swapinterval`

```c
int32_t mkfw_get_swapinterval(struct mkfw_state *state)
```

Query the current swap interval (VSync setting).

**Parameters:**
- `state` - Window state pointer

**Returns:**
- Current swap interval value (0 = no VSync, 1 = VSync, etc.)
- Returns 0 if the query extension is not available

**Notes:**
- On Windows: uses `wglGetSwapIntervalEXT`
- On Linux: uses `glXQueryDrawable` with `GLX_SWAP_INTERVAL_EXT`

**Example:**
```c
int32_t vsync = mkfw_get_swapinterval(window);
printf("VSync: %s\n", vsync ? "on" : "off");
```

---

### `mkfw_set_framebuffer_size_callback`

```c
typedef void (*framebuffer_callback_t)(struct mkfw_state *state, int32_t width, int32_t height, float aspect_ratio);
void mkfw_set_framebuffer_size_callback(struct mkfw_state *state, framebuffer_callback_t callback)
```

Register a callback for framebuffer size changes.

**Parameters:**
- `state` - Window state pointer

**Callback Parameters:**
- `state` - Window state pointer
- `width` - New framebuffer width
- `height` - New framebuffer height
- `aspect_ratio` - Configured aspect ratio (or 0.0 if none)

**Notes:**
- Called when window is resized
- Use to update viewport and projection matrices

**Example:**
```c
void on_resize(struct mkfw_state *state, int32_t w, int32_t h, float aspect) {
    glViewport(0, 0, w, h);
    update_projection_matrix(w, h);
}

mkfw_set_framebuffer_size_callback(window, on_resize);
```

---

## Timing

### `mkfw_gettime`

```c
uint64_t mkfw_gettime(struct mkfw_state *state)
```

Get high-resolution monotonic time.

**Parameters:**
- `state` - Window state pointer

**Returns:**
- Time in nanoseconds

**Example:**
```c
uint64_t start = mkfw_gettime(window);
do_work();
uint64_t elapsed = mkfw_gettime(window) - start;
printf("Took %llu ns\n", elapsed);
```

---

### `mkfw_sleep`

```c
void mkfw_sleep(uint64_t nanoseconds)
```

Sleep for a specified duration.

**Parameters:**
- `nanoseconds` - Duration to sleep

**Notes:**
- This is a basic sleep function, not a precision timer
- Accuracy: ~1-2ms on Windows, ~50µs-1ms on Linux (scheduler-dependent)
- Suitable for coarse delays, background threads, or non-critical timing
- **Not suitable for precise frame timing** - use `MKFW_TIMER` subsystem instead
- On Windows: Uses `SetWaitableTimer()` (depends on system timer resolution)
- On Linux: Uses `nanosleep()` (subject to scheduler granularity)

**Example:**
```c
// Coarse sleep for event polling thread
mkfw_sleep(5000000); // ~5ms

// For precise frame timing, use mkfw_timer instead:
// struct mkfw_timer_handle *timer = mkfw_timer_new(16666666);
// mkfw_timer_wait(timer);
```

---

## Event Processing

### `mkfw_pump_messages`

```c
void mkfw_pump_messages(struct mkfw_state *state)
```

Process pending window and input events.

**Parameters:**
- `state` - Window state pointer

**Notes:**
- Must be called regularly (typically once per frame)
- Updates keyboard and mouse state arrays
- Dispatches registered callbacks
- Handles window close requests

**Example main loop:**
```c
while (!mkfw_should_close(window)) {
    mkfw_pump_messages(window);

    // Handle input
    if (mkfw_is_key_pressed(window, MKS_KEY_ESCAPE)) {
        break;
    }

    // Update game state
    update_game();

    // Render
    render_scene();
    mkfw_swap_buffers(window);

    // Update state tracking (required for edge-detection functions)
    mkfw_update_input_state(window);
}
```

---

### `mkfw_wait_events`

```c
void mkfw_wait_events(struct mkfw_state *state)
```

Block until at least one event is available, then process all pending events.

**Parameters:**
- `state` - Window state pointer

**Notes:**
- Designed for tool/editor applications that only need to redraw on user input
- Saves CPU by sleeping instead of spinning in a poll loop
- On Linux, uses `poll()` on the X11 connection file descriptor
- On Windows, uses `WaitMessage()`
- On Emscripten, this is a no-op (browser event loop is always non-blocking)

**Example:**
```c
while (!mkfw_should_close(window)) {
    mkfw_wait_events(window);
    redraw_ui();
    mkfw_swap_buffers(window);
    mkfw_update_input_state(window);
}
```

---

### `mkfw_wait_events_timeout`

```c
void mkfw_wait_events_timeout(struct mkfw_state *state, uint64_t nanoseconds)
```

Block until an event arrives or the timeout expires, then process all pending events.

**Parameters:**
- `state` - Window state pointer
- `nanoseconds` - Maximum time to wait in nanoseconds (consistent with `mkfw_sleep` and `mkfw_gettime`)

**Notes:**
- Useful for apps that need periodic updates (blinking cursors, animations) even without user input
- On Emscripten, this is a no-op

**Example:**
```c
while (!mkfw_should_close(window)) {
    // Wait up to 500ms for input, then redraw regardless (for blinking cursor)
    mkfw_wait_events_timeout(window, 500000000ULL);
    redraw_editor();
    mkfw_swap_buffers(window);
    mkfw_update_input_state(window);
}
```

---

## Typical Usage Pattern

### Single Window Application

```c
#include "mkfw_gl_loader.h"
#include "mkfw.h"

void on_key(struct mkfw_state *state, uint32_t key, uint32_t action, uint32_t mods) {
    if (key == MKS_KEY_ESCAPE && action == MKS_PRESSED) {
        mkfw_set_should_close(state, 1);
    }
}

void on_resize(struct mkfw_state *state, int32_t w, int32_t h, float aspect) {
    glViewport(0, 0, w, h);
}

int main(void) {
    // Optional: request a specific OpenGL version (default is 3.1)
    // mkfw_set_gl_version(4, 6);

    // Initialize
    struct mkfw_state *window = mkfw_init(1280, 720);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        return -1;
    }

    mkfw_set_window_title(window, "My Application");
    mkfw_show_window(window);
    mkfw_set_swapinterval(window, 1);

    // Set callbacks
    mkfw_set_key_callback(window, on_key);
    mkfw_set_framebuffer_size_callback(window, on_resize);

    // Configure window
    mkfw_set_window_min_size_and_aspect(window, 640, 480, 16.0f, 9.0f);

    // Load OpenGL functions
    mkfw_gl_loader();

    // Main loop
    while (!mkfw_should_close(window)) {
        mkfw_pump_messages(window);

        // Input handling
        if (mkfw_is_key_pressed(window, MKS_KEY_F11)) {
            static int fullscreen = 0;
            fullscreen = !fullscreen;
            mkfw_fullscreen(window, fullscreen);
        }

        // Rendering
        glClear(GL_COLOR_BUFFER_BIT);
        // ... draw calls ...
        mkfw_swap_buffers(window);

        // Update state tracking
        mkfw_update_input_state(window);
    }

    // Cleanup
    mkfw_cleanup(window);
    return 0;
}
```

### Multiple Window Application

```c
#include "mkfw_gl_loader.h"
#include "mkfw.h"

void on_key_main(struct mkfw_state *state, uint32_t key, uint32_t action, uint32_t mods) {
    if (key == MKS_KEY_ESCAPE && action == MKS_PRESSED) {
        mkfw_set_should_close(state, 1);
    }
}

void on_key_options(struct mkfw_state *state, uint32_t key, uint32_t action, uint32_t mods) {
    // Handle options window keyboard
}

int main(void) {
    // Create main window
    struct mkfw_state *main_window = mkfw_init(1280, 720);
    if (!main_window) return -1;

    mkfw_set_window_title(main_window, "Main Window");
    mkfw_show_window(main_window);
    mkfw_set_key_callback(main_window, on_key_main);

    // Create options window
    struct mkfw_state *options_window = mkfw_init(800, 600);
    if (!options_window) {
        mkfw_cleanup(main_window);
        return -1;
    }

    mkfw_set_window_title(options_window, "Options");
    mkfw_show_window(options_window);
    mkfw_set_key_callback(options_window, on_key_options);

    // Load OpenGL functions
    mkfw_gl_loader();

    // Main loop - handle both windows
    while (!mkfw_should_close(main_window) && !mkfw_should_close(options_window)) {
        // Process events for both windows
        mkfw_pump_messages(main_window);
        mkfw_pump_messages(options_window);

        // Render main window
        mkfw_attach_context(main_window);
        glClear(GL_COLOR_BUFFER_BIT);
        // ... render main ...
        mkfw_swap_buffers(main_window);

        // Render options window
        mkfw_attach_context(options_window);
        glClear(GL_COLOR_BUFFER_BIT);
        // ... render options ...
        mkfw_swap_buffers(options_window);

        // Update state tracking
        mkfw_update_input_state(main_window);
        mkfw_update_input_state(options_window);
    }

    // Cleanup
    mkfw_cleanup(main_window);
    mkfw_cleanup(options_window);
    return 0;
}
```

---

## Threading Notes

MKFW supports multi-threaded rendering with context management:

1. **Main thread handles events:**
   - Call `mkfw_pump_messages(state)` on main thread
   - Process input state

2. **Render thread handles OpenGL:**
   - Call `mkfw_detach_context(state)` on main thread before creating render thread
   - Call `mkfw_attach_context(state)` on render thread
   - Make all OpenGL calls on render thread

**Example:**
```c
struct mkfw_state *window = mkfw_init(1280, 720);
mkfw_show_window(window);

mkfw_detach_context(window);

pthread_create(&thread, NULL, render_func, window);

// Main thread
while (running) {
    mkfw_pump_messages(window);
    mkfw_sleep(5000000); // 5ms
}

pthread_join(thread, NULL);
mkfw_cleanup(window);
```

---

## Platform-Specific Notes

### Linux (X11)

- Requires X11, GLX, XInput2, and Xrandr development libraries
- Uses XInput2 for raw mouse motion
- Uses Xrandr for monitor enumeration
- Creates OpenGL Compatibility Profile context via `glXCreateContextAttribsARB`
- Handles `_NET_WM_STATE` for fullscreen

**Linking:**
```bash
gcc -o app main.c -lX11 -lGL -lXi -lXrandr -ldl -lpthread
```

### Windows

- Requires Win32 API and OpenGL
- Uses `GWLP_USERDATA` to associate window handles with state
- Implementation in `mkfw_win32.c`

**Linking:**
```bash
gcc -o app.exe main.c -lopengl32 -lgdi32 -lwinmm
```

---

## Implementation

MKFW is a header-only library with platform-specific implementations:

- `mkfw.h` - Main header with shared definitions and inline functions
- `mkfw_linux.c` - X11/GLX implementation for Linux
- `mkfw_win32.c` - Win32/WGL implementation for Windows
- `mkfw_glx_mini.h` - Minimal GLX function declarations for Linux

Include `mkfw.h` in your project. The platform implementation is selected automatically via preprocessor macros (`#ifdef _WIN32` / `__linux__`).

---

## Compatibility

- **OpenGL Version:** Configurable (default: 3.1 Compatibility Profile)
- **GLX Version:** 1.3+ (for `glXCreateContextAttribsARB`)
- **XInput Version:** 2.0+ (for raw motion)

The library always uses the Compatibility Profile, so immediate mode functions (glBegin, glEnd, etc.) are available at any requested version. Use `mkfw_set_gl_version()` to request higher versions for features like compute shaders (4.3+), SSBOs, etc.

---

## Differences from GLFW

MKFW is simpler than GLFW:

### Removed Features

- Video mode queries (resolution/refresh rate switching)
- Window hints system

### Simplified Features

- Always uses Compatibility Profile (no core profile option)
- Configurable OpenGL version via `mkfw_set_gl_version()` (default: 3.1)
- Minimal framebuffer configuration
- Explicit state passing (no implicit current context)

### Added/Different Features

- **Explicit state passing** - All functions take `struct mkfw_state *`
- **Multiple independent windows** - Full support with isolated state
- **Modifier state arrays** - Simpler modifier tracking
- **Aspect ratio enforcement** - Integrated with minimum size
- **Character input callbacks** - Via `mkfw_set_char_callback`
- **Thread context management helpers** - Simplified context transfer
- **Per-pixel transparency** - Composited windows with alpha channel
- **NULL return on init failure** - Instead of calling exit()

MKFW is ideal for single or multi-window games and tools that don't need GLFW's extensive feature set.

---

## License

MIT License - See source files for full license text.

---

## Additional Resources

For implementation details, refer to the source files:
- `mkfw.h` - Key codes, enums, state structure, inline functions
- `mkfw_linux.c` - X11/Linux implementation details
- `mkfw_win32.c` - Windows implementation details
