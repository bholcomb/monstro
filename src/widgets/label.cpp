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
    MonstroVec2 text_pos = {pos.x + padding_x, pos.y + padding_y};
    
    uint32_t color = monstro_color_to_u32(style->text.color);
    canvas->addText(font, text_pos, color, text);
    
    monstro_add_item(ctx, outer_w, outer_h);
}
