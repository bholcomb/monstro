#ifndef MONSTRO_INTERNAL_INPUT_H
#define MONSTRO_INTERNAL_INPUT_H

#include "monstro.h"

namespace monstro
{

struct ButtonState
{
   bool down = false;
   bool pressed = false;
   bool released = false;
   bool double_clicked = false;
   float down_time = -1.0f;
   float clicked_time = -1.0f;
   float clicked_pos_x = 0.0f;
   float clicked_pos_y = 0.0f;
};

struct MouseState
{
   float pos_x = 0.0f, pos_y = 0.0f;
   float prev_pos_x = 0.0f, prev_pos_y = 0.0f;
   float delta_x = 0.0f, delta_y = 0.0f;
   float wheel_x = 0.0f, wheel_y = 0.0f;
   ButtonState buttons[MONSTRO_MOUSE_BUTTON_COUNT];

   float double_click_time = 0.30f;
   float double_click_distance = 6.0f;
   float drag_threshold = 6.0f;

   void setPos(float x, float y)
   {
      pos_x = x;
      pos_y = y;
   }
   MonstroVec2 getPos() const { return {pos_x, pos_y}; }
   MonstroVec2 getDelta() const { return {delta_x, delta_y}; }

   void setButton(MonstroMouseButton button, bool is_down);
   void setWheel(float x, float y)
   {
      wheel_x = x;
      wheel_y = y;
   }

   bool isDown(MonstroMouseButton button) const;
   bool isPressed(MonstroMouseButton button) const;
   bool isReleased(MonstroMouseButton button) const;
   bool isDoubleClicked(MonstroMouseButton button) const;
   bool isDragging(MonstroMouseButton button) const;
   MonstroVec2 getDragDelta(MonstroMouseButton button) const;
   bool isClicked(MonstroMouseButton button, float repeat_delay, float repeat_rate, float dt) const;

   bool anyButtonPressed() const;
   bool anyButtonDown() const;

   void newFrame(float dt, float current_time);
   void endFrame();
};

struct KeyState
{
   bool down = false;
   bool pressed = false;
   bool released = false;
   float down_time = -1.0f;
};

struct KeyboardState
{
   KeyState keys[MONSTRO_KEY_COUNT];
   MonstroModFlags mods = MONSTRO_MOD_NONE;
   char input_text[256] = {};
   int input_text_len = 0;

   float repeat_delay = 0.25f;
   float repeat_rate = 0.05f;

   void setKey(MonstroKey key, bool is_down);
   void setMods(MonstroModFlags m) { mods = m; }
   void addInputText(const char *text);

   bool isDown(MonstroKey key) const;
   bool isPressed(MonstroKey key) const;
   bool isReleased(MonstroKey key) const;
   MonstroModFlags getMods() const { return mods; }
   const char *getInputText() const { return input_text; }

   void newFrame(float dt);
   void endFrame();
};

} // namespace monstro

#endif
