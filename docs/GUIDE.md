# Monstro UI User Guide

Monstro is an immediate-mode graphical user interface library with a C API and
C++ internals.  It is **graphics-API agnostic**: Monstro never touches the GPU.
Instead it produces vertex buffers, index buffers, and draw commands that your
application feeds to whatever renderer it already has -- Vulkan, OpenGL, D3D12,
Metal, or anything else.

If you have used Dear ImGui, Monstro will feel familiar.  The key differences
are:

- Every API call takes an explicit `MonstroContext*` -- there is no hidden
  global state.
- GPU resources (textures, buffers) are owned by *your* application.  Monstro
  only references them through opaque `uint64_t` texture IDs.
- Style items can be solid colors, full textures, sprite atlas sub-regions, or
  nine-patch textures, making it straightforward to skin an entire application
  with artist-authored artwork.

---

## Architecture Overview

```
 Your Application          Monstro                   Your Renderer
 ┌──────────────┐     ┌────────────────┐        ┌──────────────────┐
 │ input events ├────>│ MonstroContext  ├───────>│ GPU draw calls   │
 │ (mouse, kbd) │     │ begin/end frame │        │ (vertices, idx,  │
 │              │     │ widget calls    │        │  textures, clip) │
 └──────────────┘     └────────────────┘        └──────────────────┘
```

**Key objects:**

- `MonstroContext` -- holds all UI state.  Create one per logical UI.
- `MonstroDrawBuffer` -- stores the vertex and index data produced each frame.
- `MonstroDrawCommand` -- a single batched draw call: clip rect, texture, index
  offset/count.
- `MonstroFont` -- a loaded font with its glyph atlas.

**Frame lifecycle:**

1. Call `monstro_begin_frame(ctx, delta_time)`.
2. Emit windows and widgets (`monstro_begin_window`, `monstro_button`, etc.).
3. Call `monstro_end_frame(ctx)`.
4. Read the draw buffer and draw commands, upload to GPU, issue draw calls.

---

## Getting Started -- Minimal Integration

Below is the smallest useful Monstro program.  Graphics-API-specific code is
replaced with comments -- the next sections fill those in.

```c
#include <monstro.h>

int main(void) {
    /* --- 1. Create context --- */
    MonstroContext* ctx = monstro_context_create();

    /* --- 2. Load a font --- */
    MonstroFontManager* fm = monstro_context_get_font_manager(ctx);
    MonstroTextureData atlas_data = {0};
    MonstroFont* font = monstro_font_load_ttf_from_file(
        fm, "path/to/font.ttf",
        16.0f,    /* pixel size */
        32,       /* first code point (space) */
        96,       /* code point count (ASCII printable) */
        &atlas_data
    );
    /* Upload atlas_data.data to a GPU texture, get a handle. */
    uint64_t font_texture_id = /* your_gpu_create_texture(atlas_data) */;
    monstro_font_set_atlas_texture(font, font_texture_id);
    monstro_font_add(fm, font, "default", 16.0f);
    monstro_context_set_default_font(ctx, font);

    /* --- 3. Set display size --- */
    monstro_set_display_size(ctx, 1280.0f, 720.0f);

    /* --- 4. Main loop --- */
    while (running) {
        float dt = /* seconds since last frame */;

        /* Feed input (see Windowing Library Integration) */
        monstro_set_mouse_pos(ctx, mouse_x, mouse_y);
        monstro_set_mouse_button(ctx, MONSTRO_MOUSE_BUTTON_LEFT, left_down);
        /* ... keyboard, scroll, text ... */

        /* --- 5. Build UI --- */
        monstro_begin_frame(ctx, dt);

        monstro_set_next_window_pos(ctx, 50, 50, MONSTRO_COND_FIRST_USE_EVER);
        monstro_set_next_window_size(ctx, 300, 200, MONSTRO_COND_FIRST_USE_EVER);
        if (monstro_begin_window(ctx, "Hello", NULL, MONSTRO_WINDOW_DEFAULT)) {
            monstro_label(ctx, "Hello, Monstro!");
            if (monstro_button(ctx, "Click Me", NULL)) {
                printf("clicked!\n");
            }
            monstro_end_window(ctx);
        }

        monstro_end_frame(ctx);

        /* --- 6. Render --- */
        MonstroDrawBuffer* buf = monstro_context_get_draw_buffer(ctx);
        const MonstroVertex* verts; size_t vert_bytes;
        const uint32_t* indices;    uint32_t idx_count;
        monstro_draw_buffer_get_vertex_buffer(buf, &verts, &vert_bytes);
        monstro_draw_buffer_get_index_buffer(buf, &indices, &idx_count);
        /* Upload verts/indices to GPU buffers. */

        const MonstroDrawCommand* cmds; uint32_t cmd_count;
        monstro_context_get_draw_commands(ctx, &cmds, &cmd_count);
        for (uint32_t i = 0; i < cmd_count; i++) {
            /* Set scissor from cmds[i].clip_rect */
            /* Bind texture from cmds[i].texture_id */
            /* Draw cmds[i].index_count indices starting at cmds[i].index_offset */
        }
    }

    /* --- 7. Cleanup --- */
    monstro_context_destroy(ctx);
    return 0;
}
```

---

## Windowing Library Integration

Monstro does not create windows or read input directly.  Your application
receives events from the OS (via GLFW, SDL, Win32, etc.) and forwards them to
Monstro through setter functions.

### What Monstro needs each frame

| Event | Monstro call |
|-------|-------------|
| Mouse position | `monstro_set_mouse_pos(ctx, x, y)` |
| Mouse button press/release | `monstro_set_mouse_button(ctx, button, down)` |
| Scroll wheel | `monstro_set_mouse_wheel(ctx, dx, dy)` |
| Key press/release | `monstro_set_key(ctx, key, down)` |
| Modifier keys | `monstro_set_key_mods(ctx, mods)` |
| Text input (characters) | `monstro_add_input_text(ctx, utf8_string)` |
| Window resize | `monstro_set_display_size(ctx, w, h)` |

### GLFW example

The `examples/basic/main.cpp` shows the full integration.  The critical
callbacks are:

```c
static void cursorPosCallback(GLFWwindow* w, double x, double y) {
    monstro_set_mouse_pos(ctx, (float)x, (float)y);
}

static void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods) {
    MonstroMouseButton mb = MONSTRO_MOUSE_BUTTON_LEFT;
    if (button == GLFW_MOUSE_BUTTON_RIGHT)  mb = MONSTRO_MOUSE_BUTTON_RIGHT;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) mb = MONSTRO_MOUSE_BUTTON_MIDDLE;
    monstro_set_mouse_button(ctx, mb, action == GLFW_PRESS);
}

static void scrollCallback(GLFWwindow* w, double dx, double dy) {
    monstro_set_mouse_wheel(ctx, (float)dx, (float)dy);
}

static void keyCallback(GLFWwindow* w, int key, int sc, int action, int mods) {
    MonstroKey mk = glfw_key_to_monstro(key); /* your mapping function */
    if (mk != MONSTRO_KEY_COUNT)
        monstro_set_key(ctx, mk, action != GLFW_RELEASE);

    MonstroModFlags mf = MONSTRO_MOD_NONE;
    if (mods & GLFW_MOD_CONTROL) mf |= MONSTRO_MOD_CTRL;
    if (mods & GLFW_MOD_SHIFT)   mf |= MONSTRO_MOD_SHIFT;
    if (mods & GLFW_MOD_ALT)     mf |= MONSTRO_MOD_ALT;
    if (mods & GLFW_MOD_SUPER)   mf |= MONSTRO_MOD_SUPER;
    monstro_set_key_mods(ctx, mf);
}

static void charCallback(GLFWwindow* w, unsigned int codepoint) {
    char utf8[5] = {0};
    /* encode codepoint to UTF-8 bytes */
    monstro_add_input_text(ctx, utf8);
}

static void framebufferSizeCallback(GLFWwindow* w, int width, int height) {
    monstro_set_display_size(ctx, (float)width, (float)height);
}
```

### SDL2 / SDL3

The pattern is identical.  In your `SDL_PollEvent` loop, translate
`SDL_MOUSEMOTION`, `SDL_MOUSEBUTTONDOWN`, `SDL_KEYDOWN`, `SDL_TEXTINPUT`, and
`SDL_WINDOWEVENT_RESIZED` to the same Monstro setter calls.  The only
difference is the key code mapping table.

