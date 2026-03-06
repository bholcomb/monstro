#include "internal/types.h"
#include <string.h>
#include <math.h>

MonstroWindowInternal* monstro_find_window(MonstroContext* ctx, const char* name) {
    if (!ctx || !name) return nullptr;
    
    uint64_t id = monstro_hash_string(name);
    for (int i = 0; i < ctx->window_count; i++) {
        if (ctx->windows[i].id == id) {
            return &ctx->windows[i];
        }
    }
    return nullptr;
}

void monstro_focus_window(MonstroContext* ctx, MonstroWindowInternal* window) {
    if (!ctx || !window) return;
    ctx->focused_window = window;
}

MonstroWindowInternal* monstro_find_hovered_window(MonstroContext* ctx, float x, float y) {
    if (!ctx) return nullptr;
    
    // Iterate windows in reverse order (later windows are on top)
    // Return the topmost window that contains the point
    for (int i = ctx->window_count - 1; i >= 0; i--) {
        MonstroWindowInternal* win = &ctx->windows[i];
        
        // Skip inactive windows
        if (!win->active) continue;
        
        // Check if point is inside window bounds
        if (x >= win->pos_x && x < win->pos_x + win->size_x &&
            y >= win->pos_y && y < win->pos_y + win->size_y) {
            return win;
        }
    }
    return nullptr;
}

MonstroWindowInternal* monstro_create_window(MonstroContext* ctx, const char* name, MonstroWindowFlags flags) {
    if (!ctx || !name || ctx->window_count >= MONSTRO_MAX_WINDOWS) return nullptr;
    
    MonstroWindowInternal* win = &ctx->windows[ctx->window_count++];
    memset(win, 0, sizeof(MonstroWindowInternal));
    
    strncpy(win->name, name, sizeof(win->name) - 1);
    win->id = monstro_hash_string(name);
    win->move_id = monstro_hash_combine(win->id, monstro_hash_string("##MOVE"));
    win->flags = flags;
    win->active = true;
    win->collapsed = false;
    win->set_pos_allow_flags = (MonstroCondition)(MONSTRO_COND_ALWAYS | MONSTRO_COND_ONCE | MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING);
    win->set_size_allow_flags = (MonstroCondition)(MONSTRO_COND_ALWAYS | MONSTRO_COND_ONCE | MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING);
    
    win->pos_x = 100.0f;
    win->pos_y = 100.0f;
    win->size_x = 300.0f;
    win->size_y = 200.0f;
    
    return win;
}

static bool should_apply_condition(MonstroCondition cond, MonstroCondition allowed, bool appearing, bool first_use) {
    if (cond == MONSTRO_COND_NONE) return false;
    if ((cond & MONSTRO_COND_ALWAYS) && (allowed & MONSTRO_COND_ALWAYS)) return true;
    if ((cond & MONSTRO_COND_ONCE) && (allowed & MONSTRO_COND_ONCE)) return true;
    if ((cond & MONSTRO_COND_FIRST_USE_EVER) && first_use && (allowed & MONSTRO_COND_FIRST_USE_EVER)) return true;
    if ((cond & MONSTRO_COND_APPEARING) && appearing && (allowed & MONSTRO_COND_APPEARING)) return true;
    return false;
}

void monstro_set_next_window_pos(MonstroContext* ctx, float x, float y, MonstroCondition cond) {
    if (!ctx) return;
    ctx->next_window_pos_x = x;
    ctx->next_window_pos_y = y;
    ctx->next_window_pos_cond = cond;
}

void monstro_set_next_window_size(MonstroContext* ctx, float width, float height, MonstroCondition cond) {
    if (!ctx) return;
    ctx->next_window_size_x = width;
    ctx->next_window_size_y = height;
    ctx->next_window_size_cond = cond;
}

void monstro_set_next_window_collapsed(MonstroContext* ctx, bool collapsed, MonstroCondition cond) {
    if (!ctx) return;
    ctx->next_window_collapsed = collapsed;
    ctx->next_window_collapsed_cond = cond;
}

