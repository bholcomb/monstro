#include "context.h"
#include "drawbuffer.h"
#include <cstring>
#include <cmath>
#include <algorithm>

static bool icon_lookup_from_context(void* user_data, uint32_t icon_id,
                                     uint64_t* out_texture_id,
                                     MonstroVec2* out_uv_min,
                                     MonstroVec2* out_uv_max,
                                     bool* out_monochrome) {
    auto* ctx = (monstro::Context*)user_data;
    if (!ctx || icon_id == 0) return false;
    auto it = ctx->icon_map.find(icon_id);
    if (it == ctx->icon_map.end()) return false;
    const MonstroIconEntry& e = it->second;
    if (out_texture_id) *out_texture_id = e.texture_id;
    if (out_uv_min) *out_uv_min = e.uv_min;
    if (out_uv_max) *out_uv_max = e.uv_max;
    if (out_monochrome) *out_monochrome = e.is_monochrome;
    return true;
}

namespace monstro {

/* ------------------------------------------------------------------------- */
/* Window Member Functions                                                   */
/* ------------------------------------------------------------------------- */

void Window::init(Context* context, const char* windowName, MonstroWindowFlags windowFlags) {
    std::memset(this, 0, sizeof(Window));
    ctx = context;

    std::strncpy(name, windowName, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    id = ctx->id_stack.getId(windowName);
    move_id = getChildId("MOVE");
    flags = windowFlags;

    set_pos_allow_flags = (MonstroCondition)(MONSTRO_COND_ALWAYS | MONSTRO_COND_ONCE |
                                              MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING);
    set_size_allow_flags = (MonstroCondition)(MONSTRO_COND_ALWAYS | MONSTRO_COND_ONCE |
                                               MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING);
}

uint64_t Window::getChildId(const char* childName) {
    uint64_t childId = ctx->id_stack.getId(childName);
    ctx->keepAliveId(childId);
    return childId;
}

/* ------------------------------------------------------------------------- */
/* Begin / End                                                               */
/* ------------------------------------------------------------------------- */

bool Window::begin(bool* p_closed) {
    if (p_closed && *p_closed == true)
        return false;

    p_open_ref = p_closed;
    ctx->id_stack.push(id);

    if (flags & MONSTRO_WINDOW_MODAL) {
        makeLastSibling();
    }

    if (ctx->next_window_pos_cond != MONSTRO_COND_NONE) {
        bool shouldSet = (set_pos_allow_flags & ctx->next_window_pos_cond) != 0;
        bool isValid = ctx->next_window_pos_x != -INFINITY && ctx->next_window_pos_y != -INFINITY;
        if (shouldSet && isValid) {
            setPosition(ctx->next_window_pos_x, ctx->next_window_pos_y, ctx->next_window_pos_cond);
            ctx->next_window_pos_cond = MONSTRO_COND_NONE;
            ctx->next_window_pos_x = -INFINITY;
            ctx->next_window_pos_y = -INFINITY;
        }
    }

    if (ctx->next_window_size_cond != MONSTRO_COND_NONE) {
        bool shouldSet = (set_size_allow_flags & ctx->next_window_size_cond) != 0;
        bool isValid = ctx->next_window_size_x != -INFINITY && ctx->next_window_size_y != -INFINITY;
        if (shouldSet && isValid) {
            setSize(ctx->next_window_size_x, ctx->next_window_size_y, ctx->next_window_size_cond);
            ctx->next_window_size_cond = MONSTRO_COND_NONE;
            ctx->next_window_size_x = -INFINITY;
            ctx->next_window_size_y = -INFINITY;
        }
    }

    layout_stack_top = -1;

    MonstroStyle* pad_style = &ctx->style;
    beginLayout(pad_style->window.padding[0], pad_style->window.padding[1], MONSTRO_LAYOUT_VERTICAL);
    if (layout_stack_top >= 0) {
        layout_stack[layout_stack_top].myScrollOffsetX = 0.0f;
        layout_stack[layout_stack_top].myScrollOffsetY = -scroll_y;
        float avail_w = size.x - pad_style->window.padding[0] * 2.0f;
        if (!(flags & MONSTRO_WINDOW_AUTO_RESIZE) &&
            content_height > (size.y - getTitleBarHeight() - getMenuBarHeight())) {
            avail_w -= pad_style->window.scrollbar_size[0];
        }
        layout_stack[layout_stack_top].myAvailableWidth = avail_w;
        layout_stack[layout_stack_top].myAvailableHeight = size.y - pad_style->window.padding[1] * 2.0f
            - getTitleBarHeight() - getMenuBarHeight();
    }

    if (last_frame_active != ctx->frame) {
        last_frame_active = ctx->frame;
        active = true;
    }

    if (ctx->current_window != this) {
        ctx->pushWindow(this);
    }

    canvas.init(&ctx->draw_buffer);
    canvas.icon_lookup = icon_lookup_from_context;
    canvas.icon_lookup_user_data = ctx;

    if ((flags & MONSTRO_WINDOW_CHILD) && parent) {
        MonstroRect parent_clip = parent->canvas.getClipRect();
        canvas.clip_rect_stack[0] = parent_clip;
    }

    MonstroStyle* style = &ctx->style;

    if ((flags & MONSTRO_WINDOW_TITLE_BAR) && (flags & MONSTRO_WINDOW_COLLAPSABLE)) {
        MonstroRect title_rect = getTitleBarRect();
        if (ctx->hovered_window == this &&
            monstro_rect_contains(title_rect, ctx->mouse.pos_x, ctx->mouse.pos_y) &&
            ctx->mouse.isDoubleClicked(MONSTRO_MOUSE_BUTTON_LEFT)) {
            collapsed = !collapsed;
        }
    } else {
        collapsed = false;
    }

    ctx->keepAliveId(move_id);
    if (ctx->active_id == move_id) {
        if (ctx->mouse.isDown(MONSTRO_MOUSE_BUTTON_MIDDLE)) {
            if (flags & MONSTRO_WINDOW_MOVABLE) {
                move(ctx->mouse.delta_x, ctx->mouse.delta_y);
            }
            ctx->focusWindow(this);
        } else {
            ctx->setActiveId(0);
        }
    }

    drawBackground();

    skip_items = collapsed;

    drawForeground();

    content_clip_pushed = false;
    if (!collapsed && !(flags & MONSTRO_WINDOW_MENU_BAR)) {
        pushContentClipRect();
    }

    if (!collapsed && ctx->hovered_window == this) {
        bool scroll_with_mouse = (flags & MONSTRO_WINDOW_SCROLL_WITH_MOUSE) != 0;
        if (scroll_with_mouse && ctx->mouse.wheel_y != 0.0f) {
            float scroll_speed = 40.0f;
            scroll_y -= ctx->mouse.wheel_y * scroll_speed;

            float header_h = getTitleBarHeight() + getMenuBarHeight();
            float visible_h = size.y - header_h;
            float max_scroll = content_height - visible_h;
            if (max_scroll < 0.0f) max_scroll = 0.0f;
            if (scroll_y > max_scroll) scroll_y = max_scroll;
            if (scroll_y < 0.0f) scroll_y = 0.0f;
        }
    }

    return true;
}

bool Window::end() {
    MonstroStyle* style = &ctx->style;

    if (content_clip_pushed) {
        canvas.popClipRect();
    }

    Layout* layout = getCurrentLayout();
    if (layout) {
        float header_offset = getTitleBarHeight() + getMenuBarHeight();
        content_width = layout->mySizeX + style->window.padding[0] * 2.0f;
        content_height = layout->mySizeY - header_offset + style->window.padding[1] * 2.0f;
        if (content_height < 0.0f) content_height = 0.0f;
    }

    if (flags & MONSTRO_WINDOW_AUTO_RESIZE) {
        float header_for_auto = getTitleBarHeight() + getMenuBarHeight();
        if (content_width > 0.0f) size.x = content_width;
        if (content_height > 0.0f) size.y = content_height + header_for_auto;
    }

    float header_h = getTitleBarHeight() + getMenuBarHeight();
    float visible_h = size.y - header_h;
    float max_scroll = content_height - visible_h;
    if (max_scroll < 0.0f) max_scroll = 0.0f;
    if (scroll_y > max_scroll) scroll_y = max_scroll;
    if (scroll_y < 0.0f) scroll_y = 0.0f;

    if (!(flags & MONSTRO_WINDOW_AUTO_RESIZE)) {
        drawScrollbar(max_scroll, visible_h, header_h);
    }

    if ((flags & MONSTRO_WINDOW_RESIZABLE) && !collapsed) {
        float grip_sz = 14.0f;
        float grip_x = pos.x + size.x - grip_sz;
        float grip_y = pos.y + size.y - grip_sz;
        MonstroRect grip_rect = {grip_x, grip_y, grip_sz, grip_sz};

        uint64_t resize_id = getChildId("##resize");
        ctx->keepAliveId(resize_id);

        bool grip_hovered = ctx->hovered_window == this &&
            monstro_rect_contains(grip_rect, ctx->mouse.pos_x, ctx->mouse.pos_y);

        if (grip_hovered && ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT)) {
            ctx->setActiveId(resize_id);
        }

        bool grip_held = (ctx->active_id == resize_id);
        if (grip_held) {
            if (ctx->mouse.isDown(MONSTRO_MOUSE_BUTTON_LEFT)) {
                size.x += ctx->mouse.delta_x;
                size.y += ctx->mouse.delta_y;
                if (size.x < style->window.min_size[0]) size.x = style->window.min_size[0];
                if (size.y < style->window.min_size[1]) size.y = style->window.min_size[1];
            } else {
                ctx->setActiveId(0);
            }
        }

        uint32_t grip_col;
        if (grip_held) {
            grip_col = monstro_color_to_u32(style->window.header.active.data.color);
        } else if (grip_hovered) {
            grip_col = monstro_color_to_u32(style->window.header.hover.data.color);
        } else {
            grip_col = monstro_color_to_u32(style->window.scaler.data.color);
        }

        MonstroVec2 t1 = {pos.x + size.x, pos.y + size.y - grip_sz};
        MonstroVec2 t2 = {pos.x + size.x - grip_sz, pos.y + size.y};
        MonstroVec2 t3 = {pos.x + size.x, pos.y + size.y};
        canvas.addTriangleFilled(t1, t2, t3, grip_col);
    }

    canvas.popClipRect();

    layout_stack_top = -1;
    p_open_ref = nullptr;
    ctx->id_stack.pop();
    return true;
}

/* ------------------------------------------------------------------------- */
/* Drawing                                                                   */
/* ------------------------------------------------------------------------- */

void Window::drawScrollbar(float max_scroll, float visible_h, float header_h) {
    if (max_scroll <= 0.0f) return;
    if (collapsed) return;

    MonstroStyle* style = &ctx->style;
    MonstroStyleScrollbar* ss = &style->scrollv;
    float bar_w = style->window.scrollbar_size[0];
    if (bar_w <= 0.0f) bar_w = 10.0f;

    float track_x = pos.x + size.x - bar_w;
    float track_y = pos.y + header_h;
    float track_h = visible_h;
    MonstroRect track_rect = {track_x, track_y, bar_w, track_h};

    if (ss->normal.type == MONSTRO_STYLE_ITEM_COLOR) {
        canvas.addRectFilled(track_rect, monstro_color_to_u32(ss->normal.data.color),
                             ss->rounding, MONSTRO_CORNER_ALL);
    }

    float thumb_ratio = visible_h / content_height;
    float thumb_h = track_h * thumb_ratio;
    float min_thumb_h = 20.0f;
    if (thumb_h < min_thumb_h) thumb_h = min_thumb_h;

    float scroll_ratio = (max_scroll > 0.0f) ? (scroll_y / max_scroll) : 0.0f;
    float thumb_y = track_y + scroll_ratio * (track_h - thumb_h);

    MonstroRect thumb_rect = {track_x + ss->padding[0], thumb_y, bar_w - ss->padding[0] * 2.0f, thumb_h};

    bool thumb_hovered = false;
    bool thumb_held = false;
    uint64_t sb_id = ctx->id_stack.getId("##vscroll");
    ctx->keepAliveId(sb_id);

    if (ctx->hovered_window == this) {
        float mx = ctx->mouse.pos_x;
        float my = ctx->mouse.pos_y;
        thumb_hovered = mx >= track_rect.x && mx < track_rect.x + track_rect.w &&
                        my >= track_rect.y && my < track_rect.y + track_rect.h;

        if (thumb_hovered && ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT)) {
            ctx->setActiveId(sb_id);
        }
    }

