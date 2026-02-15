# MKFW API Documentation

**Lightweight GLFW Alternative**

## Overview

MKFW is a minimal, single-header windowing and input library for OpenGL applications. It provides a simpler alternative to GLFW with platform-specific implementations for X11/Linux and Windows. By default it creates an OpenGL 3.1 Compatibility Profile context, but the version is configurable.

## Features

- Multiple independent window support
- Window creation and management
- OpenGL context creation (Compatibility Profile, configurable version)
- Keyboard and mouse input handling
- Raw mouse motion support (XInput2 on Linux)
- Fullscreen toggling
- Window aspect ratio enforcement
- VSync control
- Event callbacks
- Character input callbacks

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

- `MKS_MOD_SHIFT` (0x03) - Either shift key
- `MKS_MOD_CTRL` (0x0C) - Either control key
- `MKS_MOD_ALT` (0x30) - Either alt key

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
- `button` - Button index (0-2)

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
- `button` - Button index (0-2)

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
- `codepoint` - ASCII character code (backspace=8, printable 32-126)

**Notes:**
- Used for text input (typing characters)
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

- Requires X11, GLX, and XInput2 development libraries
- Uses XInput2 for raw mouse motion
- Creates OpenGL Compatibility Profile context via `glXCreateContextAttribsARB`
- Handles `_NET_WM_STATE` for fullscreen

**Linking:**
```bash
gcc -o app main.c -lX11 -lGL -lXi -ldl -lpthread
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

- Monitor enumeration and video mode queries
- Window hints system
- Clipboard access
- File drop events
- Window positioning API
- Iconification/restoration
- Window focus management

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
