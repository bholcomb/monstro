#include "../context.h"
#include <cstring>
#include <cmath>

/* ------------------------------------------------------------------------- */
/* Next Window Configuration                                                 */
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

void monstro_set_next_window_focus(MonstroContext* ctx) {
    if (!ctx) return;
    ctx->next_window_focus = true;
}

/* ------------------------------------------------------------------------- */
/* Window Lifecycle                                                          */
/* ------------------------------------------------------------------------- */

bool monstro_begin_window(MonstroContext* ctx, const char* name, bool* p_open, MonstroWindowFlags flags) {
    if (!ctx || !name) return false;
    
    if (p_open && *p_open == false) {
        return false;
    }
    
    MonstroWindowInternal* win = ctx->findWindow(name);
    
    if (!win) {
        win = ctx->createWindow(name, flags);
        if (!win) return false;
    }
    
    win->flags = flags;
    
    ctx->pushId(win->id);

    if (flags & MONSTRO_WINDOW_MODAL) {
        win->makeLastSibling();
    }

    // Apply setNextWindowPosition
    if (ctx->next_window_pos_cond != MONSTRO_COND_NONE) {
        bool shouldSet = (win->set_pos_allow_flags & ctx->next_window_pos_cond) != 0;
        bool isValid = ctx->next_window_pos_x != -INFINITY && ctx->next_window_pos_y != -INFINITY;
        if (shouldSet && isValid) {
            win->setPosition(ctx->next_window_pos_x, ctx->next_window_pos_y, ctx->next_window_pos_cond);
            ctx->next_window_pos_cond = MONSTRO_COND_NONE;
            ctx->next_window_pos_x = -INFINITY;
            ctx->next_window_pos_y = -INFINITY;
        }
    }

    // Apply setNextWindowSize
    if (ctx->next_window_size_cond != MONSTRO_COND_NONE) {
        bool shouldSet = (win->set_size_allow_flags & ctx->next_window_size_cond) != 0;
        bool isValid = ctx->next_window_size_x != -INFINITY && ctx->next_window_size_y != -INFINITY;
        if (shouldSet && isValid) {
            win->setSizeWithCond(ctx->next_window_size_x, ctx->next_window_size_y, ctx->next_window_size_cond);
            ctx->next_window_size_cond = MONSTRO_COND_NONE;
            ctx->next_window_size_x = -INFINITY;
            ctx->next_window_size_y = -INFINITY;
        }
    }

    // Setup layout (window-space, starting at 0,0 like C#)
    win->layout_stack_top = -1;
    win->beginLayout(MONSTRO_LAYOUT_VERTICAL);

    if (win->last_frame_active != ctx->frame) {
        win->last_frame_active = ctx->frame;
        win->active = true;
    }

    if (ctx->current_window != win) {
        ctx->pushWindow(win);
    }

    // Check for collapsing (double-click on title bar)
    MonstroStyle* style = &ctx->style;
    if ((flags & MONSTRO_WINDOW_TITLE_BAR) && (flags & MONSTRO_WINDOW_COLLAPSABLE)) {
        MonstroRect title_rect = win->getTitleBarRect(style);
        float mx = ctx->mouse.pos_x;
        float my = ctx->mouse.pos_y;
        bool in_header = mx >= title_rect.x && mx < title_rect.x + title_rect.w &&
                         my >= title_rect.y && my < title_rect.y + title_rect.h;

        if (ctx->hovered_window == win && in_header &&
            monstro_is_mouse_double_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT)) {
            win->collapsed = !win->collapsed;
        }
    } else {
        win->collapsed = false;
    }

    // Check for moving window (middle mouse)
    ctx->keepAliveId(win->move_id);
    if (ctx->active_id == win->move_id) {
        if (monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_MIDDLE)) {
            if (flags & MONSTRO_WINDOW_MOVABLE) {
                win->move(ctx->mouse.delta_x, ctx->mouse.delta_y);
            }
            ctx->focusWindow(win);
        } else {
            ctx->setActiveId(0);
        }
    }

    // --- drawWindowBackground ---
    MonstroCanvas* canvas = ctx->canvas;

    if (flags & MONSTRO_WINDOW_ROOT) {
        monstro_canvas_push_clip_rect_fullscreen(canvas);
        float saved_rounding = style->window.rounding;
        style->window.rounding = 0.0f;

        if (!win->collapsed) {
            if (flags & MONSTRO_WINDOW_BACKGROUND) {
                MonstroStyleItem* bg = &style->window.fixed_background;
                if (bg->type == MONSTRO_STYLE_ITEM_COLOR) {
                    uint32_t bg_color = monstro_color_to_u32(bg->data.color);
                    monstro_canvas_add_rect_filled(canvas, win->getRect(), bg_color, 0.0f, MONSTRO_CORNER_ALL);
                }
            }
        }

        style->window.rounding = saved_rounding;
    } else {
        MonstroRect clip_rect = win->getRect();
        clip_rect.w += 1;
        clip_rect.h += 1;
        monstro_canvas_push_clip_rect(canvas, clip_rect);

        if (!win->collapsed) {
            if (flags & MONSTRO_WINDOW_BACKGROUND) {
                MonstroStyleItem* bg = &style->window.fixed_background;
                if (bg->type == MONSTRO_STYLE_ITEM_COLOR) {
                    uint32_t bg_color = monstro_color_to_u32(bg->data.color);
                    monstro_canvas_add_rect_filled(canvas, win->getRect(), bg_color, style->window.rounding, MONSTRO_CORNER_ALL);
                }
            }
        }
    }

    // --- drawWindowForeground ---
    float saved_rounding_fg = style->window.rounding;
    if (flags & MONSTRO_WINDOW_ROOT) {
        style->window.rounding = 0.0f;
    }

    if (win->collapsed) {
        if (flags & MONSTRO_WINDOW_TITLE_BAR) {
            MonstroRect title_rect = win->getTitleBarRect(style);
            uint32_t header_color = monstro_color_to_u32(style->window.header.active.data.color);
            monstro_canvas_add_rect_filled(canvas, title_rect, header_color, style->window.rounding, MONSTRO_CORNER_ALL);

            uint32_t label_color = monstro_color_to_u32(style->window.header.label_normal);
            if (style->font) {
                monstro_canvas_add_text_ex(canvas, style->font, title_rect, label_color, win->name, MONSTRO_ALIGN_CENTER);
            }
        }
    } else {
        if (flags & MONSTRO_WINDOW_TITLE_BAR) {
            MonstroRect title_rect = win->getTitleBarRect(style);
            uint32_t header_color = monstro_color_to_u32(style->window.header.active.data.color);
            monstro_canvas_add_rect_filled(canvas, title_rect, header_color, style->window.rounding, MONSTRO_CORNER_TOP);

            uint32_t label_color = monstro_color_to_u32(style->window.header.label_normal);
            if (style->font) {
                monstro_canvas_add_text_ex(canvas, style->font, title_rect, label_color, win->name, MONSTRO_ALIGN_CENTER);
            }

            win->addItem(title_rect.w, title_rect.h);
        }

        if (flags & MONSTRO_WINDOW_MENU_BAR) {
            MonstroRect menu_rect = win->getMenuBarRect(style);
            uint32_t menu_bg_color = monstro_color_to_u32(style->window.menu_border_color);
            monstro_canvas_add_rect_filled(canvas, menu_rect, menu_bg_color, 0, MONSTRO_CORNER_NONE);
        }

        if (flags & MONSTRO_WINDOW_BORDERS) {
            uint32_t border_color = monstro_color_to_u32(style->window.border_color);
            monstro_canvas_add_rect(canvas, win->getRect(), border_color, style->window.rounding, MONSTRO_CORNER_ALL, style->window.border);

            if (!(flags & MONSTRO_WINDOW_TITLE_BAR)) {
                MonstroRect title_rect = win->getTitleBarRect(style);
                MonstroVec2 p1 = {title_rect.x + 1.0f, title_rect.y};
                MonstroVec2 p2 = {title_rect.x + title_rect.w - 1.0f, title_rect.y};
                monstro_canvas_add_line(canvas, p1, p2, border_color, style->window.border);
            }
        }
    }

    if (flags & MONSTRO_WINDOW_ROOT) {
        style->window.rounding = saved_rounding_fg;
    }

    win->skip_items = win->collapsed;
    
    ctx->want_capture_mouse = true;
    ctx->want_capture_keyboard = true;
    
    return true;
}

