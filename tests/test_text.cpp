// Text the library keeps between calls: tooltips, edit buffers, numeric edits.
#include "harness.h"
#include <cstdlib>
#include <cstring>

template <typename Draw> static void frame(Ui &ui, Draw draw, float delta_time = 1.0f / 60.0f)
{
   ui.begin(delta_time);
   draw();
   ui.end();
}

// Presses and releases the left button at x, y over two frames.
template <typename Draw> static void click(Ui &ui, float x, float y, Draw draw)
{
   ui.moveMouse(x, y);
   ui.pressLeft(true);
   frame(ui, draw);
   ui.pressLeft(false);
   frame(ui, draw);
}

// Holds a key down for one frame.
template <typename Draw> static void key(Ui &ui, MonstroKey which, Draw draw)
{
   ui.pressKey(which, true);
   frame(ui, draw);
   ui.pressKey(which, false);
   frame(ui, draw);
}

TEST(tooltip_text_is_copied_when_given)
{
   Ui ui;
   MonstroRect button = {};
   auto draw = [&]()
   {
      // The caller's string lives only for the call; the tooltip shows at the end of the frame.
      char *text = (char *)std::malloc(32);
      std::strcpy(text, "transient tooltip");
      monstro_button(ui.ctx, "Hover me", text);
      button = monstro_get_item_rect(ui.ctx);
      std::memset(text, 'X', 31);
      std::free(text);
   };
   frame(ui, draw);
   ui.moveMouse(button.x + button.w * 0.5f, button.y + button.h * 0.5f);
   uint32_t without_tooltip = 0;
   for (int i = 0; i < 6; i++)
   {
      frame(ui, draw, 0.25f);
      if (i == 0)
         without_tooltip = ui.vertexCount();
   }
   CHECK(ui.vertexCount() > without_tooltip);
}

FATAL_TEST(oversized_tooltip_aborts, "tooltip text is 2000 bytes; a tooltip holds at most 1023")
{
   Ui ui;
   static char text[2001];
   std::memset(text, 'a', 2000);
   monstro_begin_frame(ui.ctx, 0.1f);
   monstro_tooltip(ui.ctx, text);
}

TEST(text_edit_uses_the_buffer_passed_this_frame)
{
   Ui ui;
   char *buffer = (char *)std::malloc(64);
   std::strcpy(buffer, "abc");
   MonstroRect field = {};
   auto draw = [&]()
   {
      monstro_input_text(ui.ctx, "Name", buffer, 64, MONSTRO_INPUT_TEXT_NONE, nullptr);
      field = monstro_get_item_rect(ui.ctx);
   };
   frame(ui, draw);
   click(ui, field.x + field.w - 4.0f, field.y + field.h * 0.5f, draw);

   // The caller moves its text to a new buffer while the edit is active.
   char *moved = (char *)std::malloc(64);
   std::strcpy(moved, buffer);
   std::free(buffer);
   buffer = moved;

   ui.type("d");
   frame(ui, draw);
   CHECK(std::strcmp(buffer, "abcd") == 0);
   key(ui, MONSTRO_KEY_BACKSPACE, draw);
   CHECK(std::strcmp(buffer, "abc") == 0);
   std::free(buffer);
}

TEST(numeric_edits_in_two_contexts_do_not_share_text)
{
   Ui a;
   Ui b;
   float value_a = 1.0f;
   float value_b = 2.0f;
   MonstroRect field_a = {};
   MonstroRect field_b = {};
   auto draw_a = [&]()
   {
      monstro_input_float(a.ctx, "A", &value_a, 0.0f, 0.0f, "%.1f", nullptr);
      field_a = monstro_get_item_rect(a.ctx);
   };
   auto draw_b = [&]()
   {
      monstro_input_float(b.ctx, "B", &value_b, 0.0f, 0.0f, "%.1f", nullptr);
      field_b = monstro_get_item_rect(b.ctx);
   };
   frame(a, draw_a);
   frame(b, draw_b);
   click(a, field_a.x + field_a.w - 4.0f, field_a.y + field_a.h * 0.5f, draw_a);
   click(b, field_b.x + field_b.w - 4.0f, field_b.y + field_b.h * 0.5f, draw_b);

   a.type("5");
   frame(a, draw_a);
   key(b, MONSTRO_KEY_ENTER, draw_b);
   key(a, MONSTRO_KEY_ENTER, draw_a);
   CHECK(value_a == 5.0f);
   CHECK(value_b == 2.0f);
}