void monstro_set_next_window_focus(MonstroContext* ctx) {
    if (!ctx) return;
    ctx->next_window_focus = true;
}

bool monstro_begin_window(MonstroContext* ctx, const char* name, bool* p_open, MonstroWindowFlags flags) {
    if (!ctx || !name) return false;
    
    MonstroWindowInternal* win = monstro_find_window(ctx, name);
    bool first_use = (win == nullptr);
    bool appearing = first_use;
    
    if (!win) {
        win = monstro_create_window(ctx, name, flags);
        if (!win) return false;
    } else {
        appearing = (win->last_frame_active != ctx->frame - 1);
    }
    
    win->flags = flags;
    win->active = true;
    win->last_frame_active = ctx->frame;
    win->skip_items = false;
    
    if (should_apply_condition(ctx->next_window_pos_cond, win->set_pos_allow_flags, appearing, first_use)) {
        win->pos_x = ctx->next_window_pos_x;
        win->pos_y = ctx->next_window_pos_y;
    }
    
    if (should_apply_condition(ctx->next_window_size_cond, win->set_size_allow_flags, appearing, first_use)) {
        win->size_x = ctx->next_window_size_x;
        win->size_y = ctx->next_window_size_y;
    }
    
    if (ctx->next_window_collapsed_cond != MONSTRO_COND_NONE) {
        win->collapsed = ctx->next_window_collapsed;
    }
    
    ctx->next_window_pos_cond = MONSTRO_COND_NONE;
    ctx->next_window_size_cond = MONSTRO_COND_NONE;
    ctx->next_window_collapsed_cond = MONSTRO_COND_NONE;
    
    if (win->set_pos_allow_flags & MONSTRO_COND_ONCE) {
        win->set_pos_allow_flags = (MonstroCondition)(win->set_pos_allow_flags & ~MONSTRO_COND_ONCE);
    }
    if (win->set_size_allow_flags & MONSTRO_COND_ONCE) {
        win->set_size_allow_flags = (MonstroCondition)(win->set_size_allow_flags & ~MONSTRO_COND_ONCE);
    }
    
    if (ctx->next_window_focus) {
        ctx->focused_window = win;
        ctx->next_window_focus = false;
    }
    
    ctx->current_window = win;
    monstro_push_id(ctx, win->id);
    
    if (win->collapsed) {
        win->skip_items = true;
    }
    
    MonstroCanvas* canvas = ctx->canvas;
    MonstroStyle* style = &ctx->style;
    
    if (flags & MONSTRO_WINDOW_BACKGROUND) {
        uint32_t bg_color = monstro_color_to_u32(style->window.background);
        MonstroRect win_rect = {win->pos_x, win->pos_y, win->size_x, win->size_y};
        monstro_canvas_add_rect_filled(canvas, win_rect, bg_color, style->window.rounding, MONSTRO_CORNER_ALL);
    }
    
    if (flags & MONSTRO_WINDOW_BORDERS) {
        uint32_t border_color = monstro_color_to_u32(style->window.border_color);
        MonstroRect win_rect = {win->pos_x, win->pos_y, win->size_x, win->size_y};
        monstro_canvas_add_rect(canvas, win_rect, border_color, style->window.rounding, MONSTRO_CORNER_ALL, style->window.border);
    }
    
    float content_y = win->pos_y;
    
    if ((flags & MONSTRO_WINDOW_TITLE_BAR) && !win->collapsed) {
        float header_height = 28.0f;
        MonstroStyleItem* header_style;
        
        if (ctx->focused_window == win) {
            header_style = &style->window.header.active;
        } else if (ctx->hovered_window == win) {
            header_style = &style->window.header.hover;
        } else {
            header_style = &style->window.header.normal;
        }
        
        if (header_style->type == MONSTRO_STYLE_ITEM_COLOR) {
            uint32_t header_color = monstro_color_to_u32(header_style->data.color);
            MonstroRect header_rect = {win->pos_x, win->pos_y, win->size_x, header_height};
            monstro_canvas_add_rect_filled(canvas, header_rect, header_color, style->window.rounding, MONSTRO_CORNER_TOP);
        }
        
        content_y += header_height;
    }
    
    float padding_x = style->window.padding[0];
    float padding_y = style->window.padding[1];
    
    win->layout_stack_top = 0;
    MonstroLayoutInternal* layout = &win->layout_stack[0];
    layout->direction = MONSTRO_LAYOUT_VERTICAL;
    layout->pos_x = win->pos_x + padding_x;
    layout->pos_y = content_y + padding_y;
    layout->size_x = win->size_x - padding_x * 2.0f;
    layout->size_y = win->size_y - (content_y - win->pos_y) - padding_y * 2.0f;
    layout->cursor_x = layout->pos_x;
    layout->cursor_y = layout->pos_y;
    layout->max_x = layout->pos_x;
    layout->max_y = layout->pos_y;
    
    MonstroRect clip_rect = {win->pos_x, content_y, win->size_x, win->size_y - (content_y - win->pos_y)};
    monstro_canvas_push_clip_rect(canvas, clip_rect);
    
    if ((flags & MONSTRO_WINDOW_MOVABLE) && (flags & MONSTRO_WINDOW_TITLE_BAR)) {
        float mx = ctx->mouse.pos_x;
        float my = ctx->mouse.pos_y;
        float header_height = 28.0f;
        
        bool in_header = mx >= win->pos_x && mx < win->pos_x + win->size_x &&
                         my >= win->pos_y && my < win->pos_y + header_height;
        
        if (in_header && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false)) {
            monstro_set_active_id(ctx, win->move_id);
        }
        
        if (ctx->active_id == win->move_id) {
            if (monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT)) {
                win->pos_x += ctx->mouse.delta_x;
                win->pos_y += ctx->mouse.delta_y;
                monstro_keep_alive_id(ctx, win->move_id);
            } else {
                monstro_set_active_id(ctx, 0);
            }
        }
    }
    
    ctx->want_capture_mouse = true;
    ctx->want_capture_keyboard = true;
    
    return !win->skip_items;
}

