#include "context.h"
#include "drawbuffer.h"
#include <cstring>
#include <cmath>
#include <algorithm>

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

    if (last_frame_active != ctx->frame) {
        last_frame_active = ctx->frame;
        active = true;
    }

    if (ctx->current_window != this) {
        ctx->pushWindow(this);
    }

    canvas.init(&ctx->draw_buffer);

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
    return true;
}

bool Window::end() {
    MonstroStyle* style = &ctx->style;

    canvas.popClipRect();

    if (flags & MONSTRO_WINDOW_AUTO_RESIZE) {
        Layout* layout = getCurrentLayout();
        if (layout) {
            float new_width = layout->mySizeX + style->window.padding[0] * 2.0f;
            float new_height = layout->mySizeY + style->window.padding[1] * 2.0f;
            if (new_width > 0.0f) size.x = new_width;
            if (new_height > 0.0f) size.y = new_height;
        }
    }

    layout_stack_top = -1;
    ctx->id_stack.pop();
    return true;
}

/* ------------------------------------------------------------------------- */
/* Drawing                                                                   */
/* ------------------------------------------------------------------------- */

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

void Window::drawForeground() {
    MonstroStyle* style = &ctx->style;

    float saved_rounding = style->window.rounding;
    if (flags & MONSTRO_WINDOW_ROOT) {
        style->window.rounding = 0.0f;
    }

    if (collapsed) {
        MonstroRect title_rect = getTitleBarRect();
        canvas.addRectFilled(title_rect, monstro_color_to_u32(style->window.header.active.data.color), style->window.rounding, MONSTRO_CORNER_ALL);
        if (style->font) {
            canvas.addTextEx(style->font, title_rect, monstro_color_to_u32(style->window.header.label_normal), name, MONSTRO_ALIGN_CENTER);
        }
    } else {
        if (flags & MONSTRO_WINDOW_TITLE_BAR) {
            MonstroRect title_rect = getTitleBarRect();
            uint32_t header_color = monstro_color_to_u32(style->window.header.active.data.color);
            canvas.addRectFilled(title_rect, header_color, style->window.rounding, MONSTRO_CORNER_TOP);

            uint32_t label_color = monstro_color_to_u32(style->window.header.label_normal);
            if (style->font) {
                canvas.addTextEx(style->font, title_rect, label_color, name, MONSTRO_ALIGN_CENTER);
            }

            addItem(title_rect.w, title_rect.h);
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
    MonstroVec2 cursor = getCursorPos();
    return {pos.x + cursor.x, pos.y + cursor.y};
}

void Window::setCursorScreenPos(float x, float y) {
    setCursorPos(x - pos.x, y - pos.y);
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

    if (layout_stack_top >= 0) {
        MonstroVec2 cursor = getCursorPos();
        start_x = cursor.x;
        start_y = cursor.y;
    }

    layout_stack_top++;
    layout_stack[layout_stack_top].init(direction, start_x, start_y);
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
    if (active) {
        for (uint32_t i = 0; i < canvas.draw_command_count; i++) {
            if (context->final_command_count >= MONSTRO_MAX_DRAW_COMMANDS) break;
            context->final_commands[context->final_command_count++] = canvas.draw_commands[i];
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
        return findHoveredWindowRecursive(root_window, x, y, excludeChildren);
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
