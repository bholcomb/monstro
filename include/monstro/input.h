#ifndef MONSTRO_INPUT_H
#define MONSTRO_INPUT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Double-click, drag and key repeat timing are environment settings: see MonstroSettings. */

/*
 * Input Events (call from app input callbacks, or once a frame with polled state)
 *
 * Mouse moves, button and key changes and wheel movements are queued in order
 * and taken at the next begin_frame, so nothing is lost between frames: a
 * press and release that both happen before a frame still click. A frame
 * takes events in order until one would change something already changed
 * that frame (the same button or key again, or a move after a button), and
 * the rest wait for the next frames. A press after a move is taken with it:
 * the item it lands on is the one on top there in the last frame. A tap that
 * moves, presses and releases between two frames is seen over two frames: the
 * move and the press, then the release.
 *
 * Setting a button, key or position to the state it already has queues
 * nothing, and moves in a row are merged, so polling every frame is fine.
 * The queue holds MonstroContextConfig.max_input_events; overfilling it stops
 * the program.
 */

void monstro_set_mouse_pos(MonstroContext* ctx, float x, float y);
void monstro_set_mouse_button(MonstroContext* ctx, MonstroMouseButton button, bool down);
/* Adds a wheel movement; movements in a row are summed. */
void monstro_set_mouse_wheel(MonstroContext* ctx, float wheel_x, float wheel_y);

void monstro_set_key(MonstroContext* ctx, MonstroKey key, bool down);
/* Modifiers and typed text are not queued: they are current state, taken as they are. */
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