void monstro_end_window(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;
    
    MonstroWindowInternal* win = ctx->current_window;
    MonstroStyle* style = &ctx->style;
    
    monstro_canvas_pop_clip_rect(ctx->canvas);
    
    if (win->flags & MONSTRO_WINDOW_AUTO_RESIZE) {
        MonstroLayoutInternal* layout = win->getCurrentLayout();
        if (layout) {
            float new_width = layout->size_x + style->window.padding[0];
            float new_height = layout->size_y + style->window.padding[1];
            if (new_width > 0.0f) win->size_x = new_width;
            if (new_height > 0.0f) win->size_y = new_height;
        }
    }
    
    win->layout_stack_top = -1;
    ctx->popId();
    ctx->popWindow();
}

/* ------------------------------------------------------------------------- */
/* Current Window Configuration                                              */
/* ------------------------------------------------------------------------- */

void monstro_set_window_pos(MonstroContext* ctx, float x, float y, MonstroCondition cond) {
    if (!ctx || !ctx->current_window) return;
    ctx->current_window->setPosition(x, y, cond);
}

void monstro_set_window_size(MonstroContext* ctx, float width, float height, MonstroCondition cond) {
    if (!ctx || !ctx->current_window) return;
    ctx->current_window->setSizeWithCond(width, height, cond);
}

