// What cockpit screens need.
#include "context.h"
#include "harness.h"
#include <cmath>

static bool near(float a, float b, float tolerance = 0.01f) { return std::fabs(a - b) < tolerance; }

// One fixed window filling the display, without decorations, as a cockpit screen draws.
static void begin_screen(Ui &ui)
{
   monstro_set_next_window_pos(ui.ctx, 0, 0, MONSTRO_COND_ALWAYS);
   monstro_set_next_window_size(ui.ctx, 512, 512, MONSTRO_COND_ALWAYS);
   monstro_begin_window(ui.ctx, "screen", nullptr,
                        (MonstroWindowFlags)(MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_INPUTS));
}

/* ------------------------------------------------------------------------- */
/* Culling                                                                   */
/* ------------------------------------------------------------------------- */

TEST(geometry_wholly_outside_the_clip_rect_is_not_drawn)
{
   Ui ui;
   ui.begin();
   begin_screen(ui);
   MonstroCanvas *canvas = monstro_get_window_canvas(ui.ctx);
   monstro_push_clip_rect(canvas, {100, 100, 100, 100});
   uint32_t vertices = ui.vertexCount();
   uint32_t commands = ui.ctx->canvas->command_count;

   // Each of these lies wholly outside the clip rect, some touching its edge.
   monstro_draw_rect_filled(canvas, {300, 300, 50, 50}, 0xffffffff, 0, MONSTRO_CORNER_NONE);
   monstro_draw_rect_filled(canvas, {40, 100, 60, 50}, 0xffffffff, 8, MONSTRO_CORNER_ALL);
   monstro_draw_rect(canvas, {0, 0, 50, 50}, 0xffffffff, 0, MONSTRO_CORNER_NONE, 2);
   monstro_draw_line(canvas, {0, 250}, {400, 250}, 0xffffffff, 2);
   monstro_draw_circle_filled(canvas, {50, 50}, 20, 0xffffffff, 16);
   monstro_draw_circle(canvas, {350, 150}, 20, 0xffffffff, 16, 2);
   monstro_draw_triangle_filled(canvas, {0, 0}, {50, 0}, {0, 50}, 0xffffffff);
   monstro_draw_quad_filled(canvas, {300, 0}, {350, 0}, {350, 50}, {300, 50}, 0xffffffff);
   monstro_draw_ellipse_filled(canvas, {150, 300}, {40, 20}, 0.5f, 0xffffffff, 16);
   monstro_draw_image(canvas, 7, {210, 120, 30, 30}, {0, 0}, {1, 1}, 0xffffffff);
   monstro_draw_text(canvas, ui.font, {300, 120}, 0xffffffff, "far away");
   CHECK(ui.vertexCount() == vertices && ui.ctx->canvas->command_count == commands);

   // A shape partly inside is drawn whole; the clip rect cuts it.
   monstro_draw_rect_filled(canvas, {80, 80, 50, 50}, 0xffffffff, 0, MONSTRO_CORNER_NONE);
   CHECK(ui.vertexCount() == vertices + 4);
   // A line's thickness counts: one just outside but thick enough to reach in is drawn.
   monstro_draw_line(canvas, {120, 98}, {180, 98}, 0xffffffff, 6);
   CHECK(ui.vertexCount() == vertices + 8);

   // A clip rect with no area shows nothing at all.
   monstro_push_clip_rect(canvas, {300, 300, 10, 10});
   monstro_draw_rect_filled(canvas, {0, 0, 512, 512}, 0xffffffff, 0, MONSTRO_CORNER_NONE);
   CHECK(ui.vertexCount() == vertices + 8);
   monstro_pop_clip_rect(canvas);
   monstro_pop_clip_rect(canvas);
   monstro_end_window(ui.ctx);
   ui.end();
}

TEST(a_widget_clipped_away_draws_nothing_but_still_takes_its_place)
{
   Ui ui;
   MonstroRect first = {}, hidden = {}, after[2] = {};
   uint32_t before = 0, drawn_hidden = 0;
   for (int clip_it = 0; clip_it < 2; clip_it++)
   {
      ui.begin();
      begin_screen(ui);
      MonstroCanvas *canvas = monstro_get_window_canvas(ui.ctx);
      monstro_button_sized(ui.ctx, "first", 100, 30, nullptr);
      first = monstro_get_item_rect(ui.ctx);
      before = ui.vertexCount();
      if (clip_it)
         monstro_push_clip_rect(canvas, {0, 0, 0, 0});
      float progress = 0.5f;
      monstro_button_sized(ui.ctx, "hidden", 100, 30, nullptr);
      hidden = monstro_get_item_rect(ui.ctx);
      monstro_slider_float(ui.ctx, "slider", &progress, 0, 1, "%.2f", nullptr);
      monstro_progress_bar(ui.ctx, progress, 100, 20, nullptr);
      if (clip_it)
         monstro_pop_clip_rect(canvas);
      drawn_hidden = ui.vertexCount() - before;
      monstro_button_sized(ui.ctx, "after", 100, 30, nullptr);
      after[clip_it] = monstro_get_item_rect(ui.ctx);
      monstro_end_window(ui.ctx);
      ui.end();
      // Drawn, the three widgets make geometry; clipped away, none.
      CHECK(clip_it ? drawn_hidden == 0 : drawn_hidden > 0);
      CHECK(hidden.y > first.y && hidden.h == 30);
   }
   // Clipped away, they kept their room: what follows them is where it was.
   CHECK(after[1].y == after[0].y && after[1].y > hidden.y + hidden.h);
}

