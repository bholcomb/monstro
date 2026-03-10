#include "../context.h"
#include <cstring>
#include <cmath>

static void draw_arrow(monstro::Canvas* canvas, float x, float y, float size, bool open, uint32_t color) {
    float half = size * 0.5f;
    if (open) {
        MonstroVec2 p1 = {x, y};
        MonstroVec2 p2 = {x + size, y};
        MonstroVec2 p3 = {x + half, y + size};
        canvas->addTriangleFilled(p1, p2, p3, color);
    } else {
        MonstroVec2 p1 = {x, y};
        MonstroVec2 p2 = {x + size, y + half};
        MonstroVec2 p3 = {x, y + size};
        canvas->addTriangleFilled(p1, p2, p3, color);
    }
}

static bool tree_item_internal(MonstroContext* ctx, uint64_t id, const char* label,
                               MonstroTreeFlags flags, bool is_leaf, bool* out_clicked) {
    monstro::Window* win = ctx->current_window;

    MonstroStyle* style = &ctx->style;
    MonstroStyleTree* ts = &style->tree;
    MonstroFont* font = style->font;
    monstro::Canvas* canvas = &win->canvas;

    const char* display = monstro_display_text(label);
    bool default_open = (flags & MONSTRO_TREE_DEFAULT_OPEN) != 0;
    bool force_selected = (flags & MONSTRO_TREE_SELECTED) != 0;

    MonstroTreeNodeState* state = ctx->findOrCreateTreeState(id, default_open);
    if (!state) return false;
    bool is_open = state->open;

    ctx->keepAliveId(id);

    monstro_add_item(ctx, ts->padding[0], ts->padding[1]);

    float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
    float arrow_size = text_h * 0.6f;
    float arrow_pad = ts->spacing[0];

    MonstroVec2 text_size = {0.0f, text_h};
    if (font) {
        text_size = monstro_font_measure_text(font, display, 0.0f);
        if (text_size.y < text_h) text_size.y = text_h;
    } else {
        text_size.x = (float)strlen(display) * 8.0f;
    }

    float row_w = arrow_size + arrow_pad + text_size.x + ts->padding[0] * 2.0f;
    float row_h = text_size.y + ts->padding[1] * 2.0f;

    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    MonstroRect row_rect = {pos.x, pos.y, row_w, row_h};

    bool hovered = false;
    if (!ctx->disabled && win == ctx->hovered_window) {
        float mx = ctx->mouse.pos_x;
        float my = ctx->mouse.pos_y;
        hovered = mx >= row_rect.x && mx < row_rect.x + row_rect.w &&
                  my >= row_rect.y && my < row_rect.y + row_rect.h;
        if (hovered) {
            ctx->hovered_id = id;
        }
    }

    bool clicked = hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false);
    if (clicked && !is_leaf) {
        state->open = !state->open;
        is_open = state->open;
    }

    if (out_clicked) *out_clicked = clicked;

    MonstroStyleItem* bg = &ts->normal;
    MonstroColor text_col = ts->text_normal;
    if (force_selected) {
        bg = &ts->selected;
        text_col = ts->text_selected;
    } else if (clicked) {
        bg = &ts->active;
        text_col = ts->text_active;
    } else if (hovered) {
        bg = &ts->hover;
        text_col = ts->text_hover;
    }

    if (hovered || force_selected) {
        if (bg->type == MONSTRO_STYLE_ITEM_COLOR) {
            canvas->addRectFilled(row_rect, monstro_color_to_u32(bg->data.color), ts->rounding, MONSTRO_CORNER_ALL);
        }
    }

    float content_x = pos.x + ts->padding[0];
    float content_y = pos.y + ts->padding[1];

    if (!is_leaf) {
        float arrow_y = content_y + (text_size.y - arrow_size) * 0.5f;
        draw_arrow(canvas, content_x, arrow_y, arrow_size, is_open, monstro_color_to_u32(text_col));
    }

    float label_x = content_x + arrow_size + arrow_pad;
    if (font) {
        MonstroRect label_rect = {label_x, content_y, text_size.x, text_size.y};
        canvas->addTextEx(font, label_rect, monstro_color_to_u32(text_col), display, MONSTRO_ALIGN_DEFAULT);
    }

    monstro_draw_disabled_overlay(ctx, row_rect);
    monstro_add_item(ctx, row_w, row_h);
    monstro_set_last_item(ctx, id, row_rect, hovered, false, clicked, false);

    if (hovered) {
        monstro_tooltip(ctx, nullptr);
    }

    if (is_open && !is_leaf) {
        monstro_indent(ctx, ts->indent);
        ctx->id_stack.push(label);
        return true;
    }

    return false;
}

bool monstro_begin_tree_node_id(MonstroContext* ctx, uint64_t id, const char* label, MonstroTreeFlags flags) {
    if (!ctx || !ctx->current_window || !label) return false;
    if (ctx->current_window->skip_items) return false;
    return tree_item_internal(ctx, id, label, flags, false, nullptr);
}

bool monstro_begin_tree_node(MonstroContext* ctx, const char* label, MonstroTreeFlags flags) {
    if (!ctx || !ctx->current_window || !label) return false;
    uint64_t id = ctx->current_window->getChildId(label);
    return monstro_begin_tree_node_id(ctx, id, label, flags);
}

void monstro_end_tree_node(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;
    ctx->id_stack.pop();
    monstro_unindent(ctx, 0.0f);
}

