#include "../context.h"
#include <string.h>

void monstro_label(MonstroContext* ctx, const char* text) {
    if (!ctx || !ctx->current_window || !text) return;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return;
    
    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    MonstroFont* font = style->font;
    
    MonstroVec2 text_size = monstro_font_measure_text(font, text, 0.0f);
    if (text_size.x == 0.0f && text_size.y == 0.0f) {
        text_size.x = (float)strlen(text) * 8.0f;
        text_size.y = 16.0f;
    }
    
    float padding_x = style->text.padding[0];
    float padding_y = style->text.padding[1];
    float outer_w = text_size.x + padding_x * 2.0f;
    float outer_h = text_size.y + padding_y * 2.0f;
    
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    MonstroRect text_rect = {pos.x + padding_x, pos.y + padding_y, text_size.x, text_size.y};
    
    uint32_t color = monstro_color_to_u32(style->text.color);
    canvas->addTextEx(font, text_rect, color, text, style->text.alignment);
    
    monstro_add_item(ctx, outer_w, outer_h);
}

void monstro_label_sized(MonstroContext* ctx, const char* text, float width, float height) {
    if (!ctx || !ctx->current_window || !text) return;

    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return;

    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &win->canvas;
    MonstroFont* font = style->font;

    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    MonstroRect text_rect = {pos.x, pos.y, width, height};

    uint32_t color = monstro_color_to_u32(style->text.color);
    canvas->addTextEx(font, text_rect, color, text, style->text.alignment);

    monstro_add_item(ctx, width, height);
}

void monstro_label_wrapped(MonstroContext* ctx, const char* text, float wrap_width) {
    if (!ctx || !ctx->current_window || !text) return;

    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return;

    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &win->canvas;
    MonstroFont* font = style->font;

    if (wrap_width <= 0.0f) {
        monstro::Layout* layout = win->getCurrentLayout();
        if (layout) {
            wrap_width = layout->myAvailableWidth;
        }
        if (wrap_width <= 0.0f) wrap_width = 200.0f;
    }

    MonstroVec2 text_size = monstro_font_measure_text_wrapped(font, text, wrap_width);
    if (text_size.x == 0.0f && text_size.y == 0.0f) {
        text_size.x = wrap_width;
        text_size.y = 16.0f;
    }

    float padding_x = style->text.padding[0];
    float padding_y = style->text.padding[1];
    float outer_w = wrap_width + padding_x * 2.0f;
    float outer_h = text_size.y + padding_y * 2.0f;

    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    MonstroVec2 text_pos = {pos.x + padding_x, pos.y + padding_y};

    uint32_t color = monstro_color_to_u32(style->text.color);
    canvas->addTextWrapped(font, text_pos, color, text, wrap_width);

    monstro_add_item(ctx, outer_w, outer_h);
}

void monstro_label_colored(MonstroContext* ctx, const char* text, MonstroColor color) {
    if (!ctx || !ctx->current_window || !text) return;

    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return;

    MonstroStyle* style = &ctx->style;
    monstro::Canvas* canvas = &win->canvas;
    MonstroFont* font = style->font;

    MonstroVec2 text_size = monstro_font_measure_text(font, text, 0.0f);
    if (text_size.x == 0.0f && text_size.y == 0.0f) {
        text_size.x = (float)strlen(text) * 8.0f;
        text_size.y = 16.0f;
    }

    float padding_x = style->text.padding[0];
    float padding_y = style->text.padding[1];
    float outer_w = text_size.x + padding_x * 2.0f;
    float outer_h = text_size.y + padding_y * 2.0f;

    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    MonstroRect text_rect = {pos.x + padding_x, pos.y + padding_y, text_size.x, text_size.y};

    uint32_t col = monstro_color_to_u32(color);
    canvas->addTextEx(font, text_rect, col, text, style->text.alignment);

    monstro_add_item(ctx, outer_w, outer_h);
}