    if (ctx->active_id == sb_id) {
        if (ctx->mouse.isDown(MONSTRO_MOUSE_BUTTON_LEFT)) {
            thumb_held = true;
            float delta = ctx->mouse.delta_y;
            float scrollable_track = track_h - thumb_h;
            if (scrollable_track > 0.0f) {
                scroll_y += delta * (max_scroll / scrollable_track);
                if (scroll_y < 0.0f) scroll_y = 0.0f;
                if (scroll_y > max_scroll) scroll_y = max_scroll;
            }
        } else {
            ctx->setActiveId(0);
        }
    }

    MonstroStyleItem* cursor_style = &ss->cursor_normal;
    if (thumb_held) cursor_style = &ss->cursor_active;
    else if (thumb_hovered) cursor_style = &ss->cursor_hover;

    if (cursor_style->type == MONSTRO_STYLE_ITEM_COLOR) {
        canvas.addRectFilled(thumb_rect, monstro_color_to_u32(cursor_style->data.color),
                             ss->rounding_cursor, MONSTRO_CORNER_ALL);
    }

    if (ss->border > 0.0f) {
        canvas.addRect(track_rect, monstro_color_to_u32(ss->border_color),
                       ss->rounding, MONSTRO_CORNER_ALL, ss->border);
    }
    if (ss->border_cursor > 0.0f) {
        canvas.addRect(thumb_rect, monstro_color_to_u32(ss->cursor_border_color),
                       ss->rounding_cursor, MONSTRO_CORNER_ALL, ss->border_cursor);
    }
}

