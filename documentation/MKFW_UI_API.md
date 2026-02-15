# MKFW UI API Documentation

**Immediate-Mode UI System for MKFW**

## Overview

The MKFW UI API provides a lightweight immediate-mode graphical user interface system for MKFW applications. It offers simple, stateless widgets for building debug interfaces, tools, and in-application menus without external dependencies.

## Features

- Immediate-mode (stateless) design
- Embedded 8x8 bitmap font (no external assets)
- Single draw call rendering
- Dark theme by default (ImGui-inspired colors)
- Mouse and keyboard input support
- Mousewheel scrolling for sliders and lists
- Text input with backspace support
- OpenGL 3.1+ shader-based rendering

## Platform Support

- Linux (X11)
- Windows (Win32)

---

## Enabling the UI Subsystem

The UI subsystem is optional and must be enabled before including `mkfw.h`:

```c
#define MKFW_UI
#include "mkfw.h"
```

This will include the UI implementation (`mkfw_ui.c`) and make all UI functions available.

When `MKFW_UI` is not defined, all UI functions become no-op macros, allowing code to compile without `#ifdef` guards.

---

## Initialization

### `mkui_init`

```c
void mkui_init(struct mkfw_state *mkfw)
```

Initialize the UI system and set up OpenGL resources.

**Parameters:**
- `mkfw` - MKFW window state pointer

**Notes:**
- Must be called after `mkfw_init()` and `mkfw_gl_loader()`
- Creates OpenGL shader program, vertex buffer, and font texture
- Registers input callbacks with MKFW
- Returns early if already initialized (no-op on subsequent calls)

**Example:**
```c
struct mkfw_state *mkfw = mkfw_init(1280, 720);
mkfw_show_window(mkfw);
mkfw_gl_loader();

mkui_init(mkfw);
```

---

### `mkui_shutdown`

```c
void mkui_shutdown(void)
```

Clean up UI resources.

**Notes:**
- Destroys OpenGL resources (shader, buffers, textures)
- Frees memory allocated by the UI system
- Should be called before `mkfw_cleanup()`

**Example:**
```c
mkui_shutdown();
mkfw_cleanup(mkfw);
```

---

## Frame Management

### `mkui_new_frame`

```c
void mkui_new_frame(int width, int height)
```

Begin a new UI frame.

**Parameters:**
- `width` - Framebuffer width in pixels
- `height` - Framebuffer height in pixels

**Notes:**
- Must be called once per frame, before any widgets
- Sets up orthographic projection matrix
- Resets layout cursor to origin
- Typically called after `glClear()` but before rendering widgets

**Example:**
```c
int display_w, display_h;
mkfw_get_framebuffer_size(mkfw, &display_w, &display_h);

glClear(GL_COLOR_BUFFER_BIT);
mkui_new_frame(display_w, display_h);

// ... draw widgets ...
```

---

### `mkui_render`

```c
void mkui_render(void)
```

Render all UI elements to the screen.

**Notes:**
- Must be called once per frame, after all widgets
- Submits vertex data to OpenGL
- Renders in a single draw call
- Resets scroll wheel delta after rendering
- Call before `mkfw_swap_buffers()`

**Example:**
```c
mkui_new_frame(display_w, display_h);

// ... draw widgets ...

mkui_render();
mkfw_swap_buffers(mkfw);
```

---

## Text Widgets

### `mkui_text`

```c
void mkui_text(const char *text)
```

Display static text.

**Parameters:**
- `text` - Null-terminated string

**Notes:**
- Rendered in default white color
- Uses embedded 8x8 bitmap font
- Automatically advances cursor vertically

**Example:**
```c
mkui_text("Welcome to MKFW UI!");
mkui_text("This is the second line.");
```

---

### `mkui_text_colored`

```c
void mkui_text_colored(const char *text, struct mkui_color color)
```

Display colored text.

**Parameters:**
- `text` - Null-terminated string
- `color` - Color struct (use `mkui_rgb()` or `mkui_rgba()` to create)

