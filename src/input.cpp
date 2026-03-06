#include "internal/types.h"
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Input Setters                                                             */
/* ------------------------------------------------------------------------- */

void monstro_set_mouse_pos(MonstroContext* ctx, float x, float y) {
    if (!ctx) return;
    ctx->mouse.pos_x = x;
    ctx->mouse.pos_y = y;
}

void monstro_set_mouse_button(MonstroContext* ctx, MonstroMouseButton button, bool down) {
    if (!ctx || button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return;
    ctx->mouse.buttons[button].down = down;
}

void monstro_set_mouse_wheel(MonstroContext* ctx, float x, float y) {
    if (!ctx) return;
    ctx->mouse.wheel_x = x;
    ctx->mouse.wheel_y = y;
}

void monstro_set_key(MonstroContext* ctx, MonstroKey key, bool down) {
    if (!ctx || key < 0 || key >= MONSTRO_KEY_COUNT) return;
    ctx->keyboard.keys[key].down = down;
}

void monstro_set_key_mods(MonstroContext* ctx, MonstroModFlags mods) {
    if (!ctx) return;
    ctx->keyboard.mods = mods;
}

void monstro_add_input_text(MonstroContext* ctx, const char* text) {
    if (!ctx || !text) return;
    
    int available = (int)sizeof(ctx->keyboard.input_text) - ctx->keyboard.input_text_len - 1;
    if (available <= 0) return;
    
    int len = 0;
    while (text[len] && len < available) len++;
    
    memcpy(ctx->keyboard.input_text + ctx->keyboard.input_text_len, text, len);
    ctx->keyboard.input_text_len += len;
    ctx->keyboard.input_text[ctx->keyboard.input_text_len] = '\0';
}

void monstro_set_mouse_cursor(MonstroContext* ctx, MonstroMouseCursor cursor) {
    if (!ctx) return;
    ctx->mouse_cursor = cursor;
}

/* ------------------------------------------------------------------------- */
/* Input Getters                                                             */
/* ------------------------------------------------------------------------- */

bool monstro_is_mouse_down(MonstroContext* ctx, MonstroMouseButton button) {
    if (!ctx || button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    return ctx->mouse.buttons[button].down;
}

bool monstro_is_mouse_clicked(MonstroContext* ctx, MonstroMouseButton button, bool repeat) {
    if (!ctx || button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    
    if (ctx->mouse.buttons[button].pressed) return true;
    
    if (repeat && ctx->mouse.buttons[button].down) {
        float down_time = ctx->mouse.buttons[button].down_time;
        if (down_time > ctx->key_repeat_delay) {
            float repeat_interval = ctx->key_repeat_rate;
            if (repeat_interval > 0.0f) {
                float time_since_first = down_time - ctx->key_repeat_delay;
                int repeat_count = (int)(time_since_first / repeat_interval);
                float time_prev = time_since_first - ctx->delta_time;
                int prev_repeat_count = (time_prev > 0.0f) ? (int)(time_prev / repeat_interval) : -1;
                if (repeat_count > prev_repeat_count) return true;
            }
        }
    }
    
    return false;
}

bool monstro_is_mouse_released(MonstroContext* ctx, MonstroMouseButton button) {
    if (!ctx || button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    return ctx->mouse.buttons[button].released;
}

bool monstro_is_mouse_double_clicked(MonstroContext* ctx, MonstroMouseButton button) {
    if (!ctx || button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    return ctx->mouse.buttons[button].double_clicked;
}

MonstroVec2 monstro_get_mouse_pos(MonstroContext* ctx) {
    MonstroVec2 result = {0.0f, 0.0f};
    if (ctx) {
        result.x = ctx->mouse.pos_x;
        result.y = ctx->mouse.pos_y;
    }
    return result;
}

MonstroVec2 monstro_get_mouse_delta(MonstroContext* ctx) {
    MonstroVec2 result = {0.0f, 0.0f};
    if (ctx) {
        result.x = ctx->mouse.delta_x;
        result.y = ctx->mouse.delta_y;
    }
    return result;
}

void monstro_get_mouse_wheel(MonstroContext* ctx, float* x, float* y) {
    if (!ctx) {
        if (x) *x = 0;
        if (y) *y = 0;
        return;
    }
    if (x) *x = ctx->mouse.wheel_x;
    if (y) *y = ctx->mouse.wheel_y;
}

bool monstro_is_mouse_dragging(MonstroContext* ctx, MonstroMouseButton button) {
    if (!ctx || button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    
    if (!ctx->mouse.buttons[button].down) return false;
    
    float dx = ctx->mouse.pos_x - ctx->mouse.buttons[button].clicked_pos_x;
    float dy = ctx->mouse.pos_y - ctx->mouse.buttons[button].clicked_pos_y;
    
    return (dx * dx + dy * dy) > (ctx->mouse_drag_threshold * ctx->mouse_drag_threshold);
}

MonstroVec2 monstro_get_mouse_drag_delta(MonstroContext* ctx, MonstroMouseButton button) {
    MonstroVec2 result = {0.0f, 0.0f};
    if (!ctx || button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return result;
    
    if (ctx->mouse.buttons[button].down) {
        result.x = ctx->mouse.pos_x - ctx->mouse.buttons[button].clicked_pos_x;
        result.y = ctx->mouse.pos_y - ctx->mouse.buttons[button].clicked_pos_y;
    }
    return result;
}

bool monstro_is_key_down(MonstroContext* ctx, MonstroKey key) {
    if (!ctx || key < 0 || key >= MONSTRO_KEY_COUNT) return false;
    return ctx->keyboard.keys[key].down;
}

bool monstro_is_key_pressed(MonstroContext* ctx, MonstroKey key) {
    if (!ctx || key < 0 || key >= MONSTRO_KEY_COUNT) return false;
    return ctx->keyboard.keys[key].pressed;
}

bool monstro_is_key_released(MonstroContext* ctx, MonstroKey key) {
    if (!ctx || key < 0 || key >= MONSTRO_KEY_COUNT) return false;
    return ctx->keyboard.keys[key].released;
}

MonstroModFlags monstro_get_key_mods(MonstroContext* ctx) {
    if (!ctx) return MONSTRO_MOD_NONE;
    return ctx->keyboard.mods;
}

const char* monstro_get_input_text(MonstroContext* ctx) {
    if (!ctx) return "";
    return ctx->keyboard.input_text;
}

MonstroMouseCursor monstro_get_mouse_cursor(MonstroContext* ctx) {
    if (!ctx) return MONSTRO_MOUSE_CURSOR_ARROW;
    return ctx->mouse_cursor;
}

/* ------------------------------------------------------------------------- */
/* Capture State                                                             */
/* ------------------------------------------------------------------------- */

bool monstro_want_capture_mouse(MonstroContext* ctx) {
    return ctx ? ctx->want_capture_mouse : false;
}

bool monstro_want_capture_keyboard(MonstroContext* ctx) {
    return ctx ? ctx->want_capture_keyboard : false;
}

bool monstro_want_text_input(MonstroContext* ctx) {
    return ctx ? ctx->want_text_input : false;
}

/* ------------------------------------------------------------------------- */
/* Configuration                                                             */
/* ------------------------------------------------------------------------- */

void monstro_set_double_click_time(MonstroContext* ctx, float time) {
    if (!ctx) return;
    ctx->double_click_time = time;
}

void monstro_set_double_click_distance(MonstroContext* ctx, float distance) {
    if (!ctx) return;
    ctx->double_click_distance = distance;
}

void monstro_set_mouse_drag_threshold(MonstroContext* ctx, float threshold) {
    if (!ctx) return;
    ctx->mouse_drag_threshold = threshold;
}

void monstro_set_key_repeat_config(MonstroContext* ctx, float delay, float rate) {
    if (!ctx) return;
    ctx->key_repeat_delay = delay;
    ctx->key_repeat_rate = rate;
}
