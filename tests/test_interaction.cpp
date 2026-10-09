// Interaction rules: input between frames, one hovered item, clipping, active items, edges, IDs, scoping, eviction.
#include "context.h"
#include "harness.h"
#include <cstdio>
#include <cstring>

template <typename Draw> static void frame(Ui &ui, Draw draw)
{
   ui.begin();
   draw();
   ui.end();
}

static MonstroVec2 middle(MonstroRect r) { return {r.x + r.w * 0.5f, r.y + r.h * 0.5f}; }

/* ------------------------------------------------------------------------- */
/* Input events                                                              */
/* ------------------------------------------------------------------------- */

// A window with one button; counts its clicks.
struct OneButton
{
   Ui ui;
   MonstroRect rect = {};
   int clicks = 0;

   void frame()
   {
      ui.begin();
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 300, 200, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      if (monstro_button(ui.ctx, "Fire", nullptr))
         clicks++;
      rect = monstro_get_item_rect(ui.ctx);
      monstro_end_window(ui.ctx);
      ui.end();
   }
};

TEST(a_press_and_release_between_frames_still_clicks)
{
   OneButton t;
   t.frame();
   MonstroVec2 p = middle(t.rect);
   t.ui.moveMouse(p.x, p.y);
   t.frame();
   // Both changes happen before the next frame: the press is taken first, the release a frame later.
   t.ui.pressLeft(true);
   t.ui.pressLeft(false);
   t.frame();
   CHECK(t.clicks == 0 && monstro_is_mouse_down(t.ui.ctx, MONSTRO_MOUSE_BUTTON_LEFT));
   t.frame();
   CHECK(t.clicks == 1 && !monstro_is_mouse_down(t.ui.ctx, MONSTRO_MOUSE_BUTTON_LEFT));
   t.frame();
   CHECK(t.clicks == 1);
}

TEST(a_tap_between_frames_moves_presses_and_releases_over_three_frames)
{
   OneButton t;
   t.frame();
   MonstroVec2 p = middle(t.rect);
   t.ui.moveMouse(p.x, p.y);
   t.ui.pressLeft(true);
   t.ui.pressLeft(false);
   CHECK(t.ui.ctx->input_events.size() == 3);
   t.frame();
   MonstroVec2 mouse = monstro_get_mouse_pos(t.ui.ctx);
   CHECK(mouse.x == p.x && mouse.y == p.y && !monstro_is_mouse_down(t.ui.ctx, MONSTRO_MOUSE_BUTTON_LEFT));
   t.frame();
   CHECK(t.clicks == 0);
   t.frame();
   CHECK(t.clicks == 1 && t.ui.ctx->input_events.empty());
}

TEST(a_key_tapped_between_frames_is_pressed_then_released)
{
   Ui ui;
   ui.pressKey(MONSTRO_KEY_SPACE, true);
   ui.pressKey(MONSTRO_KEY_SPACE, false);
   bool pressed = false, released = false;
   frame(ui, [&]() { pressed = monstro_is_key_pressed(ui.ctx, MONSTRO_KEY_SPACE, false); });
   CHECK(pressed);
   frame(ui, [&]() { released = monstro_is_key_released(ui.ctx, MONSTRO_KEY_SPACE); });
   CHECK(released);
}

TEST(polled_input_that_does_not_change_queues_nothing)
{
   Ui ui;
   for (int i = 0; i < 100; i++)
   {
      ui.moveMouse(10, 10);
      ui.moveMouse(11, 12); // moves in a row merge
      ui.pressLeft(false);
      ui.pressKey(MONSTRO_KEY_A, false);
      monstro_set_mouse_wheel(ui.ctx, 0, 0);
      frame(ui, []() {});
   }
   CHECK(ui.ctx->input_events.empty());
   MonstroVec2 mouse = monstro_get_mouse_pos(ui.ctx);
   CHECK(mouse.x == 11 && mouse.y == 12);
}

TEST(wheel_movements_between_frames_add_up)
{
   Ui ui;
   monstro_set_mouse_wheel(ui.ctx, 0, 1);
   monstro_set_mouse_wheel(ui.ctx, 0, 2);
   float wheel = 0.0f;
   frame(ui, [&]() { wheel = monstro_get_mouse_wheel(ui.ctx); });
   CHECK(wheel == 3.0f);
   frame(ui, [&]() { wheel = monstro_get_mouse_wheel(ui.ctx); });
   CHECK(wheel == 0.0f);
}

FATAL_TEST(overfilling_the_input_queue_aborts, "input event queue full: capacity 4, needed 5",
           "raise MonstroContextConfig.max_input_events")
{
   MonstroContextConfig config = monstro_context_config_default();
   config.max_input_events = 4;
   Ui ui(false, &config);
   for (int i = 0; i < 5; i++)
      ui.pressKey(MONSTRO_KEY_A, i % 2 == 0);
}

/* ------------------------------------------------------------------------- */
/* One hovered item, the one on top                                          */
/* ------------------------------------------------------------------------- */