**Example:**
```c
mkui_text_colored("Success!", mkui_rgb(0.0f, 1.0f, 0.0f));  // Green
mkui_text_colored("Error!", mkui_rgb(1.0f, 0.0f, 0.0f));    // Red
mkui_text_colored("Warning", mkui_rgba(1.0f, 1.0f, 0.0f, 0.8f));  // Semi-transparent yellow
```

---

## Button Widget

### `mkui_button`

```c
int mkui_button(const char *label)
```

Display a clickable button.

**Parameters:**
- `label` - Button text

**Returns:**
- `1` if button was clicked this frame
- `0` otherwise

**Notes:**
- Visual states: normal, hovered (brighter), pressed (darker)
- Returns true on mouse click (press) while hovering
- Width automatically sized to text + padding

**Example:**
```c
if (mkui_button("Click Me!")) {
    printf("Button was clicked!\n");
}

if (mkui_button("Reset")) {
    reset_application_state();
}
```

---

## Checkbox Widget

### `mkui_checkbox`

```c
int mkui_checkbox(const char *label, int *checked)
```

Display a toggleable checkbox.

**Parameters:**
- `label` - Checkbox label text
- `checked` - Pointer to boolean state variable

**Returns:**
- `1` if value changed this frame
- `0` otherwise

**Notes:**
- Displays as a 16x16 box with label to the right
- Filled blue square when checked
- Click toggles the state
- Value is read from and written to `*checked`

**Example:**
```c
static int fullscreen = 0;
if (mkui_checkbox("Fullscreen", &fullscreen)) {
    printf("Fullscreen is now %s\n", fullscreen ? "ON" : "OFF");
}

static int vsync = 1;
mkui_checkbox("VSync", &vsync);
```

---

## Slider Widgets

### `mkui_slider_float`

```c
int mkui_slider_float(const char *label, float *value, float min_val, float max_val)
```

Display a horizontal slider for float values.

**Parameters:**
- `label` - Slider label text
- `value` - Pointer to float variable
- `min_val` - Minimum value
- `max_val` - Maximum value

**Returns:**
- `1` if value changed this frame
- `0` otherwise

**Notes:**
- Width: 200 pixels default
- Draggable grab handle
- Click anywhere on track to jump
- Supports mousewheel scrolling (5% of range per tick)
- Value is clamped to [min_val, max_val]

**Example:**
```c
static float volume = 0.5f;
if (mkui_slider_float("Volume", &volume, 0.0f, 1.0f)) {
    printf("Volume: %.2f\n", volume);
}

static float brightness = 0.8f;
mkui_slider_float("Brightness", &brightness, 0.0f, 1.0f);
```

---

### `mkui_slider_int`

```c
int mkui_slider_int(const char *label, int *value, int min_val, int max_val)
```

Display a horizontal slider for integer values.

**Parameters:**
- `label` - Slider label text
- `value` - Pointer to int variable
- `min_val` - Minimum value
- `max_val` - Maximum value

**Returns:**
- `1` if value changed this frame
- `0` otherwise

**Notes:**
- Same visual appearance as float slider
- Values are whole numbers only
- Supports mousewheel scrolling (~5% of range per tick, minimum 1)

**Example:**
```c
static int player_count = 4;
mkui_slider_int("Players", &player_count, 1, 8);

static int difficulty = 2;
if (mkui_slider_int("Difficulty", &difficulty, 1, 5)) {
    printf("Difficulty level: %d\n", difficulty);
}
```

---

### `mkui_slider_int64`

```c
int mkui_slider_int64(const char *label, int64_t *value, int64_t min_val, int64_t max_val)
```

Display a horizontal slider for large integer values.

**Parameters:**
- `label` - Slider label text
- `value` - Pointer to int64_t variable
- `min_val` - Minimum value
- `max_val` - Maximum value

**Returns:**
- `1` if value changed this frame
- `0` otherwise

**Notes:**
- For values larger than int32 range
- Mousewheel increment scales with range (~5% of range per tick, minimum 1)

**Example:**
```c
static int64_t file_offset = 0;
mkui_slider_int64("Offset", &file_offset, 0, 10000000);

static int64_t timestamp = 1000000;
mkui_slider_int64("Time", &timestamp, 0, 999999999);
```

---

## Radio Button Widget

