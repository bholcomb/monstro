#include "../context.h"
#include <string.h>
#include <cmath>

static bool button_behavior(MonstroContext* ctx, MonstroRect r, uint64_t id,
                            bool* out_hovered, bool* out_held, MonstroButtonFlags flags) {
    monstro::Window* win = ctx->current_window;
    
    if ((flags & MONSTRO_BUTTON_DISABLED) || win != ctx->hovered_window) {
        *out_hovered = false;
        *out_held = false;
        return false;
    }
    
    bool pressed = false;
    float mx = ctx->mouse.pos_x;
    float my = ctx->mouse.pos_y;
    
    bool hovered = mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
    if (hovered) {
        ctx->hovered_id = id;
        
        bool no_key_mods = (flags & MONSTRO_BUTTON_NO_KEY_MODIFIERS) == 0;
        bool mods_ok = no_key_mods || !(ctx->keyboard.getMods() & (MONSTRO_MOD_CTRL | MONSTRO_MOD_SHIFT | MONSTRO_MOD_ALT));
        
        if (mods_ok) {
            if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false)) {
                if (flags & MONSTRO_BUTTON_PRESSED_ON_CLICK) {
                    pressed = true;
                    ctx->setActiveId(0);
                } else {
                    ctx->setActiveId(id);
                }
                ctx->focusWindow(win);
            } else if (monstro_is_mouse_released(ctx, MONSTRO_MOUSE_BUTTON_LEFT) && 
                       (flags & MONSTRO_BUTTON_PRESSED_ON_RELEASE)) {
                pressed = true;
                ctx->setActiveId(0);
            } else if ((flags & MONSTRO_BUTTON_REPEAT) && ctx->active_id == id &&
                       monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, true)) {
                pressed = true;
            }
        }
    }
    
    bool held = false;
    if (ctx->active_id == id) {
        if (monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT)) {
            held = true;
        } else {
            if (hovered) {
                pressed = true;
            }
            ctx->setActiveId(0);
        }
    }
    
    *out_hovered = hovered;
    *out_held = held;
    return pressed;
}

static void draw_button_background(monstro::Canvas* canvas, MonstroRect r, MonstroStyle* style,
                                   bool hovered, bool held) {
    MonstroStyleItem* background = &style->button.normal;
    if (hovered) background = &style->button.hover;
    if (held) background = &style->button.active;
    
    switch (background->type) {
        case MONSTRO_STYLE_ITEM_COLOR:
            canvas->addRectFilled(r, monstro_color_to_u32(background->data.color),
                                           style->button.rounding, MONSTRO_CORNER_ALL);
            canvas->addRect(r, monstro_color_to_u32(style->button.border_color),
                                    style->button.rounding, MONSTRO_CORNER_ALL, style->button.border);
            break;
        case MONSTRO_STYLE_ITEM_IMAGE: {
            MonstroVec2 uv_min = {0.0f, 0.0f};
            MonstroVec2 uv_max = {1.0f, 1.0f};
            canvas->addImage(background->data.texture_id, r, uv_min, uv_max, 0xFFFFFFFF);
            break;
        }
    }
}

static void draw_button_text(monstro::Canvas* canvas, MonstroFont* font, MonstroRect r, MonstroStyle* style,
                             const char* text, bool hovered, bool held) {
    if (!font) return;
    
    MonstroColor text_color = style->button.text_normal;
    if (hovered) text_color = style->button.text_hover;
    if (held) text_color = style->button.text_active;
    
    canvas->pushClipRect(r);
    canvas->addTextEx(font, r, monstro_color_to_u32(text_color),
                               text, style->button.text_alignment);
    canvas->popClipRect();
}

bool monstro_button(MonstroContext* ctx, const char* label, const char* tooltip) {
    if (!ctx || !ctx->current_window || !label) return false;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return false;
    
    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    MonstroFont* font = style->font;
    
    // Add spacing before button (like C#: win.addItem(style.button.padding))
    monstro_add_item(ctx, style->button.padding[0], style->button.padding[1]);
    
    uint64_t id = win->getChildId(label);
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    
    float text_w = monstro_font_measure_text(font, label, 0.0f).x;
    float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
    float w = text_w + style->button.padding[0] * 2.0f;
    float h = text_h + style->button.padding[1] * 2.0f;
    
    MonstroRect r = {pos.x, pos.y, w, h};
    
    bool hovered = false;
    bool held = false;
    bool pressed = button_behavior(ctx, r, id, &hovered, &held, (MonstroButtonFlags)0);
    
    draw_button_background(canvas, r, style, hovered, held);
    draw_button_text(canvas, font, r, style, label, hovered, held);
    
    // Add button size (like C#: win.addItem(size))
    monstro_add_item(ctx, w, h);
    
    (void)tooltip;
    
    return pressed;
}

bool monstro_button_sized(MonstroContext* ctx, const char* label, float width, float height, const char* tooltip) {
    if (!ctx || !ctx->current_window || !label) return false;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return false;
    
    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    MonstroFont* font = style->font;
    
    // Add spacing before button (like C#: win.addItem(style.button.padding))
    monstro_add_item(ctx, style->button.padding[0], style->button.padding[1]);
    
    uint64_t id = win->getChildId(label);
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    MonstroRect r = {pos.x, pos.y, width, height};
    
    bool hovered = false;
    bool held = false;
    bool pressed = button_behavior(ctx, r, id, &hovered, &held, (MonstroButtonFlags)0);
    
    draw_button_background(canvas, r, style, hovered, held);
    draw_button_text(canvas, font, r, style, label, hovered, held);
    
    monstro_add_item(ctx, width, height);
    
    (void)tooltip;
    
    return pressed;
}

bool monstro_image_button(MonstroContext* ctx, const char* str_id, uint64_t texture_id,
                          float width, float height, const char* tooltip) {
    if (!ctx || !ctx->current_window || !str_id) return false;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return false;
    
    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    
    // Add spacing before button (like C#: win.addItem(style.button.padding))
    monstro_add_item(ctx, style->button.padding[0], style->button.padding[1]);
    
    uint64_t id = win->getChildId(str_id);
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    MonstroRect r = {pos.x, pos.y, width, height};
    
    bool hovered = false;
    bool held = false;
    bool pressed = button_behavior(ctx, r, id, &hovered, &held, (MonstroButtonFlags)0);
    
    draw_button_background(canvas, r, style, hovered, held);
    
    MonstroRect ir = r;
    ir.x += style->button.image_padding[0];
    ir.y += style->button.image_padding[1];
    ir.w -= style->button.image_padding[0] * 2.0f;
    ir.h -= style->button.image_padding[1] * 2.0f;
    
    MonstroVec2 uv_min = {0.0f, 0.0f};
    MonstroVec2 uv_max = {1.0f, 1.0f};
    canvas->addImage(texture_id, ir, uv_min, uv_max, 0xFFFFFFFF);
    
    monstro_add_item(ctx, width, height);
    
    (void)tooltip;
    
    return pressed;
}
