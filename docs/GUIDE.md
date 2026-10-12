# Monstro UI User Guide

Monstro is an immediate-mode graphical user interface library with a C API and
C++ internals.  It is **graphics-API agnostic**: Monstro never touches the GPU.
Instead it produces vertex buffers, index buffers, and draw commands that your
application feeds to whatever renderer it already has -- Vulkan, OpenGL, D3D12,
Metal, or anything else.

If you have used Dear ImGui, Monstro will feel familiar.  The key differences
are:

- Every API call takes an explicit environment or context -- there is no
  hidden global state.
- What UIs share (fonts, themes, icons, settings, frame scratch memory) lives
  once in an environment; each UI is a small context on it.
- GPU resources (textures, buffers) are owned by *your* application.  Monstro
  only references them through opaque `uint64_t` texture IDs.
- Style items can be solid colors, full textures, sprite atlas sub-regions, or
  nine-patch textures, making it straightforward to skin an entire application
  with artist-authored artwork.

---

## Architecture Overview

```
 Your Application     Monstro                                Your Renderer
 ┌──────────────┐     ┌────────────────────────┐              ┌──────────────────┐
 │ input events ├────>│ MonstroContext         │  draw data   │ GPU draw calls   │
 │ (mouse, kbd) │     │ (one per UI)           │─────────────>│ (vertices, idx,  │
 └──────────────┘     │ borrows a canvas       │              │  textures, clip) │
                      │ for each frame         │              └──────────────────┘
                      ├────────────────────────┤
                      │ MonstroEnv (shared)    │
                      │ fonts and atlas,       │
                      │ themes, icons,         │
                      │ settings, canvases     │
                      └────────────────────────┘
```

**Key objects:**

- `MonstroEnv` -- the environment, created by `monstro_init` and destroyed by
  `monstro_shutdown`.  It holds everything global, once: fonts packed into one
  atlas, themes, icons, settings (double click, drag, key repeat and tooltip
  timing), and the canvases frames are built in.
- `MonstroContext` -- one UI: a screen, a panel, a tool window.  It receives
  that UI's input, has its display size, and keeps its interaction state
  between frames (windows and their positions and scroll, popups, tab bars,
  tree nodes, tables, hot and active widgets, text editing).  Contexts are
  small; create as many as you have UIs.
- `MonstroCanvas` -- the frame scratch: vertex, index and command buffers, the
  path, clip and layer stacks, and the frame's working style and nesting
  stacks.  The environment lends one to a context for each frame.
- `MonstroDrawData` -- a finished frame: vertices, indices and draw commands
  in draw order.
- `MonstroTheme` -- a style registered in the environment, shared by any
  number of contexts.
- `MonstroFont` -- a font in the environment's atlas.

**Frame lifecycle:**

1. Feed the context its input.
2. Call `monstro_begin_frame(ctx, delta_time)`: the context borrows a canvas.
3. Emit windows and widgets (`monstro_begin_window`, `monstro_button`, etc.).
4. Call `monstro_end_frame(ctx)`: the canvas goes back to the environment and
   the call returns the frame's `MonstroDrawData`.
5. Upload and draw it.

### Ownership and lifetimes

| Object | Created by | Lives until | Owned by |
| --- | --- | --- | --- |
| Environment | `monstro_init` | `monstro_shutdown` | you |
| Context | `monstro_context_create(env, ...)` | `monstro_context_destroy` | you; destroy before its environment |
| Font | `monstro_font_load_*(env, ...)` | `monstro_shutdown` | the environment |
| Theme | `monstro_theme_register(env, ...)` | `monstro_shutdown` | the environment |
| Icon | `monstro_register_icon*(env, ...)` | `monstro_shutdown` | the environment |
| Canvas | `monstro_init` (`canvas_count` of them) | `monstro_shutdown` | the environment; lent for one frame |
| Draw data | `monstro_end_frame` | the canvas's next frame | the canvas |

Everything in the environment is set up between frames.  While any frame is
open the environment is read-only: loading a font, registering a theme or an
icon, changing settings or the atlas texture, or creating or destroying a
context then stops the program.  `monstro_shutdown` stops it too while
contexts still use the environment.

### The canvas lending rule

`monstro_begin_frame` borrows a canvas and `monstro_end_frame` returns it.  The
draw data `monstro_end_frame` returns lives in that canvas and **stays valid
until the canvas is lent again**.  With the default single canvas, that is the
next `monstro_begin_frame` of *any* context on the environment, so render a
context's frame before building the next one:

```c
monstro_begin_frame(hud, dt);    /* ... */  const MonstroDrawData* a = monstro_end_frame(hud);
render(a);                                   /* a is valid here */
monstro_begin_frame(panel, dt);  /* ... */  const MonstroDrawData* b = monstro_end_frame(panel);
/* a now shows panel's frame: the canvas has been reused */
```

Consume the draw data -- upload or render it -- before the canvas builds the
next frame; Monstro keeps no copy of it.  `frame_id` in the draw data numbers
the environment's frames, so code that holds draw data can tell whether its
canvas has built another frame since.

With `MonstroEnvConfig.canvas_count` set to N, the canvases are lent in turn,
so the draw data of the last N frames stays valid, and up to N frames (of
different contexts) can be open at once.  The design leaves room for building
different contexts on different threads later, each on its own canvas, with
the environment read-only meanwhile; today Monstro supports sequential use
from one thread.

---

## Getting Started -- Minimal Integration

Below is the smallest useful Monstro program.  Graphics-API-specific code is
replaced with comments -- the next sections fill those in.

