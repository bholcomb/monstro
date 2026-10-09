// Values shown and edited by sliders, numeric fields and progress bars.
#include "harness.h"
#include <climits>
#include <cstring>

template <typename Draw> static void frame(Ui &ui, Draw draw)
{
   ui.begin();
   draw();
   ui.end();
}

template <typename Draw> static void click(Ui &ui, float x, float y, Draw draw)
{
   ui.moveMouse(x, y);
   ui.pressLeft(true);
   frame(ui, draw);
   ui.pressLeft(false);
   frame(ui, draw);
}

template <typename Draw> static void key(Ui &ui, MonstroKey which, Draw draw)
{
   ui.pressKey(which, true);
   frame(ui, draw);
   ui.pressKey(which, false);
   frame(ui, draw);
}

// Vertices a drawing adds in a fresh context; every glyph is one quad.
template <typename Draw> static uint32_t vertices_of(Draw draw)
{
   Ui ui;
   frame(ui, [&]() { draw(ui.ctx); });
   return ui.vertexCount();
}

static uint32_t int_slider_glyphs(int value, const char *format)
{
   uint32_t with_text = vertices_of([&](MonstroContext *ctx)
                                    { monstro_slider_int(ctx, "##s", &value, INT_MIN, INT_MAX, format, nullptr); });
   uint32_t without_text =
       vertices_of([&](MonstroContext *ctx) { monstro_slider_int(ctx, "##s", &value, INT_MIN, INT_MAX, "", nullptr); });
   return (with_text - without_text) / 4;
}

TEST(int_slider_formats_its_value_as_an_int)
{
   CHECK(int_slider_glyphs(5, "%d") == 1);
   CHECK(int_slider_glyphs(12345, "%d") == 5);
   CHECK(int_slider_glyphs(-7, "%d") == 2);
   CHECK(int_slider_glyphs(12345, nullptr) == 5);
}

TEST(int_slider_rounds_negative_values_to_the_nearest_int)
{
   Ui ui;
   int value = 0;
   MonstroRect item = {};
   auto draw = [&]()
   {
      monstro_slider_int(ui.ctx, "##s", &value, -10, 0, "%d", nullptr);
      item = monstro_get_item_rect(ui.ctx);
   };
   frame(ui, draw);
   float spacing = monstro_get_slider_style(ui.ctx)->spacing[0];
   float slider_x = item.x + spacing;
   float slider_w = item.w - spacing;
   click(ui, slider_x + slider_w * 0.74f, item.y + item.h * 0.5f, draw);
   CHECK(value == -3);
}

TEST(int_slider_keeps_ints_a_float_cannot_hold)
{
   Ui ui;
   int value = 16777217;
   bool changed = true;
   frame(ui, [&]() { changed = monstro_slider_int(ui.ctx, "##s", &value, 0, INT_MAX, "%d", nullptr); });
   CHECK(!changed);
   CHECK(value == 16777217);
}

TEST(input_int_steps_ints_a_float_cannot_hold)
{
   Ui ui;
   int value = 16777217;
   MonstroRect item = {};
   auto draw = [&]()
   {
      monstro_input_int(ui.ctx, "##n", &value, 1, 10, nullptr);
      item = monstro_get_item_rect(ui.ctx);
   };
   frame(ui, draw);
   CHECK(value == 16777217);
   // The + button is the rightmost square of the field.
   click(ui, item.x + item.w - item.h * 0.5f, item.y + item.h * 0.5f, draw);
   CHECK(value == 16777218);
}

TEST(input_double_keeps_full_precision)
{
   Ui ui;
   double value = 0.1;
   bool changed = true;
   MonstroRect item = {};
   auto draw = [&]()
   {
      changed = monstro_input_double(ui.ctx, "##d", &value, 0.0, 0.0, "%.12f", nullptr);
      item = monstro_get_item_rect(ui.ctx);
   };
   frame(ui, draw);
   CHECK(!changed);
   CHECK(value == 0.1);

   click(ui, item.x + item.w - 4.0f, item.y + item.h * 0.5f, draw);
   ui.type("0.123456789012");
   frame(ui, draw);
   key(ui, MONSTRO_KEY_ENTER, draw);
   CHECK(value == 0.123456789012);
}

TEST(progress_overlay_is_plain_text)
{
   auto bar = [](const char *overlay)
   { return vertices_of([&](MonstroContext *ctx) { monstro_progress_bar(ctx, 0.5f, 200.0f, 20.0f, overlay); }); };
   CHECK((bar("%s%s%s") - bar("")) / 4 == 6);
   CHECK((bar("100%") - bar("")) / 4 == 4);
   CHECK((bar(nullptr) - bar("")) / 4 == 3);
}