### `mkui_radio_button`

```c
int mkui_radio_button(const char *label, int *selected, int value)
```

Display a radio button (mutually exclusive selection).

**Parameters:**
- `label` - Radio button label text
- `selected` - Pointer to shared selection variable
- `value` - Value this button represents

**Returns:**
- `1` if this option was selected this frame
- `0` otherwise

**Notes:**
- Multiple radio buttons share the same `selected` variable
- Only one option can be selected at a time
- Clicking sets `*selected = value`
- Displays as filled circle when `*selected == value`

**Example:**
```c
static int difficulty = 0;

mkui_text("Select difficulty:");
mkui_radio_button("Easy", &difficulty, 0);
mkui_radio_button("Normal", &difficulty, 1);
mkui_radio_button("Hard", &difficulty, 2);

// difficulty will be 0, 1, or 2
```

---

## Collapsible Header Widget

### `mkui_collapsing_header`

```c
int mkui_collapsing_header(const char *label, int *open)
```

Display a collapsible section header.

**Parameters:**
- `label` - Header text
- `open` - Pointer to boolean state (1 = expanded, 0 = collapsed)

**Returns:**
- Current value of `*open` (1 if expanded, 0 if collapsed)

**Notes:**
- Displays arrow indicator: `▼` when open, `▶` when closed
- Click to toggle open/closed state
- Use return value to conditionally render child widgets

**Example:**
```c
static int advanced_open = 0;

if (mkui_collapsing_header("Advanced Options", &advanced_open)) {
    mkui_text("These are advanced settings");
    mkui_checkbox("Debug Mode", &debug_enabled);
    mkui_slider_int("Max FPS", &max_fps, 30, 144);
}
```

---

## Text Input Widget

### `mkui_text_input`

```c
int mkui_text_input(const char *label, char *buffer, int32_t buffer_size)
```

Display an editable text field.

**Parameters:**
- `label` - Text field label
- `buffer` - Character buffer containing the text
- `buffer_size` - Maximum buffer size (including null terminator)

**Returns:**
- `1` if text was modified this frame
- `0` otherwise

**Notes:**
- Click to activate
- Type to add characters
- Backspace to delete
- Automatically deactivates when clicking elsewhere
- Cursor is shown as a static bar when active
- Text is null-terminated

**Example:**
```c
static char player_name[64] = "Player";

if (mkui_text_input("Name", player_name, sizeof(player_name))) {
    printf("Name changed to: %s\n", player_name);
}

static char server_ip[32] = "127.0.0.1";
mkui_text_input("Server IP", server_ip, sizeof(server_ip));
```

---

## Combo Box Widget

### `mkui_combo`

```c
int mkui_combo(const char *label, int *current_item, char **items, int items_count)
```

Display a dropdown selection menu.

**Parameters:**
- `label` - Combo box label
- `current_item` - Pointer to selected item index
- `items` - Array of item strings
- `items_count` - Number of items in array

**Returns:**
- `1` if selection changed this frame
- `0` otherwise

**Notes:**
- Displays currently selected item
- Click to expand dropdown list
- Click item to select
- Click outside to close without changing
- Shows arrow indicator when closed

**Example:**
```c
static int resolution = 0;
static char *resolutions[] = {
    "1280x720",
    "1920x1080",
    "2560x1440",
    "3840x2160"
};

if (mkui_combo("Resolution", &resolution, resolutions, 4)) {
    printf("Selected: %s\n", resolutions[resolution]);
}
```

---

## Listbox Widget

### `mkui_listbox`

```c
int mkui_listbox(const char *label, int *current_item, char **items,
                 int items_count, int visible_items)
```

Display a scrollable list of items.

**Parameters:**
- `label` - Listbox label
- `current_item` - Pointer to selected item index
- `items` - Array of item strings
- `items_count` - Total number of items
- `visible_items` - Number of items visible at once

**Returns:**
- `1` if selection changed this frame
- `0` otherwise

**Notes:**
- Scrollable with mousewheel when items exceed visible count
- Selected item is highlighted
- Click item to select
- Automatically scrolls to keep selection visible

