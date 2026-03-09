#include "../context.h"

void monstro_begin_group(MonstroContext* ctx, MonstroLayoutDirection layout, MonstroGroupFlags flags) {
    if (!ctx || !ctx->current_window) return;

    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return;

    MonstroStyle* style = &ctx->style;
    float pad_x = style->window.group_padding[0];
    float pad_y = style->window.group_padding[1];

    monstro_add_item(ctx, pad_x, pad_y);

    if (win->group_stack_top >= MONSTRO_MAX_LAYOUT_STACK_DEPTH - 1) return;
    win->group_stack_top++;
    win->group_flags_stack[win->group_stack_top] = flags;

    win->canvas.pushLayer(win->canvas.layer_stack[win->canvas.layer_stack_top] + 1);

    win->beginLayout(layout);

    monstro::Layout* l = win->getCurrentLayout();
    if (l) {
        l->myPosX += pad_x;
        l->myPosY += pad_y;
    }
}

void monstro_end_group(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;

    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return;
    if (win->group_stack_top < 0) return;

    MonstroGroupFlags flags = win->group_flags_stack[win->group_stack_top];
    win->group_stack_top--;

    MonstroStyle* style = &ctx->style;
    float pad_x = style->window.group_padding[0];
    float pad_y = style->window.group_padding[1];

    monstro::Layout* layout = win->getCurrentLayout();
    if (!layout) return;

    float group_w = layout->mySizeX + pad_x * 2.0f;
    float group_h = layout->mySizeY + pad_y * 2.0f;
    float group_x = win->pos.x + layout->myPosX - pad_x + layout->myScrollOffsetX;
    float group_y = win->pos.y + layout->myPosY - pad_y + layout->myScrollOffsetY;
    MonstroRect group_rect = {group_x, group_y, group_w, group_h};

    layout->mySizeX += pad_x * 2.0f;
    layout->mySizeY += pad_y * 2.0f;

    win->canvas.popLayer();

    if (flags & MONSTRO_GROUP_BACKGROUND) {
        MonstroStyleItem* bg = &style->window.fixed_background;
        if (bg->type == MONSTRO_STYLE_ITEM_COLOR) {
            win->canvas.addRectFilled(group_rect, monstro_color_to_u32(bg->data.color),
                                      style->window.rounding, MONSTRO_CORNER_ALL);
        } else if (bg->type == MONSTRO_STYLE_ITEM_IMAGE) {
            win->canvas.addImage(bg->data.texture_id, group_rect, {0, 0}, {1, 1}, 0xFFFFFFFF);
        } else if (bg->type == MONSTRO_STYLE_ITEM_NINEPATCH) {
            win->canvas.addNinePatch(&bg->data.ninepatch, group_rect, 0xFFFFFFFF);
        }
    }

    if (flags & MONSTRO_GROUP_BORDER) {
        float border = style->window.group_border;
        if (border > 0.0f) {
            win->canvas.addRect(group_rect, monstro_color_to_u32(style->window.group_border_color),
                                style->window.rounding, MONSTRO_CORNER_ALL, border);
        }
    }

    win->endLayout();
}
