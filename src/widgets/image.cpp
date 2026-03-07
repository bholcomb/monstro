#include "../context.h"

void monstro_image(MonstroContext* ctx, uint64_t texture_id, float width, float height) {
    monstro_image_ex(ctx, texture_id, width, height, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF);
}

void monstro_image_ex(MonstroContext* ctx, uint64_t texture_id, float width, float height,
                      float uv0_x, float uv0_y, float uv1_x, float uv1_y, uint32_t tint_col) {
    if (!ctx || !ctx->current_window) return;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return;
    
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    
    MonstroRect rect = {pos.x, pos.y, width, height};
    MonstroVec2 uv_min = {uv0_x, uv0_y};
    MonstroVec2 uv_max = {uv1_x, uv1_y};
    canvas->addImage(texture_id, rect, uv_min, uv_max, tint_col);
    
    monstro_add_item(ctx, width, height);
}

void monstro_icon(MonstroContext* ctx, uint64_t texture_id, float size) {
    monstro_icon_colored(ctx, texture_id, size, 0xFFFFFFFF);
}

void monstro_icon_colored(MonstroContext* ctx, uint64_t texture_id, float size, uint32_t color) {
    if (!ctx || !ctx->current_window) return;
    
    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return;
    
    monstro::Canvas* canvas = &ctx->current_window->canvas;
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    
    MonstroRect rect = {pos.x, pos.y, size, size};
    MonstroVec2 uv_min = {0.0f, 0.0f};
    MonstroVec2 uv_max = {1.0f, 1.0f};
    canvas->addImage(texture_id, rect, uv_min, uv_max, color);
    
    monstro_add_item(ctx, size, size);
}
