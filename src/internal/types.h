#ifndef MONSTRO_INTERNAL_TYPES_H
#define MONSTRO_INTERNAL_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "monstro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MONSTRO_MAX_ID_STACK_DEPTH 64
#define MONSTRO_MAX_STYLE_STACK_DEPTH 8
#define MONSTRO_MAX_LAYOUT_STACK_DEPTH 32
#define MONSTRO_MAX_WINDOWS 64
#define MONSTRO_MAX_CLIP_RECT_STACK 32
#define MONSTRO_MAX_TEXTURE_STACK 16
#define MONSTRO_MAX_LAYER_STACK 16
#define MONSTRO_MAX_DRAW_COMMANDS 4096
#define MONSTRO_MAX_VERTICES 65536
#define MONSTRO_MAX_INDICES 65536 * 3
#define MONSTRO_MAX_PATH_POINTS 1024
#define MONSTRO_MAX_POPUP_STACK 8
#define MONSTRO_MAX_FONTS 32
#define MONSTRO_MAX_GLYPHS 4096
#define MONSTRO_FONT_ATLAS_DEFAULT_SIZE 1024

/* ------------------------------------------------------------------------- */
/* Internal Layout                                                           */
/* ------------------------------------------------------------------------- */

typedef struct MonstroLayoutInternal {
    MonstroLayoutDirection direction;
    float pos_x, pos_y;
    float size_x, size_y;
    float cursor_x, cursor_y;
    float max_x, max_y;  // Track maximum extent for proper layout ending
} MonstroLayoutInternal;

/* ------------------------------------------------------------------------- */
/* Internal Window                                                           */
/* ------------------------------------------------------------------------- */

typedef struct MonstroWindowInternal {
    char name[256];
    uint64_t id;
    uint64_t move_id;
    MonstroWindowFlags flags;
    
    float pos_x, pos_y;
    float size_x, size_y;
    float scroll_x, scroll_y;
    
    bool active;
    bool collapsed;
    bool skip_items;
    int last_frame_active;
    
    MonstroLayoutInternal layout_stack[MONSTRO_MAX_LAYOUT_STACK_DEPTH];
    int layout_stack_top;
    
    struct MonstroWindowInternal* parent;
    struct MonstroWindowInternal* child;
    struct MonstroWindowInternal* sibling;
    
    MonstroCondition set_pos_allow_flags;
    MonstroCondition set_size_allow_flags;
} MonstroWindowInternal;

/* ------------------------------------------------------------------------- */
/* Internal Popup                                                            */
/* ------------------------------------------------------------------------- */

typedef struct MonstroPopupInternal {
    uint64_t popup_id;
    MonstroWindowInternal* parent_window;
    uint64_t parent_id;
    float open_pos_x, open_pos_y;
} MonstroPopupInternal;

/* ------------------------------------------------------------------------- */
/* Internal Mouse State                                                      */
/* ------------------------------------------------------------------------- */

typedef struct MonstroMouseButtonState {
    bool down;
    bool pressed;
    bool released;
    bool double_clicked;
    float down_time;
    float clicked_time;
    float clicked_pos_x, clicked_pos_y;
} MonstroMouseButtonState;

typedef struct MonstroMouseState {
    float pos_x, pos_y;
    float prev_pos_x, prev_pos_y;
    float delta_x, delta_y;
    float wheel_x, wheel_y;
    MonstroMouseButtonState buttons[MONSTRO_MOUSE_BUTTON_COUNT];
} MonstroMouseState;

/* ------------------------------------------------------------------------- */
/* Internal Keyboard State                                                   */
/* ------------------------------------------------------------------------- */

typedef struct MonstroKeyState {
    bool down;
    bool pressed;
    bool released;
    float down_time;
} MonstroKeyState;

typedef struct MonstroKeyboardState {
    MonstroKeyState keys[MONSTRO_KEY_COUNT];
    MonstroModFlags mods;
    char input_text[256];
    int input_text_len;
} MonstroKeyboardState;

/* ------------------------------------------------------------------------- */
/* Internal Canvas                                                           */
/* ------------------------------------------------------------------------- */

struct MonstroCanvas {
    MonstroVertex* vertices;
    uint32_t* indices;
    uint32_t vertex_count;
    uint32_t index_count;
    size_t vertex_capacity;
    size_t index_capacity;
    bool user_provided_vertex_buffer;
    bool user_provided_index_buffer;
    
