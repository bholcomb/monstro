#include "context.h"
#include <cstring>
#include <cmath>
#include <algorithm>

/* ------------------------------------------------------------------------- */
/* MonstroWindowInternal Member Functions                                    */
/* ------------------------------------------------------------------------- */

void MonstroWindowInternal::init(const char* windowName, MonstroWindowFlags windowFlags) {
    std::memset(this, 0, sizeof(MonstroWindowInternal));
    
    std::strncpy(name, windowName, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    
    id = monstro_hash_string(windowName);
    move_id = monstro_hash_combine(id, monstro_hash_string("MOVE"));
    flags = windowFlags;
    active = true;
    collapsed = false;
    
    set_pos_allow_flags = (MonstroCondition)(MONSTRO_COND_ALWAYS | MONSTRO_COND_ONCE | 
                                              MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING);
    set_size_allow_flags = (MonstroCondition)(MONSTRO_COND_ALWAYS | MONSTRO_COND_ONCE | 
                                               MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING);
    
    pos_x = 100.0f;
    pos_y = 100.0f;
    size_x = 300.0f;
    size_y = 200.0f;

    parent = nullptr;
    child = nullptr;
    sibling = nullptr;
}

bool MonstroWindowInternal::containsPoint(float x, float y) const {
    return x >= pos_x && x < pos_x + size_x &&
           y >= pos_y && y < pos_y + size_y;
}

MonstroRect MonstroWindowInternal::getRect() const {
    return {pos_x, pos_y, size_x, size_y};
}

float MonstroWindowInternal::getTitleBarHeight(const MonstroStyle* style) const {
    if (flags & MONSTRO_WINDOW_TITLE_BAR) {
        float font_height = style->font ? monstro_font_get_line_height(style->font) : 16.0f;
        return font_height + style->window.group_padding[1];
    }
    return 0.0f;
}

float MonstroWindowInternal::getMenuBarHeight(const MonstroStyle* style) const {
    if (flags & MONSTRO_WINDOW_MENU_BAR) {
        float font_height = style->font ? monstro_font_get_line_height(style->font) : 16.0f;
        return font_height + style->window.menu_padding[1];
    }
    return 0.0f;
}

MonstroRect MonstroWindowInternal::getTitleBarRect(const MonstroStyle* style) const {
    return {pos_x, pos_y, size_x, getTitleBarHeight(style)};
}

MonstroRect MonstroWindowInternal::getMenuBarRect(const MonstroStyle* style) const {
    float title_h = getTitleBarHeight(style);
    float menu_h = getMenuBarHeight(style);
    return {pos_x, pos_y + title_h, size_x, menu_h};
}

void MonstroWindowInternal::initLayout(float contentY, float paddingX, float paddingY) {
    layout_stack_top = 0;
    MonstroLayoutInternal* layout = &layout_stack[0];
    layout->direction = MONSTRO_LAYOUT_VERTICAL;
    layout->pos_x = pos_x + paddingX;
    layout->pos_y = contentY + paddingY;
    layout->size_x = size_x - paddingX * 2.0f;
    layout->size_y = size_y - (contentY - pos_y) - paddingY * 2.0f;
    layout->cursor_x = 0.0f;
    layout->cursor_y = 0.0f;
    layout->max_x = 0.0f;
    layout->max_y = 0.0f;
}

void MonstroWindowInternal::beginLayout(MonstroLayoutDirection direction) {
    if (layout_stack_top >= MONSTRO_MAX_LAYOUT_STACK_DEPTH - 1) return;
    
    float start_x = 0.0f;
    float start_y = 0.0f;
    
    if (layout_stack_top >= 0) {
        MonstroLayoutInternal* current = &layout_stack[layout_stack_top];
        start_x = current->pos_x + current->cursor_x;
        start_y = current->pos_y + current->cursor_y;
    }
    
    layout_stack_top++;
    MonstroLayoutInternal* layout = &layout_stack[layout_stack_top];
    layout->direction = direction;
    layout->pos_x = start_x;
    layout->pos_y = start_y;
    layout->size_x = 0.0f;
    layout->size_y = 0.0f;
    layout->cursor_x = 0.0f;
    layout->cursor_y = 0.0f;
    layout->max_x = 0.0f;
    layout->max_y = 0.0f;
}

void MonstroWindowInternal::beginLayoutAt(float x, float y, MonstroLayoutDirection direction) {
    if (layout_stack_top >= MONSTRO_MAX_LAYOUT_STACK_DEPTH - 1) return;
    
    layout_stack_top++;
    MonstroLayoutInternal* layout = &layout_stack[layout_stack_top];
    layout->direction = direction;
    layout->pos_x = x;
    layout->pos_y = y;
    layout->size_x = 0.0f;
    layout->size_y = 0.0f;
    layout->cursor_x = 0.0f;
    layout->cursor_y = 0.0f;
    layout->max_x = 0.0f;
    layout->max_y = 0.0f;
}

void MonstroWindowInternal::endLayout() {
    if (layout_stack_top <= 0) return;
    
    MonstroLayoutInternal* finishing = &layout_stack[layout_stack_top];
    float layout_width = finishing->size_x;
    float layout_height = finishing->size_y;
    
    layout_stack_top--;
    
    addItem(layout_width, layout_height);
}

void MonstroWindowInternal::addItem(float width, float height) {
    if (layout_stack_top < 0) return;
    
    MonstroLayoutInternal* layout = &layout_stack[layout_stack_top];
    
    layout->size_x = std::max(layout->size_x, layout->cursor_x + width);
    layout->size_y = std::max(layout->size_y, layout->cursor_y + height);
    
    if (layout->direction == MONSTRO_LAYOUT_VERTICAL) {
        layout->cursor_y += height;
    } else {
        layout->cursor_x += width;
    }
}

MonstroLayoutInternal* MonstroWindowInternal::getCurrentLayout() {
    if (layout_stack_top < 0 || layout_stack_top >= MONSTRO_MAX_LAYOUT_STACK_DEPTH) {
        return nullptr;
    }
    return &layout_stack[layout_stack_top];
}

void MonstroWindowInternal::setPosition(float x, float y, MonstroCondition cond) {
    if ((cond & set_pos_allow_flags) == 0)
        return;

    set_pos_allow_flags = (MonstroCondition)(set_pos_allow_flags & 
        ~(MONSTRO_COND_ONCE | MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING));

    pos_x = x;
    pos_y = y;
}

void MonstroWindowInternal::setSizeWithCond(float w, float h, MonstroCondition cond) {
    if ((cond & set_size_allow_flags) == 0)
        return;

    set_size_allow_flags = (MonstroCondition)(set_size_allow_flags & 
        ~(MONSTRO_COND_ONCE | MONSTRO_COND_FIRST_USE_EVER | MONSTRO_COND_APPEARING));

    size_x = w;
    size_y = h;
}

/* ------------------------------------------------------------------------- */
/* Window Tree Management                                                    */
/* ------------------------------------------------------------------------- */

void MonstroWindowInternal::addChild(MonstroWindowInternal* win) {
    win->parent = this;
    win->sibling = nullptr;

    if (child == nullptr) {
        child = win;
        return;
    }

    MonstroWindowInternal* temp = child;
    while (temp->sibling != nullptr) {
        temp = temp->sibling;
    }
    temp->sibling = win;
}

void MonstroWindowInternal::remove() {
    if (parent == nullptr) return;

    if (parent->child == this) {
        parent->child = sibling;
    } else {
        MonstroWindowInternal* temp = parent->child;
        while (temp->sibling != nullptr) {
            if (temp->sibling == this) {
                temp->sibling = sibling;
                break;
            }
            temp = temp->sibling;
        }
    }
}

void MonstroWindowInternal::makeLastSibling() {
    if (parent != nullptr && sibling != nullptr) {
        MonstroWindowInternal* p = parent;
        remove();
        p->addChild(this);
    }
}

/* ------------------------------------------------------------------------- */
/* MonstroContext Window Management Member Functions                         */
/* ------------------------------------------------------------------------- */

MonstroWindowInternal* MonstroContext::findWindow(const char* name) {
    if (!name) return nullptr;
    
    for (int i = 0; i < window_count; i++) {
        if (std::strcmp(windows[i].name, name) == 0) {
            return &windows[i];
        }
    }
    return nullptr;
}

MonstroWindowInternal* MonstroContext::createWindow(const char* name, MonstroWindowFlags flags) {
    if (!name || window_count >= MONSTRO_MAX_WINDOWS) return nullptr;
    
    MonstroWindowInternal* win = &windows[window_count++];
    win->init(name, flags);
    
    if (current_window) {
        current_window->addChild(win);
    }
    
    return win;
}

static MonstroWindowInternal* findHoveredWindowRecursive(
    MonstroWindowInternal* w, float x, float y, bool excludeChildren)
{
    MonstroWindowInternal* temp = nullptr;

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

    if (w->containsPoint(x, y)) return w;

    return nullptr;
}

MonstroWindowInternal* MonstroContext::findHoveredWindow(float x, float y, bool excludeChildren) {
    if (root_window != nullptr) {
        return findHoveredWindowRecursive(root_window, x, y, excludeChildren);
    }
    return nullptr;
}

void MonstroContext::focusWindow(MonstroWindowInternal* win) {
    focused_window = win;

    if (win == nullptr) return;

    if (win->flags & MONSTRO_WINDOW_BRING_TO_FRONT) return;

    win->makeLastSibling();
}

void MonstroContext::pushWindow(MonstroWindowInternal* win) {
    if (!win) return;
    if (window_stack_top >= MONSTRO_MAX_WINDOWS) return;
    
    if (window_stack_top > 0 && window_stack[window_stack_top - 1] == win) {
        return;
    }
    
    window_stack[window_stack_top++] = win;
    current_window = win;
}

void MonstroContext::popWindow() {
    if (window_stack_top <= 0) {
        current_window = nullptr;
        return;
    }
    
    window_stack_top--;
    current_window = (window_stack_top > 0) ? window_stack[window_stack_top - 1] : nullptr;
}