void Window::pushContentClipRect() {
    if (content_clip_pushed) return;
    float header_h = getTitleBarHeight() + getMenuBarHeight();
    MonstroRect content_clip = {pos.x, pos.y + header_h, size.x, size.y - header_h};
    canvas.pushClipRect(content_clip);
    content_clip_pushed = true;
}

void Window::drawBackground() {
    MonstroStyle* style = &ctx->style;

    float saved_rounding = style->window.rounding;

    if (flags & MONSTRO_WINDOW_ROOT) {
        canvas.pushClipRectFullscreen();
        style->window.rounding = 0.0f;
    } else if ((flags & MONSTRO_WINDOW_AUTO_RESIZE) && size.y <= 0.0f) {
        canvas.pushClipRectFullscreen();
    } else {
        MonstroRect clip_rect = getRect();
        clip_rect.w += 1;
        clip_rect.h += 1;
        canvas.pushClipRect(clip_rect);
    }

    if (!collapsed && (flags & MONSTRO_WINDOW_BACKGROUND)) {
        MonstroStyleItem* bg = &style->window.fixed_background;
        MonstroRect r = getRect();
        switch (bg->type) {
            case MONSTRO_STYLE_ITEM_COLOR:
                canvas.addRectFilled(r, monstro_color_to_u32(bg->data.color), style->window.rounding, MONSTRO_CORNER_ALL);
                break;
            case MONSTRO_STYLE_ITEM_IMAGE:
                canvas.addImage(bg->data.texture_id, r, {0, 0}, {1, 1}, 0xFFFFFFFF);
                break;
            case MONSTRO_STYLE_ITEM_SPRITE:
                canvas.addImage(bg->data.sprite.texture_id, r, bg->data.sprite.uv_min, bg->data.sprite.uv_max, 0xFFFFFFFF);
                break;
            case MONSTRO_STYLE_ITEM_NINEPATCH:
                canvas.addNinePatch(&bg->data.ninepatch, r, 0xFFFFFFFF);
                break;
        }
    }

    if (flags & MONSTRO_WINDOW_ROOT) {
        style->window.rounding = saved_rounding;
    }
}

