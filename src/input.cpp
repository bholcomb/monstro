#include "context.h"
#include <string.h>

namespace monstro {

/* ------------------------------------------------------------------------- */
/* MouseState                                                                */
/* ------------------------------------------------------------------------- */

void MouseState::setButton(MonstroMouseButton button, bool is_down) {
    if (button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return;
    buttons[button].down = is_down;
}

bool MouseState::isDown(MonstroMouseButton button) const {
    if (button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    return buttons[button].down;
}

bool MouseState::isPressed(MonstroMouseButton button) const {
    if (button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    return buttons[button].pressed;
}

bool MouseState::isReleased(MonstroMouseButton button) const {
    if (button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    return buttons[button].released;
}

bool MouseState::isDoubleClicked(MonstroMouseButton button) const {
    if (button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    return buttons[button].double_clicked;
}

bool MouseState::isDragging(MonstroMouseButton button) const {
    if (button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    if (!buttons[button].down) return false;

    float dx = pos_x - buttons[button].clicked_pos_x;
    float dy = pos_y - buttons[button].clicked_pos_y;
    return (dx * dx + dy * dy) > (drag_threshold * drag_threshold);
}

MonstroVec2 MouseState::getDragDelta(MonstroMouseButton button) const {
    MonstroVec2 result = {0.0f, 0.0f};
    if (button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return result;

    if (buttons[button].down) {
        result.x = pos_x - buttons[button].clicked_pos_x;
        result.y = pos_y - buttons[button].clicked_pos_y;
    }
    return result;
}

bool MouseState::isClicked(MonstroMouseButton button, float rep_delay, float rep_rate, float dt) const {
    if (button < 0 || button >= MONSTRO_MOUSE_BUTTON_COUNT) return false;
    if (buttons[button].pressed) return true;

    if (buttons[button].down) {
        float down_time = buttons[button].down_time;
        if (down_time > rep_delay && rep_rate > 0.0f) {
            float time_since_first = down_time - rep_delay;
            int repeat_count = (int)(time_since_first / rep_rate);
            float time_prev = time_since_first - dt;
            int prev_repeat_count = (time_prev > 0.0f) ? (int)(time_prev / rep_rate) : -1;
            if (repeat_count > prev_repeat_count) return true;
        }
    }
    return false;
}

bool MouseState::anyButtonPressed() const {
    for (int i = 0; i < MONSTRO_MOUSE_BUTTON_COUNT; i++) {
        if (buttons[i].pressed) return true;
    }
    return false;
}

void MouseState::newFrame(float dt, float current_time) {
    for (int i = 0; i < MONSTRO_MOUSE_BUTTON_COUNT; i++) {
        ButtonState* btn = &buttons[i];

        btn->pressed = btn->down && (btn->down_time < 0.0f);
        btn->released = !btn->down && (btn->down_time >= 0.0f);
        btn->double_clicked = false;

        if (btn->pressed) {
            float dx = pos_x - btn->clicked_pos_x;
            float dy = pos_y - btn->clicked_pos_y;
            float dist_sq = dx * dx + dy * dy;
            float time_since_click = current_time - btn->clicked_time;

            if (time_since_click < double_click_time &&
                dist_sq < double_click_distance * double_click_distance) {
                btn->double_clicked = true;
                btn->clicked_time = -1.0f;
            } else {
                btn->clicked_time = current_time;
            }

            btn->clicked_pos_x = pos_x;
            btn->clicked_pos_y = pos_y;
        }

        if (btn->down) {
            btn->down_time = (btn->down_time < 0.0f) ? 0.0f : btn->down_time + dt;
        } else {
            btn->down_time = -1.0f;
        }
    }

    delta_x = pos_x - prev_pos_x;
    delta_y = pos_y - prev_pos_y;
    prev_pos_x = pos_x;
    prev_pos_y = pos_y;
}

void MouseState::endFrame() {
    wheel_x = 0.0f;
    wheel_y = 0.0f;
}

/* ------------------------------------------------------------------------- */
/* KeyboardState                                                             */
/* ------------------------------------------------------------------------- */

void KeyboardState::setKey(MonstroKey key, bool is_down) {
    if (key < 0 || key >= MONSTRO_KEY_COUNT) return;
    keys[key].down = is_down;
}

void KeyboardState::addInputText(const char* text) {
    if (!text) return;

    int available = (int)sizeof(input_text) - input_text_len - 1;
    if (available <= 0) return;

    int len = 0;
    while (text[len] && len < available) len++;

    memcpy(input_text + input_text_len, text, len);
    input_text_len += len;
    input_text[input_text_len] = '\0';
}

bool KeyboardState::isDown(MonstroKey key) const {
    if (key < 0 || key >= MONSTRO_KEY_COUNT) return false;
    return keys[key].down;
}

bool KeyboardState::isPressed(MonstroKey key) const {
    if (key < 0 || key >= MONSTRO_KEY_COUNT) return false;
    return keys[key].pressed;
}

bool KeyboardState::isReleased(MonstroKey key) const {
    if (key < 0 || key >= MONSTRO_KEY_COUNT) return false;
    return keys[key].released;
}

void KeyboardState::newFrame(float dt) {
    for (int i = 0; i < MONSTRO_KEY_COUNT; i++) {
        KeyState* key = &keys[i];

        key->pressed = key->down && (key->down_time < 0.0f);
        key->released = !key->down && (key->down_time >= 0.0f);

        if (key->down) {
            key->down_time = (key->down_time < 0.0f) ? 0.0f : key->down_time + dt;
        } else {
            key->down_time = -1.0f;
        }
    }
}

void KeyboardState::endFrame() {
    input_text_len = 0;
    input_text[0] = '\0';
}

} // namespace monstro

/* ------------------------------------------------------------------------- */
/* C API Trampolines                                                         */
/* ------------------------------------------------------------------------- */

extern "C" {

void monstro_set_mouse_pos(MonstroContext* ctx, float x, float y) {
    if (ctx) ctx->mouse.setPos(x, y);
}

void monstro_set_mouse_button(MonstroContext* ctx, MonstroMouseButton button, bool down) {
    if (ctx) ctx->mouse.setButton(button, down);
}

void monstro_set_mouse_wheel(MonstroContext* ctx, float x, float y) {
    if (ctx) ctx->mouse.setWheel(x, y);
}

void monstro_set_key(MonstroContext* ctx, MonstroKey key, bool down) {
    if (ctx) ctx->keyboard.setKey(key, down);
}

void monstro_set_key_mods(MonstroContext* ctx, MonstroModFlags mods) {
    if (ctx) ctx->keyboard.setMods(mods);
}

void monstro_add_input_text(MonstroContext* ctx, const char* text) {
    if (ctx) ctx->keyboard.addInputText(text);
}

void monstro_set_mouse_cursor(MonstroContext* ctx, MonstroMouseCursor cursor) {
    if (!ctx) return;
    ctx->mouse_cursor = cursor;
}

bool monstro_is_mouse_down(MonstroContext* ctx, MonstroMouseButton button) {
    return ctx ? ctx->mouse.isDown(button) : false;
}

bool monstro_is_mouse_clicked(MonstroContext* ctx, MonstroMouseButton button, bool repeat) {
    if (!ctx) return false;
    if (!repeat) return ctx->mouse.isPressed(button);
    return ctx->mouse.isClicked(button, ctx->keyboard.repeat_delay, ctx->keyboard.repeat_rate, ctx->delta_time);
}

bool monstro_is_mouse_released(MonstroContext* ctx, MonstroMouseButton button) {
    return ctx ? ctx->mouse.isReleased(button) : false;
}

bool monstro_is_mouse_double_clicked(MonstroContext* ctx, MonstroMouseButton button) {
    return ctx ? ctx->mouse.isDoubleClicked(button) : false;
}

MonstroVec2 monstro_get_mouse_pos(MonstroContext* ctx) {
    if (!ctx) return {0.0f, 0.0f};
    return ctx->mouse.getPos();
}

MonstroVec2 monstro_get_mouse_delta(MonstroContext* ctx) {
    if (!ctx) return {0.0f, 0.0f};
    return ctx->mouse.getDelta();
}

float monstro_get_mouse_wheel(MonstroContext* ctx) {
    return ctx ? ctx->mouse.wheel_y : 0.0f;
}

float monstro_get_mouse_wheel_h(MonstroContext* ctx) {
    return ctx ? ctx->mouse.wheel_x : 0.0f;
}

bool monstro_is_mouse_dragging(MonstroContext* ctx, MonstroMouseButton button) {
    return ctx ? ctx->mouse.isDragging(button) : false;
}

MonstroVec2 monstro_get_mouse_drag_delta(MonstroContext* ctx, MonstroMouseButton button) {
    if (!ctx) return {0.0f, 0.0f};
    return ctx->mouse.getDragDelta(button);
}

bool monstro_is_key_down(MonstroContext* ctx, MonstroKey key) {
    return ctx ? ctx->keyboard.isDown(key) : false;
}

bool monstro_is_key_pressed(MonstroContext* ctx, MonstroKey key, bool repeat) {
    if (!ctx) return false;
    if (ctx->keyboard.isPressed(key)) return true;
    if (repeat && ctx->keyboard.isDown(key)) {
        float down_time = ctx->keyboard.keys[key].down_time;
        if (down_time > ctx->keyboard.repeat_delay && ctx->keyboard.repeat_rate > 0.0f) {
            float time_since_first = down_time - ctx->keyboard.repeat_delay;
            int repeat_count = (int)(time_since_first / ctx->keyboard.repeat_rate);
            float time_prev = time_since_first - ctx->delta_time;
            int prev_repeat_count = (time_prev > 0.0f) ? (int)(time_prev / ctx->keyboard.repeat_rate) : -1;
            if (repeat_count > prev_repeat_count) return true;
        }
    }
    return false;
}

bool monstro_is_key_released(MonstroContext* ctx, MonstroKey key) {
    return ctx ? ctx->keyboard.isReleased(key) : false;
}

MonstroModFlags monstro_get_key_mods(MonstroContext* ctx) {
    return ctx ? ctx->keyboard.getMods() : MONSTRO_MOD_NONE;
}

const char* monstro_get_input_text(MonstroContext* ctx) {
    return ctx ? ctx->keyboard.getInputText() : "";
}

MonstroMouseCursor monstro_get_mouse_cursor(MonstroContext* ctx) {
    return ctx ? ctx->mouse_cursor : MONSTRO_MOUSE_CURSOR_ARROW;
}

bool monstro_want_capture_mouse(MonstroContext* ctx) {
    return ctx ? ctx->want_capture_mouse : false;
}

bool monstro_want_capture_keyboard(MonstroContext* ctx) {
    return ctx ? ctx->want_capture_keyboard : false;
}

bool monstro_want_text_input(MonstroContext* ctx) {
    return ctx ? ctx->want_text_input : false;
}

void monstro_set_double_click_distance(MonstroContext* ctx, float distance) {
    if (ctx) ctx->mouse.double_click_distance = distance;
}

void monstro_set_mouse_drag_threshold(MonstroContext* ctx, float threshold) {
    if (ctx) ctx->mouse.drag_threshold = threshold;
}

void monstro_set_key_repeat_delay(MonstroContext* ctx, float delay) {
    if (ctx) ctx->keyboard.repeat_delay = delay;
}

void monstro_set_key_repeat_rate(MonstroContext* ctx, float rate) {
    if (ctx) ctx->keyboard.repeat_rate = rate;
}

float monstro_get_double_click_distance(MonstroContext* ctx) {
    return ctx ? ctx->mouse.double_click_distance : 6.0f;
}

float monstro_get_mouse_drag_threshold(MonstroContext* ctx) {
    return ctx ? ctx->mouse.drag_threshold : 6.0f;
}

float monstro_get_key_repeat_delay(MonstroContext* ctx) {
    return ctx ? ctx->keyboard.repeat_delay : 0.25f;
}

float monstro_get_key_repeat_rate(MonstroContext* ctx) {
    return ctx ? ctx->keyboard.repeat_rate : 0.05f;
}

} // extern "C"
