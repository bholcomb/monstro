#include "../context.h"
#include <string.h>
#include <cmath>
#include <cstdio>
#include <algorithm>

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static bool slider_behavior(MonstroContext* ctx, MonstroRect r, uint64_t id,
                            bool* out_hovered, bool* out_held) {
    monstro::Window* win = ctx->current_window;

    *out_hovered = false;
    *out_held = false;

    if (win != ctx->hovered_window) return false;

    float mx = ctx->mouse.pos_x;
    float my = ctx->mouse.pos_y;

    bool hovered = mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
    if (hovered) {
        ctx->hovered_id = id;
        if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false)) {
            ctx->setActiveId(id);
            ctx->focusWindow(win);
        }
    }

    bool held = false;
    if (ctx->active_id == id) {
        if (monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT)) {
            held = true;
        } else {
            ctx->setActiveId(0);
        }
    }

    *out_hovered = hovered || held;
    *out_held = held;
    return held;
}

bool monstro_slider_float_ex(MonstroContext* ctx, const char* label, float* value,
                             float min_val, float max_val, const char* format,
                             float width, float height, MonstroSliderFlags flags, const char* tooltip) {
    if (!ctx || !ctx->current_window || !label || !value) return false;
    (void)flags;

    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return false;

    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &win->canvas;
    MonstroFont* font = style->font;
    MonstroStyleSlider* ss = &style->slider;

    uint64_t id = win->getChildId(label);

    monstro_add_item(ctx, ss->padding[0], ss->padding[1]);

    float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
    float label_w = monstro_font_measure_text(font, label, 0.0f).x;
    if (label_w == 0.0f) label_w = (float)strlen(label) * 8.0f;

    float slider_h = height > 0.0f ? height : std::max(text_h, ss->cursor_size[1]) + ss->padding[1] * 2.0f;
    float slider_w = width > 0.0f ? width : 120.0f;
    float total_w = label_w + ss->spacing[0] + slider_w;

    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);

    MonstroRect label_rect = {pos.x, pos.y, label_w, slider_h};
    MonstroRect slider_rect = {pos.x + label_w + ss->spacing[0], pos.y, slider_w, slider_h};

    bool hovered = false;
    bool held = false;
    bool dragging = slider_behavior(ctx, slider_rect, id, &hovered, &held);

    float range = max_val - min_val;
    if (range == 0.0f) range = 1.0f;

    float old_value = *value;

    if (held) {
        float ratio = (ctx->mouse.pos_x - slider_rect.x) / slider_rect.w;
        ratio = clampf(ratio, 0.0f, 1.0f);
        *value = min_val + ratio * range;
    }

    *value = clampf(*value, min_val, max_val);
    float t = (*value - min_val) / range;

    // Background bar
    float bar_y = slider_rect.y + (slider_rect.h - ss->bar_height) * 0.5f;
    MonstroRect bar_rect = {slider_rect.x, bar_y, slider_rect.w, ss->bar_height};

    MonstroColor bar_col = hovered ? ss->bar_hover : ss->bar_normal;
    canvas->addRectFilled(bar_rect, monstro_color_to_u32(bar_col), ss->rounding, MONSTRO_CORNER_ALL);

    if (ss->border > 0.0f) {
        canvas->addRect(bar_rect, monstro_color_to_u32(ss->border_color), ss->rounding, MONSTRO_CORNER_ALL, ss->border);
    }

    // Filled portion
    if (t > 0.0f) {
        MonstroRect filled_rect = {bar_rect.x, bar_rect.y, bar_rect.w * t, bar_rect.h};
        canvas->addRectFilled(filled_rect, monstro_color_to_u32(ss->bar_filled), ss->rounding, MONSTRO_CORNER_LEFT);
    }

    // Cursor
    float cursor_x = slider_rect.x + t * slider_rect.w - ss->cursor_size[0] * 0.5f;
    float cursor_y = slider_rect.y + (slider_rect.h - ss->cursor_size[1]) * 0.5f;
    MonstroRect cursor_rect = {cursor_x, cursor_y, ss->cursor_size[0], ss->cursor_size[1]};

    MonstroStyleItem* cursor_style = &ss->cursor_normal;
    if (held) cursor_style = &ss->cursor_active;
    else if (hovered) cursor_style = &ss->cursor_hover;

    if (cursor_style->type == MONSTRO_STYLE_ITEM_COLOR) {
        canvas->addRectFilled(cursor_rect, monstro_color_to_u32(cursor_style->data.color), ss->rounding, MONSTRO_CORNER_ALL);
    }

    // Value text overlay on the slider bar
    if (format && font) {
        char buf[64];
        snprintf(buf, sizeof(buf), format, *value);
        canvas->addTextEx(font, slider_rect, monstro_color_to_u32(style->text.color), buf, MONSTRO_ALIGN_CENTER);
    }

    // Label
    if (font) {
        canvas->addTextEx(font, label_rect, monstro_color_to_u32(style->text.color), label, MONSTRO_ALIGN_LEFT);
    }

    monstro_add_item(ctx, total_w, slider_h);

    if (hovered && tooltip) {
        monstro_tooltip(ctx, tooltip);
    }

    return *value != old_value;
}

bool monstro_slider_float(MonstroContext* ctx, const char* label, float* value,
                          float min_val, float max_val, const char* format, const char* tooltip) {
    return monstro_slider_float_ex(ctx, label, value, min_val, max_val, format, 0.0f, 0.0f, MONSTRO_SLIDER_NONE, tooltip);
}

bool monstro_slider_int_ex(MonstroContext* ctx, const char* label, int* value,
                           int min_val, int max_val, const char* format,
                           float width, float height, MonstroSliderFlags flags, const char* tooltip) {
    if (!value) return false;

    float fval = (float)*value;
    bool changed = monstro_slider_float_ex(ctx, label, &fval, (float)min_val, (float)max_val, "%.0f", width, height, flags, tooltip);
    int new_val = (int)(fval + 0.5f);
    if (new_val != *value) {
        *value = new_val;
        return true;
    }
    return changed;
}

bool monstro_slider_int(MonstroContext* ctx, const char* label, int* value,
                        int min_val, int max_val, const char* format, const char* tooltip) {
    return monstro_slider_int_ex(ctx, label, value, min_val, max_val, format, 0.0f, 0.0f, MONSTRO_SLIDER_NONE, tooltip);
}