bool monstro_tree_leaf(MonstroContext* ctx, const char* label, MonstroTreeFlags flags) {
    if (!ctx || !ctx->current_window || !label) return false;
    if (ctx->current_window->skip_items) return false;
    uint64_t id = ctx->current_window->getChildId(label);
    bool clicked = false;
    tree_item_internal(ctx, id, label, flags, true, &clicked);
    return clicked;
}

bool monstro_collapsing_header(MonstroContext* ctx, const char* label, bool* p_visible) {
    if (!ctx || !ctx->current_window || !label) return false;

    monstro::Window* win = ctx->current_window;
    if (win->skip_items) return false;

    MonstroStyle* style = &ctx->style;
    MonstroStyleTree* ts = &style->tree;
    MonstroFont* font = style->font;
    monstro::Canvas* canvas = &win->canvas;

    uint64_t id = win->getChildId(label);
    const char* display = monstro_display_text(label);
    MonstroTreeNodeState* state = ctx->findOrCreateTreeState(id, true);
    if (!state) return false;
    bool is_open = state->open;

    ctx->keepAliveId(id);

    monstro_add_item(ctx, ts->padding[0], ts->padding[1]);

    float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
    float arrow_size = text_h * 0.6f;
    float arrow_pad = ts->spacing[0];

    MonstroVec2 text_size = {0.0f, text_h};
    if (font) {
        text_size = monstro_font_measure_text(font, display, 0.0f);
        if (text_size.y < text_h) text_size.y = text_h;
    } else {
        text_size.x = (float)strlen(display) * 8.0f;
    }

    float avail_w = win->size.x - style->window.padding[0] * 2.0f;
    monstro::Layout* layout = win->getCurrentLayout();
    if (layout && layout->myAvailableWidth > 0.0f) {
        avail_w = layout->myAvailableWidth;
    }

    float row_h = text_size.y + ts->padding[1] * 2.0f;

    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    MonstroRect row_rect = {pos.x, pos.y, avail_w, row_h};

    bool hovered = false;
    if (!ctx->disabled && win == ctx->hovered_window) {
        float mx = ctx->mouse.pos_x;
        float my = ctx->mouse.pos_y;
        hovered = mx >= row_rect.x && mx < row_rect.x + row_rect.w &&
                  my >= row_rect.y && my < row_rect.y + row_rect.h;
        if (hovered) ctx->hovered_id = id;
    }

    bool clicked = hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false);
    if (clicked) {
        state->open = !state->open;
        is_open = state->open;
    }

    MonstroStyleItem* bg = &ts->active;
    MonstroColor text_col = ts->text_normal;
    if (clicked) {
        text_col = ts->text_active;
    } else if (hovered) {
        bg = &ts->hover;
        text_col = ts->text_hover;
    }

    if (bg->type == MONSTRO_STYLE_ITEM_COLOR) {
        canvas->addRectFilled(row_rect, monstro_color_to_u32(bg->data.color), ts->rounding, MONSTRO_CORNER_ALL);
    }

    float content_x = pos.x + ts->padding[0];
    float content_y = pos.y + ts->padding[1];

    float arrow_y = content_y + (text_size.y - arrow_size) * 0.5f;
    draw_arrow(canvas, content_x, arrow_y, arrow_size, is_open, monstro_color_to_u32(text_col));

    float label_x = content_x + arrow_size + arrow_pad;
    if (font) {
        MonstroRect label_rect = {label_x, content_y, text_size.x, text_size.y};
        canvas->addTextEx(font, label_rect, monstro_color_to_u32(text_col), display, MONSTRO_ALIGN_DEFAULT);
    }

    if (p_visible) {
        float close_size = text_h * 0.7f;
        float close_x = pos.x + avail_w - close_size - ts->padding[0];
        float close_y = content_y + (text_size.y - close_size) * 0.5f;
        MonstroRect close_rect = {close_x, close_y, close_size, close_size};

        bool close_hovered = false;
        if (!ctx->disabled && win == ctx->hovered_window) {
            float mx = ctx->mouse.pos_x;
            float my = ctx->mouse.pos_y;
            close_hovered = mx >= close_rect.x && mx < close_rect.x + close_rect.w &&
                            my >= close_rect.y && my < close_rect.y + close_rect.h;
        }

        if (close_hovered) {
            uint32_t hover_col = monstro_color_to_u32(ts->hover.data.color);
            canvas->addRectFilled(close_rect, hover_col, ts->rounding, MONSTRO_CORNER_ALL);
            if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false)) {
                *p_visible = false;
            }
        }

        float cx = close_rect.x + close_size * 0.2f;
        float cy = close_rect.y + close_size * 0.2f;
        float cs = close_size * 0.6f;
        uint32_t x_col = monstro_color_to_u32(text_col);
        canvas->addLine({cx, cy}, {cx + cs, cy + cs}, x_col, 1.0f);
        canvas->addLine({cx + cs, cy}, {cx, cy + cs}, x_col, 1.0f);
    }

    monstro_draw_disabled_overlay(ctx, row_rect);
    monstro_add_item(ctx, avail_w, row_h);
    monstro_set_last_item(ctx, id, row_rect, hovered, false, clicked, false);

    return is_open;
}

float monstro_get_tree_indent(MonstroContext* ctx) {
    if (!ctx) return 0.0f;
    return ctx->style.tree.indent;
}

void monstro_set_tree_indent(MonstroContext* ctx, float indent) {
    if (!ctx) return;
    ctx->style.tree.indent = indent;
}
