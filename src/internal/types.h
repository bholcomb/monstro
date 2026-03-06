#ifndef MONSTRO_INTERNAL_TYPES_H
#define MONSTRO_INTERNAL_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <cstring>

#include "monstro.h"

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

struct MonstroLayoutInternal {
    MonstroLayoutDirection direction;
    float pos_x, pos_y;
    float size_x, size_y;
    float cursor_x, cursor_y;
    float max_x, max_y;
};

/* ------------------------------------------------------------------------- */
/* Internal Window                                                           */
/* ------------------------------------------------------------------------- */

struct MonstroWindowInternal {
    // Data members
    char name[256] = {};
    uint64_t id = 0;
    uint64_t move_id = 0;
    MonstroWindowFlags flags = MONSTRO_WINDOW_NONE;
    
    float pos_x = 0.0f, pos_y = 0.0f;
    float size_x = 0.0f, size_y = 0.0f;
    float scroll_x = 0.0f, scroll_y = 0.0f;
    
    bool active = false;
    bool collapsed = false;
    bool skip_items = false;
    int last_frame_active = 0;
    
    MonstroLayoutInternal layout_stack[MONSTRO_MAX_LAYOUT_STACK_DEPTH] = {};
    int layout_stack_top = 0;
    
    MonstroWindowInternal* parent = nullptr;
    MonstroWindowInternal* child = nullptr;
    MonstroWindowInternal* sibling = nullptr;
    
    MonstroCondition set_pos_allow_flags = MONSTRO_COND_NONE;
    MonstroCondition set_size_allow_flags = MONSTRO_COND_NONE;
    
    // Initialize window with name and flags
    void init(const char* windowName, MonstroWindowFlags windowFlags);
    
    // Geometry helpers
    bool containsPoint(float x, float y) const;
    MonstroRect getRect() const;
    MonstroVec2 getPos() const { return {pos_x, pos_y}; }
    MonstroVec2 getSize() const { return {size_x, size_y}; }
    
    // Layout management
    void initLayout(float contentY, float paddingX, float paddingY);
    MonstroLayoutInternal* getCurrentLayout();
    
    // State management
    void setPos(float x, float y) { pos_x = x; pos_y = y; }
    void setSize(float w, float h) { size_x = w; size_y = h; }
    void move(float dx, float dy) { pos_x += dx; pos_y += dy; }
};

/* ------------------------------------------------------------------------- */
/* Internal Popup                                                            */
/* ------------------------------------------------------------------------- */

struct MonstroPopupInternal {
    uint64_t popup_id;
    MonstroWindowInternal* parent_window;
    uint64_t parent_id;
    float open_pos_x, open_pos_y;
};

/* ------------------------------------------------------------------------- */
/* Internal Mouse State                                                      */
/* ------------------------------------------------------------------------- */

struct MonstroMouseButtonState {
    bool down = false;
    bool pressed = false;
    bool released = false;
    bool double_clicked = false;
    float down_time = -1.0f;
    float clicked_time = -1.0f;
    float clicked_pos_x = 0.0f;
    float clicked_pos_y = 0.0f;
};

struct MonstroMouseState {
    float pos_x = 0.0f, pos_y = 0.0f;
    float prev_pos_x = 0.0f, prev_pos_y = 0.0f;
    float delta_x = 0.0f, delta_y = 0.0f;
    float wheel_x = 0.0f, wheel_y = 0.0f;
    MonstroMouseButtonState buttons[MONSTRO_MOUSE_BUTTON_COUNT];
};

/* ------------------------------------------------------------------------- */
/* Internal Keyboard State                                                   */
/* ------------------------------------------------------------------------- */

struct MonstroKeyState {
    bool down = false;
    bool pressed = false;
    bool released = false;
    float down_time = -1.0f;
};

struct MonstroKeyboardState {
    MonstroKeyState keys[MONSTRO_KEY_COUNT];
    MonstroModFlags mods = MONSTRO_MOD_NONE;
    char input_text[256] = {};
    int input_text_len = 0;
};

/* ------------------------------------------------------------------------- */
/* Internal Canvas                                                           */
/* ------------------------------------------------------------------------- */

struct MonstroCanvas {
    MonstroVertex* vertices = nullptr;
    uint32_t* indices = nullptr;
    uint32_t vertex_count = 0;
    uint32_t index_count = 0;
    size_t vertex_capacity = 0;
    size_t index_capacity = 0;
    bool user_provided_vertex_buffer = false;
    bool user_provided_index_buffer = false;
    
    MonstroDrawCommand draw_commands[MONSTRO_MAX_DRAW_COMMANDS];
    uint32_t draw_command_count = 0;
    
    MonstroRect clip_rect_stack[MONSTRO_MAX_CLIP_RECT_STACK];
    int clip_rect_stack_top = -1;
    
    uint64_t texture_stack[MONSTRO_MAX_TEXTURE_STACK];
    int texture_stack_top = -1;
    
    int layer_stack[MONSTRO_MAX_LAYER_STACK];
    int layer_stack_top = -1;
    
    MonstroVec2 path[MONSTRO_MAX_PATH_POINTS];
    int path_count = 0;
    
    bool dirty = false;
    uint64_t default_texture_id = 0;
};

