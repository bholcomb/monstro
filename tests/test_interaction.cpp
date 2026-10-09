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
