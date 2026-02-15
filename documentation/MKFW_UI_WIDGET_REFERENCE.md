# MKFW UI Widget Visual Reference

This document shows what each widget looks like and how it behaves.

## Color Theme (Default Dark)

The default theme uses ImGui-inspired colors:
- **Background**: Dark gray `rgb(0.06, 0.06, 0.06)`
- **Text**: White `rgb(1.0, 1.0, 1.0)`
- **Accent**: Blue `rgb(0.26, 0.59, 0.98)`
- **Borders**: Light gray `rgba(0.43, 0.43, 0.50, 0.50)`

## Widgets

### Text
```
┌─────────────────────────┐
│ This is text            │
└─────────────────────────┘
```
- Simple text rendering
- 8x8 bitmap font
- White by default

**Usage:**
```c
mkui_text("This is text");
```

---

### Text Colored
```
┌─────────────────────────┐
│ Colored text            │  (in custom color)
└─────────────────────────┘
```
- Same as text but with custom color
- Useful for warnings, errors, success messages

**Usage:**
```c
mkui_text_colored("Success!", mkui_rgb(0.0f, 1.0f, 0.0f));  // Green
mkui_text_colored("Error!", mkui_rgb(1.0f, 0.0f, 0.0f));    // Red
```

---

### Button
```
Normal state:
┌───────────────┐
│ Click Me!     │  (medium blue)
└───────────────┘

Hovered state:
┌───────────────┐
│ Click Me!     │  (bright blue)
└───────────────┘

Active (pressed) state:
┌───────────────┐
│ Click Me!     │  (dark blue)
└───────────────┘
```

- Rectangle with text
- Color changes on hover
- Darker when pressed
- Returns 1 when clicked

**Usage:**
```c
if(mkui_button("Click Me!")) {
    printf("Button was clicked!\n");
}
```

---

### Checkbox
```
Unchecked:
┌──┐ Enable feature
│  │
└──┘

Checked:
┌──┐ Enable feature
│██│  (blue checkmark)
└──┘

Hovered:
┌──┐ Enable feature  (lighter background)
│  │
└──┘
```

- 16x16 pixel box
- Text label to the right
- Click to toggle
- Filled square when checked
- Returns 1 when value changes

**Usage:**
```c
static int enabled = 0;
if(mkui_checkbox("Enable feature", &enabled)) {
    printf("Checkbox is now %s\n", enabled ? "checked" : "unchecked");
}
```

---

### Slider Float
```
┌─────────────────────────────────┐ Label
│░░░░░░██░░░░░░░░░░░░░░░░░░░░░░░│
└─────────────────────────────────┘
        ↑
    grab handle (8px wide)

Dragging:
┌─────────────────────────────────┐ Label
│░░░░░░░░░░░░██░░░░░░░░░░░░░░░░░│ (brighter grab)
└─────────────────────────────────┘
```

- Horizontal bar (200px default width)
- Draggable handle
- Handle position represents value
- Click anywhere to jump
- Drag to adjust smoothly
- Returns 1 when value changes

**Usage:**
```c
static float volume = 0.5f;
if(mkui_slider_float("Volume", &volume, 0.0f, 1.0f)) {
    printf("Volume changed to %.2f\n", volume);
}
```

---

### Separator
```
─────────────────────────────────
```

- Thin horizontal line
- Uses border color
- Adds vertical spacing

**Usage:**
```c
mkui_separator();
```

---

### Window
```
┌──────────────────────────────────┐
│ Window Title                     │ (title bar - darker)
├──────────────────────────────────┤
│                                  │
│  Content area                    │ (semi-transparent dark bg)
│  (widgets go here)               │
│                                  │
│                                  │
└──────────────────────────────────┘
```

- Rectangular container
- Title bar at top (24px high)
- Border around entire window
- Semi-transparent background
- Position and size specified manually

**Usage:**
```c
static float win_x = 50.0f, win_y = 50.0f;
mkui_begin_window("My Window", &win_x, &win_y, 400, 300);

mkui_text("Content goes here");
mkui_button("OK");

mkui_end_window();
```

---

## Layout

