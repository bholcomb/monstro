#include "internal/types.h"
#include <cstring>
#include <cmath>

/* ------------------------------------------------------------------------- */
/* MonstroWindowInternal Member Functions                                    */
/* ------------------------------------------------------------------------- */

void MonstroWindowInternal::init(const char* windowName, MonstroWindowFlags windowFlags) {
    std::memset(this, 0, sizeof(MonstroWindowInternal));
    
    std::strncpy(name, windowName, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    
    id = monstro_hash_string(windowName);
    move_id = monstro_hash_combine(id, monstro_hash_string("##MOVE"));
    flags = windowFlags;
    active = true;
    collapsed = false;
    
    set_pos_allow_flags = (MonstroCondition)(MONSTRO_COND_ALWAYS | MONSTRO_COND_ONCE | 
                                              MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING);
    set_size_allow_flags = (MonstroCondition)(MONSTRO_COND_ALWAYS | MONSTRO_COND_ONCE | 
                                               MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING);
    
    pos_x = 100.0f;
    pos_y = 100.0f;
    size_x = 300.0f;
    size_y = 200.0f;
}

bool MonstroWindowInternal::containsPoint(float x, float y) const {
    return x >= pos_x && x < pos_x + size_x &&
           y >= pos_y && y < pos_y + size_y;
}

MonstroRect MonstroWindowInternal::getRect() const {
    return {pos_x, pos_y, size_x, size_y};
}

void MonstroWindowInternal::initLayout(float contentY, float paddingX, float paddingY) {
    layout_stack_top = 0;
    MonstroLayoutInternal* layout = &layout_stack[0];
    layout->direction = MONSTRO_LAYOUT_VERTICAL;
    layout->pos_x = pos_x + paddingX;
    layout->pos_y = contentY + paddingY;
    layout->size_x = size_x - paddingX * 2.0f;
    layout->size_y = size_y - (contentY - pos_y) - paddingY * 2.0f;
    layout->cursor_x = layout->pos_x;
    layout->cursor_y = layout->pos_y;
    layout->max_x = layout->pos_x;
    layout->max_y = layout->pos_y;
}

MonstroLayoutInternal* MonstroWindowInternal::getCurrentLayout() {
    if (layout_stack_top < 0 || layout_stack_top >= MONSTRO_MAX_LAYOUT_STACK_DEPTH) {
        return nullptr;
    }
    return &layout_stack[layout_stack_top];
}

/* ------------------------------------------------------------------------- */
/* MonstroContext Window Management Member Functions                         */
/* ------------------------------------------------------------------------- */

MonstroWindowInternal* MonstroContext::findWindow(const char* name) {
    if (!name) return nullptr;
    
    uint64_t windowId = monstro_hash_string(name);
    for (int i = 0; i < window_count; i++) {
        if (windows[i].id == windowId) {
            return &windows[i];
        }
    }
    return nullptr;
}

MonstroWindowInternal* MonstroContext::createWindow(const char* name, MonstroWindowFlags flags) {
    if (!name || window_count >= MONSTRO_MAX_WINDOWS) return nullptr;
    
    MonstroWindowInternal* win = &windows[window_count++];
    win->init(name, flags);
    return win;
}

MonstroWindowInternal* MonstroContext::findHoveredWindow(float x, float y) {
    for (int i = window_count - 1; i >= 0; i--) {
        MonstroWindowInternal* win = &windows[i];
        if (!win->active) continue;
        
        if (win->containsPoint(x, y)) {
            return win;
        }
    }
    return nullptr;
}

/* ------------------------------------------------------------------------- */
/* Condition Helpers                                                         */
/* ------------------------------------------------------------------------- */

static bool should_apply_condition(MonstroCondition cond, MonstroCondition allowed, bool appearing, bool first_use) {
    if (cond == MONSTRO_COND_NONE) return false;
    if ((cond & MONSTRO_COND_ALWAYS) && (allowed & MONSTRO_COND_ALWAYS)) return true;
    if ((cond & MONSTRO_COND_ONCE) && (allowed & MONSTRO_COND_ONCE)) return true;
    if ((cond & MONSTRO_COND_FIRST_USE_EVER) && first_use && (allowed & MONSTRO_COND_FIRST_USE_EVER)) return true;
    if ((cond & MONSTRO_COND_APPEARING) && appearing && (allowed & MONSTRO_COND_APPEARING)) return true;
    return false;
}

/* ------------------------------------------------------------------------- */
/* C API Implementation                                                      */
/* ------------------------------------------------------------------------- */

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
    (void)p_open;
    
    MonstroWindowInternal* win = ctx->findWindow(name);
    bool first_use = (win == nullptr);
    bool appearing = first_use;
    
    if (!win) {
        win = ctx->createWindow(name, flags);
        if (!win) return false;
    } else {
        appearing = (win->last_frame_active != ctx->frame - 1);
    }
    
    win->flags = flags;
    win->active = true;
    win->last_frame_active = ctx->frame;
    win->skip_items = false;
    
    // Apply next window settings
    if (should_apply_condition(ctx->next_window_pos_cond, win->set_pos_allow_flags, appearing, first_use)) {
        win->setPos(ctx->next_window_pos_x, ctx->next_window_pos_y);
    }
    
    if (should_apply_condition(ctx->next_window_size_cond, win->set_size_allow_flags, appearing, first_use)) {
        win->setSize(ctx->next_window_size_x, ctx->next_window_size_y);
    }
    
    if (ctx->next_window_collapsed_cond != MONSTRO_COND_NONE) {
        win->collapsed = ctx->next_window_collapsed;
    }
    
    // Clear next window conditions
    ctx->next_window_pos_cond = MONSTRO_COND_NONE;
    ctx->next_window_size_cond = MONSTRO_COND_NONE;
    ctx->next_window_collapsed_cond = MONSTRO_COND_NONE;
    
    // Clear ONCE flags after first use
    if (win->set_pos_allow_flags & MONSTRO_COND_ONCE) {
        win->set_pos_allow_flags = (MonstroCondition)(win->set_pos_allow_flags & ~MONSTRO_COND_ONCE);
    }
    if (win->set_size_allow_flags & MONSTRO_COND_ONCE) {
        win->set_size_allow_flags = (MonstroCondition)(win->set_size_allow_flags & ~MONSTRO_COND_ONCE);
    }
    
    // Handle focus
    if (ctx->next_window_focus) {
        ctx->focusWindow(win);
        ctx->next_window_focus = false;
    }
    
    ctx->current_window = win;
    ctx->pushId(win->id);
    
    if (win->collapsed) {
        win->skip_items = true;
    }
    
    MonstroCanvas* canvas = ctx->canvas;
    MonstroStyle* style = &ctx->style;
    
    // Draw background
    if (flags & MONSTRO_WINDOW_BACKGROUND) {
        uint32_t bg_color = monstro_color_to_u32(style->window.background);
        monstro_canvas_add_rect_filled(canvas, win->getRect(), bg_color, style->window.rounding, MONSTRO_CORNER_ALL);
    }
    
    // Draw border
    if (flags & MONSTRO_WINDOW_BORDERS) {
        uint32_t border_color = monstro_color_to_u32(style->window.border_color);
        monstro_canvas_add_rect(canvas, win->getRect(), border_color, style->window.rounding, MONSTRO_CORNER_ALL, style->window.border);
    }
    
    float content_y = win->pos_y;
    
    // Draw title bar
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
    
    // Initialize layout
    float padding_x = style->window.padding[0];
    float padding_y = style->window.padding[1];
    win->initLayout(content_y, padding_x, padding_y);
    
    // Set clip rect
    MonstroRect clip_rect = {win->pos_x, content_y, win->size_x, win->size_y - (content_y - win->pos_y)};
    monstro_canvas_push_clip_rect(canvas, clip_rect);
    
    // Handle window dragging
    if ((flags & MONSTRO_WINDOW_MOVABLE) && (flags & MONSTRO_WINDOW_TITLE_BAR)) {
        float mx = ctx->mouse.pos_x;
        float my = ctx->mouse.pos_y;
        float header_height = 28.0f;
        
        bool in_header = mx >= win->pos_x && mx < win->pos_x + win->size_x &&
                         my >= win->pos_y && my < win->pos_y + header_height;
        
        if (in_header && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false)) {
            ctx->setActiveId(win->move_id);
        }
        
        if (ctx->active_id == win->move_id) {
            if (monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT)) {
                win->move(ctx->mouse.delta_x, ctx->mouse.delta_y);
                ctx->keepAliveId(win->move_id);
            } else {
                ctx->setActiveId(0);
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
    ctx->popId();
    ctx->current_window = nullptr;
}

MonstroRect monstro_get_window_rect(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return {0, 0, 0, 0};
    return ctx->current_window->getRect();
}

MonstroVec2 monstro_get_window_pos(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return {0.0f, 0.0f};
    return ctx->current_window->getPos();
}

MonstroVec2 monstro_get_window_size(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return {0.0f, 0.0f};
    return ctx->current_window->getSize();
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
    return ctx->current_window->containsPoint(ctx->mouse.pos_x, ctx->mouse.pos_y);
}
