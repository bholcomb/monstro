#ifndef MONSTRO_INPUT_H
#define MONSTRO_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MonstroContext MonstroContext;

typedef enum MonstroKey {
    MONSTRO_KEY_NONE = 0,
    MONSTRO_KEY_TAB,
    MONSTRO_KEY_LEFT_ARROW,
    MONSTRO_KEY_RIGHT_ARROW,
    MONSTRO_KEY_UP_ARROW,
    MONSTRO_KEY_DOWN_ARROW,
    MONSTRO_KEY_PAGE_UP,
    MONSTRO_KEY_PAGE_DOWN,
    MONSTRO_KEY_HOME,
    MONSTRO_KEY_END,
    MONSTRO_KEY_INSERT,
    MONSTRO_KEY_DELETE,
    MONSTRO_KEY_BACKSPACE,
    MONSTRO_KEY_SPACE,
    MONSTRO_KEY_ENTER,
    MONSTRO_KEY_ESCAPE,
    MONSTRO_KEY_LEFT_CTRL,
    MONSTRO_KEY_LEFT_SHIFT,
    MONSTRO_KEY_LEFT_ALT,
    MONSTRO_KEY_LEFT_SUPER,
    MONSTRO_KEY_RIGHT_CTRL,
    MONSTRO_KEY_RIGHT_SHIFT,
    MONSTRO_KEY_RIGHT_ALT,
    MONSTRO_KEY_RIGHT_SUPER,
    MONSTRO_KEY_MENU,
    MONSTRO_KEY_0, MONSTRO_KEY_1, MONSTRO_KEY_2, MONSTRO_KEY_3, MONSTRO_KEY_4,
    MONSTRO_KEY_5, MONSTRO_KEY_6, MONSTRO_KEY_7, MONSTRO_KEY_8, MONSTRO_KEY_9,
    MONSTRO_KEY_A, MONSTRO_KEY_B, MONSTRO_KEY_C, MONSTRO_KEY_D, MONSTRO_KEY_E,
    MONSTRO_KEY_F, MONSTRO_KEY_G, MONSTRO_KEY_H, MONSTRO_KEY_I, MONSTRO_KEY_J,
    MONSTRO_KEY_K, MONSTRO_KEY_L, MONSTRO_KEY_M, MONSTRO_KEY_N, MONSTRO_KEY_O,
    MONSTRO_KEY_P, MONSTRO_KEY_Q, MONSTRO_KEY_R, MONSTRO_KEY_S, MONSTRO_KEY_T,
    MONSTRO_KEY_U, MONSTRO_KEY_V, MONSTRO_KEY_W, MONSTRO_KEY_X, MONSTRO_KEY_Y,
    MONSTRO_KEY_Z,
    MONSTRO_KEY_F1, MONSTRO_KEY_F2, MONSTRO_KEY_F3, MONSTRO_KEY_F4, MONSTRO_KEY_F5,
    MONSTRO_KEY_F6, MONSTRO_KEY_F7, MONSTRO_KEY_F8, MONSTRO_KEY_F9, MONSTRO_KEY_F10,
    MONSTRO_KEY_F11, MONSTRO_KEY_F12,
    MONSTRO_KEY_APOSTROPHE,
    MONSTRO_KEY_COMMA,
    MONSTRO_KEY_MINUS,
    MONSTRO_KEY_PERIOD,
    MONSTRO_KEY_SLASH,
    MONSTRO_KEY_SEMICOLON,
    MONSTRO_KEY_EQUAL,
    MONSTRO_KEY_LEFT_BRACKET,
    MONSTRO_KEY_BACKSLASH,
    MONSTRO_KEY_RIGHT_BRACKET,
    MONSTRO_KEY_GRAVE_ACCENT,
    MONSTRO_KEY_CAPS_LOCK,
    MONSTRO_KEY_SCROLL_LOCK,
    MONSTRO_KEY_NUM_LOCK,
    MONSTRO_KEY_PRINT_SCREEN,
    MONSTRO_KEY_PAUSE,
    MONSTRO_KEY_KEYPAD_0, MONSTRO_KEY_KEYPAD_1, MONSTRO_KEY_KEYPAD_2,
    MONSTRO_KEY_KEYPAD_3, MONSTRO_KEY_KEYPAD_4, MONSTRO_KEY_KEYPAD_5,
    MONSTRO_KEY_KEYPAD_6, MONSTRO_KEY_KEYPAD_7, MONSTRO_KEY_KEYPAD_8,
    MONSTRO_KEY_KEYPAD_9,
    MONSTRO_KEY_KEYPAD_DECIMAL,
    MONSTRO_KEY_KEYPAD_DIVIDE,
    MONSTRO_KEY_KEYPAD_MULTIPLY,
    MONSTRO_KEY_KEYPAD_SUBTRACT,
    MONSTRO_KEY_KEYPAD_ADD,
    MONSTRO_KEY_KEYPAD_ENTER,
    MONSTRO_KEY_KEYPAD_EQUAL,
    MONSTRO_KEY_COUNT
} MonstroKey;

typedef enum MonstroMouseButton {
    MONSTRO_MOUSE_BUTTON_LEFT = 0,
    MONSTRO_MOUSE_BUTTON_RIGHT = 1,
    MONSTRO_MOUSE_BUTTON_MIDDLE = 2,
    MONSTRO_MOUSE_BUTTON_COUNT = 5
} MonstroMouseButton;

typedef enum MonstroModFlags {
    MONSTRO_MOD_NONE = 0,
    MONSTRO_MOD_CTRL = 1 << 0,
    MONSTRO_MOD_SHIFT = 1 << 1,
    MONSTRO_MOD_ALT = 1 << 2,
    MONSTRO_MOD_SUPER = 1 << 3
} MonstroModFlags;

typedef enum MonstroMouseCursor {
    MONSTRO_MOUSE_CURSOR_NONE = -1,
    MONSTRO_MOUSE_CURSOR_ARROW = 0,
    MONSTRO_MOUSE_CURSOR_TEXT_INPUT,
    MONSTRO_MOUSE_CURSOR_RESIZE_ALL,
    MONSTRO_MOUSE_CURSOR_RESIZE_NS,
    MONSTRO_MOUSE_CURSOR_RESIZE_EW,
    MONSTRO_MOUSE_CURSOR_RESIZE_NESW,
    MONSTRO_MOUSE_CURSOR_RESIZE_NWSE,
    MONSTRO_MOUSE_CURSOR_HAND,
    MONSTRO_MOUSE_CURSOR_NOT_ALLOWED,
    MONSTRO_MOUSE_CURSOR_CROSSHAIR,
    MONSTRO_MOUSE_CURSOR_WAIT,
    MONSTRO_MOUSE_CURSOR_COUNT
} MonstroMouseCursor;

/*
 * Input Configuration
 */

void monstro_set_double_click_distance(MonstroContext* ctx, float distance);
void monstro_set_mouse_drag_threshold(MonstroContext* ctx, float threshold);
void monstro_set_key_repeat_delay(MonstroContext* ctx, float delay);
void monstro_set_key_repeat_rate(MonstroContext* ctx, float rate);

float monstro_get_double_click_distance(MonstroContext* ctx);
float monstro_get_mouse_drag_threshold(MonstroContext* ctx);
float monstro_get_key_repeat_delay(MonstroContext* ctx);
float monstro_get_key_repeat_rate(MonstroContext* ctx);

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
