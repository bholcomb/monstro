#ifndef MONSTRO_INTERNAL_CONTEXT_H
#define MONSTRO_INTERNAL_CONTEXT_H

#include "monstro.h"
#include "constants.h"
#include "window.h"
#include "popup.h"
#include "input.h"
#include "canvas.h"
#include "font.h"
#include "helpers.h"

struct MonstroContext {
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

    MonstroWindowInternal* window_stack[MONSTRO_MAX_WINDOWS] = {};
    int window_stack_top = 0;

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
    bool next_window_focus = false;
    MonstroCondition next_window_pos_cond = MONSTRO_COND_NONE;
    MonstroCondition next_window_size_cond = MONSTRO_COND_NONE;
    MonstroCondition next_window_focus_cond = MONSTRO_COND_NONE;

    MonstroFont* default_font = nullptr;

    MonstroContext();
    ~MonstroContext();

    MonstroContext(const MonstroContext&) = delete;
    MonstroContext& operator=(const MonstroContext&) = delete;

    void beginFrame(float deltaTime);
    void endFrame();

    void setDefaultFont(MonstroFont* font);
    MonstroFont* getDefaultFont() const { return style.font; }

    void pushId(uint64_t id);
    void popId();
    uint64_t getId(uint64_t id);

    void setActiveId(uint64_t id);
    void keepAliveId(uint64_t id);

    MonstroWindowInternal* findWindow(const char* name);
    MonstroWindowInternal* createWindow(const char* name, MonstroWindowFlags flags);
    MonstroWindowInternal* findHoveredWindow(float x, float y, bool excludeChildren = false);
    void focusWindow(MonstroWindowInternal* window);
    void pushWindow(MonstroWindowInternal* win);
    void popWindow();

    MonstroLayoutInternal* getCurrentLayout();

private:
    void processMouseInput(float deltaTime);
    void processKeyboardInput(float deltaTime);
};

#endif