static void draw_collapse_arrow(Canvas& canvas, float cx, float cy, float sz, bool pointing_right, uint32_t color) {
    float half = sz * 0.5f;
    if (pointing_right) {
        MonstroVec2 p1 = {cx - half * 0.4f, cy - half};
        MonstroVec2 p2 = {cx + half * 0.6f, cy};
        MonstroVec2 p3 = {cx - half * 0.4f, cy + half};
        canvas.addTriangleFilled(p1, p2, p3, color);
    } else {
        MonstroVec2 p1 = {cx - half, cy - half * 0.4f};
        MonstroVec2 p2 = {cx + half, cy - half * 0.4f};
        MonstroVec2 p3 = {cx, cy + half * 0.6f};
        canvas.addTriangleFilled(p1, p2, p3, color);
    }
}

void Window::drawForeground() {
    MonstroStyle* style = &ctx->style;

    float saved_rounding = style->window.rounding;
    if (flags & MONSTRO_WINDOW_ROOT) {
        style->window.rounding = 0.0f;
    }

    if (collapsed) {
        MonstroRect title_rect = getTitleBarRect();
        canvas.addRectFilled(title_rect, monstro_color_to_u32(style->window.header.active.data.color), style->window.rounding, MONSTRO_CORNER_ALL);

        float btn_sz = title_rect.h * 0.6f;
        float arrow_x = title_rect.x + style->window.header.padding[0] + btn_sz * 0.5f;
        float arrow_y = title_rect.y + title_rect.h * 0.5f;

        uint64_t collapse_id = getChildId("##collapse");
        MonstroRect collapse_hit = {title_rect.x + style->window.header.padding[0], title_rect.y, btn_sz + 4.0f, title_rect.h};
        bool col_hovered = ctx->hovered_window == this &&
            monstro_rect_contains(collapse_hit, ctx->mouse.pos_x, ctx->mouse.pos_y);

        if (col_hovered) {
            ctx->hovered_id = collapse_id;
            if (ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT)) {
                collapsed = false;
            }
        }

        uint32_t arrow_col = monstro_color_to_u32(style->window.header.label_normal);
        draw_collapse_arrow(canvas, arrow_x, arrow_y, btn_sz, true, arrow_col);

        float text_left = arrow_x + btn_sz * 0.5f + 4.0f;
        MonstroRect text_rect = {text_left, title_rect.y, title_rect.w - (text_left - title_rect.x), title_rect.h};
        if (style->font) {
            canvas.addTextEx(style->font, text_rect, monstro_color_to_u32(style->window.header.label_normal), name, MONSTRO_ALIGN_LEFT);
        }
    } else {
        if (flags & MONSTRO_WINDOW_TITLE_BAR) {
            MonstroRect title_rect = getTitleBarRect();
            uint32_t header_color = monstro_color_to_u32(style->window.header.active.data.color);
            canvas.addRectFilled(title_rect, header_color, style->window.rounding, MONSTRO_CORNER_TOP);

            float btn_sz = title_rect.h * 0.6f;
            float content_left = title_rect.x + style->window.header.padding[0];
            float content_right = title_rect.x + title_rect.w - style->window.header.padding[0];

            if (flags & MONSTRO_WINDOW_COLLAPSABLE) {
                float arrow_x = content_left + btn_sz * 0.5f;
                float arrow_y = title_rect.y + title_rect.h * 0.5f;

                uint64_t collapse_id = getChildId("##collapse");
                MonstroRect collapse_hit = {content_left, title_rect.y, btn_sz + 4.0f, title_rect.h};
                bool col_hovered = ctx->hovered_window == this &&
                    monstro_rect_contains(collapse_hit, ctx->mouse.pos_x, ctx->mouse.pos_y);

                if (col_hovered) {
                    ctx->hovered_id = collapse_id;
                    if (ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT)) {
                        collapsed = true;
                    }
                }

                uint32_t arrow_col = monstro_color_to_u32(style->window.header.label_normal);
                draw_collapse_arrow(canvas, arrow_x, arrow_y, btn_sz, false, arrow_col);

                content_left += btn_sz + 4.0f;
            }

            if (p_open_ref != nullptr) {
                float close_x = content_right - btn_sz;
                float close_y = title_rect.y + (title_rect.h - btn_sz) * 0.5f;
                MonstroRect close_rect = {close_x, close_y, btn_sz, btn_sz};

                uint64_t close_id = getChildId("##close");
                bool close_hovered = ctx->hovered_window == this &&
                    monstro_rect_contains(close_rect, ctx->mouse.pos_x, ctx->mouse.pos_y);

                uint32_t close_col = monstro_color_to_u32(style->window.header.label_normal);
                if (close_hovered) {
                    ctx->hovered_id = close_id;
                    canvas.addRectFilled(close_rect, monstro_color_to_u32(style->window.header.hover.data.color),
                                         style->window.rounding, MONSTRO_CORNER_ALL);
                    if (ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT)) {
                        *p_open_ref = true;
                    }
                }

                float pad = btn_sz * 0.2f;
                MonstroVec2 x1a = {close_rect.x + pad, close_rect.y + pad};
                MonstroVec2 x1b = {close_rect.x + btn_sz - pad, close_rect.y + btn_sz - pad};
                MonstroVec2 x2a = {close_rect.x + btn_sz - pad, close_rect.y + pad};
                MonstroVec2 x2b = {close_rect.x + pad, close_rect.y + btn_sz - pad};
                canvas.addLine(x1a, x1b, close_col, 2.0f);
                canvas.addLine(x2a, x2b, close_col, 2.0f);

                content_right -= btn_sz + 4.0f;
            }

            MonstroRect text_rect = {content_left, title_rect.y, content_right - content_left, title_rect.h};
            uint32_t label_color = monstro_color_to_u32(style->window.header.label_normal);
            if (style->font) {
                canvas.addTextEx(style->font, text_rect, label_color, name, MONSTRO_ALIGN_CENTER);
            }

            Layout* l = getCurrentLayout();
            if (l) l->myCursorY += title_rect.h;
        }

        if (flags & MONSTRO_WINDOW_MENU_BAR) {
            MonstroRect menu_rect = getMenuBarRect();
            canvas.addRectFilled(menu_rect, monstro_color_to_u32(style->window.menu_border_color), 0, MONSTRO_CORNER_NONE);
        }

        if (flags & MONSTRO_WINDOW_BORDERS) {
            MonstroStyleItem* border = &style->window.border_style;
            MonstroRect r = getRect();
            switch (border->type) {
                case MONSTRO_STYLE_ITEM_COLOR:
                    canvas.addRect(r, monstro_color_to_u32(border->data.color), style->window.rounding, MONSTRO_CORNER_ALL, style->window.border);
                    break;
                case MONSTRO_STYLE_ITEM_IMAGE:
                    canvas.addImage(border->data.texture_id, r, {0, 0}, {1, 1}, 0xFFFFFFFF);
                    break;
                case MONSTRO_STYLE_ITEM_SPRITE:
                    canvas.addImage(border->data.sprite.texture_id, r, border->data.sprite.uv_min, border->data.sprite.uv_max, 0xFFFFFFFF);
                    break;
                case MONSTRO_STYLE_ITEM_NINEPATCH:
                    canvas.addNinePatch(&border->data.ninepatch, r, 0xFFFFFFFF);
                    break;
            }

            if (!(flags & MONSTRO_WINDOW_TITLE_BAR)) {
                MonstroRect title_rect = getTitleBarRect();
                MonstroVec2 p1 = {title_rect.x + 1.0f, title_rect.y};
                MonstroVec2 p2 = {title_rect.x + title_rect.w - 1.0f, title_rect.y};
                canvas.addLine(p1, p2, monstro_color_to_u32(style->window.border_color), style->window.border);
            }
        }
    }

    if (flags & MONSTRO_WINDOW_ROOT) {
        style->window.rounding = saved_rounding;
    }
}