### Input routing

Use these to decide whether your game should consume input or let Monstro have
it:

```c
if (monstro_want_capture_mouse(ctx)) {
    /* Monstro is interacting with a widget -- don't send to game camera */
}
if (monstro_want_capture_keyboard(ctx)) {
    /* Monstro has a text field active -- don't send to game input */
}
```

---

## Graphics Backend Integration

### What Monstro produces

After `monstro_end_frame`, three things are ready:

**Vertex buffer** -- an array of `MonstroVertex`:

```c
typedef struct MonstroVertex {
    float position[2];   /* screen-space pixel coordinates */
    float texcoord[2];   /* UV for font atlas or user textures */
    uint32_t color;      /* packed RGBA: R in the least significant byte, A in the most significant */
} MonstroVertex;
```

**Index buffer** -- an array of `uint32_t` indices into the vertex buffer.

**Draw commands** -- an array of `MonstroDrawCommand`:

```c
typedef struct MonstroDrawCommand {
    float clip_rect[4];    /* x, y, width, height in pixels */
    int layer;
    uint32_t order;
    uint64_t texture_id;   /* 0 = no texture (use white pixel) */
    uint32_t index_offset;
    uint32_t index_count;
    MonstroCustomDrawCallback custom_draw;  /* NULL for normal draws */
    void* user_data;
} MonstroDrawCommand;
```

### Generic rendering loop

```c
const MonstroDrawCommand* cmds;
uint32_t count;
monstro_context_get_draw_commands(ctx, &cmds, &count);

for (uint32_t i = 0; i < count; i++) {
    const MonstroDrawCommand* dc = &cmds[i];

    if (dc->custom_draw) {
        dc->custom_draw(dc->user_data, dc->clip_rect, dc->layer);
        continue;
    }

    set_scissor_rect(dc->clip_rect[0], dc->clip_rect[1],
                     dc->clip_rect[2], dc->clip_rect[3]);

    bind_texture(resolve_texture_id(dc->texture_id));

    draw_indexed(dc->index_count, dc->index_offset);
}
```

### Pipeline requirements

- **Blending**: standard alpha blending
  (`srcAlpha, oneMinusSrcAlpha` for color; `one, oneMinusSrcAlpha` for alpha).
- **Depth test**: disabled.
- **Culling**: disabled (none).
- **Vertex input**: the `MonstroVertex` layout above, or bindless via buffer
  device address.

### Vulkan worked example

The `examples/basic/main.cpp` demonstrates a full Vulkan integration using the
VulkEase helper library.  Key excerpts:

**Buffer upload** -- each frame, read the draw buffer and upload to GPU:

```c
MonstroDrawBuffer* buf = monstro_context_get_draw_buffer(ctx);
const MonstroVertex* verts; size_t vert_bytes;
const uint32_t* indices;    uint32_t idx_count;
monstro_draw_buffer_get_vertex_buffer(buf, &verts, &vert_bytes);
monstro_draw_buffer_get_index_buffer(buf, &indices, &idx_count);
/* vkCmdUpdateBuffer or memcpy to mapped memory */
```

**Scissor** -- set from `clip_rect`:

```c
VkRect2D scissor = {
    .offset = { (int32_t)dc->clip_rect[0], (int32_t)dc->clip_rect[1] },
    .extent = { (uint32_t)dc->clip_rect[2], (uint32_t)dc->clip_rect[3] }
};
vkCmdSetScissor(cmd, 0, 1, &scissor);
```

**Texture resolution** -- map `texture_id` to your `VkImageView` / descriptor
index (see the Texture IDs section under Styling and Theming).

### OpenGL notes

```c
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDisable(GL_DEPTH_TEST);
glDisable(GL_CULL_FACE);

/* For each draw command: */
glScissor(clip_x, fb_height - clip_y - clip_h, clip_w, clip_h);
glBindTexture(GL_TEXTURE_2D, resolve_texture_id(dc->texture_id));
glDrawElements(GL_TRIANGLES, dc->index_count, GL_UNSIGNED_INT,
               (void*)(dc->index_offset * sizeof(uint32_t)));
```

### User-provided buffers

By default Monstro allocates its own vertex and index memory.  If you want to
write directly into GPU-mapped memory (avoiding a copy), provide your own
buffers before the frame:

```c
monstro_draw_buffer_set_vertex_buffer(buf, mapped_verts, max_verts);
monstro_draw_buffer_set_index_buffer(buf, mapped_indices, max_indices);
```

Monstro will write into these buffers during widget calls.  After
`monstro_end_frame`, the data is ready to draw without an upload step.

---

## Font System

### Loading TTF fonts

```c
MonstroFontManager* fm = monstro_context_get_font_manager(ctx);
MonstroTextureData atlas = {0};

MonstroFont* font = monstro_font_load_ttf_from_file(
    fm,
    "fonts/MyFont.ttf",
    18.0f,  /* pixel size */
    32,     /* first codepoint (space) */
    96,     /* count (ASCII printable range) */
    &atlas
);
```

The `atlas` output contains:
- `atlas.data` -- pixel data
- `atlas.width`, `atlas.height` -- dimensions
- `atlas.data_size` -- byte count
- `atlas.format` -- `MONSTRO_TEXTURE_FORMAT_R8` (single channel) or
  `MONSTRO_TEXTURE_FORMAT_RGBA8`

Upload this to a GPU texture, then tell Monstro the texture ID:

```c
uint64_t tex_id = upload_to_gpu(atlas.data, atlas.width, atlas.height, atlas.format);
monstro_font_set_atlas_texture(font, tex_id);
```

Register with the font manager and set as default:

```c
monstro_font_add(fm, font, "default", 18.0f);
monstro_context_set_default_font(ctx, font);
```

### Loading bitmap fonts

```c
MonstroFont* bmp_font = monstro_font_load_bitmap_from_file(
    fm,
    "fonts/bitmap_8x8.png",
    16,    /* grid columns */
    16,    /* grid rows */
    0,     /* first codepoint */
    8.0f,  /* font size in pixels */
    &atlas
);
```

### Multiple fonts

Load additional fonts at different sizes or families.  Use `monstro_font_find`
to retrieve them later:

```c
MonstroFont* large = monstro_font_load_ttf_from_file(fm, "font.ttf", 32.0f, 32, 96, &atlas);
/* upload atlas, set texture id ... */
monstro_font_add(fm, large, "default", 32.0f);

/* Later: */
MonstroFont* f = monstro_font_find(fm, "default", 32.0f);
```

### Text measurement

```c
MonstroVec2 size = monstro_font_measure_text(font, "Hello World", 0.0f);
MonstroVec2 wrapped = monstro_font_measure_text_wrapped(font, long_text, 200.0f);
```

---

## Core Concepts

### Immediate mode

Monstro is *immediate-mode*: the UI is declared from scratch every frame.
There are no persistent widget objects.  The application owns all state (a
`bool` for a checkbox, a `float` for a slider) and passes pointers to Monstro
each frame.

```c
static bool show_debug = false;
static float volume = 0.5f;

/* Every frame: */
monstro_checkbox(ctx, "Show Debug", &show_debug, NULL);
monstro_slider_float(ctx, "Volume", &volume, 0.0f, 1.0f, "%.2f", NULL);
```

### ID system

Every widget needs a unique ID within its window.  The ID is computed by
hashing the entire label string.  The **displayed text** is everything
*after* the `##` separator.  Everything before `##` is hidden and exists
only to make the ID unique.  This is the opposite of Dear ImGui's convention.

```c
monstro_button(ctx, "position##Reset", NULL);  /* displays "Reset" */
monstro_button(ctx, "rotation##Reset", NULL);  /* displays "Reset", different ID */
```

If the label contains no `##`, the entire string is used as both the ID and
the display text.

For widgets generated in a loop, push and pop ID scopes:

```c
for (int i = 0; i < count; i++) {
    monstro_push_id(ctx, item_names[i]);
    monstro_button(ctx, "Delete", NULL);
    monstro_pop_id(ctx);
}
```

### Window flags

`MONSTRO_WINDOW_DEFAULT` enables background, title bar, borders, movable,
collapsable, resizable, input handling, and scrolling.  You can compose flags
manually:

```c
MonstroWindowFlags flags = MONSTRO_WINDOW_BACKGROUND
                         | MONSTRO_WINDOW_TITLE_BAR
                         | MONSTRO_WINDOW_BORDERS
                         | MONSTRO_WINDOW_MOVABLE;
```

