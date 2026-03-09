#include "context.h"
#include <algorithm>

/* ------------------------------------------------------------------------- */
/* Layout member functions                                                   */
/* ------------------------------------------------------------------------- */

namespace monstro {

void Layout::init(MonstroLayoutDirection direction, float posX, float posY) {
    myDirection = direction;
    myPosX = posX;
    myPosY = posY;
    mySizeX = 0.0f;
    mySizeY = 0.0f;
    myCursorX = 0.0f;
    myCursorY = 0.0f;
    myMaxX = 0.0f;
    myMaxY = 0.0f;
    myScrollOffsetX = 0.0f;
    myScrollOffsetY = 0.0f;
    myAvailableWidth = 0.0f;
    myAvailableHeight = 0.0f;
    myLastItemX = 0.0f;
    myLastItemY = 0.0f;
    myLastItemW = 0.0f;
    myLastItemH = 0.0f;
    myCurrentLineY = 0.0f;
    myCurrentLineMaxH = 0.0f;
}

void Layout::addItem(float width, float height) {
    myLastItemX = myCursorX;
    myLastItemY = myCursorY;
    myLastItemW = width;
    myLastItemH = height;
    myCurrentLineMaxH = std::max(myCurrentLineMaxH, myCursorY - myCurrentLineY + height);

    mySizeX = std::max(mySizeX, myCursorX + width);
    mySizeY = std::max(mySizeY, myCursorY + height);

    if (myDirection == MONSTRO_LAYOUT_VERTICAL) {
        myCursorY += height;
    } else {
        myCursorX += width;
    }
}

void Layout::nextLine() {
    if (myDirection == MONSTRO_LAYOUT_HORIZONTAL) {
        myCursorX = 0.0f;
        myCursorY += mySizeY;
    } else {
        myCursorX += mySizeX;
        myCursorY = 0.0f;
    }
}

} // namespace monstro

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

const char* monstro_display_text(const char* label) {
    if (!label) return label;
    for (const char* p = label; *p; p++) {
        if (p[0] == '#' && p[1] == '#') return p + 2;
    }
    return label;
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
    monstro::Layout* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return;
    
    if (indent_width <= 0.0f) {
        indent_width = ctx->style.tree.indent;
    }
    
    layout->myPosX += indent_width;
    layout->myCursorX += indent_width;
    layout->mySizeX -= indent_width;
}

void monstro_unindent(MonstroContext* ctx, float indent_width) {
    monstro::Layout* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return;
    
    if (indent_width <= 0.0f) {
        indent_width = ctx->style.tree.indent;
    }
    
    layout->myPosX -= indent_width;
    layout->myCursorX -= indent_width;
    layout->mySizeX += indent_width;
}

void monstro_spacer(MonstroContext* ctx, float size) {
    monstro::Layout* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return;
    
    if (layout->myDirection == MONSTRO_LAYOUT_VERTICAL) {
        layout->myCursorY += size;
    } else {
        layout->myCursorX += size;
    }
}

void monstro_dummy(MonstroContext* ctx, float width, float height) {
    if (!ctx || !ctx->current_window) return;
    monstro_add_item(ctx, width, height);
}

void monstro_same_line(MonstroContext* ctx, float offset_x, float spacing) {
    monstro::Layout* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return;

    if (offset_x > 0.0f) {
        layout->myCursorX = offset_x;
    } else {
        layout->myCursorX = layout->myLastItemX + layout->myLastItemW + spacing;
    }
    layout->myCursorY = layout->myCurrentLineY;
}

float monstro_percent(MonstroContext* ctx, float percent, MonstroLayoutDirection direction) {
    monstro::Layout* layout = ctx ? ctx->getCurrentLayout() : nullptr;
    if (!layout) return 0.0f;

    if (direction == MONSTRO_LAYOUT_HORIZONTAL) {
        return layout->myAvailableWidth * (percent / 100.0f);
    } else {
        return layout->myAvailableHeight * (percent / 100.0f);
    }
}

MonstroRect monstro_get_item_rect(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return {0, 0, 0, 0};
    monstro::Layout* layout = ctx->getCurrentLayout();
    if (!layout) return {0, 0, 0, 0};
    
    float x = ctx->current_window->pos.x + layout->myPosX + layout->myCursorX + layout->myScrollOffsetX;
    float y = ctx->current_window->pos.y + layout->myPosY + layout->myCursorY + layout->myScrollOffsetY;
    return {x, y, layout->mySizeX, 0};
}
