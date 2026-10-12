// What cockpit screens need: culling, ellipses, turned text, vertical sliders and bars, absolute knobs, page tabs,
// host textures.
#include "context.h"
#include "harness.h"
#include <cmath>

static bool near(float a, float b, float tolerance = 0.01f) { return std::fabs(a - b) < tolerance; }

static MonstroVec2 middle(MonstroRect r) { return {r.x + r.w * 0.5f, r.y + r.h * 0.5f}; }

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

/* ------------------------------------------------------------------------- */
/* Vertical sliders                                                          */
/* ------------------------------------------------------------------------- */

// A screen with one control placed at (100, 100), drawn by draw each frame.
template <typename Draw> struct OneControl
{
   Ui ui;
   Draw draw;
   MonstroRect rect = {};
   bool changed = false, activated = false, deactivated = false;

   explicit OneControl(Draw d) : draw(d) {}

   void frame()
   {
      ui.begin();
      begin_screen(ui);
      monstro_set_cursor_screen_pos(ui.ctx, 100, 100);
      changed = draw(ui.ctx);
      rect = monstro_get_item_rect(ui.ctx);
      activated = monstro_is_item_activated(ui.ctx);
      deactivated = monstro_is_item_deactivated(ui.ctx);
      monstro_end_window(ui.ctx);
      ui.end();
   }

   void press(float x, float y)
   {
      ui.moveMouse(x, y);
      ui.pressLeft(true);
      frame();
   }

   void release()
   {
      ui.pressLeft(false);
      frame();
   }

   // Pressed and released between two frames, then one frame.
   void tap(float x, float y)
   {
      ui.moveMouse(x, y);
      ui.pressLeft(true);
      ui.pressLeft(false);
      frame();
   }
};

template <typename Draw> static OneControl<Draw> one_control(Draw draw) { return OneControl<Draw>(draw); }

TEST(a_vertical_slider_runs_from_its_minimum_at_the_bottom_to_its_maximum_at_the_top)
{
   float value = 0.0f;
   auto t = one_control(
       [&](MonstroContext *ctx)
       {
          return monstro_slider_float_ex(ctx, "lift##", &value, 0.0f, 100.0f, nullptr, 30.0f, 200.0f,
                                         MONSTRO_SLIDER_VERTICAL, nullptr);
       });
   t.frame();
   // No shown label: the slider is its bar alone, at the cursor (below the slider's padding).
   MonstroRect r = t.rect;
   CHECK(r.x == 100 && r.w == 30 && r.h == 200);

   // Held at 75 (the 16 px cursor's centre travels 184 px, from 8 px in at either end), then dragged to the bottom.
   t.press(r.x + 15, r.y + 8 + 0.25f * 184);
   CHECK(t.changed && t.activated && near(value, 75.0f));
   t.ui.moveMouse(r.x + 15, r.y + 300);
   t.frame();
   CHECK(near(value, 0.0f));
   t.release();
   CHECK(t.deactivated && near(value, 0.0f));

   // A tap sets the value where it lands and starts and ends in its frame.
   t.tap(r.x + 15, r.y + 100);
   CHECK(t.changed && t.activated && t.deactivated && near(value, 50.0f));
   t.frame();
   CHECK(!t.changed && near(value, 50.0f));
}

TEST(a_vertical_slider_fills_from_the_bottom)
{
   float value = 25.0f;
   uint32_t first = 0;
   const MonstroDrawData *data = nullptr;
   Ui ui;
   ui.begin();
   begin_screen(ui);
   monstro_set_cursor_screen_pos(ui.ctx, 100, 100);
   first = ui.vertexCount();
   monstro_slider_float_ex(ui.ctx, "lift##", &value, 0.0f, 100.0f, nullptr, 30.0f, 200.0f, MONSTRO_SLIDER_VERTICAL,
                           nullptr);
   MonstroRect r = monstro_get_item_rect(ui.ctx);
   monstro_end_window(ui.ctx);
   data = ui.end();
   // The filled part is drawn in the slider's fill colour, from the bottom to the cursor's centre at 25.
   uint32_t fill = monstro_color_to_u32(monstro_get_slider_style(ui.ctx)->bar_filled);
   float top = 1e9f, bottom = -1e9f;
   for (uint32_t i = first; i < data->vertex_count; i++)
      if (data->vertices[i].color == fill)
      {
         top = std::fmin(top, data->vertices[i].position[1]);
         bottom = std::fmax(bottom, data->vertices[i].position[1]);
      }
   CHECK(near(top, r.y + 200.0f - (8.0f + 0.25f * 184.0f), 0.5f) && near(bottom, r.y + 200.0f, 0.5f));
}

/* ------------------------------------------------------------------------- */
/* Absolute knobs                                                            */
/* ------------------------------------------------------------------------- */