Notable flags:
- `MONSTRO_WINDOW_MENU_BAR` -- reserves space for a menu bar below the title
  bar and draws its background.  Add this flag when your window uses
  `monstro_begin_menu_bar` / `monstro_end_menu_bar`.  It is **not** included
  in `MONSTRO_WINDOW_DEFAULT` -- you must opt in explicitly.
- `MONSTRO_WINDOW_AUTO_RESIZE` -- window sizes to fit its content.
- `MONSTRO_WINDOW_SCROLL_WITH_MOUSE` -- mouse wheel scrolls the window.
- `MONSTRO_WINDOW_COLLAPSABLE` -- adds a collapse toggle.
- `MONSTRO_WINDOW_RESIZABLE` -- adds a resize grip.

### Conditions

Functions like `monstro_set_next_window_pos` take a `MonstroCondition`:

- `MONSTRO_COND_ALWAYS` -- apply every frame.
- `MONSTRO_COND_ONCE` -- apply once, then stop.
- `MONSTRO_COND_FIRST_USE_EVER` -- apply only the first time the window is
  created (persists across frames).
- `MONSTRO_COND_APPEARING` -- apply each time the window appears.

### Begin/End pairs

When `monstro_begin_window` (or `begin_child`, `begin_popup`, etc.) returns
`true`, the window is active and you must call the matching `monstro_end_*`
when you are done emitting content.  When it returns `false`, the window was
not processed (hidden, off-screen, or closed), and you should **not** call
the matching `end` function:

```c
if (monstro_begin_window(ctx, "My Window", NULL, MONSTRO_WINDOW_DEFAULT)) {
    monstro_label(ctx, "visible content");
    monstro_end_window(ctx);
}
```

---

## Widget Reference

All widget functions take `MonstroContext*` as their first argument.  Functions
that modify a value return `true` when the value changed.  The optional
`tooltip` parameter (on widgets that accept one) shows a tooltip on hover; pass
`NULL` to disable.

### Labels

```c
void monstro_label(ctx, "Static text");
void monstro_label_sized(ctx, "Text", 200.0f, 30.0f);
void monstro_label_wrapped(ctx, "Long text that wraps...", 0.0f);  /* 0 = use available width */
void monstro_label_colored(ctx, "Red!", (MonstroColor){1,0,0,1});
```

### Buttons

```c
if (monstro_button(ctx, "Click", "tooltip text")) { /* pressed */ }
if (monstro_button_sized(ctx, "Wide", 200.0f, 40.0f, NULL)) { }
if (monstro_image_button(ctx, "icon_btn", texture_id, 32.0f, 32.0f, NULL)) { }
```

### Checkbox and Radio

```c
static bool enabled = false;
monstro_checkbox(ctx, "Enable", &enabled, NULL);

static int choice = 0;
monstro_radio_button(ctx, "Option A", &choice, 0, NULL);
monstro_radio_button(ctx, "Option B", &choice, 1, NULL);
monstro_radio_button(ctx, "Option C", &choice, 2, NULL);
```

### Sliders

```c
static float val = 0.5f;
monstro_slider_float(ctx, "Volume", &val, 0.0f, 1.0f, "%.2f", NULL);

static int ival = 5;
monstro_slider_int(ctx, "Count", &ival, 0, 10, "%d", NULL);

/* Extended: custom width, height, flags */
monstro_slider_float_ex(ctx, "Custom", &val, 0, 1, "%.2f",
                        200.0f, 30.0f, MONSTRO_SLIDER_NONE, NULL);
```

### Knobs

```c
static float gain = 0.5f;
monstro_knob(ctx, "Gain", &gain, 0.0f, 1.0f, "%.2f", NULL);
monstro_knob_sized(ctx, "Rate", &gain, 0.0f, 100.0f, "%.1f", 60.0f, NULL);
```

### Progress bar

```c
monstro_progress_bar(ctx, 0.75f, 120.0f, 20.0f, NULL);          /* shows "75%" */
monstro_progress_bar(ctx, fraction, 120.0f, 20.0f, "%.1f%%");   /* custom format */
```

### Text input

```c
static char name[256] = "";
monstro_input_text(ctx, "Name", name, sizeof(name), MONSTRO_INPUT_TEXT_NONE, NULL);

static char notes[1024] = "";
monstro_input_text_multiline(ctx, "Notes", notes, sizeof(notes),
                              200.0f, 80.0f, MONSTRO_INPUT_TEXT_NONE, NULL);
```

Flags: `MONSTRO_INPUT_TEXT_PASSWORD`, `MONSTRO_INPUT_TEXT_READ_ONLY`,
`MONSTRO_INPUT_TEXT_CHARS_DECIMAL`, `MONSTRO_INPUT_TEXT_CHARS_HEXADECIMAL`.

### Numeric input

```c
static int count = 0;
monstro_input_int(ctx, "Count", &count, 1, 10, NULL);

static float amount = 0.0f;
monstro_input_float(ctx, "Amount", &amount, 0.1f, 1.0f, "%.2f", NULL);

static double precise = 0.0;
monstro_input_double(ctx, "Precise", &precise, 0.001, 0.1, "%.6f", NULL);
```

### Selectable

```c
static bool selected = false;
monstro_selectable(ctx, "Item", &selected, MONSTRO_SELECTABLE_NONE, NULL);
```

### Combo box

```c
static int current = 0;
const char* items[] = {"Apple", "Banana", "Cherry"};
monstro_combo(ctx, "Fruit", &current, items, 3, NULL);

/* Or manual combo for custom content: */
if (monstro_begin_combo(ctx, "Custom", items[current], NULL)) {
    for (int i = 0; i < 3; i++) {
        bool is_sel = (current == i);
        if (monstro_selectable(ctx, items[i], &is_sel, MONSTRO_SELECTABLE_NONE, NULL))
            current = i;
    }
    monstro_end_combo(ctx);
}
```

### Image and Icon

```c
monstro_image(ctx, my_texture_id, 128.0f, 128.0f);
monstro_icon(ctx, icon_texture_id, 24.0f);
```

### Separator

```c
monstro_separator(ctx);
monstro_separator_text(ctx, "Section Title");
```

### Tooltip

Any widget that accepts a `tooltip` parameter will show a tooltip
automatically.  For manual control:

```c
monstro_tooltip(ctx, "Hover text");
```

### Groups

```c
monstro_begin_group(ctx, MONSTRO_LAYOUT_VERTICAL, MONSTRO_GROUP_BORDER);
    monstro_label(ctx, "Inside a bordered group");
    monstro_checkbox(ctx, "Option", &val, NULL);
monstro_end_group(ctx);
```

Flags: `MONSTRO_GROUP_BORDER`, `MONSTRO_GROUP_BACKGROUND`.

### Tabs

```c
if (monstro_begin_tab_bar(ctx, "tabs", MONSTRO_TAB_BAR_NONE)) {
    if (monstro_begin_tab_item(ctx, "General", NULL)) {
        monstro_label(ctx, "General content");
        monstro_end_tab_item(ctx);
    }
    if (monstro_begin_tab_item(ctx, "Advanced", NULL)) {
        monstro_label(ctx, "Advanced content");
        monstro_end_tab_item(ctx);
    }
    monstro_end_tab_bar(ctx);
}
```

### Trees

```c
if (monstro_begin_tree_node(ctx, "Scene", MONSTRO_TREE_DEFAULT_OPEN)) {
    if (monstro_tree_leaf(ctx, "Camera", MONSTRO_TREE_NONE)) {
        /* Camera was clicked */
    }
    if (monstro_begin_tree_node(ctx, "Meshes", MONSTRO_TREE_NONE)) {
        monstro_tree_leaf(ctx, "Cube", MONSTRO_TREE_NONE);
        monstro_end_tree_node(ctx);
    }
    monstro_end_tree_node(ctx);
}
```

`monstro_begin_tree_node` returns `true` when the node is open.  Call
`monstro_end_tree_node` when it returns `true`.  Any widgets can be placed
between begin and end -- sliders, buttons, nested trees, etc.

`monstro_tree_leaf` draws a terminal item and returns `true` when clicked.
No end call is needed.

`monstro_collapsing_header` is a full-width header that collapses content:

```c
if (monstro_collapsing_header(ctx, "Settings", NULL)) {
    /* content shown when open */
}
```

