#include "internal/types.h"

/* ------------------------------------------------------------------------- */
/* Layout C API                                                              */
/* ------------------------------------------------------------------------- */

void monstro_begin_layout(MonstroContext* ctx, MonstroLayoutDirection direction) {
    if (!ctx || !ctx->current_window) return;
    
    MonstroWindowInternal* win = ctx->current_window;
    MonstroLayoutInternal* parent = ctx->getCurrentLayout();
    if (!parent || win->layout_stack_top >= MONSTRO_MAX_LAYOUT_STACK_DEPTH - 1) return;
    
    win->layout_stack_top++;
    MonstroLayoutInternal* layout = &win->layout_stack[win->layout_stack_top];
    
    layout->direction = direction;
    layout->pos_x = parent->cursor_x;
    layout->pos_y = parent->cursor_y;
    layout->size_x = parent->size_x - (parent->cursor_x - parent->pos_x);
    layout->size_y = parent->size_y - (parent->cursor_y - parent->pos_y);
    layout->cursor_x = layout->pos_x;
    layout->cursor_y = layout->pos_y;
    layout->max_x = layout->pos_x;
    layout->max_y = layout->pos_y;
}

void monstro_begin_layout_at(MonstroContext* ctx, float x, float y, MonstroLayoutDirection direction) {
    if (!ctx || !ctx->current_window) return;
    
    MonstroWindowInternal* win = ctx->current_window;
    if (win->layout_stack_top >= MONSTRO_MAX_LAYOUT_STACK_DEPTH - 1) return;
    
    win->layout_stack_top++;
    MonstroLayoutInternal* layout = &win->layout_stack[win->layout_stack_top];
    
    layout->direction = direction;
    layout->pos_x = x;
    layout->pos_y = y;
    layout->size_x = ctx->display_width - x;
    layout->size_y = ctx->display_height - y;
    layout->cursor_x = x;
    layout->cursor_y = y;
    layout->max_x = x;
    layout->max_y = y;
}

void monstro_end_layout(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;
    
    MonstroWindowInternal* win = ctx->current_window;
    if (win->layout_stack_top <= 0) return;
    
    MonstroLayoutInternal* finishing = &win->layout_stack[win->layout_stack_top];
    win->layout_stack_top--;
    
    float layout_width = finishing->max_x - finishing->pos_x;
    float layout_height = finishing->max_y - finishing->pos_y;
    
    monstro_add_item(ctx, layout_width, layout_height);
}

void monstro_add_item(MonstroContext* ctx, float width, float height) {
    MonstroLayoutInternal* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return;
    
    float item_right = layout->cursor_x + width;
    float item_bottom = layout->cursor_y + height;
    if (item_right > layout->max_x) layout->max_x = item_right;
    if (item_bottom > layout->max_y) layout->max_y = item_bottom;
    
    float spacing_x = ctx->style.window.spacing[0];
    float spacing_y = ctx->style.window.spacing[1];
    
    if (layout->direction == MONSTRO_LAYOUT_VERTICAL) {
        layout->cursor_y += height + spacing_y;
    } else {
        layout->cursor_x += width + spacing_x;
    }
}

MonstroVec2 monstro_get_cursor_pos(MonstroContext* ctx) {
    MonstroVec2 result = {0.0f, 0.0f};
    MonstroLayoutInternal* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (layout) {
        result.x = layout->cursor_x - layout->pos_x;
        result.y = layout->cursor_y - layout->pos_y;
    }
    return result;
}

void monstro_set_cursor_pos(MonstroContext* ctx, float x, float y) {
    MonstroLayoutInternal* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return;
    layout->cursor_x = layout->pos_x + x;
    layout->cursor_y = layout->pos_y + y;
}

MonstroVec2 monstro_get_cursor_screen_pos(MonstroContext* ctx) {
    MonstroVec2 result = {0.0f, 0.0f};
    MonstroLayoutInternal* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (layout) {
        result.x = layout->cursor_x;
        result.y = layout->cursor_y;
    }
    return result;
}

void monstro_set_cursor_screen_pos(MonstroContext* ctx, float x, float y) {
    MonstroLayoutInternal* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return;
    layout->cursor_x = x;
    layout->cursor_y = y;
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
    MonstroLayoutInternal* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return {0, 0, 0, 0};
    
    return {layout->cursor_x, layout->cursor_y, layout->size_x, 0};
}