void monstro_set_window_focus(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;
    ctx->focusWindow(ctx->current_window);
}

void monstro_set_window_layout(MonstroContext* ctx, MonstroLayoutDirection direction) {
    if (!ctx || !ctx->current_window) return;
    MonstroLayoutInternal* layout = ctx->current_window->getCurrentLayout();
    if (layout) {
        layout->direction = direction;
    }
}

/* ------------------------------------------------------------------------- */
/* Window Configuration by Name                                              */
/* ------------------------------------------------------------------------- */

void monstro_set_window_pos_named(MonstroContext* ctx, const char* name, float x, float y, MonstroCondition cond) {
    if (!ctx) return;
    MonstroWindowInternal* win = ctx->findWindow(name);
    if (win) win->setPosition(x, y, cond);
}

void monstro_set_window_size_named(MonstroContext* ctx, const char* name, float width, float height, MonstroCondition cond) {
    if (!ctx) return;
    MonstroWindowInternal* win = ctx->findWindow(name);
    if (win) win->setSizeWithCond(width, height, cond);
}

void monstro_set_window_focus_named(MonstroContext* ctx, const char* name) {
    if (!ctx) return;
    MonstroWindowInternal* win = ctx->findWindow(name);
    if (win) ctx->focusWindow(win);
}

void monstro_set_window_layout_named(MonstroContext* ctx, const char* name, MonstroLayoutDirection direction) {
    if (!ctx) return;
    MonstroWindowInternal* win = ctx->findWindow(name);
    if (win) {
        MonstroLayoutInternal* layout = win->getCurrentLayout();
        if (layout) {
            layout->direction = direction;
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Window Properties                                                         */
/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */
/* Scrolling                                                                 */
/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */
/* Window State Queries                                                      */
/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */
/* Layout Helpers                                                            */
/* ------------------------------------------------------------------------- */

void monstro_next_line(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;
    MonstroLayoutInternal* layout = ctx->current_window->getCurrentLayout();
    if (!layout) return;

    if (layout->direction == MONSTRO_LAYOUT_HORIZONTAL) {
        layout->cursor_x = 0;
        layout->cursor_y += layout->size_y;
    } else {
        layout->cursor_x += layout->size_x;
        layout->cursor_y = 0;
    }
}