**Example:**
```c
static int selected_file = 0;
static char *files[] = {
    "file1.txt", "file2.txt", "file3.txt", "file4.txt",
    "file5.txt", "file6.txt", "file7.txt", "file8.txt",
    "file9.txt", "file10.txt"
};

if (mkui_listbox("Files", &selected_file, files, 10, 5)) {
    printf("Selected: %s\n", files[selected_file]);
}
```

---

## Window Container

### `mkui_begin_window`

```c
void mkui_begin_window(const char *title, float *x, float *y, float width, float height)
```

Begin a window container.

**Parameters:**
- `title` - Window title text
- `x` - Pointer to window X position
- `y` - Pointer to window Y position
- `width` - Window width
- `height` - Window height

**Notes:**
- Must be paired with `mkui_end_window()`
- Renders background, border, and title bar
- All widgets between begin/end are positioned relative to window
- Window can be dragged by its title bar
- Use pointers for x/y so the dragging can update the position

**Example:**
```c
static float win_x = 50.0f;
static float win_y = 50.0f;

mkui_begin_window("Settings", &win_x, &win_y, 400, 300);

mkui_text("Window content goes here");
mkui_button("OK");

mkui_end_window();
```

---

### `mkui_end_window`

```c
void mkui_end_window(void)
```

End the current window container.

**Notes:**
- Must match a previous `mkui_begin_window()` call
- Restores cursor to global space

**Example:**
```c
mkui_begin_window("Info", &x, &y, 200, 150);
mkui_text("Information");
mkui_end_window();
```

---

## Layout Functions

### `mkui_separator`

```c
void mkui_separator(void)
```

Draw a horizontal separator line.

**Notes:**
- Uses border color from style
- Adds vertical spacing above and below
- Useful for grouping UI sections

**Example:**
```c
mkui_text("Video Settings");
mkui_separator();
mkui_checkbox("Fullscreen", &fullscreen);
mkui_separator();
mkui_text("Audio Settings");
```

---

### `mkui_same_line`

```c
void mkui_same_line(void)
```

Place next widget on the same line as previous widget.

**Notes:**
- Moves cursor horizontally instead of vertically
- Useful for button rows or label+value pairs

**Example:**
```c
mkui_text("Name:");
mkui_same_line();
mkui_button("Edit");

// Creates: "Name:  [Edit]" on one line
```

---

### `mkui_set_cursor_pos`

```c
void mkui_set_cursor_pos(float x, float y)
```

Set absolute cursor position.

**Parameters:**
- `x` - Horizontal position in pixels
- `y` - Vertical position in pixels

**Notes:**
- Positions are relative to current window if inside `begin_window/end_window`
- Useful for absolute positioning

**Example:**
```c
mkui_set_cursor_pos(100, 200);
mkui_text("At position (100, 200)");
```

---

## Image Rendering

### `mkui_image`

```c
void mkui_image(GLuint texture_id, float width, float height)
```

Display an OpenGL texture.

**Parameters:**
- `texture_id` - OpenGL texture ID
- `width` - Display width in pixels
- `height` - Display height in pixels

**Notes:**
- Currently a placeholder implementation (draws a gray rectangle)
- Full texture rendering is not yet implemented

**Example:**
```c
GLuint my_texture = /* ... create texture ... */;
mkui_image(my_texture, 64, 64);
```

---

### `mkui_image_rgba`

```c
void mkui_image_rgba(uint32_t *rgba_buffer, int width, int height)
```

Display raw RGBA pixel data.

**Parameters:**
- `rgba_buffer` - RGBA pixel data (one uint32_t per pixel)
- `width` - Image width in pixels
- `height` - Image height in pixels

**Notes:**
- Currently a placeholder implementation (draws a gray rectangle)
- Full texture creation from pixel data is not yet implemented

**Example:**
```c
uint32_t pixels[64 * 64];
// ... fill pixels with RGBA data ...
mkui_image_rgba(pixels, 64, 64);
```

---

## Style and Theming

### `mkui_rgb`

```c
struct mkui_color mkui_rgb(float r, float g, float b)
```

Create an opaque color.

**Parameters:**
- `r` - Red component (0.0 to 1.0)
- `g` - Green component (0.0 to 1.0)
- `b` - Blue component (0.0 to 1.0)

