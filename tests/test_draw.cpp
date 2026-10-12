// Drawing output.
#include "harness.h"
#include <cmath>
#include <cstring>

TEST(rounded_rect_corners_lie_on_their_arcs)
{
   Ui ui(false);
   ui.begin();
   uint32_t before = ui.vertexCount();
   MonstroRect rect = {100.0f, 100.0f, 80.0f, 60.0f};
   float radius = 10.0f;
   monstro_draw_rect_filled(monstro_get_window_canvas(ui.ctx), rect, 0xffffffff, radius, MONSTRO_CORNER_ALL);
   const MonstroDrawData *data = ui.end();

   const MonstroVertex *vertices = data->vertices;
   CHECK(data->vertex_count - before == 16);

   // The first arc is the upper-left corner, centred radius in from the corner.
   float cx = rect.x + radius;
   float cy = rect.y + radius;
   for (uint32_t i = before; i < before + 4; i++)
   {
      float dx = vertices[i].position[0] - cx;
      float dy = vertices[i].position[1] - cy;
      CHECK(std::fabs(std::sqrt(dx * dx + dy * dy) - radius) < 0.001f);
      CHECK(dx <= 0.001f && dy <= 0.001f);
   }
}

TEST(color_conversion_clamps_channels)
{
   CHECK(monstro_color_to_u32(monstro_color_rgba(2.0f, -1.0f, NAN, 0.5f)) == 0x800000ffu);
   CHECK(monstro_color_to_u32(monstro_color_rgba(1.0f, 0.0f, INFINITY, -INFINITY)) == 0x00ff00ffu);
   CHECK(monstro_color_to_u32(monstro_color_from_u32(0xaabbccddu)) == 0xaabbccddu);
}

// Where the first glyph of an aligned "H" lands in a frame of its own.
static MonstroVec2 aligned_origin(Ui &ui, MonstroRect rect, int align)
{
   ui.begin();
   uint32_t first = ui.vertexCount();
   monstro_draw_text_aligned(monstro_get_window_canvas(ui.ctx), ui.font, rect, 0xffffffff, "H", (MonstroAlign)align);
   const MonstroDrawData *data = ui.end();
   return {data->vertices[first].position[0], data->vertices[first].position[1]};
}

static bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

TEST(text_aligns_left_centre_and_right_as_named)
{
   Ui ui(true);
   MonstroRect rect = {100.0f, 100.0f, 200.0f, 80.0f};
   MonstroVec2 size = monstro_font_measure_text(ui.font, "H", 0.0f);

   MonstroVec2 left = aligned_origin(ui, rect, MONSTRO_ALIGN_LEFT | MONSTRO_ALIGN_TOP);
   MonstroVec2 centre = aligned_origin(ui, rect, MONSTRO_ALIGN_HCENTER | MONSTRO_ALIGN_TOP);
   MonstroVec2 right = aligned_origin(ui, rect, MONSTRO_ALIGN_RIGHT | MONSTRO_ALIGN_TOP);
   // The glyph sits at its own small offset from the text's origin, the same in every case.
   CHECK(left.x >= rect.x && left.x < rect.x + 4.0f);
   CHECK(near(centre.x - left.x, (rect.w - size.x) * 0.5f));
   CHECK(near(right.x - left.x, rect.w - size.x));
   CHECK(left.y == centre.y && centre.y == right.y);

   MonstroVec2 middle = aligned_origin(ui, rect, MONSTRO_ALIGN_LEFT | MONSTRO_ALIGN_VCENTER);
   MonstroVec2 bottom = aligned_origin(ui, rect, MONSTRO_ALIGN_LEFT | MONSTRO_ALIGN_BOTTOM);
   CHECK(near(middle.y - left.y, (rect.h - size.y) * 0.5f));
   CHECK(near(bottom.y - left.y, rect.h - size.y));
   CHECK(middle.x == left.x && bottom.x == left.x);

   // One flag alone keeps its edge, no flag is the top left, and the presets combine both axes.
   MonstroVec2 only_left = aligned_origin(ui, rect, MONSTRO_ALIGN_LEFT);
   MonstroVec2 none = aligned_origin(ui, rect, 0);
   MonstroVec2 preset_default = aligned_origin(ui, rect, MONSTRO_ALIGN_DEFAULT);
   MonstroVec2 preset_centre = aligned_origin(ui, rect, MONSTRO_ALIGN_CENTER);
   CHECK(only_left.x == left.x && only_left.y == left.y);
   CHECK(none.x == left.x && none.y == left.y);
   CHECK(preset_default.x == left.x && preset_default.y == middle.y);
   CHECK(preset_centre.x == centre.x && preset_centre.y == middle.y);
}

// A bitmap font's glyph fills the line its text starts on, from the top of the text to font_size below it, as a
// TrueType font's glyphs sit within their line.
TEST(bitmap_font_text_draws_on_its_own_line)
{
   Ui ui(false);
   const float size = 16.0f;
   static uint8_t pixels[16 * 8 * 4];
   memset(pixels, 0xff, sizeof(pixels));
   MonstroFont *font = monstro_font_load_bitmap(ui.env, "cells", pixels, 16, 8, 2, 1, 'A', size);
   CHECK(font != nullptr);

   ui.begin();
   uint32_t first = ui.vertexCount();
   monstro_draw_text(monstro_get_window_canvas(ui.ctx), font, {100.0f, 100.0f}, 0xffffffff, "A");
   const MonstroDrawData *data = ui.end();

   CHECK(data->vertex_count > first);
   float top = INFINITY, bottom = -INFINITY;
   for (uint32_t i = first; i < data->vertex_count; i++)
   {
      top = std::fmin(top, data->vertices[i].position[1]);
      bottom = std::fmax(bottom, data->vertices[i].position[1]);
   }
   CHECK(near(top, 100.0f));
   CHECK(near(bottom, 100.0f + size));
}