// Two buttons drawn at the same place; the second is drawn over the first.
struct OverlappingButtons
{
   Ui ui;
   MonstroRect under_rect = {}, over_rect = {};
   bool under_hovered = false, over_hovered = false;
   int under_clicks = 0, over_clicks = 0;

   void frame()
   {
      ui.begin();
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 300, 200, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      MonstroVec2 at = monstro_get_cursor_pos(ui.ctx);
      if (monstro_button_sized(ui.ctx, "Under", 120, 40, nullptr))
         under_clicks++;
      under_rect = monstro_get_item_rect(ui.ctx);
      under_hovered = monstro_is_item_hovered(ui.ctx);
      monstro_set_cursor_pos(ui.ctx, at.x + 40, at.y + 10);
      if (monstro_button_sized(ui.ctx, "Over", 120, 40, nullptr))
         over_clicks++;
      over_rect = monstro_get_item_rect(ui.ctx);
      over_hovered = monstro_is_item_hovered(ui.ctx);
      monstro_end_window(ui.ctx);
      ui.end();
   }
};

TEST(of_overlapping_buttons_only_the_one_on_top_hovers_and_fires)
{
   OverlappingButtons t;
   t.frame();
   // A point inside both.
   float x = t.over_rect.x + 10, y = t.over_rect.y + 10;
   CHECK(monstro_rect_contains(t.under_rect, x, y) && monstro_rect_contains(t.over_rect, x, y));
   t.ui.moveMouse(x, y);
   for (int i = 0; i < 3; i++)
   {
      t.frame();
      CHECK(!(t.under_hovered && t.over_hovered));
   }
   CHECK(t.over_hovered && !t.under_hovered);

   t.ui.pressLeft(true);
   t.frame();
   t.ui.pressLeft(false);
   t.frame();
   CHECK(t.over_clicks == 1 && t.under_clicks == 0);

   // Where only the lower button is, it hovers and fires.
   t.ui.moveMouse(t.under_rect.x + 5, t.under_rect.y + 5);
   t.frame();
   CHECK(t.under_hovered && !t.over_hovered);
   t.ui.pressLeft(true);
   t.frame();
   t.ui.pressLeft(false);
   t.frame();
   CHECK(t.over_clicks == 1 && t.under_clicks == 1);
}

TEST(a_tap_on_overlapping_buttons_fires_only_the_one_on_top)
{
   OverlappingButtons t;
   t.frame();
   t.ui.moveMouse(t.under_rect.x + 5, t.under_rect.y + 5);
   t.frame();
   // Between two frames, the mouse moves into the overlap, presses and releases.
   t.ui.moveMouse(t.over_rect.x + 10, t.over_rect.y + 10);
   t.ui.pressLeft(true);
   t.ui.pressLeft(false);
   for (int i = 0; i < 4; i++)
      t.frame();
   CHECK(t.over_clicks == 1 && t.under_clicks == 0);
}

/* ------------------------------------------------------------------------- */
/* Clipping                                                                  */
/* ------------------------------------------------------------------------- */

TEST(an_item_is_not_hovered_where_it_is_clipped_away)
{
   Ui ui;
   MonstroRect rect = {};
   bool hovered = false;
   int clicks = 0;
   auto draw = [&]()
   {
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 300, 200, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      MonstroVec2 at = monstro_get_cursor_screen_pos(ui.ctx);
      // Only the left half of the button is drawn.
      MonstroCanvas *canvas = monstro_get_window_canvas(ui.ctx);
      monstro_push_clip_rect(canvas, {at.x, at.y, 60, 40});
      if (monstro_button_sized(ui.ctx, "Half", 120, 40, nullptr))
         clicks++;
      rect = monstro_get_item_rect(ui.ctx);
      hovered = monstro_is_item_hovered(ui.ctx);
      monstro_pop_clip_rect(canvas);
      monstro_end_window(ui.ctx);
   };
   frame(ui, draw);
   ui.moveMouse(rect.x + 90, rect.y + 20);
   frame(ui, draw);
   CHECK(!hovered);
   ui.pressLeft(true);
   frame(ui, draw);
   ui.pressLeft(false);
   frame(ui, draw);
   CHECK(clicks == 0);
   ui.moveMouse(rect.x + 30, rect.y + 20);
   frame(ui, draw);
   CHECK(hovered);
}

// A child panel scrolled so that its first button is half above its top.
struct ScrolledPanel
{
   Ui ui;
   MonstroRect first = {}, second = {}, panel = {};
   bool first_hovered = false, second_hovered = false;
   int first_clicks = 0;

   void frame()
   {
      ui.begin();
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 300, 300, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      monstro_label(ui.ctx, "Above the list");
      monstro_begin_child(ui.ctx, "list", 200, 100, MONSTRO_WINDOW_BORDERS);
      monstro_set_scroll_y(ui.ctx, 20);
      panel = ui.ctx->current_window->hit_rect;
      for (int i = 0; i < 10; i++)
      {
         char label[32];
         std::snprintf(label, sizeof(label), "Item %d", i);
         bool clicked = monstro_button_sized(ui.ctx, label, 150, 30, nullptr);
         if (i == 0)
         {
            first_clicks += clicked;
            first = monstro_get_item_rect(ui.ctx);
            first_hovered = monstro_is_item_hovered(ui.ctx);
         }
         if (i == 1)
         {
            second = monstro_get_item_rect(ui.ctx);
            second_hovered = monstro_is_item_hovered(ui.ctx);
         }
      }
      monstro_end_child(ui.ctx);
      monstro_end_window(ui.ctx);
      ui.end();
   }
};