### Tables

```c
if (monstro_begin_table(ctx, "tbl", 3,
        MONSTRO_TABLE_BORDERS | MONSTRO_TABLE_ROW_BACKGROUND, 0, 0)) {
    monstro_table_setup_column(ctx, "Name",   MONSTRO_TABLE_COLUMN_NONE, 0);
    monstro_table_setup_column(ctx, "Value",  MONSTRO_TABLE_COLUMN_NONE, 0);
    monstro_table_setup_column(ctx, "Action", MONSTRO_TABLE_COLUMN_WIDTH_FIXED, 80);
    monstro_table_headers_row(ctx);

    monstro_table_next_row(ctx);
    monstro_table_next_column(ctx); monstro_label(ctx, "Alpha");
    monstro_table_next_column(ctx); monstro_label(ctx, "1.0");
    monstro_table_next_column(ctx); monstro_button(ctx, "a##Reset", NULL);

    monstro_end_table(ctx);
}
```

### Menus

To use a menu bar, add `MONSTRO_WINDOW_MENU_BAR` to the window flags:

```c
MonstroWindowFlags flags = (MonstroWindowFlags)(MONSTRO_WINDOW_DEFAULT | MONSTRO_WINDOW_MENU_BAR);
if (monstro_begin_window(ctx, "My Window", NULL, flags)) {
    if (monstro_begin_menu_bar(ctx)) {
        if (monstro_begin_menu(ctx, "File", true)) {
            if (monstro_menu_item(ctx, "Open", NULL, true)) { /* ... */ }
            if (monstro_menu_item(ctx, "Save", NULL, true)) { /* ... */ }
            monstro_separator(ctx);
            monstro_menu_item(ctx, "Exit", NULL, true);
            monstro_end_menu(ctx);
        }
        monstro_end_menu_bar(ctx);
    }
    /* ... window content ... */
    monstro_end_window(ctx);
}
```

The `MONSTRO_WINDOW_MENU_BAR` flag reserves space for the menu bar and draws
its background.  The `begin_menu_bar` / `end_menu_bar` calls populate it with
menu items.  If you set the flag but never call `begin_menu_bar`, the bar area
is drawn but left empty.

### Popups and Modals

```c
/* Regular popup */
if (monstro_button(ctx, "Show Popup", NULL))
    monstro_open_popup(ctx, "my_popup");

if (monstro_begin_popup(ctx, "my_popup", 0)) {
    monstro_label(ctx, "Popup content");
    monstro_end_popup(ctx);
}

/* Modal dialog */
static bool modal_open = true;
if (monstro_button(ctx, "Confirm", NULL)) {
    modal_open = true;
    monstro_open_popup_ex(ctx, "confirm_dlg", true);
}
if (monstro_begin_popup_modal(ctx, "confirm_dlg", &modal_open, 0)) {
    monstro_label(ctx, "Are you sure?");
    if (monstro_button(ctx, "OK", NULL)) {
        modal_open = false;
        monstro_close_current_popup(ctx);
    }
    monstro_end_popup(ctx);
}
```

Modals dim the background and block input to other windows.  Press Escape or
click the close button (if `p_open` is provided) to dismiss.

---

## Layout System

By default, widgets stack vertically inside a window.  Use layouts to change
this.

### Horizontal layout

```c
monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
    monstro_button(ctx, "A", NULL);
    monstro_button(ctx, "B", NULL);
    monstro_button(ctx, "C", NULL);
monstro_end_layout(ctx);
```

### Percentage sizing

```c
float half = monstro_percent(ctx, 50.0f, MONSTRO_LAYOUT_HORIZONTAL);
monstro_button_sized(ctx, "Half Width", half, 30.0f, NULL);
```

### Spacers and Dummy items

```c
monstro_spacer(ctx, 20.0f);            /* advances cursor by 20px */
monstro_dummy(ctx, 100.0f, 50.0f);     /* invisible 100x50 box */
```

### Indentation

```c
monstro_indent(ctx, 20.0f);
monstro_label(ctx, "indented");
monstro_unindent(ctx, 20.0f);
```

### Columns

Columns provide aligned multi-column layouts without the overhead of tables.
There are no headers, borders, or sorting -- just evenly (or custom) sized
columns that align vertically across rows.

Equal-width columns:

```c
monstro_begin_columns(ctx, "info", 3);
    monstro_label(ctx, "Name");
    monstro_next_column(ctx);
    monstro_label(ctx, "Value");
    monstro_next_column(ctx);
    monstro_label(ctx, "Action");
    monstro_next_column(ctx);
monstro_end_columns(ctx);
```

Custom widths with `_ex` -- pass an array of pixel widths where `0.0f` means
"take remaining space equally":

```c
float widths[] = { 100.0f, 0.0f, 80.0f };
monstro_begin_columns_ex(ctx, "form", 3, widths);
    monstro_label(ctx, "Name:");
    monstro_next_column(ctx);
    monstro_input_text(ctx, "n##Name", name, sizeof(name), 0, NULL);
    monstro_next_column(ctx);
    monstro_button(ctx, "Clear", NULL);
    monstro_next_column(ctx);   /* wraps to column 0 of next row */

    monstro_label(ctx, "Volume:");
    monstro_next_column(ctx);
    monstro_slider_float(ctx, "v##Vol", &vol, 0, 1, "%.2f", NULL);
    monstro_next_column(ctx);
    monstro_button(ctx, "Reset", NULL);
    monstro_next_column(ctx);
monstro_end_columns(ctx);
```

Call `monstro_next_column` after each column's content, including the last
column in a row -- this wraps to column 0 of the next row.  The final
`next_column` before `end_columns` is optional but harmless.

### Cursor control

```c
MonstroVec2 pos = monstro_get_cursor_pos(ctx);         /* local to window */
MonstroVec2 spos = monstro_get_cursor_screen_pos(ctx); /* screen pixels */
monstro_set_cursor_pos(ctx, 10.0f, 50.0f);
```

### Remaining content space

```c
MonstroVec2 avail = monstro_get_content_region_avail(ctx);
monstro_button_sized(ctx, "Fill Width", avail.x, 30.0f, NULL);
```

This returns the visible content space remaining from the current cursor
position to the edge of the current layout region.

---

## Windows

### Creating a window

```c
monstro_set_next_window_pos(ctx, 100, 100, MONSTRO_COND_FIRST_USE_EVER);
monstro_set_next_window_size(ctx, 400, 300, MONSTRO_COND_FIRST_USE_EVER);

if (monstro_begin_window(ctx, "My Window", NULL, MONSTRO_WINDOW_DEFAULT)) {
    /* widgets */
    monstro_end_window(ctx);
}
```

### Close button

Pass a `bool*` as `p_open`.  When the user clicks the close button, Monstro
sets `*p_open` to `false`.  When `*p_open` is `false`, `begin_window` returns
`false` and the window is not drawn.  Pass `NULL` to hide the close button:

```c
static bool window_open = true;
if (monstro_begin_window(ctx, "Closeable", &window_open, flags)) {
    /* content */
    monstro_end_window(ctx);
}
```

### Child windows (scroll regions)

Create an independently scrollable sub-region:

```c
if (monstro_begin_child(ctx, "list", 0, 150,
        MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_BACKGROUND)) {
    for (int i = 0; i < 100; i++) {
        char buf[32]; snprintf(buf, 32, "Item %d", i);
        monstro_label(ctx, buf);
    }
    monstro_end_child(ctx);
}
```

Width or height of `0` means "use the remaining visible content space" on
that axis.  Negative values reserve space from that remaining region, so
`-30` means "use all but 30 pixels".

### Window state queries

```c
if (monstro_is_window_focused(ctx)) { /* ... */ }
if (monstro_is_window_hovered(ctx)) { /* ... */ }
if (monstro_is_window_collapsed(ctx)) { /* ... */ }
```

---

## Item State and Disabled Widgets

### Querying the last widget

Call these *immediately* after a widget to inspect its state:

```c
monstro_button(ctx, "Test", NULL);

if (monstro_is_item_hovered(ctx))  { /* mouse is over it */ }
if (monstro_is_item_active(ctx))   { /* being interacted with (held/dragging) */ }
if (monstro_is_item_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT)) { /* was clicked */ }
if (monstro_is_item_edited(ctx))   { /* value changed this frame */ }
if (monstro_is_item_deactivated_after_edit(ctx)) { /* just released after editing */ }

MonstroRect r = monstro_get_item_rect(ctx);       /* screen-space rect */
MonstroVec2 size = monstro_get_item_rect_size(ctx);
```

