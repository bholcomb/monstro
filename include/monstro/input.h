#ifndef MONSTRO_INPUT_H
#define MONSTRO_INPUT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Double-click, drag and key repeat timing are environment settings: see MonstroSettings. */

/*
 * Input State Setters (call from app input callbacks)
 */

void monstro_set_mouse_pos(MonstroContext* ctx, float x, float y);
void monstro_set_mouse_button(MonstroContext* ctx, MonstroMouseButton button, bool down);
void monstro_set_mouse_wheel(MonstroContext* ctx, float wheel_x, float wheel_y);

void monstro_set_key(MonstroContext* ctx, MonstroKey key, bool down);
void monstro_set_key_mods(MonstroContext* ctx, MonstroModFlags mods);
void monstro_add_input_text(MonstroContext* ctx, const char* utf8_text);

/*
 * Mouse State Queries
 */

MonstroVec2 monstro_get_mouse_pos(MonstroContext* ctx);
MonstroVec2 monstro_get_mouse_delta(MonstroContext* ctx);

bool monstro_is_mouse_down(MonstroContext* ctx, MonstroMouseButton button);
bool monstro_is_mouse_clicked(MonstroContext* ctx, MonstroMouseButton button, bool repeat);
bool monstro_is_mouse_released(MonstroContext* ctx, MonstroMouseButton button);
bool monstro_is_mouse_double_clicked(MonstroContext* ctx, MonstroMouseButton button);
bool monstro_is_mouse_dragging(MonstroContext* ctx, MonstroMouseButton button);
MonstroVec2 monstro_get_mouse_drag_delta(MonstroContext* ctx, MonstroMouseButton button);

float monstro_get_mouse_wheel(MonstroContext* ctx);
float monstro_get_mouse_wheel_h(MonstroContext* ctx);

/*
 * Keyboard State Queries
 */

bool monstro_is_key_down(MonstroContext* ctx, MonstroKey key);
bool monstro_is_key_pressed(MonstroContext* ctx, MonstroKey key, bool repeat);
bool monstro_is_key_released(MonstroContext* ctx, MonstroKey key);
MonstroModFlags monstro_get_key_mods(MonstroContext* ctx);
const char* monstro_get_input_text(MonstroContext* ctx);

/*
 * Input Capture Flags
 */

bool monstro_want_capture_mouse(MonstroContext* ctx);
bool monstro_want_capture_keyboard(MonstroContext* ctx);
bool monstro_want_text_input(MonstroContext* ctx);

/*
 * Cursor
 */

void monstro_set_mouse_cursor(MonstroContext* ctx, MonstroMouseCursor cursor);
MonstroMouseCursor monstro_get_mouse_cursor(MonstroContext* ctx);

#ifdef __cplusplus
}
#endif

#endif