/* ------------------------------------------------------------------------- */
/* Position / Cursor                                                         */
/* ------------------------------------------------------------------------- */

MonstroVec2 Window::getCursorPos() const {
    if (layout_stack_top < 0) return {0.0f, 0.0f};
    const Layout* l = &layout_stack[layout_stack_top];
    return {l->myPosX + l->myCursorX, l->myPosY + l->myCursorY};
}

void Window::setCursorPos(float x, float y) {
    if (layout_stack_top < 0) return;
    Layout* l = &layout_stack[layout_stack_top];
    l->myCursorX = x - l->myPosX;
    l->myCursorY = y - l->myPosY;
}

MonstroVec2 Window::getCursorScreenPos() const {
    if (layout_stack_top < 0) return {pos.x, pos.y};
    const Layout* l = &layout_stack[layout_stack_top];
    return {pos.x + l->myPosX + l->myCursorX + l->myScrollOffsetX,
            pos.y + l->myPosY + l->myCursorY + l->myScrollOffsetY};
}

void Window::setCursorScreenPos(float x, float y) {
    if (layout_stack_top < 0) return;
    const Layout* l = &layout_stack[layout_stack_top];
    setCursorPos(x - pos.x - l->myScrollOffsetX, y - pos.y - l->myScrollOffsetY);
}

