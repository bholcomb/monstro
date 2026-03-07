#include "context.h"
#include <new>
#include <stdexcept>
#include <cmath>
#include <cstring>

/* ------------------------------------------------------------------------- */
/* MonstroContext Implementation                                             */
/* ------------------------------------------------------------------------- */

MonstroContext::MonstroContext() {
    canvas = monstro_canvas_create();
    font_manager = monstro_font_manager_create();
    
    monstro_style_set_default(&style);
    
    id_stack[0] = 0;
}

MonstroContext::~MonstroContext() {
    if (font_manager) {
        monstro_font_manager_destroy(font_manager);
    }
    if (canvas) {
        monstro_canvas_destroy(canvas);
    }
}

void MonstroContext::beginFrame(float dt) {
    delta_time = dt;
    time += dt;
    frame++;
    
    hovered_id_prev_frame = hovered_id;
    hovered_id = 0;
    
    // Clear active_id if it wasn't kept alive in the previous frame
    if (!active_id_is_alive && active_id_prev_frame == active_id && active_id != 0) {
        setActiveId(0);
    }
    active_id_prev_frame = active_id;
    active_id_is_alive = false;
    
    processMouseInput(dt);
    processKeyboardInput(dt);
    
    // Find hovered window using previous frame's window data
    hovered_window = findHoveredWindow(mouse.pos_x, mouse.pos_y, false);
    
    // Reset all windows to inactive for this frame
    for (int i = 0; i < window_count; i++) {
        windows[i].active = false;
    }
    
    monstro_canvas_clear(canvas);
    
    next_window_pos_cond = MONSTRO_COND_NONE;
    next_window_size_cond = MONSTRO_COND_NONE;
    next_window_focus_cond = MONSTRO_COND_NONE;
    next_window_pos_x = -INFINITY;
    next_window_pos_y = -INFINITY;
    next_window_size_x = -INFINITY;
    next_window_size_y = -INFINITY;
    
    want_capture_mouse = false;
    want_capture_keyboard = false;
    want_text_input = false;
    
    if (window_stack_top != 0) {
        throw std::runtime_error("Unbalanced window stack: missing monstro_end_window() calls");
    }
    if (id_stack_top != 0) {
        throw std::runtime_error("Unbalanced ID stack: missing monstro_pop_id() calls");
    }

    // Begin root window (matching C# beginFrame)
    next_window_size_x = display_width;
    next_window_size_y = display_height;
    next_window_size_cond = MONSTRO_COND_ALWAYS;
    next_window_pos_x = 0.0f;
    next_window_pos_y = 0.0f;
    next_window_pos_cond = MONSTRO_COND_ALWAYS;
    MonstroWindowFlags root_flags = (MonstroWindowFlags)(MONSTRO_WINDOW_ROOT | MONSTRO_WINDOW_INPUTS);
    monstro_begin_window(this, "root", nullptr, root_flags);
    root_window = findWindow("root");
}

void MonstroContext::endFrame() {
    // End root window (matching C# endFrame)
    monstro_end_window(this);

    // Focus on click (matching C# endFrame)
    if (hovered_window != nullptr &&
        active_id == 0 &&
        (mouse.buttons[MONSTRO_MOUSE_BUTTON_LEFT].pressed ||
         mouse.buttons[MONSTRO_MOUSE_BUTTON_RIGHT].pressed ||
         mouse.buttons[MONSTRO_MOUSE_BUTTON_MIDDLE].pressed)) {
        focusWindow(hovered_window);
    }

    // Window move initiation on middle-click over title bar (matching C# endFrame)
    if (hovered_window != nullptr &&
        active_id == 0 &&
        hovered_id == 0 &&
        mouse.buttons[MONSTRO_MOUSE_BUTTON_MIDDLE].pressed) {
        MonstroRect title_rect = hovered_window->getTitleBarRect(&style);
        if (mouse.pos_x >= title_rect.x && mouse.pos_x < title_rect.x + title_rect.w &&
            mouse.pos_y >= title_rect.y && mouse.pos_y < title_rect.y + title_rect.h) {
            setActiveId(hovered_window->move_id);
        }
    }

    mouse.wheel_x = 0.0f;
    mouse.wheel_y = 0.0f;
    keyboard.input_text_len = 0;
    keyboard.input_text[0] = '\0';
}

void MonstroContext::processMouseInput(float dt) {
    for (int i = 0; i < MONSTRO_MOUSE_BUTTON_COUNT; i++) {
        MonstroMouseButtonState* btn = &mouse.buttons[i];
        
        // C# pattern: down_time < 0 means "not pressed", >= 0 means "pressed"
        btn->pressed = btn->down && (btn->down_time < 0.0f);
        btn->released = !btn->down && (btn->down_time >= 0.0f);
        btn->double_clicked = false;
        
        if (btn->pressed) {
            float dx = mouse.pos_x - btn->clicked_pos_x;
            float dy = mouse.pos_y - btn->clicked_pos_y;
            float dist_sq = dx * dx + dy * dy;
            float time_since_click = time - btn->clicked_time;
            
            if (time_since_click < double_click_time &&
                dist_sq < double_click_distance * double_click_distance) {
                btn->double_clicked = true;
                btn->clicked_time = -1.0f;
            } else {
                btn->clicked_time = time;
            }
            
            btn->clicked_pos_x = mouse.pos_x;
            btn->clicked_pos_y = mouse.pos_y;
        }
        
        if (btn->down) {
            btn->down_time = (btn->down_time < 0.0f) ? 0.0f : btn->down_time + dt;
        } else {
            btn->down_time = -1.0f;
        }
    }

    mouse.delta_x = mouse.pos_x - mouse.prev_pos_x;
    mouse.delta_y = mouse.pos_y - mouse.prev_pos_y;
    mouse.prev_pos_x = mouse.pos_x;
    mouse.prev_pos_y = mouse.pos_y;
}