TEST(an_item_scrolled_out_of_a_child_panel_is_not_hovered)
{
   ScrolledPanel t;
   t.frame();
   t.frame();
   // Just above the panel, where its first button is scrolled out of view.
   float x = t.first.x + 10, y = t.panel.y - 2;
   CHECK(monstro_rect_contains(t.first, x, y));
   t.ui.moveMouse(x, y);
   t.frame();
   CHECK(t.ui.ctx->hovered_window == t.ui.ctx->findWindow("Panel"));
   CHECK(!t.first_hovered);
   t.ui.pressLeft(true);
   t.frame();
   t.ui.pressLeft(false);
   t.frame();
   CHECK(t.first_clicks == 0);

   // The part of it still in view hovers.
   t.ui.moveMouse(x, t.panel.y + 2);
   t.frame();
   CHECK(t.first_hovered);
   t.ui.moveMouse(t.second.x + 10, t.second.y + 10);
   t.frame();
   CHECK(t.second_hovered && !t.first_hovered);
}

TEST(a_child_panel_scrolled_under_its_windows_title_bar_does_not_take_the_mouse_there)
{
   Ui ui;
   MonstroRect first = {}, title = {};
   bool first_hovered = false;
   auto draw = [&]()
   {
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 300, 200, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Outer", nullptr, MONSTRO_WINDOW_DEFAULT);
      title = ui.ctx->current_window->getTitleBarRect();
      monstro_set_scroll_y(ui.ctx, 20);
      monstro_begin_child(ui.ctx, "list", 200, 150, MONSTRO_WINDOW_BORDERS);
      monstro_button_sized(ui.ctx, "First", 150, 30, nullptr);
      first = monstro_get_item_rect(ui.ctx);
      first_hovered = monstro_is_item_hovered(ui.ctx);
      monstro_end_child(ui.ctx);
      for (int i = 0; i < 10; i++)
         monstro_label(ui.ctx, "filler");
      monstro_end_window(ui.ctx);
   };
   frame(ui, draw);
   frame(ui, draw);
   monstro::Window *outer = ui.ctx->findWindow("Outer");
   // The child panel and its first button reach up under the title bar.
   float x = first.x + 10, y = title.y + title.h - 2;
   CHECK(monstro_rect_contains(first, x, y));
   ui.moveMouse(x, y);
   frame(ui, draw);
   frame(ui, draw);
   CHECK(ui.ctx->hovered_window == outer && !first_hovered);
}

/* ------------------------------------------------------------------------- */
/* Active items                                                              */
/* ------------------------------------------------------------------------- */

TEST(while_a_slider_is_dragged_nothing_else_hovers_or_fires)
{
   Ui ui;
   float value = 0.5f;
   MonstroRect slider = {}, button = {};
   bool button_hovered = false;
   int clicks = 0;
   auto draw = [&]()
   {
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 300, 200, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      monstro_slider_float(ui.ctx, "Throttle", &value, 0.0f, 1.0f, "%.2f", nullptr);
      slider = monstro_get_item_rect(ui.ctx);
      if (monstro_button(ui.ctx, "Eject", nullptr))
         clicks++;
      button = monstro_get_item_rect(ui.ctx);
      button_hovered = monstro_is_item_hovered(ui.ctx);
      monstro_end_window(ui.ctx);
   };
   frame(ui, draw);
   ui.moveMouse(slider.x + slider.w * 0.5f, slider.y + slider.h * 0.5f);
   frame(ui, draw);
   ui.pressLeft(true);
   frame(ui, draw);
   // Drag down onto the button and release there.
   ui.moveMouse(button.x + button.w * 0.5f, button.y + button.h * 0.5f);
   frame(ui, draw);
   CHECK(!button_hovered && ui.ctx->hovered_id == 0);
   frame(ui, draw);
   CHECK(!button_hovered);
   ui.pressLeft(false);
   frame(ui, draw);
   CHECK(clicks == 0 && ui.ctx->active_id == 0);
   // Once released, the button under the mouse hovers again.
   frame(ui, draw);
   CHECK(button_hovered);
}

TEST(a_button_held_when_the_mouse_leaves_its_window_lets_go_on_release)
{
   OneButton t;
   t.frame();
   MonstroVec2 p = middle(t.rect);
   t.ui.moveMouse(p.x, p.y);
   t.frame();
   t.ui.pressLeft(true);
   t.frame();
   CHECK(t.ui.ctx->active_id != 0);
   t.ui.moveMouse(700, 500);
   t.frame();
   t.ui.pressLeft(false);
   t.frame();
   CHECK(t.ui.ctx->active_id == 0 && t.clicks == 0);
}