### Disabled state

Wrap widgets in a disabled scope to grey them out and block interaction:

```c
monstro_begin_disabled(ctx, true);
    monstro_button(ctx, "Can't click", NULL);
    monstro_slider_float(ctx, "Locked", &val, 0, 1, "%.2f", NULL);
monstro_end_disabled(ctx);
```

The `bool` parameter controls whether the scope is actually disabled.  This
makes it easy to toggle:

```c
monstro_begin_disabled(ctx, !user_has_permission);
    /* widgets */
monstro_end_disabled(ctx);
```

---

## Styling and Theming

### Built-in themes

```c
MonstroStyle* style = monstro_get_style(ctx);
monstro_style_set_default(style);  /* grey theme */
monstro_style_set_dark(style);     /* dark theme */
monstro_style_set_light(style);    /* light theme */
```

### Style struct hierarchy

`MonstroStyle` contains a sub-struct for each widget type:

- `style->button` (`MonstroStyleButton`) -- normal/hover/active items, colors,
  padding, rounding
- `style->checkbox` (`MonstroStyleToggle`)
- `style->slider` (`MonstroStyleSlider`)
- `style->edit` (`MonstroStyleEdit`)
- `style->tree` (`MonstroStyleTree`)
- `style->table` (`MonstroStyleTable`)
- `style->window` (`MonstroStyleWindow`)
- `style->menu`, `style->combo`, `style->tab`, `style->knob`,
  `style->progress`, `style->tooltip`, `style->separator`, ...

Each sub-struct has `MonstroStyleItem` fields for visual states (normal, hover,
active) plus scalar fields for rounding, border thickness, padding, and colors.

### Push/pop style overrides

Temporarily override an entire widget sub-style, then restore it:

```c
/* Make buttons have zero padding for table cells */
MonstroStyleButton btn = *monstro_get_button_style(ctx);
btn.padding[0] = 0.0f;
btn.padding[1] = 0.0f;
monstro_push_button_style(ctx, &btn);

monstro_button(ctx, "Compact", NULL);

monstro_pop_style(ctx, 1);  /* count = number of pushes to undo */
```

Each widget type has a matching `get`, `set`, and `push` function:

```c
/* Get the current live style (read-only) */
const MonstroStyleButton* cur = monstro_get_button_style(ctx);

/* Set the style permanently (no undo) */
MonstroStyleButton my_btn = *cur;
my_btn.rounding = 8.0f;
my_btn.text_normal = my_color;
monstro_set_button_style(ctx, &my_btn);

/* Push for temporary override (restore with pop) */
MonstroStyleText txt = *monstro_get_text_style(ctx);
txt.alignment = MONSTRO_ALIGN_CENTER;
monstro_push_text_style(ctx, &txt);
/* ... widgets ... */
monstro_pop_style(ctx, 1);
```

You can push multiple widget styles and pop them all at once:

```c
monstro_push_button_style(ctx, &my_btn);
monstro_push_edit_style(ctx, &my_edit);
monstro_push_text_style(ctx, &my_text);
/* ... widgets ... */
monstro_pop_style(ctx, 3);  /* pops text, then edit, then button */
```

**Style pushes and pops must be balanced within a frame.**  Monstro will throw
an exception at the next `monstro_begin_frame` if the stack is not empty.

### Creating a custom theme

```c
MonstroStyle* s = monstro_get_style(ctx);
monstro_style_set_dark(s);  /* start from dark base */

/* Customize via per-widget set functions */
MonstroStyleWindow win = *monstro_get_window_style(ctx);
win.background = monstro_color_rgba(0.12f, 0.12f, 0.15f, 1.0f);
win.rounding = 6.0f;
monstro_set_window_style(ctx, &win);

MonstroStyleButton btn = *monstro_get_button_style(ctx);
btn.rounding = 4.0f;
btn.normal.type = MONSTRO_STYLE_ITEM_COLOR;
btn.normal.data.color = monstro_color_rgba(0.2f, 0.4f, 0.8f, 1.0f);
monstro_set_button_style(ctx, &btn);
```

### Texture IDs and the renderer mapping

Monstro does not own or manage GPU textures.  A `texture_id` is an opaque
`uint64_t` that flows through unchanged from style items and font atlases into
`MonstroDrawCommand::texture_id`.  **Your renderer** is responsible for mapping
these IDs to real GPU texture handles at draw time.

**Choosing IDs:** The simplest approach is to cast your GPU texture handle
directly:

```c
GLuint gl_tex;
glGenTextures(1, &gl_tex);
/* ... upload pixel data ... */
uint64_t monstro_id = (uint64_t)gl_tex;
```

For Vulkan with bindless descriptors, use the descriptor index:

```c
uint64_t monstro_id = (uint64_t)vk_texture_index;
```

**Font atlas example** (from `examples/basic/main.cpp`):

```c
app.fontAtlasTextureId = (uint64_t)app.fontAtlasTexture;
monstro_font_set_atlas_texture(app.font, app.fontAtlasTextureId);
```

**Resolving in the draw loop:**

```c
for (uint32_t i = 0; i < cmd_count; i++) {
    uint64_t tex_id = cmds[i].texture_id;

    if (tex_id == font_atlas_id) {
        bind_texture(font_atlas_gpu_handle);
    } else if (tex_id != 0) {
        bind_texture(lookup_user_texture(tex_id));
    } else {
        bind_texture(white_pixel_texture);  /* fallback for untextured geometry */
    }
    /* ... draw ... */
}
```

**`texture_id == 0`** means untextured geometry (solid-colored shapes).  Your
renderer should bind a 1x1 white pixel texture so that vertex colors display
correctly.

**Registering additional textures** for use with `monstro_image`,
`monstro_image_button`, sprites, and nine-patches:

```c
/* Load your image into a GPU texture using whatever API you have */
GLuint my_icon;
glGenTextures(1, &my_icon);
/* ... upload ... */

uint64_t icon_id = (uint64_t)my_icon;

/* Now use it in Monstro: */
monstro_image(ctx, icon_id, 64.0f, 64.0f);
monstro_image_button(ctx, "btn_icon", icon_id, 32.0f, 32.0f, NULL);
```

There is no "register texture" API call.  You simply create a GPU texture in
your renderer, choose a `uint64_t` that identifies it, and use that value
wherever Monstro expects a `texture_id`.  Your draw loop must know how to
resolve that ID back to a GPU handle.

### Icons

Monstro provides an icon registry that maps user-chosen numeric IDs
(`MonstroIconId`, a `uint32_t`) to texture sub-regions. Icons can be
registered from sprite sheet atlases (full color) or icon font glyph ranges
(monochrome, tinted by text color). For icon fonts, the codepoint *is* the
icon ID, so you can reference icons directly from the font's documentation.
Once registered, icons can be embedded inline in any widget label using the
`{icon:ID}` tag syntax.

**Registering a sprite sheet atlas:**

```c
/* Upload your icon atlas to the GPU and get a texture_id */
uint64_t atlas_id = upload_texture("icons.png", &atlas_w, &atlas_h);

/* Register a grid of icons starting at ID 100: 16x16 pixel icons, 8 columns, 4 rows */
monstro_register_icon_atlas(ctx, 100, atlas_id,
                            16, 16,        /* icon size */
                            128, 64,       /* atlas size */
                            8, 4);         /* cols, rows */
/* Icons are assigned IDs 100..131 */
```

Icons are laid out left-to-right, top-to-bottom. The returned ID is the first
icon; subsequent icons are numbered sequentially.

**Registering a single icon with explicit UVs:**

```c
MonstroVec2 uv_min = {0.0f, 0.0f};
MonstroVec2 uv_max = {0.5f, 0.5f};
monstro_register_icon(ctx, 42, my_texture, uv_min, uv_max);
```

**Registering icon font glyphs (IDs are the codepoints):**

```c
/* Register FontAwesome codepoints 0xF000..0xF0FF as monochrome icons.
   Each icon's ID is its codepoint (e.g., 0xF000, 0xF001, ...). */
monstro_register_icon_font(ctx, fa_font, 0xF000, 256);
```

Monochrome icons are tinted with the current text color at render time.
For icon fonts, you can use the codepoint directly in `{icon:ID}` tags,
making it easy to reference icons from font documentation.