/* ------------------------------------------------------------------------- */
/* Geometry                                                                  */
/* ------------------------------------------------------------------------- */

MonstroRect Window::getRect() const {
    return {pos.x, pos.y, size.x, size.y};
}

bool Window::mouseOverTitle() const {
    MonstroRect r = getTitleBarRect();
    return monstro_rect_contains(r, ctx->mouse.pos_x, ctx->mouse.pos_y);
}

float Window::getTitleBarHeight() const {
    if (flags & MONSTRO_WINDOW_TITLE_BAR) {
        const MonstroStyle* style = &ctx->style;
        float font_height = style->font ? monstro_font_get_line_height(style->font) : 16.0f;
        return font_height + style->window.group_padding[1];
    }
    return 0.0f;
}

float Window::getMenuBarHeight() const {
    if (flags & MONSTRO_WINDOW_MENU_BAR) {
        const MonstroStyle* style = &ctx->style;
        float font_height = style->font ? monstro_font_get_line_height(style->font) : 16.0f;
        return font_height + style->window.menu_padding[1];
    }
    return 0.0f;
}

MonstroRect Window::getTitleBarRect() const {
    return {pos.x, pos.y, size.x, getTitleBarHeight()};
}

MonstroRect Window::getMenuBarRect() const {
    float title_h = getTitleBarHeight();
    float menu_h = getMenuBarHeight();
    return {pos.x, pos.y + title_h, size.x, menu_h};
}

