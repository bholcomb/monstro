#include "../context.h"
#include <string.h>
#include <cmath>

static bool checkbox_behavior(MonstroContext* ctx, MonstroRect r, uint64_t id,
                              bool* out_hovered, bool* out_held) {
    monstro::Window* win = ctx->current_window;
    
    if (win != ctx->hovered_window) {
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
        
        if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false)) {
            ctx->setActiveId(id);
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

bool monstro_checkbox(MonstroContext* ctx, const char* label, bool* value, const char* tooltip) {
    if (!ctx || !ctx->current_window || !label || !value) return false;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return false;
    
    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    MonstroFont* font = style->font;
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    
    float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
    float box_size = text_h;
    float text_w = monstro_font_measure_text(font, label, 0.0f).x;
    if (text_w == 0.0f) text_w = (float)strlen(label) * 8.0f;
    float spacing = style->checkbox.spacing;
    
    float total_w = box_size + spacing + text_w;
    float total_h = fmaxf(box_size, text_h);
    
    MonstroRect r = {pos.x, pos.y, total_w, total_h};
    uint64_t id = win->getChildId(label);
    
    bool hovered = false;
    bool held = false;
    bool pressed = checkbox_behavior(ctx, r, id, &hovered, &held);
    
    if (pressed) {
        *value = !*value;
    }
    
    MonstroStyleItem* bg_style = &style->checkbox.normal;
    if (hovered) bg_style = &style->checkbox.hover;
    if (held) bg_style = &style->checkbox.active;
    
    MonstroRect box_rect = {pos.x, pos.y + (total_h - box_size) * 0.5f, box_size, box_size};
    
    // Draw checkbox box background
    if (bg_style->type == MONSTRO_STYLE_ITEM_COLOR) {
        canvas->addRectFilled(box_rect, monstro_color_to_u32(bg_style->data.color),
                                       style->checkbox.rounding, MONSTRO_CORNER_ALL);
    }
    
    // Draw checkbox border
    canvas->addRect(box_rect, monstro_color_to_u32(style->checkbox.border_color),
                            style->checkbox.rounding, MONSTRO_CORNER_ALL, style->checkbox.border);
    
    // Draw checkmark if checked
    if (*value) {
        MonstroStyleItem* cursor_style = hovered ? &style->checkbox.cursor_hover : &style->checkbox.cursor_normal;
        if (cursor_style->type == MONSTRO_STYLE_ITEM_COLOR) {
            float pad = 3.0f;
            MonstroRect check_rect = {box_rect.x + pad, box_rect.y + pad, box_rect.w - pad * 2.0f, box_rect.h - pad * 2.0f};
            canvas->addRectFilled(check_rect, monstro_color_to_u32(cursor_style->data.color),
                                           0.0f, MONSTRO_CORNER_NONE);
        }
    }
    
    MonstroVec2 text_pos = {pos.x + box_size + spacing, pos.y + (total_h - text_h) * 0.5f};
    canvas->addText(font, text_pos, monstro_color_to_u32(style->text.color), label);
    
    monstro_add_item(ctx, total_w, total_h);
    
    (void)tooltip;
    
    return pressed;
}