**Using icons in labels:**

Icons are embedded in widget label strings using the `{icon:ID}` tag. The tag
is parsed at render time and replaced with the icon graphic, sized to match the
current font line height.

```c
/* Build an icon tag string */
char ib[32];
monstro_icon_str(100, ib, sizeof(ib));  /* writes "{icon:100}" */

/* Use in a button label */
char label[64];
snprintf(label, sizeof(label), "save_btn##%sSave", ib);
monstro_button(ctx, label, NULL);  /* displays: [icon] Save */
```

The `##` separator works as usual: everything before `##` is ID-only,
everything after `##` is displayed. Icon tags in the ID portion (before `##`)
are not rendered -- they are just characters in the ID hash.

Icons work in any widget that displays a text label: buttons, tree nodes, menu
items, labels, checkboxes, selectables, tabs, and more. Multiple icon tags can
appear in a single label.

```c
/* Icon-only button (no visible text, icon in the display portion) */
snprintf(label, sizeof(label), "del##%s", ib);
monstro_button(ctx, label, NULL);

/* Multiple icons */
char ib2[32];
monstro_icon_str(101, ib2, sizeof(ib2));
snprintf(label, sizeof(label), "multi##%s%sStatus", ib, ib2);
monstro_label(ctx, label);
```

**Standalone icon widget:**

```c
monstro_icon(ctx, first, 24.0f);  /* draw icon at 24x24 pixels */
```

**Looking up icon data for manual drawing:**

```c
uint64_t tex;
MonstroVec2 uv_min, uv_max;
if (monstro_get_icon(ctx, first, &tex, &uv_min, &uv_max)) {
    /* use tex, uv_min, uv_max with monstro_image or custom drawing */
}
```

### Custom drawing

Monstro exposes the full set of canvas drawing primitives through the C API.
Every function operates on an opaque `MonstroCanvas*` handle, obtained from
the current window:

```c
MonstroCanvas* canvas = monstro_get_window_canvas(ctx);
```

**Shape primitives:**

```c
monstro_draw_line(canvas, p1, p2, color, thickness);
monstro_draw_rect(canvas, rect, color, rounding, MONSTRO_CORNER_ALL, thickness);
monstro_draw_rect_filled(canvas, rect, color, rounding, MONSTRO_CORNER_ALL);
monstro_draw_circle(canvas, center, radius, color, 12, thickness);
monstro_draw_circle_filled(canvas, center, radius, color, 12);
monstro_draw_triangle(canvas, p1, p2, p3, color, thickness);
monstro_draw_triangle_filled(canvas, p1, p2, p3, color);
monstro_draw_quad(canvas, p1, p2, p3, p4, color, thickness);
monstro_draw_quad_filled(canvas, p1, p2, p3, p4, color);
```

**Complex shapes:**

```c
monstro_draw_polyline(canvas, points, count, color, closed, thickness);
monstro_draw_convex_poly_filled(canvas, points, count, color);
monstro_draw_bezier_cubic(canvas, p1, cp1, cp2, p2, color, thickness, 0);
monstro_draw_bezier_quadratic(canvas, p1, cp, p2, color, thickness, 0);
```

Pass `0` for `num_segments` to use the default tessellation.

**Text and images:**

```c
monstro_draw_text(canvas, font, pos, color, "Hello");
monstro_draw_text_aligned(canvas, font, rect, color, "Centered", MONSTRO_ALIGN_CENTER);
monstro_draw_text_wrapped(canvas, font, pos, color, "Long text...", wrap_width);
monstro_draw_image(canvas, texture_id, rect, uv_min, uv_max, 0xFFFFFFFF);
monstro_draw_image_rounded(canvas, texture_id, rect, uv_min, uv_max, 0xFFFFFFFF, 4.0f, MONSTRO_CORNER_ALL);
monstro_draw_nine_patch(canvas, &patch, rect, 0xFFFFFFFF);
```

**Path API for complex shapes:**

```c
monstro_path_clear(canvas);
monstro_path_move_to(canvas, start);
monstro_path_line_to(canvas, mid);
monstro_path_arc_to(canvas, center, radius, 0.0f, 3.14f, 12);
monstro_path_bezier_cubic_to(canvas, cp1, cp2, end_pt, 0);
monstro_path_stroke(canvas, color, true, 2.0f);   /* or monstro_path_fill(canvas, color); */
```

**Clip rects** restrict drawing to a sub-region:

```c
monstro_push_clip_rect(canvas, region);
/* ... drawing calls are clipped to region ... */
monstro_pop_clip_rect(canvas);
```

**Custom draw callback** lets you inject your own rendering commands (e.g. for
GPU-native effects) into the draw stream:

```c
monstro_draw_custom(canvas, my_callback, my_user_data);
```

### Invisible button

`monstro_invisible_button` creates a clickable/hoverable layout region that
draws nothing.  You supply the visuals yourself using the draw API.  This is
the fundamental building block for custom interactive widgets.

```c
MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
if (monstro_invisible_button(ctx, "my_widget", 200.0f, 100.0f)) {
    /* clicked */
}
MonstroCanvas* canvas = monstro_get_window_canvas(ctx);
MonstroRect r = {pos.x, pos.y, 200.0f, 100.0f};

uint32_t bg = monstro_is_item_hovered(ctx) ? 0xFF444444 : 0xFF333333;
monstro_draw_rect_filled(canvas, r, bg, 4.0f, MONSTRO_CORNER_ALL);
```

After `monstro_invisible_button`, all the standard item queries work:
`monstro_is_item_hovered`, `monstro_is_item_active`, `monstro_is_item_clicked`,
`monstro_get_item_rect`.

### Custom widget authoring

To build a fully integrated custom widget, use these public primitives together:

1. `monstro_get_cursor_screen_pos` -- where to draw
2. `monstro_invisible_button` -- create an interactive region
3. `monstro_get_window_canvas` + draw functions -- render your visuals
4. `monstro_is_item_hovered` / `monstro_is_item_active` -- read interaction state
5. `monstro_add_item` -- if you need to advance layout manually
6. `monstro_set_last_item` -- if you need to set custom interaction state

**Example: simple sparkline widget**

```c
void sparkline(MonstroContext* ctx, const char* id, const float* values,
               int count, float width, float height) {
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    monstro_invisible_button(ctx, id, width, height);

    MonstroCanvas* canvas = monstro_get_window_canvas(ctx);
    MonstroRect r = {pos.x, pos.y, width, height};

    uint32_t bg = 0xFF1A1A2E;
    uint32_t line_color = monstro_is_item_hovered(ctx) ? 0xFF00DDFF : 0xFF0099CC;
    monstro_draw_rect_filled(canvas, r, bg, 2.0f, MONSTRO_CORNER_ALL);

    if (count < 2) return;
    float v_min = values[0], v_max = values[0];
    for (int i = 1; i < count; i++) {
        if (values[i] < v_min) v_min = values[i];
        if (values[i] > v_max) v_max = values[i];
    }
    float range = v_max - v_min;
    if (range < 0.0001f) range = 1.0f;

    MonstroVec2 points[256];
    int n = count < 256 ? count : 256;
    for (int i = 0; i < n; i++) {
        float t = (float)i / (float)(n - 1);
        float v = (values[i] - v_min) / range;
        points[i].x = pos.x + t * width;
        points[i].y = pos.y + height - v * height;
    }

    monstro_push_clip_rect(canvas, r);
    monstro_draw_polyline(canvas, points, n, line_color, false, 1.5f);
    monstro_pop_clip_rect(canvas);
}
```

### MonstroStyleItem -- the multi-type style value

Every visual property on a widget (normal, hover, active backgrounds, cursors,
indicators) is a `MonstroStyleItem`.  This is a tagged union supporting four
rendering modes:

**Color** -- a solid fill.  This is the default for all built-in themes.

```c
MonstroStyleItem item;
item.type = MONSTRO_STYLE_ITEM_COLOR;
item.data.color = monstro_color_rgba(0.2f, 0.6f, 1.0f, 1.0f);
style->button.hover = item;
```

**Image** -- a full texture stretched to fill the widget rect.

```c
MonstroStyleItem item;
item.type = MONSTRO_STYLE_ITEM_IMAGE;
item.data.texture_id = my_button_texture;
style->button.normal = item;
```

The entire texture is drawn.  Use this when each state has its own dedicated
texture file.