/* ------------------------------------------------------------------------- */
/* Internal Context                                                          */
/* ------------------------------------------------------------------------- */

struct MonstroContext {
    // Data members
    MonstroCanvas* canvas = nullptr;
    MonstroFontManager* font_manager = nullptr;
    
    uint64_t id_stack[MONSTRO_MAX_ID_STACK_DEPTH] = {};
    int id_stack_top = 0;
    
    MonstroStyle style = {};
    MonstroStyle style_stack[MONSTRO_MAX_STYLE_STACK_DEPTH] = {};
    int style_stack_top = 0;
    
    MonstroWindowInternal windows[MONSTRO_MAX_WINDOWS] = {};
    int window_count = 0;
    MonstroWindowInternal* root_window = nullptr;
    MonstroWindowInternal* current_window = nullptr;
    MonstroWindowInternal* hovered_window = nullptr;
    MonstroWindowInternal* focused_window = nullptr;
    
    MonstroPopupInternal popup_stack[MONSTRO_MAX_POPUP_STACK] = {};
    int popup_stack_top = 0;
    
    uint64_t hovered_id = 0;
    uint64_t hovered_id_prev_frame = 0;
    uint64_t active_id = 0;
    uint64_t active_id_prev_frame = 0;
    bool active_id_is_alive = false;
    
    MonstroMouseState mouse;
    MonstroKeyboardState keyboard;
    MonstroMouseCursor mouse_cursor = MONSTRO_MOUSE_CURSOR_ARROW;
    
    float double_click_time = 0.30f;
    float double_click_distance = 6.0f;
    float mouse_drag_threshold = 6.0f;
    float key_repeat_delay = 0.25f;
    float key_repeat_rate = 0.05f;
    
    bool want_capture_mouse = false;
    bool want_capture_keyboard = false;
    bool want_text_input = false;
    
    float display_width = 0.0f;
    float display_height = 0.0f;
    float delta_time = 0.0f;
    float time = 0.0f;
    int frame = 0;
    
    float next_window_pos_x = 0.0f, next_window_pos_y = 0.0f;
    float next_window_size_x = 0.0f, next_window_size_y = 0.0f;
    bool next_window_collapsed = false;
    bool next_window_focus = false;
    MonstroCondition next_window_pos_cond = MONSTRO_COND_NONE;
    MonstroCondition next_window_size_cond = MONSTRO_COND_NONE;
    MonstroCondition next_window_collapsed_cond = MONSTRO_COND_NONE;
    
    MonstroFont* default_font = nullptr;
    
    // Constructor / Destructor
    MonstroContext();
    ~MonstroContext();
    
    // Prevent copying
    MonstroContext(const MonstroContext&) = delete;
    MonstroContext& operator=(const MonstroContext&) = delete;
    
    // Frame management
    void beginFrame(float displayWidth, float displayHeight, float deltaTime);
    void endFrame();
    
    // Font management
    void setDefaultFont(MonstroFont* font);
    MonstroFont* getDefaultFont() const { return style.font; }
    
    // ID stack management
    void pushId(uint64_t id);
    void popId();
    uint64_t getId(uint64_t id);
    
    // Active/Hovered ID management
    void setActiveId(uint64_t id);
    void keepAliveId(uint64_t id);
    
    // Window management
    MonstroWindowInternal* findWindow(const char* name);
    MonstroWindowInternal* createWindow(const char* name, MonstroWindowFlags flags);
    MonstroWindowInternal* findHoveredWindow(float x, float y);
    void focusWindow(MonstroWindowInternal* window) { focused_window = window; }
    
    // Layout management
    MonstroLayoutInternal* getCurrentLayout();
    
private:
    void processMouseInput(float deltaTime);
    void processKeyboardInput(float deltaTime);
};

/* ------------------------------------------------------------------------- */
/* Internal Font                                                             */
/* ------------------------------------------------------------------------- */

struct MonstroFont {
    char name[64] = {};
    float size = 0.0f;
    float line_height = 0.0f;
    float ascent = 0.0f;
    float descent = 0.0f;
    
    MonstroGlyph* glyphs = nullptr;
    uint32_t glyph_count = 0;
    uint32_t first_code_point = 0;
    
    uint64_t atlas_texture_id = 0;
    uint32_t atlas_width = 0;
    uint32_t atlas_height = 0;
    
    uint8_t* atlas_data = nullptr;
    size_t atlas_data_size = 0;
    MonstroTextureFormat atlas_format = MONSTRO_TEXTURE_FORMAT_R8;
};

struct MonstroFontManager {
    MonstroFont* fonts[MONSTRO_MAX_FONTS] = {};
    int font_count = 0;
};

/* ------------------------------------------------------------------------- */
/* Internal Helpers                                                          */
/* ------------------------------------------------------------------------- */

// Hash functions (defined in context.cpp)
uint64_t monstro_hash_combine(uint64_t seed, uint64_t value);
uint64_t monstro_hash_data(const void* data, size_t size, uint64_t seed);

// Layout (defined in layout.cpp)
void monstro_add_item(MonstroContext* ctx, float width, float height);

#endif // MONSTRO_INTERNAL_TYPES_H