```c
#include <monstro.h>

int main(void) {
    /* --- 1. Create the environment and load a font --- */
    MonstroEnv* env = monstro_init(NULL);           /* NULL: default capacities */
    monstro_font_load_ttf_from_file(env, "default", "path/to/font.ttf",
                                    16.0f,          /* pixel size */
                                    32,             /* first code point (space) */
                                    96);            /* code point count (ASCII printable) */

    /* Upload the atlas (fonts and the white texel solid shapes use) to a GPU texture. */
    MonstroTextureData atlas = monstro_env_get_atlas(env);
    uint64_t atlas_texture = /* your_gpu_create_texture(atlas) */;
    monstro_env_set_atlas_texture(env, atlas_texture);

    /* --- 2. Create a context for the UI --- */
    MonstroContext* ctx = monstro_context_create(env, NULL);
    monstro_set_display_size(ctx, 1280.0f, 720.0f);

    /* --- 3. Main loop --- */
    while (running) {
        float dt = /* seconds since last frame */;

        /* Feed input (see Windowing Library Integration) */
        monstro_set_mouse_pos(ctx, mouse_x, mouse_y);
        monstro_set_mouse_button(ctx, MONSTRO_MOUSE_BUTTON_LEFT, left_down);
        /* ... keyboard, scroll, text ... */

        /* --- 4. Build UI --- */
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

        const MonstroDrawData* draw = monstro_end_frame(ctx);

        /* --- 5. Render --- */
        if (draw->changed) {
            /* Upload draw->vertices (draw->vertex_count) and draw->indices (draw->index_count). */
        }
        for (uint32_t i = 0; i < draw->command_count; i++) {
            const MonstroDrawCommand* dc = &draw->commands[i];
            /* Set scissor from dc->clip_rect, bind dc->texture_id, then draw
               dc->index_count indices from dc->index_offset with base vertex dc->vertex_offset. */
        }
    }

    /* --- 6. Cleanup --- */
    monstro_context_destroy(ctx);
    monstro_shutdown(env);
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

Input is queued, not sampled: each call records a change, and the next frames
take the changes in order, so a press and release between two frames click in
the next frame.  Calling a setter with the state it already has records nothing, so
forwarding events from callbacks and polling the whole state once a frame both
work.  See [Input between frames](#input-between-frames).

Each context has its own input and display size.  The timing that interprets
input -- double click time and distance, drag threshold, key repeat delay and
rate, tooltip delay -- is shared, in the environment's settings:

```c
MonstroSettings settings = monstro_env_get_settings(env);
settings.key_repeat_delay = 0.4f;
monstro_env_set_settings(env, &settings);   /* between frames */
```

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

`monstro_end_frame` returns the frame's draw data:

```c
typedef struct MonstroDrawData {
    const MonstroVertex* vertices;       /* NULL when drawn into host buffers */
    uint32_t vertex_count;
    const uint32_t* indices;
    uint32_t index_count;
    const MonstroDrawCommand* commands;  /* in draw order */
    uint32_t command_count;
    float display_size[2];
    uint64_t content_hash;               /* equal hashes: identical frames */
    bool changed;                        /* differs from this context's previous frame */
    uint64_t atlas_version;              /* the atlas when the frame was built */
    uint64_t frame_id;                   /* the environment's frame number */
} MonstroDrawData;
```

**Vertices** -- `MonstroVertex`:

```c
typedef struct MonstroVertex {
    float position[2];   /* screen-space pixel coordinates */
    float texcoord[2];   /* UV in the command's texture */
    uint32_t color;      /* packed RGBA: R in the least significant byte, A in the most significant */
} MonstroVertex;
```

**Draw commands** -- `MonstroDrawCommand`:

```c
typedef struct MonstroDrawCommand {
    float clip_rect[4];    /* x, y, width, height in pixels */
    uint64_t texture_id;   /* the atlas texture ID for text and solid shapes */
    uint32_t vertex_offset;
    uint32_t vertex_count;
    uint32_t index_offset;
    uint32_t index_count;
    int layer;
    MonstroCustomDrawCallback custom_draw;  /* NULL for normal draws */
    void* user_data;
} MonstroDrawCommand;
```

Each command's vertices are the `vertex_count` vertices from `vertex_offset`,
and its indices are the `index_count` entries from `index_offset`, **counting
from `vertex_offset`**.  A renderer binds the vertex range as the base vertex
(`vkCmdDrawIndexed`'s `vertexOffset`, `glDrawElementsBaseVertex`) and draws the
index range as is, or slices one command's vertices and indices into its own
mesh without touching them.

Text and solid shapes are drawn from the same texture: every font lives in the
environment's atlas, and the atlas has a white texel that solid shapes sample.
So a window's fills and text batch into few commands.

Commands come in draw order: a window, then the windows begun after it at the
same level (its later siblings, which a click brings forward), then its own
popups, menus and combo lists, which so stay on top of the windows that follow
it.  Child panels (`monstro_begin_child`) are part of their window: they draw
right after it, so a window in front covers them.  Tooltips draw last, over
everything.  Within a window, commands go by layer, then in the order things
were drawn.  Hovering follows the same order: of the windows under the mouse,
the one drawn on top takes it (tooltips never take the mouse).

### Generic rendering loop

```c
const MonstroDrawData* draw = monstro_end_frame(ctx);
if (draw->changed)
    upload(draw->vertices, draw->vertex_count, draw->indices, draw->index_count);

for (uint32_t i = 0; i < draw->command_count; i++) {
    const MonstroDrawCommand* dc = &draw->commands[i];

    if (dc->custom_draw) {
        dc->custom_draw(dc->user_data, dc->clip_rect, dc->layer);
        continue;
    }

    set_scissor_rect(dc->clip_rect[0], dc->clip_rect[1],
                     dc->clip_rect[2], dc->clip_rect[3]);

    bind_texture(resolve_texture_id(dc->texture_id));

    draw_indexed(dc->index_count, dc->index_offset, /* base vertex */ dc->vertex_offset);
}
```

`content_hash` covers the commands and every vertex and index value written,
so a renderer that keeps last frame's buffers on the GPU can skip the upload
when `changed` is false (or compare hashes itself, for example across
contexts).  Upload the atlas again when `monstro_env_get_atlas(env).version`
changes; loading a font changes it.

### Pipeline requirements

- **Blending**: standard alpha blending
  (`srcAlpha, oneMinusSrcAlpha` for color; `one, oneMinusSrcAlpha` for alpha).
- **Depth test**: disabled.
- **Culling**: disabled (none).
- **Vertex input**: the `MonstroVertex` layout above, or bindless via buffer
  device address.
- **Textures**: RGBA, multiplied by the vertex color.  The atlas is RGBA8 with
  white glyphs whose coverage is in alpha.

### Vulkan worked example

`examples/common/example_app.h` demonstrates a full Vulkan integration using
the VulkEase helper library.  Key excerpts:

**Buffer upload** -- after each frame, when it changed:

```c
const MonstroDrawData* data = monstro_end_frame(app.ui);
if (data->changed)
    ea_upload_buffers(&app, data);   /* memcpy data->vertices and data->indices to GPU buffers */
