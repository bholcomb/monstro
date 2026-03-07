#ifndef MONSTRO_INTERNAL_INPUT_H
#define MONSTRO_INTERNAL_INPUT_H

#include "monstro.h"

struct MonstroMouseButtonState {
    bool down = false;
    bool pressed = false;
    bool released = false;
    bool double_clicked = false;
    float down_time = -1.0f;
    float clicked_time = -1.0f;
    float clicked_pos_x = 0.0f;
    float clicked_pos_y = 0.0f;
};

struct MonstroMouseState {
    float pos_x = 0.0f, pos_y = 0.0f;
    float prev_pos_x = 0.0f, prev_pos_y = 0.0f;
    float delta_x = 0.0f, delta_y = 0.0f;
    float wheel_x = 0.0f, wheel_y = 0.0f;
    MonstroMouseButtonState buttons[MONSTRO_MOUSE_BUTTON_COUNT];
};

struct MonstroKeyState {
    bool down = false;
    bool pressed = false;
    bool released = false;
    float down_time = -1.0f;
};

struct MonstroKeyboardState {
    MonstroKeyState keys[MONSTRO_KEY_COUNT];
    MonstroModFlags mods = MONSTRO_MOD_NONE;
    char input_text[256] = {};
    int input_text_len = 0;
};

#endif