void monstro_end_window(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;
    
    monstro_canvas_pop_clip_rect(ctx->canvas);
    monstro_pop_id(ctx);
    ctx->current_window = nullptr;
}

MonstroRect monstro_get_window_rect(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return {0, 0, 0, 0};
    MonstroWindowInternal* win = ctx->current_window;
    return {win->pos_x, win->pos_y, win->size_x, win->size_y};
}

MonstroVec2 monstro_get_window_pos(MonstroContext* ctx) {
    MonstroVec2 result = {0.0f, 0.0f};
    if (ctx && ctx->current_window) {
        result.x = ctx->current_window->pos_x;
        result.y = ctx->current_window->pos_y;
    }
    return result;
}

MonstroVec2 monstro_get_window_size(MonstroContext* ctx) {
    MonstroVec2 result = {0.0f, 0.0f};
    if (ctx && ctx->current_window) {
        result.x = ctx->current_window->size_x;
        result.y = ctx->current_window->size_y;
    }
    return result;
}

float monstro_get_scroll_x(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return 0.0f;
    return ctx->current_window->scroll_x;
}

float monstro_get_scroll_y(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return 0.0f;
    return ctx->current_window->scroll_y;
}

void monstro_set_scroll_x(MonstroContext* ctx, float scroll_x) {
    if (!ctx || !ctx->current_window) return;
    ctx->current_window->scroll_x = scroll_x;
}

void monstro_set_scroll_y(MonstroContext* ctx, float scroll_y) {
    if (!ctx || !ctx->current_window) return;
    ctx->current_window->scroll_y = scroll_y;
}

bool monstro_is_window_collapsed(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return false;
    return ctx->current_window->collapsed;
}

bool monstro_is_window_focused(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return false;
    return ctx->focused_window == ctx->current_window;
}

bool monstro_is_window_hovered(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return false;
    MonstroWindowInternal* win = ctx->current_window;
    float mx = ctx->mouse.pos_x;
    float my = ctx->mouse.pos_y;
    return mx >= win->pos_x && mx < win->pos_x + win->size_x &&
           my >= win->pos_y && my < win->pos_y + win->size_y;
}
