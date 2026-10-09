// Drawing output.
#include "harness.h"
#include <cmath>

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