/* ------------------------------------------------------------------------- */
/* Ellipses, turned quads and turned text                                    */
/* ------------------------------------------------------------------------- */

TEST(an_ellipse_outline_lies_on_its_turned_radii)
{
   Ui ui(false);
   ui.begin();
   MonstroCanvas *canvas = monstro_get_window_canvas(ui.ctx);
   uint32_t first = ui.vertexCount();
   MonstroVec2 center = {200, 150};
   const float quarter = 1.5707963f;
   monstro_draw_ellipse_filled(canvas, center, {60, 20}, 0.0f, 0xff00ff00, 8);
   monstro_draw_ellipse_filled(canvas, center, {60, 20}, quarter, 0xff00ff00, 8);
   const MonstroDrawData *data = ui.end();
   CHECK(data->vertex_count - first == 16);
   const MonstroVertex *flat = data->vertices + first;
   const MonstroVertex *turned = flat + 8;
   // Unturned, the first point is at the end of the x radius and the third at the end of the y radius (+y down).
   CHECK(near(flat[0].position[0], 260) && near(flat[0].position[1], 150));
   CHECK(near(flat[2].position[0], 200) && near(flat[2].position[1], 170));
   // A quarter turn clockwise puts the x radius straight down.
   CHECK(near(turned[0].position[0], 200) && near(turned[0].position[1], 210));
   CHECK(near(turned[2].position[0], 180) && near(turned[2].position[1], 150));
   for (int i = 0; i < 16; i++)
      CHECK(flat[i].color == 0xff00ff00);
}

TEST(an_ellipse_outline_is_its_segments_stroked)
{
   Ui ui(false);
   ui.begin();
   uint32_t first = ui.vertexCount();
   monstro_draw_ellipse(monstro_get_window_canvas(ui.ctx), {200, 150}, {60, 20}, 0.3f, 0xffffffff, 12, 2.0f);
   const MonstroDrawData *data = ui.end();
   // One quad per segment, closed.
   CHECK(data->vertex_count - first == 12 * 4);
}

FATAL_TEST(an_ellipse_of_two_segments_aborts, "monstro_draw_ellipse: num_segments is 2")
{
   Ui ui(false);
   ui.begin();
   monstro_draw_ellipse(monstro_get_window_canvas(ui.ctx), {200, 150}, {60, 20}, 0.0f, 0xffffffff, 2, 1.0f);
}

TEST(a_turned_quad_keeps_its_corners_and_colour)
{
   Ui ui(false);
   ui.begin();
   uint32_t first = ui.vertexCount();
   // A horizon's ground, rolled 30 degrees about the screen's centre.
   MonstroVec2 corners[4] = {{120.0f, 200.0f}, {420.0f, 373.2f}, {320.0f, 546.4f}, {20.0f, 373.2f}};
   monstro_draw_quad_filled(monstro_get_window_canvas(ui.ctx), corners[0], corners[1], corners[2], corners[3],
                            0xff204060);
   const MonstroDrawData *data = ui.end();
   CHECK(data->vertex_count - first == 4);
   for (int i = 0; i < 4; i++)
   {
      const MonstroVertex &v = data->vertices[first + i];
      CHECK(v.position[0] == corners[i].x && v.position[1] == corners[i].y && v.color == 0xff204060);
   }
}

TEST(turned_text_is_the_aligned_text_turned_about_its_pivot)
{
   Ui ui;
   MonstroRect rect = {100, 100, 120, 40};
   MonstroVec2 pivot = {160, 200};
   const float angle = 0.5f;
   ui.begin();
   MonstroCanvas *canvas = monstro_get_window_canvas(ui.ctx);
   uint32_t upright_first = ui.vertexCount();
   monstro_draw_text_aligned(canvas, ui.font, rect, 0xffffffff, "10", MONSTRO_ALIGN_CENTER);
   uint32_t turned_first = ui.vertexCount();
   monstro_draw_text_rotated(canvas, ui.font, rect, 0xffffffff, "10", MONSTRO_ALIGN_CENTER, pivot, angle);
   uint32_t unturned_first = ui.vertexCount();
   monstro_draw_text_rotated(canvas, ui.font, rect, 0xffffffff, "10", MONSTRO_ALIGN_CENTER, pivot, 0.0f);
   const MonstroDrawData *data = ui.end();

   uint32_t count = turned_first - upright_first;
   CHECK(count == 8 && unturned_first - turned_first == count && data->vertex_count - unturned_first == count);
   float c = std::cos(angle), s = std::sin(angle);
   for (uint32_t i = 0; i < count; i++)
   {
      const MonstroVertex &upright = data->vertices[upright_first + i];
      const MonstroVertex &turned = data->vertices[turned_first + i];
      const MonstroVertex &unturned = data->vertices[unturned_first + i];
      float dx = upright.position[0] - pivot.x, dy = upright.position[1] - pivot.y;
      CHECK(near(turned.position[0], pivot.x + dx * c - dy * s) && near(turned.position[1], pivot.y + dx * s + dy * c));
      CHECK(turned.texcoord[0] == upright.texcoord[0] && turned.texcoord[1] == upright.texcoord[1]);
      CHECK(near(unturned.position[0], upright.position[0]) && near(unturned.position[1], upright.position[1]));
   }
}