/* ------------------------------------------------------------------------- */
/* Conditional Position / Size                                               */
/* ------------------------------------------------------------------------- */

void Window::setPosition(float x, float y, MonstroCondition cond) {
    if ((cond & set_pos_allow_flags) == 0)
        return;

    set_pos_allow_flags = (MonstroCondition)(set_pos_allow_flags &
        ~(MONSTRO_COND_ONCE | MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING));

    pos.x = x;
    pos.y = y;
}

void Window::setSize(float w, float h, MonstroCondition cond) {
    if ((cond & set_size_allow_flags) == 0)
        return;

    set_size_allow_flags = (MonstroCondition)(set_size_allow_flags &
        ~(MONSTRO_COND_ONCE | MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING));

    size.x = w;
    if (h != 0.0f || !(flags & MONSTRO_WINDOW_AUTO_RESIZE)) {
        size.y = h;
    }
}

/* ------------------------------------------------------------------------- */
/* Layout                                                                    */
/* ------------------------------------------------------------------------- */

Layout* Window::getCurrentLayout() {
    if (layout_stack_top < 0 || layout_stack_top >= MONSTRO_MAX_LAYOUT_STACK_DEPTH) {
        return nullptr;
    }
    return &layout_stack[layout_stack_top];
}

void Window::setLayoutDirection(MonstroLayoutDirection direction) {
    if (layout_stack_top < 0) return;
    layout_stack[layout_stack_top].myDirection = direction;
}

void Window::addItem(float width, float height) {
    Layout* layout = getCurrentLayout();
    if (!layout) return;
    layout->addItem(width, height);
}

void Window::nextLine() {
    Layout* layout = getCurrentLayout();
    if (!layout) return;
    layout->nextLine();
}

void Window::beginLayout(MonstroLayoutDirection direction) {
    if (layout_stack_top >= MONSTRO_MAX_LAYOUT_STACK_DEPTH - 1) return;

    float start_x = 0.0f;
    float start_y = 0.0f;
    float parent_scroll_x = 0.0f;
    float parent_scroll_y = 0.0f;

    float parent_avail_w = 0.0f;
    float parent_avail_h = 0.0f;

    if (layout_stack_top >= 0) {
        MonstroVec2 cursor = getCursorPos();
        start_x = cursor.x;
        start_y = cursor.y;
        parent_scroll_x = layout_stack[layout_stack_top].myScrollOffsetX;
        parent_scroll_y = layout_stack[layout_stack_top].myScrollOffsetY;
        Layout& parent = layout_stack[layout_stack_top];
        parent_avail_w = parent.myAvailableWidth;
        parent_avail_h = parent.myAvailableHeight;
    }

    layout_stack_top++;
    layout_stack[layout_stack_top].init(direction, start_x, start_y);
    layout_stack[layout_stack_top].myScrollOffsetX = parent_scroll_x;
    layout_stack[layout_stack_top].myScrollOffsetY = parent_scroll_y;
    layout_stack[layout_stack_top].myAvailableWidth = parent_avail_w;
    layout_stack[layout_stack_top].myAvailableHeight = parent_avail_h;
}

void Window::beginLayout(float x, float y, MonstroLayoutDirection direction) {
    if (layout_stack_top >= MONSTRO_MAX_LAYOUT_STACK_DEPTH - 1) return;

    layout_stack_top++;
    layout_stack[layout_stack_top].init(direction, x, y);
}

void Window::endLayout() {
    if (layout_stack_top <= 0) return;

    Layout* finishing = &layout_stack[layout_stack_top];
    float layout_width = finishing->mySizeX;
    float layout_height = finishing->mySizeY;

    layout_stack_top--;

    addItem(layout_width, layout_height);
}

/* ------------------------------------------------------------------------- */
/* Window Tree Management                                                    */
/* ------------------------------------------------------------------------- */

