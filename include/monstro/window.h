#ifndef MONSTRO_WINDOW_H
#define MONSTRO_WINDOW_H

#include <stdbool.h>
#include <stdint.h>

#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MonstroContext MonstroContext;

typedef enum MonstroWindowFlags {
    MONSTRO_WINDOW_NONE                 = 0,

    /* Decorations */
    MONSTRO_WINDOW_BACKGROUND           = 1 << 0,
    MONSTRO_WINDOW_TITLE_BAR            = 1 << 1,
    MONSTRO_WINDOW_MENU_BAR             = 1 << 2,
    MONSTRO_WINDOW_BORDERS              = 1 << 3,
    MONSTRO_WINDOW_HORIZONTAL_SCROLLBAR = 1 << 4,
    MONSTRO_WINDOW_VERTICAL_SCROLLBAR   = 1 << 5,
    MONSTRO_WINDOW_SCROLLBARS           = MONSTRO_WINDOW_HORIZONTAL_SCROLLBAR | MONSTRO_WINDOW_VERTICAL_SCROLLBAR,

    /* Behavior */
    MONSTRO_WINDOW_MOVABLE              = 1 << 10,
    MONSTRO_WINDOW_COLLAPSABLE          = 1 << 11,
    MONSTRO_WINDOW_RESIZABLE            = 1 << 12,
    MONSTRO_WINDOW_INPUTS               = 1 << 13,
    MONSTRO_WINDOW_SCROLL_WITH_MOUSE    = 1 << 14,
    MONSTRO_WINDOW_FOCUS_ON_APPEARING   = 1 << 15,
    MONSTRO_WINDOW_BRING_TO_FRONT       = 1 << 16,
    MONSTRO_WINDOW_AUTO_RESIZE          = 1 << 17,

    /* Window Types */
    MONSTRO_WINDOW_ROOT                 = 1 << 20,
    MONSTRO_WINDOW_CHILD                = 1 << 21,
    MONSTRO_WINDOW_COMBO                = 1 << 22,
    MONSTRO_WINDOW_TOOLTIP              = 1 << 23,
    MONSTRO_WINDOW_POPUP                = 1 << 24,
    MONSTRO_WINDOW_MODAL                = 1 << 25,
    MONSTRO_WINDOW_CHILD_MENU           = 1 << 26,

    /* Common Presets */
    MONSTRO_WINDOW_DEFAULT = MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_TITLE_BAR | MONSTRO_WINDOW_MENU_BAR |
                             MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_MOVABLE | MONSTRO_WINDOW_COLLAPSABLE |
                             MONSTRO_WINDOW_RESIZABLE | MONSTRO_WINDOW_INPUTS | MONSTRO_WINDOW_BRING_TO_FRONT |
                             MONSTRO_WINDOW_CHILD,

    MONSTRO_WINDOW_ROOT_DEFAULT = MONSTRO_WINDOW_MENU_BAR | MONSTRO_WINDOW_INPUTS | MONSTRO_WINDOW_ROOT
} MonstroWindowFlags;

typedef enum MonstroCondition {
    MONSTRO_COND_NONE           = 0,
    MONSTRO_COND_ALWAYS         = 1 << 0,
    MONSTRO_COND_ONCE           = 1 << 1,
    MONSTRO_COND_FIRST_USE_EVER = 1 << 2,
    MONSTRO_COND_APPEARING      = 1 << 3
} MonstroCondition;

/*
 * Window Lifecycle
 */

bool monstro_begin_window(MonstroContext* ctx, const char* name, bool* p_open, MonstroWindowFlags flags);
void monstro_end_window(MonstroContext* ctx);

/*
 * Next Window Configuration (call before begin_window)
 */

void monstro_set_next_window_pos(MonstroContext* ctx, float x, float y, MonstroCondition cond);
void monstro_set_next_window_size(MonstroContext* ctx, float width, float height, MonstroCondition cond);
void monstro_set_next_window_collapsed(MonstroContext* ctx, bool collapsed, MonstroCondition cond);
void monstro_set_next_window_focus(MonstroContext* ctx);

/*
 * Window Properties (call after begin_window)
 */

MonstroRect monstro_get_window_rect(MonstroContext* ctx);
MonstroVec2 monstro_get_window_pos(MonstroContext* ctx);
MonstroVec2 monstro_get_window_size(MonstroContext* ctx);

/*
 * Scrolling
 */

float monstro_get_scroll_x(MonstroContext* ctx);
float monstro_get_scroll_y(MonstroContext* ctx);
void monstro_set_scroll_x(MonstroContext* ctx, float scroll_x);
void monstro_set_scroll_y(MonstroContext* ctx, float scroll_y);

/*
 * Window State Queries
 */

bool monstro_is_window_collapsed(MonstroContext* ctx);
bool monstro_is_window_focused(MonstroContext* ctx);
bool monstro_is_window_hovered(MonstroContext* ctx);

#ifdef __cplusplus
}
#endif

#endif