**Returns:**
- Color struct with alpha = 1.0

**Example:**
```c
struct mkui_color red = mkui_rgb(1.0f, 0.0f, 0.0f);
struct mkui_color green = mkui_rgb(0.0f, 1.0f, 0.0f);
mkui_text_colored("Success!", green);
```

---

### `mkui_rgba`

```c
struct mkui_color mkui_rgba(float r, float g, float b, float a)
```

Create a color with alpha transparency.

**Parameters:**
- `r` - Red component (0.0 to 1.0)
- `g` - Green component (0.0 to 1.0)
- `b` - Blue component (0.0 to 1.0)
- `a` - Alpha component (0.0 = transparent, 1.0 = opaque)

**Returns:**
- Color struct

**Example:**
```c
struct mkui_color semi_transparent = mkui_rgba(1.0f, 1.0f, 1.0f, 0.5f);
mkui_text_colored("Faded text", semi_transparent);
```

---

### `mkui_get_style`

```c
struct mkui_style *mkui_get_style(void)
```

Get pointer to global style structure.

**Returns:**
- Pointer to style struct containing all colors and spacing values

**Notes:**
- Modify style to customize appearance
- Changes affect all subsequent rendering
- Style persists until program exit or manual reset

**Example:**
```c
struct mkui_style *style = mkui_get_style();
style->text = mkui_rgb(1.0f, 1.0f, 0.0f);  // Yellow text
style->button = mkui_rgba(0.0f, 0.8f, 0.0f, 1.0f);  // Green buttons
```

---

### `mkui_style_set_color`

```c
void mkui_style_set_color(int color_id, float r, float g, float b, float a)
```

Set a specific style color.

**Parameters:**
- `color_id` - Integer color identifier
- `r` - Red component (0.0 to 1.0)
- `g` - Green component (0.0 to 1.0)
- `b` - Blue component (0.0 to 1.0)
- `a` - Alpha component (0.0 to 1.0)

**Color IDs:**
- `0` - Text color
- `1` - Window background
- `2` - Button color
- `3` - Button hovered color
- `4` - Button active color

**Example:**
```c
// Set button color to green
mkui_style_set_color(2, 0.0f, 0.8f, 0.0f, 1.0f);

// Make window background more transparent
mkui_style_set_color(1, 0.06f, 0.06f, 0.06f, 0.7f);
```

---

## Usage Pattern

### Typical Frame Loop

```c
while (!mkfw_should_close(mkfw)) {
    mkfw_pump_messages(mkfw);

    // Get window size
    int w, h;
    mkfw_get_framebuffer_size(mkfw, &w, &h);

    // Clear background
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Start UI frame
    mkui_new_frame(w, h);

    // Draw your UI
    static float x = 50, y = 50;
    mkui_begin_window("My Window", &x, &y, 300, 200);

    mkui_text("Hello, world!");

    static int enabled = 0;
    mkui_checkbox("Enable feature", &enabled);

    if (mkui_button("Click me!")) {
        printf("Button clicked!\n");
    }

    mkui_end_window();

    // Render UI
    mkui_render();

    // Swap buffers
    mkfw_swap_buffers(mkfw);
}
```

---

## Performance Characteristics

- **Single draw call** per frame (all widgets batched)
- **Maximum capacity**: 65,536 vertices (~2,700 buttons or ~10,000 characters)
- **Memory footprint**: ~256 KB (vertex buffer + font texture + context)
- **OpenGL state changes**: Minimal (shader, blend mode, one texture)

The system is designed to handle hundreds of widgets per frame without performance issues.

---

## Design Philosophy

The MKFW UI system follows the **immediate-mode** paradigm:

1. **No retained state** - Widgets don't "remember" anything between frames
2. **Code is the interface** - UI structure is defined by code flow
3. **Stateless rendering** - Same code produces same visuals
4. **Application owns data** - Widget values live in your variables

This makes the API simple, predictable, and easy to reason about.

---

## Common Patterns

### Dynamic UI Based on State