void Window::addChild(Window* win) {
    win->parent = this;
    win->sibling = nullptr;

    if (child == nullptr) {
        child = win;
        return;
    }

    Window* temp = child;
    while (temp->sibling != nullptr) {
        temp = temp->sibling;
    }
    temp->sibling = win;
}

void Window::remove() {
    if (parent == nullptr) return;

    if (parent->child == this) {
        parent->child = sibling;
    } else {
        Window* temp = parent->child;
        while (temp->sibling != nullptr) {
            if (temp->sibling == this) {
                temp->sibling = sibling;
                break;
            }
            temp = temp->sibling;
        }
    }
}

void Window::makeLastSibling() {
    if (parent != nullptr && sibling != nullptr) {
        Window* p = parent;
        remove();
        p->addChild(this);
    }
}

void Window::collectDrawCommands(Context* context) const {
    if (active && canvas.draw_command_count > 0) {
        uint32_t start = context->final_command_count;
        for (uint32_t i = 0; i < canvas.draw_command_count; i++) {
            if (context->final_command_count >= MONSTRO_MAX_DRAW_COMMANDS) break;
            context->final_commands[context->final_command_count++] = canvas.draw_commands[i];
        }
        uint32_t end = context->final_command_count;
        if (end - start > 1) {
            std::stable_sort(context->final_commands + start, context->final_commands + end,
                [](const MonstroDrawCommand& a, const MonstroDrawCommand& b) {
                    return a.layer < b.layer;
                });
        }
    }
    if (sibling) sibling->collectDrawCommands(context);
    if (child) child->collectDrawCommands(context);
}

/* ------------------------------------------------------------------------- */
/* Context Window Management Member Functions                                */
/* ------------------------------------------------------------------------- */

Window* Context::findWindow(const char* name) {
    if (!name) return nullptr;

    for (int i = 0; i < window_count; i++) {
        if (std::strcmp(windows[i].name, name) == 0) {
            return &windows[i];
        }
    }
    return nullptr;
}

Window* Context::createWindow(const char* name, MonstroWindowFlags flags) {
    if (!name || window_count >= MONSTRO_MAX_WINDOWS) return nullptr;

    Window* win = &windows[window_count++];
    win->init(this, name, flags);

    if (current_window) {
        current_window->addChild(win);
    }

    return win;
}

static Window* findHoveredWindowRecursive(
    Window* w, float x, float y, bool excludeChildren)
{
    Window* temp = nullptr;

    if (w->sibling != nullptr) {
        temp = findHoveredWindowRecursive(w->sibling, x, y, excludeChildren);
        if (temp != nullptr) return temp;
    }

    if (w->child != nullptr) {
        temp = findHoveredWindowRecursive(w->child, x, y, excludeChildren);
        if (temp != nullptr) return temp;
    }

    if (!w->active) return nullptr;
    if (!(w->flags & MONSTRO_WINDOW_INPUTS)) return nullptr;
    if (excludeChildren && (w->flags & MONSTRO_WINDOW_CHILD)) return nullptr;

    if (monstro_rect_contains(w->getRect(), x, y)) return w;

    return nullptr;
}

Window* Context::findHoveredWindow(float x, float y, bool excludeChildren) {
    if (root_window != nullptr) {
        Window* result = findHoveredWindowRecursive(root_window, x, y, excludeChildren);

        if (result && popup_stack_top > 0 && popup_stack[popup_stack_top - 1].is_modal) {
            if (!(result->flags & MONSTRO_WINDOW_MODAL) &&
                !(result->flags & MONSTRO_WINDOW_POPUP)) {
                return nullptr;
            }
        }

        return result;
    }
    return nullptr;
}

void Context::focusWindow(Window* win) {
    focused_window = win;

    if (win == nullptr) return;

    if (win->flags & MONSTRO_WINDOW_BRING_TO_FRONT) return;

    win->makeLastSibling();
}

void Context::pushWindow(Window* win) {
    if (!win) return;
    if (window_stack_top >= MONSTRO_MAX_WINDOWS) return;

    if (window_stack_top > 0 && window_stack[window_stack_top - 1] == win) {
        return;
    }

    window_stack[window_stack_top++] = win;
    current_window = win;
}

void Context::popWindow() {
    if (window_stack_top <= 0) {
        current_window = nullptr;
        return;
    }

    window_stack_top--;
    current_window = (window_stack_top > 0) ? window_stack[window_stack_top - 1] : nullptr;
}

} // namespace monstro