```

**Scissor** -- set from `clip_rect`:

```c
VkRect2D scissor = {
    .offset = { (int32_t)dc->clip_rect[0], (int32_t)dc->clip_rect[1] },
    .extent = { (uint32_t)dc->clip_rect[2], (uint32_t)dc->clip_rect[3] }
};
vkCmdSetScissor(cmd, 0, 1, &scissor);
```

**Draw** -- with the command's base vertex:

```c
vkCmdDrawIndexed(cmd, dc->index_count, 1, dc->index_offset, (int32_t)dc->vertex_offset, 0);
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
glDrawElementsBaseVertex(GL_TRIANGLES, dc->index_count, GL_UNSIGNED_INT,
                         (void*)(dc->index_offset * sizeof(uint32_t)), dc->vertex_offset);
```

### Host buffers

By default a frame's geometry goes into the canvas's own buffers.  A host can
have it written straight into its own memory instead -- persistently mapped
GPU buffers, or arrays its engine takes as is -- and rotate two or three of
them while the GPU reads an earlier frame.

Where each vertex attribute goes is described once, in the environment's
vertex layout.  Formats are fixed (position: two floats; texcoord: two floats;
color: one packed `uint32_t`); only their placement varies.  Each attribute
names a buffer and its byte offset within that buffer's stride:

```c
MonstroEnvConfig config = monstro_env_config_default();
/* Positions in one stream; color then texcoord in a second. */
config.vertex_layout.buffer_count = 2;
config.vertex_layout.stride[0] = 8;
config.vertex_layout.stride[1] = 12;
config.vertex_layout.position = (MonstroVertexAttribute){ 0, 0 };
config.vertex_layout.color    = (MonstroVertexAttribute){ 1, 0 };
config.vertex_layout.texcoord = (MonstroVertexAttribute){ 1, 4 };
MonstroEnv* env = monstro_init(&config);
```

The default layout is Monstro's own interleaved vertex: one buffer, stride
`sizeof(MonstroVertex)`, offsets 0, 8 and 16.  `monstro_init` checks the
layout and stops the program if a buffer index is out of range, an attribute
does not fit its buffer's stride, two attributes overlap, or a buffer holds no
attribute.

Each frame, pass the buffers to `monstro_begin_frame_into`:

```c
MonstroFrameBuffers buffers = {0};
buffers.vertex_buffers[0] = positions[ring];   /* one pointer per layout buffer */
buffers.vertex_buffers[1] = attributes[ring];
buffers.vertex_capacity = max_vertices;        /* in vertices */
buffers.indices = indices[ring];
buffers.index_capacity = max_indices;
monstro_begin_frame_into(ctx, dt, &buffers);
/* ... */
const MonstroDrawData* draw = monstro_end_frame(ctx);   /* draw->vertices is NULL; the vertices are in your buffers */
```

Leave `vertex_buffers[0]` null to keep vertices in the canvas, or `indices`
null to keep indices there.  The buffers are used for that frame only; Monstro
keeps no pointer to them afterwards.

Monstro only ever **writes** host buffers, never reads them back, so
write-combined GPU memory is fine; the content hash is taken from the values
as they are written.  Their capacity is yours: a frame that needs more stops
the program naming the host buffer, its capacity and what was needed.  The
host keeps its own buffers: Monstro writes a frame into them and forgets them.

---

## Font System

Fonts belong to the environment.  Each is loaded once, packed into the
environment's single RGBA8 atlas, and shared by every context.  They live until
`monstro_shutdown`.

### Loading TTF fonts

```c
MonstroFont* font = monstro_font_load_ttf_from_file(
    env,
    "body",               /* name, for monstro_font_find */
    "fonts/MyFont.ttf",
    18.0f,                /* pixel size */
    32,                   /* first codepoint (space) */
    96                    /* count (ASCII printable range) */
);
```

`monstro_font_load_ttf` takes the font file's bytes instead of a path; Monstro
reads them only during the call.  Both return `NULL` when the data cannot be
read as a font.  Running out of atlas space (`atlas_width` x `atlas_height`) or
font slots (`max_fonts`) stops the program.

The first font loaded becomes the environment's default font, which themes
without a font of their own use.  `monstro_env_set_default_font` changes it.

### The atlas

```c
MonstroTextureData atlas = monstro_env_get_atlas(env);
/* atlas.data, atlas.width, atlas.height, atlas.data_size,
   atlas.format (MONSTRO_TEXTURE_FORMAT_RGBA8), atlas.version */
uint64_t tex_id = upload_to_gpu(atlas.data, atlas.width, atlas.height);
monstro_env_set_atlas_texture(env, tex_id);
```

Draw commands for text and solid shapes carry this texture ID (0 until you set
one).  Every font load changes `atlas.version`; upload the pixels again when it
does.  The atlas also holds a white texel that solid shapes sample, so fills
and text batch into the same commands.

### Loading bitmap fonts

```c
MonstroFont* mono = monstro_font_load_bitmap_from_file(
    env,
    "mono",
    "data/fonts/DejaVuSansMono-bitmap.png",
    16,    /* grid columns */
    6,     /* grid rows */
    32,    /* first codepoint: space */
    16.0f  /* font size in pixels */
);
```

The image is a grid of equal cells, one code point per cell from the first,
left to right and top to bottom; each cell is drawn as a square of the font
size and advances 0.6 of it.  The image is copied into the atlas.
`monstro_font_load_bitmap` takes RGBA8 pixels instead of a path.
`tools/make_bitmap_font.c` renders a TrueType font into such a grid; the
console example's font is made with it.

### Multiple fonts

Load additional fonts at different sizes or families.  Use `monstro_font_find`
to retrieve them later:

```c
MonstroFont* large = monstro_font_load_ttf_from_file(env, "body", "font.ttf", 32.0f, 32, 96);

