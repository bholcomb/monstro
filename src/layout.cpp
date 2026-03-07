#include "context.h"

/* ------------------------------------------------------------------------- */
/* Layout C API - delegates to window methods (matching C# architecture)     */
/* ------------------------------------------------------------------------- */

void monstro_begin_layout(MonstroContext* ctx, MonstroLayoutDirection direction) {
    if (!ctx || !ctx->current_window) return;
    ctx->current_window->beginLayout(direction);
}

void monstro_begin_layout_at(MonstroContext* ctx, float x, float y, MonstroLayoutDirection direction) {
    if (!ctx || !ctx->current_window) return;
    ctx->current_window->beginLayout(x, y, direction);
}

void monstro_end_layout(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;
    ctx->current_window->endLayout();
}

void monstro_add_item(MonstroContext* ctx, float width, float height) {
    if (!ctx || !ctx->current_window) return;
    ctx->current_window->addItem(width, height);
}

MonstroVec2 monstro_get_cursor_pos(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return {0.0f, 0.0f};
    return ctx->current_window->getCursorPos();
}

void monstro_set_cursor_pos(MonstroContext* ctx, float x, float y) {
    if (!ctx || !ctx->current_window) return;
    ctx->current_window->setCursorPos(x, y);
}

MonstroVec2 monstro_get_cursor_screen_pos(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return {0.0f, 0.0f};
    return ctx->current_window->getCursorScreenPos();
}

void monstro_set_cursor_screen_pos(MonstroContext* ctx, float x, float y) {
    if (!ctx || !ctx->current_window) return;
    ctx->current_window->setCursorScreenPos(x, y);
}

void monstro_indent(MonstroContext* ctx, float indent_width) {
    MonstroLayoutInternal* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return;
    
    if (indent_width <= 0.0f) {
        indent_width = ctx->style.tree.indent;
    }
    
    layout->pos_x += indent_width;
    layout->cursor_x += indent_width;
    layout->size_x -= indent_width;
}

void monstro_unindent(MonstroContext* ctx, float indent_width) {
    MonstroLayoutInternal* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return;
    
    if (indent_width <= 0.0f) {
        indent_width = ctx->style.tree.indent;
    }
    
    layout->pos_x -= indent_width;
    layout->cursor_x -= indent_width;
    layout->size_x += indent_width;
}

void monstro_spacer(MonstroContext* ctx, float size) {
    MonstroLayoutInternal* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return;
    
    if (layout->direction == MONSTRO_LAYOUT_VERTICAL) {
        layout->cursor_y += size;
    } else {
        layout->cursor_x += size;
    }
}

float monstro_percent(MonstroContext* ctx, float percent, MonstroLayoutDirection direction) {
    MonstroLayoutInternal* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return 0.0f;
    
    if (direction == MONSTRO_LAYOUT_HORIZONTAL) {
        return layout->size_x * (percent / 100.0f);
    } else {
        return layout->size_y * (percent / 100.0f);
    }
}

MonstroRect monstro_get_item_rect(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return {0, 0, 0, 0};
    MonstroLayoutInternal* layout = ctx->getCurrentLayout();
    if (!layout) return {0, 0, 0, 0};
    
    float x = ctx->current_window->pos.x + layout->pos_x + layout->cursor_x;
    float y = ctx->current_window->pos.y + layout->pos_y + layout->cursor_y;
    return {x, y, layout->size_x, 0};
}
