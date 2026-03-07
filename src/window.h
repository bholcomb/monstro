#ifndef MONSTRO_INTERNAL_WINDOW_H
#define MONSTRO_INTERNAL_WINDOW_H

#include "monstro.h"
#include "constants.h"
#include "layout.h"

struct MonstroWindowInternal {
    char name[256] = {};
    uint64_t id = 0;
    uint64_t move_id = 0;
    MonstroWindowFlags flags = MONSTRO_WINDOW_NONE;

    float pos_x = 0.0f, pos_y = 0.0f;
    float size_x = 0.0f, size_y = 0.0f;
    float scroll_x = 0.0f, scroll_y = 0.0f;

    bool active = false;
    bool collapsed = false;
    bool skip_items = false;
    int last_frame_active = 0;

    MonstroLayoutInternal layout_stack[MONSTRO_MAX_LAYOUT_STACK_DEPTH] = {};
    int layout_stack_top = 0;

    MonstroWindowInternal* parent = nullptr;
    MonstroWindowInternal* child = nullptr;
    MonstroWindowInternal* sibling = nullptr;

    MonstroCondition set_pos_allow_flags = MONSTRO_COND_NONE;
    MonstroCondition set_size_allow_flags = MONSTRO_COND_NONE;

    void init(const char* windowName, MonstroWindowFlags windowFlags);

    bool containsPoint(float x, float y) const;
    MonstroRect getRect() const;
    MonstroVec2 getPos() const { return {pos_x, pos_y}; }
    MonstroVec2 getSize() const { return {size_x, size_y}; }

    float getTitleBarHeight(const struct MonstroStyle* style) const;
    float getMenuBarHeight(const struct MonstroStyle* style) const;
    MonstroRect getTitleBarRect(const struct MonstroStyle* style) const;
    MonstroRect getMenuBarRect(const struct MonstroStyle* style) const;

    void initLayout(float contentY, float paddingX, float paddingY);
    void beginLayout(MonstroLayoutDirection direction);
    void beginLayoutAt(float x, float y, MonstroLayoutDirection direction);
    void endLayout();
    void addItem(float width, float height);
    MonstroLayoutInternal* getCurrentLayout();

    void setPos(float x, float y) { pos_x = x; pos_y = y; }
    void setSize(float w, float h) { size_x = w; size_y = h; }
    void move(float dx, float dy) { pos_x += dx; pos_y += dy; }
    void setPosition(float x, float y, MonstroCondition cond);
    void setSizeWithCond(float w, float h, MonstroCondition cond);

    void addChild(MonstroWindowInternal* win);
    void remove();
    void makeLastSibling();
};

#endif
