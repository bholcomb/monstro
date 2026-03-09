#ifndef MONSTRO_INTERNAL_CONTEXT_H
#define MONSTRO_INTERNAL_CONTEXT_H

#include "monstro.h"
#include "constants.h"
#include "window.h"
#include "popup.h"
#include "input.h"
#include "drawbuffer.h"
#include "font.h"
#include "helpers.h"
#include "idStack.h"
#include "tooltip.h"
#include "editstate.h"

struct MonstroTreeNodeState {
    uint64_t id = 0;
    bool open = false;
    int last_frame_active = 0;
};

struct MonstroTabBarState {
    uint64_t id;
    uint64_t selected_tab;
    int last_frame_active;
    float header_start_x;
    float header_start_y;
    float header_height;
    bool content_active;
};

namespace monstro {

struct Context {
    
    
    Context();
    ~Context();

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    void beginFrame(float deltaTime);
    void endFrame();

    void setDefaultFont(MonstroFont* font);
    MonstroFont* getDefaultFont() const { return style.font; }

    void setActiveId(uint64_t id);
    void keepAliveId(uint64_t id);

    Window* findWindow(const char* name);
    Window* createWindow(const char* name, MonstroWindowFlags flags);
    Window* findHoveredWindow(float x, float y, bool excludeChildren = false);
    void focusWindow(Window* window);
    void pushWindow(Window* win);
    void popWindow();

    Layout* getCurrentLayout();



    
    DrawBuffer draw_buffer;

    MonstroDrawCommand final_commands[MONSTRO_MAX_DRAW_COMMANDS];
    uint32_t final_command_count = 0;

    MonstroFontManager* font_manager = nullptr;

    IdStack id_stack;

    MonstroStyle style = {};
    MonstroStyle style_stack[MONSTRO_MAX_STYLE_STACK_DEPTH] = {};
    int style_stack_top = 0;

    Window windows[MONSTRO_MAX_WINDOWS] = {};
    int window_count = 0;
    Window* root_window = nullptr;
    Window* current_window = nullptr;
    Window* hovered_window = nullptr;
    Window* focused_window = nullptr;

    Window* window_stack[MONSTRO_MAX_WINDOWS] = {};
    int window_stack_top = 0;

    MonstroPopupInternal popup_stack[MONSTRO_MAX_POPUP_STACK] = {};
    int popup_stack_top = 0;

    MonstroTabBarState tab_bars[MONSTRO_MAX_TAB_BARS] = {};
    int tab_bar_count = 0;
    MonstroTabBarState* current_tab_bar = nullptr;

    MonstroTreeNodeState tree_states[MONSTRO_MAX_TREE_STATES] = {};
    int tree_state_count = 0;
    MonstroTreeNodeState* findOrCreateTreeState(uint64_t id, bool default_open);

    uint64_t hovered_id = 0;
    uint64_t hovered_id_prev_frame = 0;
    uint64_t active_id = 0;
    uint64_t active_id_prev_frame = 0;
    bool active_id_is_alive = false;

    MouseState mouse;
    KeyboardState keyboard;
    MonstroMouseCursor mouse_cursor = MONSTRO_MOUSE_CURSOR_ARROW;

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
    bool next_window_focus = false;
    MonstroCondition next_window_pos_cond = MONSTRO_COND_NONE;
    MonstroCondition next_window_size_cond = MONSTRO_COND_NONE;
    MonstroCondition next_window_focus_cond = MONSTRO_COND_NONE;

    MonstroFont* default_font = nullptr;

    TooltipState tooltip;
    EditState edit_state;
};

} // namespace monstro

typedef monstro::Context MonstroContext;

#endif
