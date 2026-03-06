#include "../internal/types.h"
#include <string.h>
#include <cmath>

void monstro_separator(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;
    
    MonstroWindowInternal* win = ctx->current_window;
    if (win->skip_items) return;
    
    MonstroLayoutInternal* layout = &win->layout_stack[win->layout_stack_top];
    float avail_w = layout->size_x - (layout->cursor_x - layout->pos_x);
    
    MonstroStyle* style = &ctx->style;
    MonstroCanvas* canvas = ctx->canvas;
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    
    float pad_y = style->separator.padding[1];
    float thickness = style->separator.thickness;
    uint32_t color = monstro_color_to_u32(style->separator.color);
    
    MonstroVec2 p1 = {pos.x, pos.y + pad_y + thickness * 0.5f};
    MonstroVec2 p2 = {pos.x + avail_w, pos.y + pad_y + thickness * 0.5f};
    monstro_canvas_add_line(canvas, p1, p2, color, thickness);
    
    monstro_add_item(ctx, avail_w, pad_y * 2.0f + thickness);
}

void monstro_separator_text(MonstroContext* ctx, const char* text) {
    if (!ctx || !ctx->current_window || !text) return;
    
    MonstroWindowInternal* win = ctx->current_window;
    if (win->skip_items) return;
    
    MonstroLayoutInternal* layout = &win->layout_stack[win->layout_stack_top];
    float avail_w = layout->size_x - (layout->cursor_x - layout->pos_x);
    
    MonstroStyle* style = &ctx->style;
    MonstroCanvas* canvas = ctx->canvas;
    MonstroFont* font = style->font;
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    
    float pad_y = style->separator.padding[1];
    float thickness = style->separator.thickness;
    float text_pad_x = style->separator.text_padding[0];
    
    MonstroVec2 text_size = monstro_font_measure_text(font, text, 0.0f);
    if (text_size.x == 0.0f) text_size.x = (float)strlen(text) * 8.0f;
    if (text_size.y == 0.0f) text_size.y = 16.0f;
    
    float total_h = fmaxf(text_size.y, thickness) + pad_y * 2.0f;
    uint32_t sep_color = monstro_color_to_u32(style->separator.color);
    
    float line_y = pos.y + total_h * 0.5f;
    float text_x = pos.x + (avail_w - text_size.x) * 0.5f;
    
    MonstroVec2 p1 = {pos.x, line_y};
    MonstroVec2 p2 = {text_x - text_pad_x, line_y};
    monstro_canvas_add_line(canvas, p1, p2, sep_color, thickness);
    
    MonstroVec2 p3 = {text_x + text_size.x + text_pad_x, line_y};
    MonstroVec2 p4 = {pos.x + avail_w, line_y};
    monstro_canvas_add_line(canvas, p3, p4, sep_color, thickness);
    
    MonstroVec2 text_pos = {text_x, pos.y + (total_h - text_size.y) * 0.5f};
    monstro_canvas_add_text(canvas, font, text_pos, monstro_color_to_u32(style->text.color), text);
    
    monstro_add_item(ctx, avail_w, total_h);
}