/* Later: */
MonstroFont* f = monstro_font_find(env, "body", 32.0f);
```

A theme picks its font through `MonstroStyle.font`.

### Text measurement

```c
MonstroVec2 size = monstro_font_measure_text(font, "Hello World", 0.0f);
MonstroVec2 wrapped = monstro_font_measure_text_wrapped(font, long_text, 200.0f);
```

Wrapped text breaks at newlines and at spaces; a word longer than a whole line
breaks where the line is full. Measuring and drawing break lines the same way.

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

Every widget needs an ID that is unique within its scope: its window, and any
IDs pushed inside it.  A label's text **before** `##` is its ID; the text
**after** `##` is what is shown.  This is the opposite of Dear ImGui's
convention.  Without `##`, the whole label is both the ID and the text.

```c
monstro_button(ctx, "position##Reset", NULL);  /* shows "Reset", ID "position" */
monstro_button(ctx, "rotation##Reset", NULL);  /* shows "Reset", ID "rotation" */
monstro_input_text(ctx, "cmd##", buf, sizeof(buf), 0, NULL);  /* shows no label */
```

Because only the text before `##` is the ID, a label can show a live value and
keep its state.  A slider being dragged, an open tree node or a text field
being edited stays what it is as its text changes:

```c
char label[64];
snprintf(label, sizeof(label), "speed##Speed: %.1f m/s", speed);
monstro_slider_float(ctx, label, &speed, 0.0f, 100.0f, "%.1f", NULL);
```

Without the `##`, `"Speed: 12.3"` would be a new widget every time the number
changed.  A label whose ID would be empty, such as `"##cmd"`, stops the
program: write `"cmd##"` to show nothing.  Window names follow the same rule
(`"main##Hello World"` is the window `main` with the title "Hello World").

Arguments named `str_id` -- invisible and image buttons, tables, tab bars,
column sets -- are never shown, so they are used whole, as are the names given
to `monstro_push_id` and `monstro_get_id`.

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
- `MONSTRO_WINDOW_CHILD` -- marks a child region, clipped by its parent.
  `monstro_begin_child` sets it; it is not part of `MONSTRO_WINDOW_DEFAULT`.

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