```c
static int show_advanced = 0;

mkui_checkbox("Show Advanced", &show_advanced);

if (show_advanced) {
    mkui_slider_float("Detail Level", &detail, 0.0f, 1.0f);
    mkui_slider_int("LOD Distance", &lod_dist, 100, 1000);
}
```

### Action Buttons in a Row

```c
if (mkui_button("Apply")) {
    apply_settings();
}
mkui_same_line();

if (mkui_button("Cancel")) {
    reset_settings();
}
mkui_same_line();

if (mkui_button("Reset to Default")) {
    load_default_settings();
}
```

### Grouped Settings

```c
mkui_text("Video Settings");
mkui_separator();
mkui_checkbox("Fullscreen", &fullscreen);
mkui_slider_int("Resolution Scale", &res_scale, 50, 200);

mkui_separator();

mkui_text("Audio Settings");
mkui_separator();
mkui_slider_float("Master Volume", &volume, 0.0f, 1.0f);
```

---

## When UI is Disabled

When `MKFW_UI` is not defined, all UI functions become no-op macros:

```c
#ifndef MKFW_UI
#define mkui_init(mkfw) ((void)0)
#define mkui_shutdown() ((void)0)
#define mkui_button(label) (0)
// ... etc
#endif
```

This allows you to write code that uses the UI without `#ifdef` guards:

```c
// This compiles fine whether MKFW_UI is defined or not
mkui_button("Debug Menu");
```

When UI is disabled:
- Zero runtime overhead
- Zero code size
- Widget functions return sensible defaults (0 for buttons, etc.)

---

## Thread Safety

The UI system is **not thread-safe**. All UI functions must be called from the same thread that:
- Created the MKFW window
- Calls `mkui_init()`
- Renders frames

This is typically the main thread.

---

## Limitations

- **No docking** - Windows cannot be docked together
- **No menus** - No menu bar or context menu support
- **Single font** - Only embedded 8x8 bitmap font
- **No rich text** - No markup, inline styles, or mixed formatting
- **Fixed layout** - No automatic grid or flex layouts

These limitations keep the implementation simple and lightweight. For complex UIs, consider using a full-featured library like Dear ImGui.

---

## Comparison to Alternatives

| Feature | MKFW UI | Dear ImGui | Nuklear |
|---------|---------|------------|---------|
| Lines of code | ~1,400 | ~31,000 | ~8,000 |
| Language | Pure C | C++ | Pure C |
| External deps | None | None | None |
| Font | Embedded | External or embedded | External or embedded |
| Compile time | Fast | Moderate | Moderate |
| Widget variety | Basic | Extensive | Moderate |
| Docking | No | Yes | No |

**MKFW UI is best for:**
- Debug interfaces
- Simple in-game tools
- Minimal dependencies
- Fast compile times
- Unity build projects

**Use alternatives for:**
- Production UI
- Complex tool GUIs
- Extensive customization needs

---

## Example Application

See [mkfw_ui_example.c](../ui_example/mkfw_ui_example.c) for a complete working example demonstrating all widgets.

To compile and run:

```bash
cd platform/mkfw

# Linux
gcc mkfw_ui_example.c -I../.. -lX11 -lXi -lGL -lm -o ui_example
./ui_example

# Windows (MinGW)
gcc mkfw_ui_example.c -I../.. -lopengl32 -lgdi32 -o ui_example.exe
ui_example.exe
```

---

## Troubleshooting

### UI doesn't appear

- Ensure `mkui_init()` is called after `mkfw_gl_loader()`
- Verify `mkui_new_frame()` is called before widgets
- Check that `mkui_render()` is called after widgets
- Make sure `MKFW_UI` is defined before including `mkfw.h`

### Text is blurry

- The font uses nearest-neighbor filtering and should be crisp
- Check that framebuffer size matches window size
- Ensure no scaling is applied in projection matrix

### Mouse clicks don't work

- Verify MKFW mouse callbacks are not overridden
- Check that `mkui_init()` received valid MKFW state
- Ensure `mkui_new_frame()` uses correct window dimensions

### Sliders jump around

- Make sure variable pointers passed to sliders remain valid
- Don't pass temporary variables or stack addresses that change

---

## License

MIT License - See source files for full license text.