// The point at angle degrees clockwise from the right (+y down), 40 pixels from the centre of a knob at rect.
static MonstroVec2 on_knob(MonstroRect knob, float degrees)
{
   float a = degrees * 3.14159265f / 180.0f;
   MonstroVec2 c = middle(knob);
   return {c.x + 40.0f * std::cos(a), c.y + 40.0f * std::sin(a)};
}

TEST(an_absolute_knob_points_where_the_pointer_is)
{
   float value = 3.0f;
   auto t = one_control(
       [&](MonstroContext *ctx) {
          return monstro_knob_ex(ctx, "trim##", &value, 0.0f, 12.0f, nullptr, 100.0f, MONSTRO_KNOB_ABSOLUTE, nullptr);
       });
   t.frame();
   // No shown label: the item is the knob alone.
   CHECK(t.rect.x == 100 && t.rect.w == 100 && t.rect.h == 100);
   MonstroVec2 c = middle(t.rect);

   // The sweep runs clockwise from the lower left (135 degrees) over the top to the lower right (405).
   MonstroVec2 top = on_knob(t.rect, 270);
   t.press(top.x, top.y);
   CHECK(t.changed && t.activated && near(value, 6.0f));
   MonstroVec2 right = on_knob(t.rect, 360);
   t.ui.moveMouse(right.x, right.y);
   t.frame();
   CHECK(near(value, 10.0f));
   // Into the gap at the bottom: the nearer end.
   t.ui.moveMouse(c.x + 10, c.y + 40);
   t.frame();
   CHECK(near(value, 12.0f));
   t.ui.moveMouse(c.x - 10, c.y + 40);
   t.frame();
   CHECK(near(value, 0.0f));
   t.release();
   CHECK(t.deactivated);

   // A tap points the knob in its frame.
   MonstroVec2 left = on_knob(t.rect, 180);
   t.tap(left.x, left.y);
   CHECK(t.changed && t.activated && t.deactivated && near(value, 2.0f));
}

TEST(a_relative_knob_does_not_jump_where_it_is_pressed)
{
   float value = 3.0f;
   auto t = one_control([&](MonstroContext *ctx)
                        { return monstro_knob_sized(ctx, "trim##", &value, 0.0f, 12.0f, nullptr, 100.0f, nullptr); });
   t.frame();
   MonstroVec2 top = on_knob(t.rect, 270);
   t.press(top.x, top.y);
   CHECK(t.activated && value == 3.0f);
   // Dragged up by 50 pixels: a quarter of the range per diameter.
   t.ui.moveMouse(top.x, top.y - 50);
   t.frame();
   CHECK(near(value, 6.0f));
   t.release();
}

/* ------------------------------------------------------------------------- */
/* Progress bars                                                             */
/* ------------------------------------------------------------------------- */

// The vertices a progress bar adds to a frame of its own, and their colours.
static uint32_t progress_vertices(Ui &ui, float fraction, const char *overlay, MonstroProgressFlags flags,
                                  float *fill_top = nullptr, float *fill_bottom = nullptr)
{
   ui.begin();
   begin_screen(ui);
   monstro_set_cursor_screen_pos(ui.ctx, 100, 100);
   uint32_t first = ui.vertexCount();
   monstro_progress_bar_ex(ui.ctx, fraction, 20.0f, 100.0f, overlay, flags);
   MonstroRect r = monstro_get_item_rect(ui.ctx);
   monstro_end_window(ui.ctx);
   const MonstroDrawData *data = ui.end();
   const MonstroStyleProgress *ps = monstro_get_progress_style(ui.ctx);
   uint32_t fill = monstro_color_to_u32(ps->cursor_normal.data.color);
   if (fill_top && fill_bottom)
   {
      *fill_top = 1e9f;
      *fill_bottom = -1e9f;
      for (uint32_t i = first; i < data->vertex_count; i++)
         if (data->vertices[i].color == fill)
         {
            *fill_top = std::fmin(*fill_top, data->vertices[i].position[1] - r.y);
            *fill_bottom = std::fmax(*fill_bottom, data->vertices[i].position[1] - r.y);
         }
   }
   return data->vertex_count - first;
}

TEST(a_vertical_progress_bar_fills_from_the_bottom)
{
   Ui ui;
   float top = 0, bottom = 0;
   progress_vertices(ui, 0.25f, "", MONSTRO_PROGRESS_VERTICAL, &top, &bottom);
   // Measured from the top of the bar, 100 high.
   CHECK(near(top, 75.0f, 0.5f) && near(bottom, 100.0f, 0.5f));
}