A missing or extra `end` stops the program; see
[Errors and Limits](#errors-and-limits).

---

## Interaction Rules

These hold for every widget and window; the `interaction` example puts each
one on a page to click through.

### Hover: one item, the one on top

At most one item is hovered in a frame, and only it can be pressed.  Across
windows, the window drawn on top under the mouse takes it (popups over their
window, windows in front over windows behind, child panels at their window's
depth).  Within a window, items drawn later cover items drawn earlier, so of
two overlapping buttons only the second lights up and fires.  A disabled item
is never hovered but still covers what is under it.

Monstro learns which item is on top only as the frame is built, after the
items under it have been drawn, so it takes the answer from the previous
frame: every item is recorded with the rect where it shows, and when a frame
begins, the last item recorded under the mouse in the window on top is the
one on top there, wherever the mouse has moved since.  A press that arrives
with a move goes to that item in the same frame.  An item drawn for the first
time is not in the record yet; where nothing on top is known, the first item
under the mouse is hovered for that frame.  The record holds `max_items` items
a frame; a frame that draws more stops the program.

### Clipping

An item takes the mouse only where it is drawn: the part of it inside the
current clip rect.  A button scrolled half out of a child panel can be clicked
on its visible half only, and a child panel scrolled under its window's title
bar does not take the mouse there.

What lies wholly outside the clip rect is not drawn at all: a shape, image or
glyph clipped away adds no vertices, indices or draw command.  A widget
clipped away still runs and keeps its place in the layout, so what follows it
does not move; it only costs nothing to render.  A long list scrolled far, or
many cockpit screens each clipping its pages, draws only what shows.

### Active items

An item is active while it is held (a pressed button, a dragged slider, knob
or scrollbar, a resize grip), edited (a text or number field) or open (a
combo).  While a held item is active, no other item hovers or activates:
dragging a slider across a button leaves the button dark, and releasing over
it does not click it.  A held item lets go when the button is released,
wherever the mouse is, or when it is disabled.  A tap holds an item for one
frame: it becomes active and lets go in that frame.

### Activated and deactivated

`monstro_is_item_activated` holds in the frame an item becomes active and
`monstro_is_item_deactivated` in the frame it stops;
`monstro_is_item_deactivated_after_edit` holds when it stops and its value
changed at any point while it was active.  A tap reports both in its frame.
A host that sends one command per change, rather than one per frame, acts on
these edges:

```c
monstro_slider_float(ctx, "throttle##Throttle", &throttle, 0.0f, 1.0f, "%.2f", NULL);
if (monstro_is_item_activated(ctx))
    begin_throttle_change();
if (monstro_is_item_deactivated_after_edit(ctx))
    send_throttle(throttle);
```

At most four items are active at once (in practice one is held and one is
being edited); more stops the program.

### IDs

A label's text before `##` is its ID and the text after it is shown, so a
label can show a live value and keep its state: `"speed##Speed: %.1f"`.  See
[ID system](#id-system).

### Popups and child panels

A popup's ID comes from the ID scope it is opened in, and its window is named
by that ID, so two windows that each open a popup called `"menu"` have two
popups, each with its own position and content, and both can be open at once.
Child panels are named the same way: two `"list"` panels under different
pushed IDs are two panels.  Top-level windows are found by name within their
context; windows of different contexts never meet.

### Input between frames

The input setters queue changes (up to `max_input_events`), and each frame
takes them in order until one would change something that frame already
changed.  A press is taken with the move before it, and a release with the
press before it, so a click, or a tap on a touch screen that moves, presses
and releases between two frames, is seen in the next frame as the button
pressed and released (and not down), and the item tapped fires in that frame.
An item that follows the mouse while held is held for that frame and let go
at its end: a slider jumps to where it was tapped and reports activated and
deactivated after edit together.

The rest wait for the following frames: a move after a button change, so the
button changes where the mouse was; a release then a press of the same
button, so a held item lets go first; a second tap; and a key's second
change.  A double tap between two frames is seen as two taps in two frames,
and still double-clicks.  Every change reaches the UI in the order it
happened.

Which item a tap lands on comes from the last frame, the usual immediate-mode
caveat: it goes to the item the last frame showed on top at that point, which
is what the user saw when they tapped.  Where that changed since -- the host
scrolled or moved things between frames, or a wheel movement taken with the
tap scrolls the panel under it -- the tap can land on nothing for that frame.
The frame a button is pressed in may also carry the move that brought the
mouse there: drags by movement (window resize and move, scrollbars, table
column grips, knobs) start from where it pressed, but
`monstro_get_mouse_delta` includes that move, so a custom widget that drags
by the delta skips the frame of the press, as the node example does.

Wheel movements are summed, moves in a row are merged, and setting a state
that is already current queues nothing.  Modifiers and typed text are not
queued.

---

## Errors and Limits

Monstro treats a broken rule as a bug to fix, not a condition to survive.
Nothing in the C API throws, returns an error code, or quietly ignores a
mistake: the library prints where it caught the problem, what was wrong and
the values involved to `stderr`, then calls `abort()`.  Run under a debugger
and the stack shows the call that caused it.

```
monstro: fatal error at src/canvas.cpp:245: requirement 'needed_vertices <= vertex_capacity' failed: vertex buffer overflow: capacity 65536, needed 65539 (window 'Status', frame 812); simplify that UI or raise MonstroEnvConfig.max_vertices
```

These checks are always on, in release builds too.  They catch:

- **Null handles and arguments.** A null environment, context, canvas, font or
  theme, and a null label, text or value pointer a function needs, stop the
  program at the call.  Arguments documented as optional (a tooltip, `p_open`,
  a format) may be `NULL`.
- **Calls outside a frame.** Window and widget calls, and drawing on a canvas,
  belong between `monstro_begin_frame` and `monstro_end_frame`.
- **Changing the environment during a frame.** It is read-only while any
  frame is open.
- **Unbalanced pairs.** Every `begin`/`end` and `push`/`pop` must close:
  windows (and children, popups, menus, combos), IDs, styles, disabled
  scopes, columns, tables and tab bars are checked at the end of the frame;
  layouts, groups and clip rects when their window ends.  Popping an empty
  stack fails where it happens, and so does ending a child region with
  `monstro_end_window` or a window with `monstro_end_child`.
- **Out-of-range arguments**, such as a table with more than 128 columns, a
  column set of more than 16, a vertex layout whose attributes do not fit, an
  ellipse of fewer than three segments, or page tabs whose selection is not a
  tab.
- **Exhausted capacities.** Monstro never grows its storage to hide a UI that
  outgrew it; running out stops with what ran out, its capacity and what was
  needed, so you can simplify that UI or size the capacity for it
  deliberately.

Allocation failures are different: `monstro_init` and
`monstro_context_create` return `NULL` when memory runs out, and font
loading returns `NULL` when the data or file cannot be read as a font.

### Capacities

Capacities are fixed and explicit.  What the environment holds is set when it
is created:

```c
MonstroEnvConfig config = monstro_env_config_default();
config.max_vertices = 262144;
config.max_indices = 786432;
MonstroEnv* env = monstro_init(&config);
```

| Environment capacity (`MonstroEnvConfig`) | Default |
| --- | --- |
| `canvas_count`: canvases lent in turn | 1 |
| `max_vertices`: vertices per frame in a canvas's own buffer | 65,536 |
| `max_indices`: indices per frame in a canvas's own buffer | 196,608 |
| `max_draw_commands`: draw commands per frame | 4,096 |
| `max_path_points`: points in one canvas path | 1,024 |
| `atlas_width` x `atlas_height`: the font atlas | 1,024 x 1,024 |
| `max_fonts` | 16 |
| `max_themes` (three are built in) | 16 |
| `max_icons` | 512 |

Host buffers passed to `monstro_begin_frame_into` bring their own capacities.

What a context keeps between frames is set when it is created; the defaults
suit a cockpit screen or a tool window:

```c
MonstroContextConfig config = monstro_context_config_default();
config.max_windows = 64;
MonstroContext* ctx = monstro_context_create(env, &config);
```

| Context capacity (`MonstroContextConfig`) | Default |
| --- | --- |
| `max_windows`: windows, child regions, popups and menus with remembered state, the root included | 32 |
| `max_tree_nodes`: tree nodes and collapsing headers with remembered state | 128 |
| `max_tab_bars` | 16 |
| `max_tables` | 8 |
| `max_popups`: popups open at once | 8 |
| `max_input_events`: input events waiting for a frame | 64 |
| `max_items`: items drawn in a frame where they show, recorded to find the one on top | 512 |

A default context holds about 48 KB; a default environment about 7.4 MB
(a 2.9 MB canvas and a 4 MB atlas).

The remaining limits are nesting depths and per-frame scratch, fixed for now:

| Limit | Value |
| --- | --- |
| Nested IDs (each open window, table, tab bar, tree node and menu bar uses one) | 64 |
| Style pushes, disabled scopes | 64 each |
| Open layouts, across the open windows (each window uses two) | 64 |
| Open groups | 32 |
| Nested windows (windows, children, popups open at once) | 32 |
| Clip rects, across the open windows (each window uses up to three) | 64 |
| Layers, across the open windows (each window uses one; each group adds one) | 32 |
| Nested tab bars | 8 |
| Table columns | 128 |
| Columns in one set, nested column sets | 16, 8 |
| Text typed in one frame (`monstro_add_input_text`) | 255 bytes |
| Tooltip text | 1,023 bytes |
| Items active at once (see [Activated and deactivated](#activated-and-deactivated)) | 4 |

### Text

All text is UTF-8.  Invalid or truncated sequences draw as U+FFFD (when the
font has that glyph), and decoding never reads past the terminator.  Where
Monstro shortens text to fit its own storage, such as window titles (255
bytes) and table column labels (127 bytes), it cuts between code points.
Text fields move the cursor, backspace and delete by whole code points, and a
password field shows one `*` per code point.

Monstro keeps no pointer you pass it beyond the call: tooltip text is copied,
and text fields edit the buffer passed in the current call, so the buffer may
move between frames as long as it holds the same text.

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

The format formats the value shown on the bar: a float slider passes its value
as a `double` (`"%.2f"`), an int slider as an `int` (`"%d"`, the default when the
format is `NULL`). Int sliders snap to whole numbers.

`MONSTRO_SLIDER_VERTICAL` stands a slider up: its minimum is at the bottom and
its maximum at the top, it fills from the bottom, and it is 120 pixels tall
unless given a height (its width is its thickness).  A lever on a cockpit
screen:

```c
monstro_set_cursor_screen_pos(ctx, 456, 196);
monstro_slider_float_ex(ctx, "lift##", &lift, 0.0f, 1.0f, NULL,
                        28.0f, 96.0f, MONSTRO_SLIDER_VERTICAL, NULL);
```

A slider whose label shows nothing (`"lift##"`) is its bar alone: it takes no
space for the label, so it fills the size it is given.

### Knobs

```c
static float gain = 0.5f;
monstro_knob(ctx, "Gain", &gain, 0.0f, 1.0f, "%.2f", NULL);
monstro_knob_sized(ctx, "Rate", &gain, 0.0f, 100.0f, "%.1f", 60.0f, NULL);
```

The label shows to the left of the knob and the value to its right, in room
for the widest of the minimum, maximum and value.  A knob turns by dragging
up and down, from where it was.  An absolute knob
(`MONSTRO_KNOB_ABSOLUTE`, through `monstro_knob_ex`) points where the pointer
is instead: while held, its value is the pointer's angle about its centre,
over the sweep from the minimum at the lower left, clockwise over the top, to
the maximum at the lower right; in the gap at the bottom it is the nearer end.
A press or a tap sets the value where it lands.  So a host whose value lives
elsewhere, such as a simulation, can pass the committed value every frame and
read back the value the pointer means, without keeping a drag of its own:

```c
float heading = ship->heading_bug;   /* the committed value */
if (monstro_knob_ex(ctx, "hdg##", &heading, 0.0f, 360.0f, NULL, 84.0f,
                    MONSTRO_KNOB_ABSOLUTE, NULL))
    send_set_heading(heading);
```

As with sliders, a knob whose label shows nothing is the knob alone.

### Progress bar

```c
monstro_progress_bar(ctx, 0.75f, 120.0f, 20.0f, NULL);      /* shows "75%" */

/* The overlay is plain text, drawn as given; format it yourself. */
char overlay[32];
snprintf(overlay, sizeof(overlay), "%d of %d files", done, total);
monstro_progress_bar(ctx, fraction, 120.0f, 20.0f, overlay);

/* No text at all: an empty overlay. */
monstro_progress_bar(ctx, fraction, 120.0f, 20.0f, "");

/* A gauge standing up, filling from the bottom. */
monstro_progress_bar_ex(ctx, fuel, 22.0f, 98.0f, "", MONSTRO_PROGRESS_VERTICAL);
```

A vertical bar is 120 pixels tall unless given a height.  To tone a gauge by
its value (caution, warning), push a progress style with that fill colour
(`cursor_normal`) for the one bar.


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

Each field keeps its own type's precision: `monstro_input_int` edits whole
numbers and shows them with `"%d"`, and `monstro_input_double` never passes its
value through a `float`. The float and double formats receive a `double`.

### Selectable

```c
static bool selected = false;
monstro_selectable(ctx, "Item", &selected, MONSTRO_SELECTABLE_NONE, NULL);
```

The text sits in the selectable as its style's `text_alignment` says: at the
top left in the default themes.  A lit button, such as a toggle on a cockpit
screen, is a sized selectable lit while its value is on, with its text
centred:

```c
MonstroStyleSelectable lit = *monstro_get_selectable_style(ctx);
lit.text_alignment = MONSTRO_ALIGN_CENTER;
lit.padding[0] = lit.padding[1] = 0.0f;
monstro_push_selectable_style(ctx, &lit);
bool on = ramp_open;                 /* the committed value */
if (monstro_selectable_sized(ctx, "ramp##RAMP", &on, 244.0f, 60.0f, MONSTRO_SELECTABLE_NONE, NULL))
    send_toggle_ramp();
monstro_pop_style(ctx, 1);
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

An image is any texture of the host's, named by its own ID (see [Texture IDs
and the renderer mapping](#texture-ids-and-the-renderer-mapping)): a camera
feed on a cockpit screen is the camera's render target drawn with
`monstro_image` or `monstro_draw_image`.  It becomes a draw command of its own
with that texture ID, in draw order among the rest, so what is drawn after it
(a frame, a reticle) lies over it.

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

Tab bars nest: a tab's content may hold another tab bar, and once the inner
bar ends, tab items go to the outer bar again.

A tab bar keeps its own selection and sizes each tab to its text.  **Page
tabs** are for a selection the host keeps, such as the page of a cockpit
screen, which lives in the simulation: a row of tabs sharing a box evenly,
the selected one lit.  Pass the host's selection every frame; a click on
another tab writes that tab's index and returns `true`, and the host decides
what happens (it may change its page a tick later, and until then the tabs
keep showing the page it passes):

```c
const char* pages[] = {"flt##FLT", "sys##SYS", "cargo##CARGO"};
int shown = ship->page;
monstro_set_cursor_screen_pos(ctx, 0, 448);
if (monstro_page_tabs(ctx, "pages", pages, 3, &shown, 512.0f, 64.0f))
    send_select_page(shown);
```

Each tab's ID is its label's, within the strip's ID, so a live label keeps it.
A width of 0 fills the available width, a height of 0 fits the text, and a
selection that is not a tab's index stops the program.  They use the tab
style.

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

A cell gives the widgets in it the cell's width: a text or number field,
slider or combo fills its cell.  Rows are `style->table.row_height` tall and
do not grow to fit; widgets keep their style's padding in a cell, so for
compact rows push zero padding for the labels, buttons and fields in the
table, as the widget tour (`examples/basic`) does.

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

Modals dim everything drawn below them and block input to other windows.
Press Escape or click the close button (if `p_open` is provided) to dismiss.

`monstro_close_current_popup` closes the popup being built -- the innermost
popup whose `begin` is open, even when you call it from a child region inside
it or after a nested popup has ended -- together with any popups opened from
it.  Calling it outside a popup stops the program.

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
    monstro_input_text(ctx, "n##", name, sizeof(name), 0, NULL);
    monstro_next_column(ctx);
    monstro_button(ctx, "Clear", NULL);
    monstro_next_column(ctx);   /* wraps to column 0 of next row */

    monstro_label(ctx, "Volume:");
    monstro_next_column(ctx);
    monstro_slider_float(ctx, "v##", &vol, 0, 1, "%.2f", NULL);
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
monstro_set_cursor_screen_pos(ctx, 320.0f, 64.0f);    /* at a point on the display */
```

A screen laid out in pixels, such as a cockpit display, places each control
with `monstro_set_cursor_screen_pos` at its box rather than letting the layout
flow.

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

Window names can include `##` to separate the internal window identifier from
the displayed title.  The part before `##` is used to persist window state;
the part after `##` is shown in the title bar.  If `##` is not present, the
same string is used for both.

```c
if (monstro_begin_window(ctx, "main_window##Hello World", NULL,
        MONSTRO_WINDOW_DEFAULT)) {
    /* internal id: main_window, title: Hello World */
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

### Changing the title

Change the title of the current window or a named window without changing its
internal identifier:

```c
monstro_set_window_title(ctx, "Runtime Title");
monstro_set_window_title_named(ctx, "main_window##Hello World", "Renamed");
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

End a child region with `monstro_end_child` and a window with
`monstro_end_window`; mixing them up stops the program.

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
if (monstro_is_item_activated(ctx)) { /* became active this frame: a press, drag or edit began */ }
if (monstro_is_item_deactivated(ctx)) { /* stopped being active this frame */ }
if (monstro_is_item_deactivated_after_edit(ctx)) { /* stopped, and changed its value while active */ }

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

### Themes

A theme is a `MonstroStyle` registered once in the environment.  It is
immutable from then on, and any number of contexts reference it.
`monstro_init` registers three built-in themes, `"default"`, `"dark"` and
`"light"`; a new context uses `"default"`.

```c
MonstroTheme* dark = monstro_theme_find(env, "dark");
monstro_context_set_theme(ctx, dark);        /* between frames */
```

Each frame starts from a working copy of the context's theme.  Changes pushed
during the frame apply to that copy only: never to the theme, and never to
another context sharing it.

A theme whose `font` is `NULL` (all three built-in themes) uses the
environment's default font.

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

Each widget type has a matching `get` and `push` function.  The getters
return the frame's working copy inside a frame and the theme's style between
frames; both are read-only.

```c
const MonstroStyleButton* cur = monstro_get_button_style(ctx);

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

**Style pushes and pops must be balanced within a frame.**  `monstro_end_frame`
stops the program if a push is left open, and popping more than was pushed
stops it at the pop.  Pushes happen inside a frame.

### Creating a custom theme

Build a style, then register it:

```c
MonstroStyle s;
monstro_style_set_dark(&s);  /* start from the dark base */
s.font = monstro_font_find(env, "body", 18.0f);

s.window.background = monstro_color_rgba(0.12f, 0.12f, 0.15f, 1.0f);
s.window.rounding = 6.0f;

s.button.rounding = 4.0f;
s.button.normal.type = MONSTRO_STYLE_ITEM_COLOR;
s.button.normal.data.color = monstro_color_rgba(0.2f, 0.4f, 0.8f, 1.0f);

MonstroTheme* hud = monstro_theme_register(env, "hud", &s);
monstro_context_set_theme(cockpit_left, hud);
monstro_context_set_theme(cockpit_right, hud);
```

Theme names are unique: registering a name twice stops the program.  To vary
a theme, register another one.

### Texture IDs and the renderer mapping

Monstro does not own or manage GPU textures.  A `texture_id` is an opaque
`uint64_t` that flows through unchanged from style items, images and icons
into `MonstroDrawCommand::texture_id`.  **Your renderer** is responsible for
mapping these IDs to real GPU texture handles at draw time.

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

**The atlas** is the one texture Monstro produces pixels for.  Text and solid
shapes use it, with the ID you give it (from `examples/common/example_app.h`):

```c
MonstroTextureData atlas = monstro_env_get_atlas(app->env);
veCreateTexture2D(app->device, atlas.width, atlas.height, VK_FORMAT_R8G8B8A8_UNORM, usage, "EAAtlas",
                  &app->atlasTexture);
/* VulkEase shaders index bindless textures by view, so the ID is the texture's view */
monstro_env_set_atlas_texture(app->env, (uint64_t)veGetDefaultTextureView(app->device, app->atlasTexture));
veHostWriteTextureRegion(app->device, app->atlasTexture, atlas.data, atlas.data_size, nullptr);
```

**Resolving in the draw loop:**

```c
for (uint32_t i = 0; i < draw->command_count; i++) {
    uint64_t tex_id = draw->commands[i].texture_id;

    if (tex_id == atlas_id) {
        bind_texture(atlas_gpu_handle);
    } else {
        bind_texture(lookup_user_texture(tex_id));
    }
    /* ... draw ... */
}
```

Until you set an atlas ID, text and solid shapes carry `texture_id == 0`;
whatever you bind for 0 must then be the atlas.

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

Monstro provides an icon registry in the environment that maps user-chosen
numeric IDs (`MonstroIconId`, a `uint32_t`) to texture sub-regions, shared by
every context.  Icons can be registered from sprite sheet atlases (full color)
or icon font glyph ranges (monochrome, tinted by text color). For icon fonts,
the codepoint *is* the icon ID, so you can reference icons directly from the
font's documentation.  Once registered, icons can be embedded inline in any
widget label using the `{icon:ID}` tag syntax.

The registry is a fixed table of `MonstroEnvConfig.max_icons` slots.
Registering an ID again replaces it; running out of slots stops the program.
Register icons between frames.

**Registering a sprite sheet atlas:**

```c
/* Upload your icon atlas to the GPU and get a texture_id */
uint64_t atlas_id = upload_texture("icons.png", &atlas_w, &atlas_h);

/* Register a grid of icons starting at ID 100: 16x16 pixel icons, 8 columns, 4 rows */
monstro_register_icon_atlas(env, 100, atlas_id,
                            16, 16,        /* icon size */
                            128, 64,       /* atlas size */
                            8, 4);         /* cols, rows */
/* Icons are assigned IDs 100..131 */
```

Icons are laid out left-to-right, top-to-bottom, numbered from the first ID.

**Registering a single icon with explicit UVs:**

```c
MonstroVec2 uv_min = {0.0f, 0.0f};
MonstroVec2 uv_max = {0.5f, 0.5f};
monstro_register_icon(env, 42, my_texture, uv_min, uv_max);
```

**Registering icon font glyphs (IDs are the codepoints):**

```c
/* Register FontAwesome codepoints 0xF000..0xF0FF as monochrome icons.
   Each icon's ID is its codepoint (e.g., 0xF000, 0xF001, ...). */
monstro_register_icon_font(env, fa_font, 0xF000, 256);
```

Icon font glyphs live in the atlas, and monochrome icons are tinted with the
current text color.  For icon fonts, you can use the codepoint directly in
`{icon:ID}` tags, making it easy to reference icons from font documentation.

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
if (monstro_get_icon(env, first, &tex, &uv_min, &uv_max)) {
    /* use tex, uv_min, uv_max with monstro_image or custom drawing */
}
```

### Custom drawing

Monstro exposes the full set of canvas drawing primitives through the C API.
Every function operates on an opaque `MonstroCanvas*` handle: the canvas lent
to the context for this frame.

```c
MonstroCanvas* canvas = monstro_get_window_canvas(ctx);
```

What you draw belongs to the current window: it is clipped and stacked with
that window.  The handle is valid only until `monstro_end_frame`; drawing on a
canvas that is not lent to a frame stops the program, and so does a null font
or text.

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
monstro_draw_ellipse(canvas, center, radii, rotation, color, 32, thickness);
monstro_draw_ellipse_filled(canvas, center, radii, rotation, color, 32);
```

Colors are packed 8-bit RGBA (`0xAABBGGRR`) and reach the vertices unchanged.
Angles are radians, and since +y is down, a positive angle turns clockwise on
screen.  A quad takes its four corners as given, so a rotated rectangle (a
horizon's sky and ground, rolled) is its corners turned.  An ellipse takes its
two radii along its own axes, turned by `rotation`; it needs at least three
segments.

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
monstro_draw_text_rotated(canvas, font, rect, color, "10", MONSTRO_ALIGN_CENTER, pivot, angle);
monstro_draw_image(canvas, texture_id, rect, uv_min, uv_max, 0xFFFFFFFF);
monstro_draw_image_rounded(canvas, texture_id, rect, uv_min, uv_max, 0xFFFFFFFF, 4.0f, MONSTRO_CORNER_ALL);
monstro_draw_nine_patch(canvas, &patch, rect, 0xFFFFFFFF);
```

Turned text is laid out in `rect` as `monstro_draw_text_aligned` lays it out,
then turned with the rect by `angle` about `pivot`: a pitch ladder's numbers,
laid out level beside their rungs, turn with the horizon about the point it
rolls about.

**Path API for complex shapes:**

```c
monstro_path_clear(canvas);
monstro_path_move_to(canvas, start);
monstro_path_line_to(canvas, mid);
monstro_path_arc_to(canvas, center, radius, 0.0f, 3.14f, 12);
monstro_path_bezier_cubic_to(canvas, cp1, cp2, end_pt, 0);
monstro_path_stroke(canvas, color, true, 2.0f);   /* or monstro_path_fill(canvas, color); */
```

A dial's scale is an arc: clear the path, add the arc, stroke it open.  A
band in another colour is a second arc over part of the sweep:

```c
monstro_path_clear(canvas);
monstro_path_arc_to(canvas, centre, radius, 0.75f * PI, 2.25f * PI, 32);
monstro_path_stroke(canvas, NORMAL, false, 3.0f);
```

**Clip rects** restrict drawing to a sub-region:

```c
monstro_push_clip_rect(canvas, region);
/* ... drawing calls are clipped to region ... */
monstro_pop_clip_rect(canvas);
```

A clip rect is intersected with the one around it, and what lies wholly
outside it is not drawn (see [Clipping](#clipping)).

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

**Populate a style and register it as a theme:**

```c
MonstroStyle style;
monstro_style_set_default(&style);
MonstroStyle* s = &style;

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

monstro_context_set_theme(ctx, monstro_theme_register(env, "skin", &style));
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
        monstro_input_text(ctx, "path##", s->current_path, sizeof(s->current_path),
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
        if (monstro_input_text(ctx, "hex##", s->hex, sizeof(s->hex),
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
        monstro_button_sized(ctx, "preview##", 120.0f, 40.0f, NULL);
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
that handles GLFW window creation, Vulkan rendering via VulkEase, the Monstro
environment and context, input forwarding, and the main loop.  To create a new example,
define an `ExampleAppDesc` with init/update callbacks and call
`example_app_run`.  To build and run them, see
[Building Monstro](BUILD.md#examples).

### basic

The kitchen-sink demo (`examples/basic/main.cpp`).  Exercises most of the
widget catalog in a single window: labels, buttons, checkboxes, sliders, knobs,
text input, tables, trees, tabs, color picker, child scroll regions, file
dialog, menus, and more.  Also serves as the reference for GLFW/Vulkan
integration.

### interaction

A page to click through the interaction rules (see
[Interaction rules](#interaction-rules)): overlapping buttons where only the
top one lights up and fires, a scrolled list whose buttons take the mouse only
where they show, a slider and knob that leave other buttons dark while dragged
and log their activated and deactivated edges, labels that show live values
and keep their IDs, two windows each with a popup named "menu", and a button
that counts every click however fast.  It is a page, not a library.

### cockpit

A cockpit screen (`examples/cockpit/main.cpp`): one 512 x 512 multi-function
display, as a game draws an MFD on a render target.  The flight page has an
artificial horizon (sky and ground rolled and pitched in a clip rect, a pitch
ladder whose numbers turn with it), two dials with caution bands, readouts,
vertical rotor bars without text, an absolute heading knob and a vertical
lift lever; the systems and cargo pages have gauges, lit buttons and gear
lamps drawn as ellipses.  The page tabs light the page the example's
simulation keeps.  It is a page, not a library.

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

**Balanced stacks.** Style pushes (`monstro_push_*_style`), ID pushes
(`monstro_push_id`), disabled scopes (`monstro_begin_disabled`), layouts,
groups and clip rects must be balanced with their pops/ends within each frame
(layouts, groups and clip rects within their window).  Monstro checks this at
window end and at the end of the frame and stops the program if anything is
left open; see Errors and Limits.

**One environment, many contexts.** Load fonts and register themes and icons
once, in one environment, and give each UI (each screen, panel or tool window)
its own context.  A context costs about 48 KB with the default capacities, so
contexts are cheap; the canvas and atlas are paid once.

**Render a frame before building the next.** Draw data lives in the canvas
until the canvas is lent again, which with one canvas is the next
`monstro_begin_frame` of any context.  Upload or render it first.

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