void MonstroContext::processKeyboardInput(float dt) {
    for (int i = 0; i < MONSTRO_KEY_COUNT; i++) {
        MonstroKeyState* key = &keyboard.keys[i];
        
        key->pressed = key->down && (key->down_time < 0.0f);
        key->released = !key->down && (key->down_time >= 0.0f);
        
        if (key->down) {
            key->down_time = (key->down_time < 0.0f) ? 0.0f : key->down_time + dt;
        } else {
            key->down_time = -1.0f;
        }
    }
}

void MonstroContext::setDefaultFont(MonstroFont* font) {
    default_font = font;
    style.font = font;
}

void MonstroContext::pushId(uint64_t id) {
    if (id_stack_top >= MONSTRO_MAX_ID_STACK_DEPTH - 1) return;
    id_stack_top++;
    id_stack[id_stack_top] = id;
}

void MonstroContext::popId() {
    if (id_stack_top <= 0) return;
    id_stack_top--;
}

uint64_t MonstroContext::getId(uint64_t id) {
    uint64_t combined_id = monstro_hash_combine(id_stack[id_stack_top], id);
    keepAliveId(combined_id);
    return combined_id;
}

void MonstroContext::setActiveId(uint64_t id) {
    active_id = id;
}

void MonstroContext::keepAliveId(uint64_t id) {
    if (active_id == id) {
        active_id_is_alive = true;
    }
}

MonstroLayoutInternal* MonstroContext::getCurrentLayout() {
    if (!current_window) return nullptr;
    MonstroWindowInternal* win = current_window;
    if (win->layout_stack_top < 0) return nullptr;
    return &win->layout_stack[win->layout_stack_top];
}

/* ------------------------------------------------------------------------- */
/* C API Implementation                                                      */
/* ------------------------------------------------------------------------- */

extern "C" {

MonstroContext* monstro_context_create(void) {
    MonstroContext* ctx = new (std::nothrow) MonstroContext();
    if (!ctx) return nullptr;
    
    // Verify initialization succeeded
    if (!ctx->canvas || !ctx->font_manager) {
        delete ctx;
        return nullptr;
    }
    
    return ctx;
}

void monstro_context_destroy(MonstroContext* ctx) {
    delete ctx;
}

MonstroCanvas* monstro_context_get_canvas(MonstroContext* ctx) {
    return ctx ? ctx->canvas : nullptr;
}

void monstro_context_set_default_font(MonstroContext* ctx, MonstroFont* font) {
    if (ctx) ctx->setDefaultFont(font);
}

MonstroFont* monstro_context_get_default_font(MonstroContext* ctx) {
    return ctx ? ctx->getDefaultFont() : nullptr;
}

void monstro_set_display_size(MonstroContext* ctx, float width, float height) {
    if (!ctx) return;
    ctx->display_width = width;
    ctx->display_height = height;
}

void monstro_begin_frame(MonstroContext* ctx, float delta_time) {
    if (ctx) ctx->beginFrame(delta_time);
}

void monstro_end_frame(MonstroContext* ctx) {
    if (ctx) ctx->endFrame();
}

float monstro_get_delta_time(MonstroContext* ctx) {
    return ctx ? ctx->delta_time : 0.0f;
}

float monstro_get_time(MonstroContext* ctx) {
    return ctx ? ctx->time : 0.0f;
}

MonstroCanvas* monstro_get_canvas(MonstroContext* ctx) {
    return ctx ? ctx->canvas : nullptr;
}

MonstroStyle* monstro_get_style(MonstroContext* ctx) {
    return ctx ? &ctx->style : nullptr;
}

} // extern "C"

/* ------------------------------------------------------------------------- */
/* Internal Hash Helpers                                                     */
/* ------------------------------------------------------------------------- */

constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME = 1099511628211ULL;

uint64_t monstro_hash_data(const void* data, size_t size, uint64_t seed) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t hash = seed;
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

uint64_t monstro_hash_combine(uint64_t seed, uint64_t value) {
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

/* ------------------------------------------------------------------------- */
/* ID Stack C API                                                            */
/* ------------------------------------------------------------------------- */

extern "C" {

uint64_t monstro_hash_string(const char* str) {
    if (!str) return 0;
    return monstro_hash_data(str, std::strlen(str), FNV_OFFSET_BASIS);
}

void monstro_push_id(MonstroContext* ctx, uint64_t id) {
    if (ctx) ctx->pushId(id);
}

void monstro_pop_id(MonstroContext* ctx) {
    if (ctx) ctx->popId();
}

uint64_t monstro_get_id(MonstroContext* ctx, uint64_t id) {
    return ctx ? ctx->getId(id) : 0;
}

} // extern "C"