### Vertical Stacking (Default)
```
mkui_text("Line 1");
mkui_text("Line 2");
mkui_text("Line 3");

Result:
┌──────────┐
│ Line 1   │
│ Line 2   │
│ Line 3   │
└──────────┘
```

Widgets automatically stack vertically with spacing.

---

### Same Line
```
mkui_text("Name:");
mkui_same_line();
mkui_button("Edit");

Result:
┌────────────────────┐
│ Name:       [Edit] │
└────────────────────┘
```

Use `mkui_same_line()` to place next widget on the same line.

---

### Manual Positioning
```c
mkui_set_cursor_pos(100, 200);
mkui_text("At position (100, 200)");
```

Manually set cursor position for absolute placement.

---

## State Management

### Immediate Mode
```c
// Widget state is managed by YOUR variables
static int checkbox_state = 0;
static float slider_value = 0.5f;

// Every frame, you pass your state to the widgets
mkui_checkbox("Option", &checkbox_state);
mkui_slider_float("Value", &slider_value, 0.0f, 1.0f);
```

**Key concept:** The UI doesn't "remember" anything. You pass the same variables every frame, and the UI reads/modifies them.

---

## Interaction States

### Hot Item
When the mouse hovers over a widget, it becomes "hot":
- Lighter color
- Visual feedback
- Indicates interactivity

### Active Item
When you click and hold on a widget, it becomes "active":
- Even darker/different color
- Stays active while mouse is down
- For sliders: can drag while active

---

## Example: Complete Window

```c
static float settings_x = 100.0f, settings_y = 100.0f;
mkui_begin_window("Settings", &settings_x, &settings_y, 350, 400);

mkui_text("Video Settings");
mkui_separator();

static int fullscreen = 0;
mkui_checkbox("Fullscreen", &fullscreen);

static int vsync = 1;
mkui_checkbox("VSync", &vsync);

static float brightness = 0.8f;
mkui_slider_float("Brightness", &brightness, 0.0f, 1.0f);

mkui_separator();

mkui_text("Audio Settings");
mkui_separator();

static float master_volume = 0.7f;
mkui_slider_float("Master", &master_volume, 0.0f, 1.0f);

static float music_volume = 0.5f;
mkui_slider_float("Music", &music_volume, 0.0f, 1.0f);

mkui_separator();

if(mkui_button("Apply")) {
    printf("Settings applied!\n");
}

mkui_same_line();

if(mkui_button("Cancel")) {
    printf("Cancelled\n");
}

mkui_end_window();
```

This creates a complete settings window with sections, sliders, checkboxes, and buttons.

---

## Visual Feedback Summary

| State | Button | Checkbox | Slider |
|-------|--------|----------|--------|
| Normal | Medium blue | Dark frame | Dark track, blue grab |
| Hovered | Bright blue | Light frame | Same |
| Active/Pressed | Dark blue | N/A | Bright grab |
| Checked | N/A | Blue fill | N/A |

---

## Font

The embedded font is a classic 8x8 bitmap font with full ASCII support (characters 32-127).

Character size: **8 pixels wide × 8 pixels tall**

Sample:
```
 !"#$%&'()*+,-./0123456789:;<=>?
@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
`abcdefghijklmnopqrstuvwxyz{|}~
```

All rendering uses nearest-neighbor filtering for crisp, pixel-perfect text.

---

## Tips

1. **Layout spacing** is controlled by `mkui_ctx->item_spacing` (default: 4px)
2. **Padding** inside widgets is `mkui_ctx->frame_padding` (default: 4px)
3. **Window padding** is `mkui_ctx->window_padding` (default: 8px)
4. **Colors** can be customized via `mkui_get_style()`
5. **Z-order**: Widgets are drawn in the order they're called (later = on top)

---

## Performance

Each widget adds approximately:
- **Text**: 6 vertices per character
- **Button**: 12 vertices (2 triangles for background + text)
- **Checkbox**: 24 vertices (box + optional checkmark)
- **Slider**: 24 vertices (track + grab)
- **Window**: 24 vertices (background + border + title bar)

Maximum: **65,536 vertices per frame** = roughly 2,700 buttons or 10,000 text characters.

This is more than enough for any typical UI!
