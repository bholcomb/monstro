#include "../context.h"
#include <cstring>
#include <cmath>

static bool button_behavior(MonstroContext* ctx, MonstroRect r, uint64_t id,
                           bool* out_hovered, bool* out_held, MonstroSelectableFlags flags) {
    monstro::Window* win = ctx->current_window;
    
    *out_hovered = false;
    *out_held = false;
    
    if (flags & MONSTRO_SELECTABLE_DISABLED) {
        return false;
    }
    
    float mx = ctx->mouse.pos_x;
    float my = ctx->mouse.pos_y;
    
    bool hovered = mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
    if (hovered && win == ctx->hovered_window) {
        ctx->hovered_id = id;
        *out_hovered = true;
    } else {
        hovered = false;
    }
    
    bool pressed = false;
    bool held = false;
    
    if (hovered) {
        if (flags & MONSTRO_SELECTABLE_MENU) {
            if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false)) {
                pressed = true;
            }
        } else if (flags & MONSTRO_SELECTABLE_MENU_ITEM) {
            if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false)) {
                pressed = true;
            }
            if (monstro_is_mouse_released(ctx, MONSTRO_MOUSE_BUTTON_LEFT)) {
                pressed = true;
            }
        } else {
            if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false)) {
                ctx->setActiveId(id);
                pressed = true;
            }
        }
    }
    
    if (ctx->active_id == id) {
        if (monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT)) {
            held = true;
        } else {
            ctx->setActiveId(0);
        }
    }
    
    *out_held = held;
    return pressed;
}

bool monstro_selectable(MonstroContext* ctx, const char* label, bool* selected,
                        MonstroSelectableFlags flags, const char* tooltip) {
    if (!ctx || !ctx->current_window || !label) return false;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return false;
    
    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    MonstroFont* font = style->font;
    
    uint64_t id = win->getChildId(label);
    const char* display = monstro_display_text(label);
    MonstroVec2 label_size = monstro_font_measure_text(font, display, 0.0f);
    if (label_size.x == 0.0f) label_size.x = (float)strlen(display) * 8.0f;
    if (label_size.y == 0.0f) label_size.y = 16.0f;
    
    float pad_x = style->selectable.padding[0];
    float pad_y = style->selectable.padding[1];
    
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    
    float w = label_size.x + pad_x * 2.0f;
    float h = label_size.y + pad_y * 2.0f;
    
    MonstroRect outer = {pos.x, pos.y, w, h};
    MonstroRect inner = {pos.x + pad_x, pos.y + pad_y, label_size.x, label_size.y};
    
    bool hovered = false;
    bool held = false;
    bool pressed = button_behavior(ctx, outer, id, &hovered, &held, flags);
    
    if (hovered) {
        MonstroColor col = style->selectable.text_hover_active;
        canvas->addRectFilled(outer, monstro_color_to_u32(col),
                                       style->selectable.rounding, MONSTRO_CORNER_ALL);
    }
    
    MonstroVec2 text_pos = {inner.x, inner.y};
    canvas->addText(font, text_pos, 
                            monstro_color_to_u32(style->selectable.text_normal), display);
    
    if (pressed && !(flags & MONSTRO_SELECTABLE_DONT_CLOSE_POPUP)) {
        if (win->flags & MONSTRO_WINDOW_POPUP) {
            monstro_close_current_popup(ctx);
        }
    }
    
    if (pressed && selected) {
        *selected = !*selected;
    }
    
    monstro_add_item(ctx, outer.w, outer.h);

    if (hovered && tooltip) {
        monstro_tooltip(ctx, tooltip);
    }

    return pressed;
}

bool monstro_selectable_sized(MonstroContext* ctx, const char* label, bool* selected,
                              float width, float height, MonstroSelectableFlags flags, const char* tooltip) {
    if (!ctx || !ctx->current_window || !label) return false;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return false;
    
    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    MonstroFont* font = style->font;
    
    uint64_t id = win->getChildId(label);
    const char* display = monstro_display_text(label);
    MonstroVec2 label_size = monstro_font_measure_text(font, display, 0.0f);
    if (label_size.x == 0.0f) label_size.x = (float)strlen(display) * 8.0f;
    if (label_size.y == 0.0f) label_size.y = 16.0f;
    
    float pad_x = style->selectable.padding[0];
    float pad_y = style->selectable.padding[1];
    
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    
    float size_x = (width != 0.0f) ? width : label_size.x;
    float size_y = (height != 0.0f) ? height : label_size.y;
    
    MonstroRect outer = {pos.x, pos.y, size_x + pad_x * 2.0f, size_y + pad_y * 2.0f};
    MonstroRect inner = {pos.x + pad_x, pos.y + pad_y, size_x, size_y};
    
    bool hovered = false;
    bool held = false;
    bool pressed = button_behavior(ctx, outer, id, &hovered, &held, flags);
    
    if (hovered) {
        MonstroColor col = style->selectable.text_hover_active;
        canvas->addRectFilled(outer, monstro_color_to_u32(col),
                                       style->selectable.rounding, MONSTRO_CORNER_ALL);
    }
    
    MonstroVec2 text_pos = {inner.x, inner.y};
    canvas->addText(font, text_pos, 
                            monstro_color_to_u32(style->selectable.text_normal), display);
    
    if (pressed && !(flags & MONSTRO_SELECTABLE_DONT_CLOSE_POPUP)) {
        if (win->flags & MONSTRO_WINDOW_POPUP) {
            monstro_close_current_popup(ctx);
        }
    }
    
    if (pressed && selected) {
        *selected = !*selected;
    }
    
    monstro_add_item(ctx, outer.w, outer.h);

    if (hovered && tooltip) {
        monstro_tooltip(ctx, tooltip);
    }

    return pressed;
}
