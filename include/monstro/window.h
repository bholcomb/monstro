#ifndef MONSTRO_WINDOW_H
#define MONSTRO_WINDOW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

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
void monstro_set_next_window_focus(MonstroContext* ctx);

/*
 * Current Window Configuration (call after begin_window)
 */

void monstro_set_window_pos(MonstroContext* ctx, float x, float y, MonstroCondition cond);
void monstro_set_window_size(MonstroContext* ctx, float width, float height, MonstroCondition cond);
void monstro_set_window_focus(MonstroContext* ctx);
void monstro_set_window_layout(MonstroContext* ctx, MonstroLayoutDirection direction);
void monstro_set_window_title(MonstroContext* ctx, const char* title);

/*
 * Window Configuration by Name
 */

void monstro_set_window_pos_named(MonstroContext* ctx, const char* name, float x, float y, MonstroCondition cond);
void monstro_set_window_size_named(MonstroContext* ctx, const char* name, float width, float height, MonstroCondition cond);
void monstro_set_window_focus_named(MonstroContext* ctx, const char* name);
void monstro_set_window_layout_named(MonstroContext* ctx, const char* name, MonstroLayoutDirection direction);
void monstro_set_window_title_named(MonstroContext* ctx, const char* name, const char* title);

/*
 * Window Properties (call after begin_window)
 */

MonstroRect monstro_get_window_rect(MonstroContext* ctx);
MonstroVec2 monstro_get_window_pos(MonstroContext* ctx);
MonstroVec2 monstro_get_window_size(MonstroContext* ctx);
MonstroCanvas* monstro_get_window_canvas(MonstroContext* ctx);

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

/*
 * Child Windows / Scroll Regions
 */

bool monstro_begin_child(MonstroContext* ctx, const char* str_id, float width, float height, MonstroWindowFlags flags);
void monstro_end_child(MonstroContext* ctx);

#ifdef __cplusplus
}
#endif

#endif