TEST(a_progress_bar_shows_no_text_when_its_overlay_is_empty)
{
   Ui ui;
   uint32_t with_percentage = progress_vertices(ui, 0.5f, nullptr, MONSTRO_PROGRESS_NONE);
   uint32_t without_text = progress_vertices(ui, 0.5f, "", MONSTRO_PROGRESS_NONE);
   uint32_t with_overlay = progress_vertices(ui, 0.5f, "fuel", MONSTRO_PROGRESS_NONE);
   // "50%" is three glyphs and "fuel" four, four vertices each.
   CHECK(with_percentage == without_text + 12 && with_overlay == without_text + 16);
}

/* ------------------------------------------------------------------------- */
/* Page tabs                                                                 */
/* ------------------------------------------------------------------------- */

struct PageTabs
{
   Ui ui;
   const char *labels[3] = {"flt##FLT", "sys##SYS", "cargo##CARGO"};
   int page = 0;  // the host's page, as the simulation has it
   int shown = 0; // the selection passed in, and what the tabs wrote back
   bool clicked = false;
   MonstroRect strip = {};
   const MonstroDrawData *data = nullptr;
   uint32_t first = 0;

   void frame()
   {
      ui.begin();
      begin_screen(ui);
      monstro_set_cursor_screen_pos(ui.ctx, 0, 448);
      shown = page;
      first = ui.vertexCount();
      clicked = monstro_page_tabs(ui.ctx, "pages", labels, 3, &shown, 512, 64);
      strip = monstro_get_item_rect(ui.ctx);
      monstro_end_window(ui.ctx);
      data = ui.end();
   }

   MonstroRect tab(int i) const
   {
      float spacing = monstro_get_tab_style(ui.ctx)->spacing[0];
      float w = (strip.w - spacing * 2) / 3;
      return {strip.x + (w + spacing) * i, strip.y, w, strip.h};
   }

   // The tab whose background is drawn lit; every lit vertex must lie on it.
   int lit() const
   {
      uint32_t active = monstro_color_to_u32(monstro_get_tab_style(ui.ctx)->tab_maximize_button.active.data.color);
      int found = -1;
      for (uint32_t i = first; i < data->vertex_count; i++)
      {
         if (data->vertices[i].color != active)
            continue;
         float x = data->vertices[i].position[0];
         int at = -1;
         for (int t = 0; t < 3; t++)
            if (x >= tab(t).x - 0.5f && x <= tab(t).x + tab(t).w + 0.5f)
               at = t;
         if (at < 0 || (found >= 0 && found != at))
            return -2;
         found = at;
      }
      return found;
   }

   void tap(int i)
   {
      MonstroVec2 p = middle(tab(i));
      ui.moveMouse(p.x, p.y);
      ui.pressLeft(true);
      ui.pressLeft(false);
      frame();
   }
};

TEST(page_tabs_light_the_tab_the_host_selects)
{
   PageTabs t;
   t.frame();
   CHECK(t.strip.x == 0 && t.strip.y == 448 && t.strip.w == 512 && t.strip.h == 64);
   CHECK(t.lit() == 0 && !t.clicked);
   // The page changes in the simulation; the tabs follow it, with no click.
   t.page = 2;
   t.frame();
   CHECK(t.lit() == 2 && !t.clicked && t.shown == 2);
}

TEST(page_tabs_report_the_tab_clicked_and_keep_the_hosts_selection)
{
   PageTabs t;
   t.page = 1;
   t.frame();
   MonstroVec2 p = middle(t.tab(0));
   t.ui.moveMouse(p.x, p.y);
   t.ui.pressLeft(true);
   t.frame();
   // The click is reported, and the clicked tab lights in its frame.
   CHECK(t.clicked && t.shown == 0 && t.lit() == 0);
   // Until the host changes its page, it passes its own selection, and that is what shows.
   t.frame();
   CHECK(!t.clicked && t.shown == 1 && t.lit() == 1);
   t.ui.pressLeft(false);
   t.frame();
   CHECK(!t.clicked && t.lit() == 1);
   // A click on the tab already selected changes nothing.
   p = middle(t.tab(1));
   t.ui.moveMouse(p.x, p.y);
   t.ui.pressLeft(true);
   t.frame();
   CHECK(!t.clicked && t.shown == 1);
   t.ui.pressLeft(false);
   t.frame();
}

TEST(a_tap_on_a_page_tab_selects_it_in_the_next_frame)
{
   PageTabs t;
   t.frame();
   t.tap(2);
   CHECK(t.clicked && t.shown == 2);
   t.page = t.shown;
   t.frame();
   CHECK(!t.clicked && t.lit() == 2);
}

TEST(page_tab_ids_are_their_labels_ids)
{
   PageTabs t;
   t.frame();
   MonstroVec2 p = middle(t.tab(1));
   t.ui.moveMouse(p.x, p.y);
   t.ui.pressLeft(true);
   t.frame();
   CHECK(t.clicked && t.shown == 1);
   // A live label keeps its tab's ID: the hover stays on the tab as its text changes.
   uint64_t hovered = t.ui.ctx->hovered_id;
   t.labels[1] = "sys##SYS 2";
   t.frame();
   CHECK(t.ui.ctx->hovered_id == hovered && hovered != 0);
   t.ui.pressLeft(false);
   t.frame();
}