**Sprite** -- a sub-region of a texture atlas, defined by UV coordinates.
This is the most common mode for skinned UIs because many images can be packed
into a single atlas texture.

```c
MonstroStyleItem item;
item.type = MONSTRO_STYLE_ITEM_SPRITE;
item.data.sprite.texture_id = atlas_id;
item.data.sprite.uv_min = (MonstroVec2){ 0.0f,  0.0f };   /* top-left UV */
item.data.sprite.uv_max = (MonstroVec2){ 0.25f, 0.125f };  /* bottom-right UV */
style->button.normal = item;
```

To calculate UVs from pixel coordinates in your atlas:

```c
float u_min = pixel_x / (float)atlas_width;
float v_min = pixel_y / (float)atlas_height;
float u_max = (pixel_x + sprite_w) / (float)atlas_width;
float v_max = (pixel_y + sprite_h) / (float)atlas_height;
```

**Nine-Patch (Nine-Slice)** -- a texture sub-region that scales intelligently.

A nine-patch divides a texture into a 3x3 grid:

```
┌─────────┬───────────────┬─────────┐
│ corner  │  top edge     │ corner  │
│ (fixed) │  (stretch-x)  │ (fixed) │
├─────────┼───────────────┼─────────┤
│ left    │    center     │  right  │
│ edge    │  (stretch-xy) │  edge   │
│(str.-y) │               │ (str-y) │
├─────────┼───────────────┼─────────┤
│ corner  │ bottom edge   │ corner  │
│ (fixed) │  (stretch-x)  │ (fixed) │
└─────────┴───────────────┴─────────┘
```

- The four **corners** stay their original size regardless of widget
  dimensions.  This preserves rounded corners, bevels, or decorative details.
- The four **edges** stretch in one axis only.
- The **center** stretches in both axes to fill the remaining space.

This lets a single small texture (e.g. 32x32 pixels) produce a
correctly-rendered button, panel, or window of *any* size.

```c
MonstroStyleItem item;
item.type = MONSTRO_STYLE_ITEM_NINEPATCH;
item.data.ninepatch.texture_id = panel_atlas_id;
item.data.ninepatch.uv_min = (MonstroVec2){ 0.0f, 0.0f };
item.data.ninepatch.uv_max = (MonstroVec2){ 0.5f, 0.5f };
item.data.ninepatch.border_left   = 8.0f;   /* pixels from left edge to first vertical cut */
item.data.ninepatch.border_right  = 8.0f;
item.data.ninepatch.border_top    = 8.0f;
item.data.ninepatch.border_bottom = 8.0f;
item.data.ninepatch.texture_width  = 256;   /* full atlas width in pixels */
item.data.ninepatch.texture_height = 256;
style->window.fixed_background = item;
```

The `border_*` values define where the cuts happen in pixel coordinates
relative to the sprite sub-region.  The `texture_width`/`texture_height` fields
are needed so Monstro can convert pixel borders to UV offsets.

**Temporarily overriding a style item:**

```c
MonstroStyleItem highlight;
highlight.type = MONSTRO_STYLE_ITEM_COLOR;
highlight.data.color = monstro_color_rgba(1, 0.8f, 0, 1);

MonstroStyleButton btn = *monstro_get_button_style(ctx);
btn.normal = highlight;
monstro_push_button_style(ctx, &btn);
monstro_button(ctx, "Highlighted!", NULL);
monstro_pop_style(ctx, 1);
```

### Building a texture-based theme

To skin your entire UI with artwork:

**Design the atlas.** Create a texture atlas image containing all widget
states.  Tools like TexturePacker or manual layout in an image editor work.
Organize sprites by widget: button-normal, button-hover, button-active,
panel-background, slider-track, slider-thumb, etc.

**Load and register.** Upload the atlas to a GPU texture and pick a
`texture_id`:

```c
uint64_t atlas_id = upload_atlas_to_gpu("ui_skin.png", &atlas_w, &atlas_h);
```

**Define UV regions.** Map each sprite to its UV rectangle:

```c
MonstroSprite btn_normal = { atlas_id, {0.0f, 0.0f}, {0.25f, 0.125f} };
MonstroSprite btn_hover  = { atlas_id, {0.25f, 0.0f}, {0.5f, 0.125f} };
```

**Populate the style:**

```c
MonstroStyle* s = monstro_get_style(ctx);

s->button.normal.type = MONSTRO_STYLE_ITEM_SPRITE;
s->button.normal.data.sprite = btn_normal;

s->button.hover.type = MONSTRO_STYLE_ITEM_SPRITE;
s->button.hover.data.sprite = btn_hover;

/* Use nine-patch for window backgrounds so they scale cleanly */
s->window.fixed_background.type = MONSTRO_STYLE_ITEM_NINEPATCH;
s->window.fixed_background.data.ninepatch = (MonstroNinePatch){
    .texture_id = atlas_id,
    .uv_min = {0.0f, 0.5f}, .uv_max = {0.5f, 1.0f},
    .border_left = 12, .border_right = 12,
    .border_top = 24, .border_bottom = 12,
    .texture_width = atlas_w, .texture_height = atlas_h
};
```

**Multiple atlases.** Different sprites can reference different `texture_id`
values.  The draw commands will batch by texture automatically.  Keep
frequently-used elements in the same atlas to minimize texture switches.

---

## Extending: Building Complex Dialogs

Monstro provides primitive widgets.  Complex dialogs like file browsers and
color pickers are built by composing these primitives.

### Pattern: state struct + update function

Define a struct holding the dialog's state, and a function that emits the
widgets each frame.  The caller checks the return value or reads the state to
get results.

```c
typedef struct ColorPickerState {
    bool open;
    float r, g, b, a;
    char hex[8];
    bool confirmed;
} ColorPickerState;

void color_picker_init(ColorPickerState* s, float r, float g, float b, float a);
bool color_picker_update(MonstroContext* ctx, ColorPickerState* s);
```

### Pseudocode: file browser

```c
typedef struct FileBrowserState {
    bool open;
    char current_path[1024];
    char selected_file[256];
    char filter[64];
    /* ... directory listing cache ... */
    bool confirmed;
} FileBrowserState;

bool file_browser_update(MonstroContext* ctx, FileBrowserState* s) {
    if (!s->open) return false;

    monstro_open_popup_ex(ctx, "file_browser", true);
    if (monstro_begin_popup_modal(ctx, "file_browser", &s->open, 0)) {
        /* Path bar */
        monstro_input_text(ctx, "##path", s->current_path, sizeof(s->current_path),
                           MONSTRO_INPUT_TEXT_NONE, NULL);

        monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
        {
            /* Left panel: directory tree */
            if (monstro_begin_child(ctx, "dirs", 200, 300,
                    MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_BACKGROUND)) {
                /* monstro_begin_tree_node for each directory ... */
                monstro_end_child(ctx);
            }

            /* Right panel: file list */
            if (monstro_begin_child(ctx, "files", 0, 300,
                    MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_BACKGROUND)) {
                /* monstro_selectable for each file ... */
                monstro_end_child(ctx);
            }
        }
        monstro_end_layout(ctx);

        /* Bottom bar: filename + OK/Cancel */
        monstro_input_text(ctx, "name##File", s->selected_file,
                           sizeof(s->selected_file), MONSTRO_INPUT_TEXT_NONE, NULL);
        monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
        if (monstro_button(ctx, "Open", NULL)) {
            s->confirmed = true;
            s->open = false;
            monstro_close_current_popup(ctx);
        }
        if (monstro_button(ctx, "Cancel", NULL)) {
            s->open = false;
            monstro_close_current_popup(ctx);
        }
        monstro_end_layout(ctx);

        monstro_end_popup(ctx);
    }
    return s->confirmed;
}
```

### Full example: color picker