    MonstroDrawCommand draw_commands[MONSTRO_MAX_DRAW_COMMANDS];
    uint32_t draw_command_count;
    
    MonstroRect clip_rect_stack[MONSTRO_MAX_CLIP_RECT_STACK];
    int clip_rect_stack_top;
    
    uint64_t texture_stack[MONSTRO_MAX_TEXTURE_STACK];
    int texture_stack_top;
    
    int layer_stack[MONSTRO_MAX_LAYER_STACK];
    int layer_stack_top;
    
    MonstroVec2 path[MONSTRO_MAX_PATH_POINTS];
    int path_count;
    
    bool dirty;
    uint64_t default_texture_id;
};

/* ------------------------------------------------------------------------- */
/* Internal Context                                                          */
/* ------------------------------------------------------------------------- */

struct MonstroContext {
    MonstroCanvas* canvas;
    MonstroFontManager* font_manager;
    
    uint64_t id_stack[MONSTRO_MAX_ID_STACK_DEPTH];
    int id_stack_top;
    
    MonstroStyle style;
    MonstroStyle style_stack[MONSTRO_MAX_STYLE_STACK_DEPTH];
    int style_stack_top;
    
    MonstroWindowInternal windows[MONSTRO_MAX_WINDOWS];
    int window_count;
    MonstroWindowInternal* root_window;
    MonstroWindowInternal* current_window;
    MonstroWindowInternal* hovered_window;
    MonstroWindowInternal* focused_window;
    
    MonstroPopupInternal popup_stack[MONSTRO_MAX_POPUP_STACK];
    int popup_stack_top;
    
    uint64_t hovered_id;
    uint64_t hovered_id_prev_frame;
    uint64_t active_id;
    uint64_t active_id_prev_frame;
    bool active_id_is_alive;
    
    MonstroMouseState mouse;
    MonstroKeyboardState keyboard;
    MonstroMouseCursor mouse_cursor;
    
    float double_click_time;
    float double_click_distance;
    float mouse_drag_threshold;
    float key_repeat_delay;
    float key_repeat_rate;
    
    bool want_capture_mouse;
    bool want_capture_keyboard;
    bool want_text_input;
    
    float display_width;
    float display_height;
    float delta_time;
    float time;
    int frame;
    
    float next_window_pos_x, next_window_pos_y;
    float next_window_size_x, next_window_size_y;
    bool next_window_collapsed;
    bool next_window_focus;
    MonstroCondition next_window_pos_cond;
    MonstroCondition next_window_size_cond;
    MonstroCondition next_window_collapsed_cond;
    
    MonstroFont* default_font;
};

/* ------------------------------------------------------------------------- */
/* Internal Font                                                             */
/* ------------------------------------------------------------------------- */

struct MonstroFont {
    char name[64];
    float size;
    float line_height;
    float ascent;
    float descent;
    
    MonstroGlyph* glyphs;
    uint32_t glyph_count;
    uint32_t first_code_point;
    
    uint64_t atlas_texture_id;
    uint32_t atlas_width;
    uint32_t atlas_height;
    
    uint8_t* atlas_data;
    size_t atlas_data_size;
    MonstroTextureFormat atlas_format;
};

struct MonstroFontManager {
    MonstroFont* fonts[MONSTRO_MAX_FONTS];
    int font_count;
};

/* ------------------------------------------------------------------------- */
/* Internal Helpers                                                          */
/* ------------------------------------------------------------------------- */

uint64_t monstro_hash_combine(uint64_t seed, uint64_t value);
uint64_t monstro_hash_data(const void* data, size_t size, uint64_t seed);

MonstroWindowInternal* monstro_find_window(MonstroContext* ctx, const char* name);
MonstroWindowInternal* monstro_create_window(MonstroContext* ctx, const char* name, MonstroWindowFlags flags);
MonstroWindowInternal* monstro_find_hovered_window(MonstroContext* ctx, float x, float y);

void monstro_set_active_id(MonstroContext* ctx, uint64_t id);
void monstro_keep_alive_id(MonstroContext* ctx, uint64_t id);
void monstro_focus_window(MonstroContext* ctx, MonstroWindowInternal* window);

void monstro_add_item(MonstroContext* ctx, float width, float height);

#ifdef __cplusplus
}
#endif

#endif