FATAL_TEST(page_tabs_with_a_selection_out_of_range_abort, "monstro_page_tabs 'pages': selected is 3; there are 3 tabs")
{
   PageTabs t;
   t.page = 3;
   t.frame();
}

/* ------------------------------------------------------------------------- */
/* Host textures: a camera feed on a screen                                  */
/* ------------------------------------------------------------------------- */

TEST(a_host_texture_is_drawn_with_its_own_id_in_draw_order)
{
   Ui ui(false);
   monstro_env_set_atlas_texture(ui.env, 1);
   const uint64_t camera = 0xCA3E5A;
   ui.begin();
   begin_screen(ui);
   MonstroCanvas *canvas = monstro_get_window_canvas(ui.ctx);
   uint32_t first_command = ui.ctx->canvas->command_count;
   uint32_t first = ui.vertexCount();
   monstro_draw_image(canvas, camera, {16, 16, 320, 240}, {0, 0}, {1, 1}, 0xffffffff);
   monstro_draw_rect(canvas, {16, 16, 320, 240}, 0xff00ff00, 0, MONSTRO_CORNER_NONE, 2); // a frame over the feed
   monstro_set_cursor_screen_pos(ui.ctx, 16, 300);
   monstro_image(ui.ctx, camera, 160, 120); // the same feed as a widget
   monstro_end_window(ui.ctx);
   const MonstroDrawData *data = ui.end();

   // The feed, then the frame in the atlas, then the feed again: three commands, in that order.
   const MonstroDrawCommand *commands = data->commands + first_command;
   CHECK(data->command_count - first_command == 3);
   CHECK(commands[0].texture_id == camera && commands[1].texture_id == 1 && commands[2].texture_id == camera);
   CHECK(commands[0].index_count == 6 && commands[2].index_count == 6);
   const MonstroVertex *feed = data->vertices + first;
   CHECK(feed[0].position[0] == 16 && feed[0].position[1] == 16 && feed[2].position[0] == 336 &&
         feed[2].position[1] == 256);
   CHECK(feed[0].texcoord[0] == 0 && feed[0].texcoord[1] == 0 && feed[2].texcoord[0] == 1 && feed[2].texcoord[1] == 1);
   CHECK(feed[0].color == 0xffffffff);
}

/* ------------------------------------------------------------------------- */
/* Lit buttons: a selectable with its text centred                           */
/* ------------------------------------------------------------------------- */

// Where the first glyph of a selectable's text lands, and the selectable's rect.
static MonstroVec2 selectable_text(Ui &ui, MonstroAlign align, MonstroRect *rect)
{
   ui.begin();
   begin_screen(ui);
   MonstroStyleSelectable style = *monstro_get_selectable_style(ui.ctx);
   style.text_alignment = align;
   style.padding[0] = style.padding[1] = 0.0f;
   monstro_push_selectable_style(ui.ctx, &style);
   monstro_set_cursor_screen_pos(ui.ctx, 100, 100);
   bool on = true;
   uint32_t first = ui.vertexCount();
   monstro_selectable_sized(ui.ctx, "ramp##RAMP", &on, 200, 60, MONSTRO_SELECTABLE_NONE, nullptr);
   *rect = monstro_get_item_rect(ui.ctx);
   monstro_pop_style(ui.ctx, 1);
   monstro_end_window(ui.ctx);
   const MonstroDrawData *data = ui.end();
   // The lit background is first, then the glyphs.
   const MonstroVertex &glyph = data->vertices[first + 4];
   return {glyph.position[0], glyph.position[1]};
}

TEST(a_selectable_places_its_text_as_its_style_aligns_it)
{
   Ui ui;
   CHECK(monstro_get_selectable_style(ui.ctx)->text_alignment == MONSTRO_ALIGN_LEFT);
   MonstroRect rect = {};
   MonstroVec2 left = selectable_text(ui, MONSTRO_ALIGN_LEFT, &rect);
   MonstroVec2 centre = selectable_text(ui, MONSTRO_ALIGN_CENTER, &rect);
   MonstroVec2 size = monstro_font_measure_text(ui.font, "RAMP", 0.0f);
   // By default at the top left, as before; centred, in the middle of the button.
   CHECK(left.x >= rect.x && left.x < rect.x + 4.0f && left.y < rect.y + 16.0f);
   CHECK(near(centre.x - left.x, (rect.w - size.x) * 0.5f) && near(centre.y - left.y, (rect.h - size.y) * 0.5f));
}
