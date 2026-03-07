#include "../context.h"
#include <string.h>

void monstro_label(MonstroContext* ctx, const char* text) {
    if (!ctx || !ctx->current_window || !text) return;
    
    uint32_t color = monstro_color_to_u32(ctx->style.text.color);
    monstro_label_ex(ctx, text, color);
}

void monstro_label_ex(MonstroContext* ctx, const char* text, uint32_t color) {
    if (!ctx || !ctx->current_window || !text) return;
    
    MonstroWindowInternal* win = ctx->current_window;
    if (win->skip_items) return;
    
    MonstroStyle* style = &ctx->style;
    MonstroCanvas* canvas = ctx->canvas;
    MonstroFont* font = style->font;
    
    MonstroVec2 text_size = monstro_font_measure_text(font, text, 0.0f);
    if (text_size.x == 0.0f && text_size.y == 0.0f) {
        text_size.x = (float)strlen(text) * 8.0f;
        text_size.y = 16.0f;
    }
    
    // C# pattern: outer size includes padding, text is drawn inside
    float padding_x = style->text.padding[0];
    float padding_y = style->text.padding[1];
    float outer_w = text_size.x + padding_x * 2.0f;
    float outer_h = text_size.y + padding_y * 2.0f;
    
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    MonstroVec2 text_pos = {pos.x + padding_x, pos.y + padding_y};
    
    monstro_canvas_add_text(canvas, font, text_pos, color, text);
    
    monstro_add_item(ctx, outer_w, outer_h);
}