```c
#include <monstro.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct ColorPickerState {
    bool open;
    float r, g, b, a;
    char hex[8];
    bool confirmed;
} ColorPickerState;

void color_picker_init(ColorPickerState* s, float r, float g, float b, float a) {
    memset(s, 0, sizeof(*s));
    s->r = r; s->g = g; s->b = b; s->a = a;
    snprintf(s->hex, sizeof(s->hex), "%02X%02X%02X",
             (int)(r * 255), (int)(g * 255), (int)(b * 255));
}

static void sync_hex_from_rgb(ColorPickerState* s) {
    snprintf(s->hex, sizeof(s->hex), "%02X%02X%02X",
             (int)(s->r * 255), (int)(s->g * 255), (int)(s->b * 255));
}

static void sync_rgb_from_hex(ColorPickerState* s) {
    unsigned int rv = 0, gv = 0, bv = 0;
    if (sscanf(s->hex, "%02x%02x%02x", &rv, &gv, &bv) == 3) {
        s->r = rv / 255.0f;
        s->g = gv / 255.0f;
        s->b = bv / 255.0f;
    }
}

bool color_picker_update(MonstroContext* ctx, ColorPickerState* s) {
    s->confirmed = false;
    if (!s->open) return false;

    if (monstro_begin_popup_modal(ctx, "ColorPicker", &s->open, 0)) {
        bool changed = false;

        changed |= monstro_slider_float(ctx, "R", &s->r, 0, 1, "%.2f", NULL);
        changed |= monstro_slider_float(ctx, "G", &s->g, 0, 1, "%.2f", NULL);
        changed |= monstro_slider_float(ctx, "B", &s->b, 0, 1, "%.2f", NULL);
        changed |= monstro_slider_float(ctx, "A", &s->a, 0, 1, "%.2f", NULL);

        if (changed) sync_hex_from_rgb(s);

        monstro_spacer(ctx, 4.0f);
        monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
        monstro_label(ctx, "Hex:");
        if (monstro_input_text(ctx, "##hex", s->hex, sizeof(s->hex),
                               MONSTRO_INPUT_TEXT_CHARS_HEXADECIMAL, NULL)) {
            sync_rgb_from_hex(s);
        }
        monstro_end_layout(ctx);

        /* Preview swatch */
        monstro_spacer(ctx, 8.0f);
        MonstroStyleItem preview;
        preview.type = MONSTRO_STYLE_ITEM_COLOR;
        preview.data.color = monstro_color_rgba(s->r, s->g, s->b, s->a);
        MonstroStyleButton swatch_btn = *monstro_get_button_style(ctx);
        swatch_btn.normal = preview;
        swatch_btn.hover = preview;
        swatch_btn.active = preview;
        monstro_push_button_style(ctx, &swatch_btn);
        monstro_button_sized(ctx, "##preview", 120.0f, 40.0f, NULL);
        monstro_pop_style(ctx, 1);

        monstro_spacer(ctx, 8.0f);
        monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
        {
            if (monstro_button(ctx, "OK", NULL)) {
                s->confirmed = true;
                s->open = false;
                monstro_close_current_popup(ctx);
            }
            monstro_spacer(ctx, 8.0f);
            if (monstro_button(ctx, "Cancel", NULL)) {
                s->open = false;
                monstro_close_current_popup(ctx);
            }
        }
        monstro_end_layout(ctx);

        monstro_end_popup(ctx);
    }

    return s->confirmed;
}
```

Usage:

```c
static ColorPickerState picker;

if (monstro_button(ctx, "Pick Color", NULL)) {
    color_picker_init(&picker, cur_r, cur_g, cur_b, 1.0f);
    picker.open = true;
    monstro_open_popup_ex(ctx, "ColorPicker", true);
}

if (color_picker_update(ctx, &picker)) {
    cur_r = picker.r;
    cur_g = picker.g;
    cur_b = picker.b;
}
```

---

## Examples

The `examples/` directory contains runnable demos that show Monstro in action.
Each example beyond the basic demo is structured as a **reusable library**:
the feature logic lives in its own `.h` / `.cpp` pair with a plain C API,
while `main.cpp` is a thin harness that wires it up.  This means you can drop
the library files directly into your own project and call the same `_init`,
`_draw`, or `_update` functions -- no need to extract code from a monolithic
demo.

All examples share a common application shell (`examples/common/example_app.h`)
that handles GLFW window creation, Vulkan rendering via VulkEase, Monstro
context setup, input forwarding, and the main loop.  To create a new example,
define an `ExampleAppDesc` with init/update callbacks and call
`example_app_run`.

### basic

The kitchen-sink demo (`examples/basic/main.cpp`).  Exercises most of the
widget catalog in a single window: labels, buttons, checkboxes, sliders, knobs,
text input, tables, trees, tabs, color picker, child scroll regions, file
dialog, menus, and more.  Also serves as the reference for GLFW/Vulkan
integration.

### console

An interactive debug console with log levels (info, warning, error, debug),
text filtering, command history, auto-scroll, and a pluggable command system.

- **Library:** `console.h` / `console.cpp`
- **API:** `console_init`, `console_log`, `console_add_command`,
  `console_draw`, `console_clear`

### node

A node graph editor with draggable nodes, typed input/output ports, and
link connections drawn as Bezier curves.  Demonstrates custom canvas drawing
inside a Monstro window.

- **Library:** `node.h` / `node.cpp`
- **API:** `node_editor_init`, `node_editor_add_node`,
  `node_editor_add_port`, `node_editor_add_link`, `node_editor_draw`

### plot

Line, bar, and scatter chart widgets with configurable axis ranges, multiple
series, and automatic legend rendering.

- **Library:** `plot.h` / `plot.cpp`
- **API:** `plot_init`, `plot_add_series`, `plot_set_data`, `plot_set_range`,
  `plot_draw`

### curve

A Bezier curve editor with draggable control points, useful for animation
curves, color ramps, or audio envelopes.

- **Library:** `curve.h` / `curve.cpp`
- **API:** `curve_editor_init`, `curve_editor_add_point`, `curve_editor_draw`

### timeline

A multi-track timeline with clips and a playhead, suited for animation
sequencing, audio arrangement, or video editing UIs.

- **Library:** `timeline.h` / `timeline.cpp`
- **API:** `timeline_init`, `timeline_add_track`, `timeline_add_clip`,
  `timeline_draw`

### property

A property inspector panel that supports bool, int, float, string, color,
and enum fields organized into collapsible sections.

- **Library:** `property.h` / `property.cpp`
- **API:** `property_editor_init`, `property_editor_add_section`,
  `property_editor_add_bool`, `property_editor_add_int`,
  `property_editor_add_float`, `property_editor_add_string`,
  `property_editor_add_color`, `property_editor_add_enum`,
  `property_editor_draw`

### shortcut

A keyboard shortcut editor for defining and displaying key bindings with
modifier keys.

- **Library:** `shortcut.h` / `shortcut.cpp`
- **API:** `shortcut_editor_init`, `shortcut_editor_add`,
  `shortcut_editor_draw`

### toast

A toast notification system that displays timed popup messages in the corner
of the screen with per-type color coding (info, success, warning, error) and
fade-in / fade-out animation.

- **Library:** `toast.h` / `toast.cpp`
- **API:** `toast_init`, `toast_show`, `toast_draw`

---

## Tips and Best Practices

**ID uniqueness.** Use `##` prefixes and `monstro_push_id`/`monstro_pop_id`
when generating widgets in loops.  The text before `##` is the unique ID
portion; the text after `##` is what gets displayed.  Duplicate IDs within the
same window cause unpredictable behavior.

**Performance.** For large lists (hundreds of items), use `monstro_begin_child`
to create a scroll region.  Only the visible items consume draw commands.
Minimize the total widget count per frame.

**Input routing.** Check `monstro_want_capture_mouse` and
`monstro_want_capture_keyboard` before sending input events to your game or
application logic.  This prevents camera movement while dragging a slider, for
example.

**Balanced stacks.** Style pushes (`monstro_push_style_*`), ID pushes
(`monstro_push_id`), and disabled scopes (`monstro_begin_disabled`) must be
balanced with their corresponding pops/ends within each frame.  Monstro
validates stack balance at the start of each frame and throws an exception if
any stack is unbalanced.

**`end` calls go inside the `if` block.** When a `begin_*` function returns
`false` (window closed, off-screen, etc.), the window was not processed and you
should not call the matching `end_*`.  Only call `end_*` when `begin_*`
returned `true`.

**Conditions for window placement.** Use `MONSTRO_COND_FIRST_USE_EVER` for
initial placement so that user-repositioned windows are not reset every frame.
Use `MONSTRO_COND_ALWAYS` only when you need programmatic control (e.g.
centering a modal).

**Custom draw callbacks.** The `MonstroDrawCommand::custom_draw` field lets you
inject your own rendering code (e.g. a 3D viewport) into the draw command
stream.  Monstro will call your function with the clip rect and layer at the
appropriate point in the draw order.
