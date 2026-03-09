#include "../context.h"

void monstro_image(MonstroContext* ctx, uint64_t texture_id, float width, float height) {
    if (!ctx || !ctx->current_window) return;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return;
    
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    
    MonstroRect rect = {pos.x, pos.y, width, height};
    MonstroVec2 uv_min = {0.0f, 0.0f};
    MonstroVec2 uv_max = {1.0f, 1.0f};
    canvas->addImage(texture_id, rect, uv_min, uv_max, 0xFFFFFFFF);
    
    monstro_add_item(ctx, width, height);
}

void monstro_icon(MonstroContext* ctx, uint64_t texture_id, float size) {
    if (!ctx || !ctx->current_window) return;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return;
    
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    
    MonstroRect rect = {pos.x, pos.y, size, size};
    MonstroVec2 uv_min = {0.0f, 0.0f};
    MonstroVec2 uv_max = {1.0f, 1.0f};
    canvas->addImage(texture_id, rect, uv_min, uv_max, 0xFFFFFFFF);
    
    monstro_add_item(ctx, size, size);
}
