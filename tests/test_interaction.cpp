// Interaction rules: input between frames, one hovered item, clipping, active items, edges, IDs, scoping.
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

struct Edges
{
   bool active = false, activated = false, deactivated = false, after_edit = false, edited = false;

   void read(MonstroContext *ctx)
   {
      active = monstro_is_item_active(ctx);
      activated = monstro_is_item_activated(ctx);
      deactivated = monstro_is_item_deactivated(ctx);
      after_edit = monstro_is_item_deactivated_after_edit(ctx);
      edited = monstro_is_item_edited(ctx);
   }
   bool none() const { return !activated && !deactivated && !after_edit; }
};

// A window with one button; counts its clicks.
struct OneButton
{
   Ui ui;
   MonstroRect rect = {};
   int clicks = 0;
   Edges edges;

   void frame()
   {
      ui.begin();
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 300, 200, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      if (monstro_button(ui.ctx, "Fire", nullptr))
         clicks++;
      rect = monstro_get_item_rect(ui.ctx);
      edges.read(ui.ctx);
      monstro_end_window(ui.ctx);
      ui.end();
   }
};

TEST(a_press_and_release_between_frames_click_in_the_next_frame)
{
   OneButton t;
   t.frame();
   MonstroVec2 p = middle(t.rect);
   t.ui.moveMouse(p.x, p.y);
   t.frame();
   // Both changes happen before the next frame, which sees the button pressed and released.
   t.ui.pressLeft(true);
   t.ui.pressLeft(false);
   t.frame();
   CHECK(t.clicks == 1 && !monstro_is_mouse_down(t.ui.ctx, MONSTRO_MOUSE_BUTTON_LEFT));
   t.frame();
   CHECK(t.clicks == 1);
}

TEST(a_tap_between_frames_fires_a_button_in_the_next_frame)
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
   CHECK(monstro_is_mouse_clicked(t.ui.ctx, MONSTRO_MOUSE_BUTTON_LEFT, false) &&
         monstro_is_mouse_released(t.ui.ctx, MONSTRO_MOUSE_BUTTON_LEFT));
   CHECK(t.clicks == 1 && t.ui.ctx->input_events.empty() && t.ui.ctx->active_id == 0);
   t.frame();
   CHECK(t.clicks == 1);
}

// Everything that acts on a click, in one window; each tap below lands on one of them.
struct EveryKind
{
   Ui ui;
   enum Kind
   {
      CHECKBOX,
      RADIO,
      SELECTABLE,
      TREE_NODE,
      TAB,
      COMBO,
      MENU,
      MENU_ITEM,
      KINDS
   };
   MonstroRect rects[KINDS] = {};
   bool checked = false, selected = false, tree_open = false, second_tab = false, combo_open = false, menu_open = false;
   int radio = 0, quits = 0;

   void frame()
   {
      ui.begin();
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 400, 500, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr,
                           (MonstroWindowFlags)(MONSTRO_WINDOW_DEFAULT | MONSTRO_WINDOW_MENU_BAR));
      if (monstro_begin_menu_bar(ui.ctx))
      {
         menu_open = monstro_begin_menu(ui.ctx, "File", true);
         if (menu_open)
         {
            if (monstro_menu_item(ui.ctx, "Quit", nullptr, true))
               quits++;
            rects[MENU_ITEM] = monstro_get_item_rect(ui.ctx);
            monstro_end_menu(ui.ctx);
         }
         else
         {
            rects[MENU] = monstro_get_item_rect(ui.ctx);
         }
         monstro_end_menu_bar(ui.ctx);
      }
      monstro_checkbox(ui.ctx, "Armed", &checked, nullptr);
      rects[CHECKBOX] = monstro_get_item_rect(ui.ctx);
      monstro_radio_button(ui.ctx, "Port", &radio, 0, nullptr);
      monstro_radio_button(ui.ctx, "Starboard", &radio, 1, nullptr);
      rects[RADIO] = monstro_get_item_rect(ui.ctx);
      monstro_selectable(ui.ctx, "Contact", &selected, MONSTRO_SELECTABLE_NONE, nullptr);
      rects[SELECTABLE] = monstro_get_item_rect(ui.ctx);
      tree_open = monstro_begin_tree_node(ui.ctx, "Cargo", MONSTRO_TREE_NONE);
      rects[TREE_NODE] = monstro_get_item_rect(ui.ctx);
      if (tree_open)
         monstro_end_tree_node(ui.ctx);
      if (monstro_begin_tab_bar(ui.ctx, "tabs", MONSTRO_TAB_BAR_NONE))
      {
         if (monstro_begin_tab_item(ui.ctx, "Helm", nullptr))
            monstro_end_tab_item(ui.ctx);
         second_tab = monstro_begin_tab_item(ui.ctx, "Comms", nullptr);
         rects[TAB] = ui.ctx->drawn_items.top().rect; // tabs do not set the item rect
         if (second_tab)
            monstro_end_tab_item(ui.ctx);
         monstro_end_tab_bar(ui.ctx);
      }
      combo_open = monstro_begin_combo(ui.ctx, "Mode", "Orbit", nullptr);
      rects[COMBO] = monstro_get_item_rect(ui.ctx);
      if (combo_open)
         monstro_end_combo(ui.ctx);
      monstro_end_window(ui.ctx);
      ui.end();
   }

   // A tap at the middle of the item's rect (at the right end of a combo, where its box is), then one frame.
   void tap(Kind kind)
   {
      MonstroRect r = rects[kind];
      float x = kind == COMBO ? r.x + r.w - 10 : r.x + r.w * 0.5f;
      ui.moveMouse(x, r.y + r.h * 0.5f);
      ui.pressLeft(true);
      ui.pressLeft(false);
      frame();
   }
};

TEST(a_tap_fires_everything_that_acts_on_a_click_in_the_next_frame)
{
   EveryKind t;
   t.frame();
   // A menu or combo list opened by a tap shows from the next frame, as when opened by a held press.
   t.tap(EveryKind::MENU);
   CHECK(t.ui.ctx->popup_stack.size() == 1);
   t.frame();
   CHECK(t.menu_open);
   t.frame(); // the menu fits itself to its items
   t.tap(EveryKind::MENU_ITEM);
   CHECK(t.quits == 1);
   t.frame();
   CHECK(!t.menu_open && t.quits == 1);
   t.tap(EveryKind::CHECKBOX);
   CHECK(t.checked);
   t.tap(EveryKind::RADIO);
   CHECK(t.radio == 1);
   t.tap(EveryKind::SELECTABLE);
   CHECK(t.selected);
   t.tap(EveryKind::TREE_NODE);
   CHECK(t.tree_open);
   t.frame(); // the tree node's children move what is below it
   t.tap(EveryKind::TAB);
   CHECK(t.second_tab);
   t.tap(EveryKind::COMBO);
   CHECK(t.ui.ctx->popup_stack.size() == 1);
   t.frame();
   CHECK(t.combo_open);
}

TEST(a_double_tap_between_frames_is_two_taps_in_two_frames)
{
   OneButton t;
   t.frame();
   MonstroVec2 p = middle(t.rect);
   t.ui.moveMouse(p.x, p.y);
   for (int i = 0; i < 2; i++)
   {
      t.ui.pressLeft(true);
      t.ui.pressLeft(false);
   }
   t.frame();
   CHECK(t.clicks == 1 && t.ui.ctx->input_events.size() == 2);
   t.frame();
   CHECK(t.clicks == 2 && t.ui.ctx->input_events.empty());
   CHECK(monstro_is_mouse_double_clicked(t.ui.ctx, MONSTRO_MOUSE_BUTTON_LEFT));
}

TEST(a_release_and_press_between_frames_take_a_frame_each)
{
   OneButton t;
   t.frame();
   MonstroVec2 p = middle(t.rect);
   t.ui.moveMouse(p.x, p.y);
   t.ui.pressLeft(true);
   t.frame();
   CHECK(t.ui.ctx->active_id != 0);
   // Released and pressed again: the release clicks, then the press holds the button again.
   t.ui.pressLeft(false);
   t.ui.pressLeft(true);
   t.frame();
   CHECK(t.clicks == 1 && !monstro_is_mouse_down(t.ui.ctx, MONSTRO_MOUSE_BUTTON_LEFT));
   t.frame();
   CHECK(t.clicks == 1 && t.edges.activated && monstro_is_mouse_down(t.ui.ctx, MONSTRO_MOUSE_BUTTON_LEFT));
}

TEST(a_grip_pressed_where_the_mouse_just_moved_does_not_jump)
{
   Ui ui;
   monstro::Window *win = nullptr;
   auto draw = [&]()
   {
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_FIRST_USE_EVER);
      monstro_set_next_window_size(ui.ctx, 300, 200, MONSTRO_COND_FIRST_USE_EVER);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      win = ui.ctx->current_window;
      monstro_end_window(ui.ctx);
   };
   frame(ui, draw);
   frame(ui, draw);
   // The resize grip is the window's bottom right corner.
   ui.moveMouse(20 + 300 - 7, 20 + 200 - 7);
   ui.pressLeft(true);
   frame(ui, draw);
   CHECK(ui.ctx->active_id != 0 && win->size.x == 300 && win->size.y == 200);
   ui.moveMouse(20 + 300 - 7 + 30, 20 + 200 - 7 + 10);
   frame(ui, draw);
   CHECK(win->size.x == 330 && win->size.y == 210);
   ui.pressLeft(false);
   frame(ui, draw);
   CHECK(ui.ctx->active_id == 0);
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

TEST(a_press_where_the_mouse_just_moved_lands_on_the_button_on_top_there)
{
   OverlappingButtons t;
   t.frame();
   t.ui.moveMouse(t.under_rect.x + 5, t.under_rect.y + 5);
   t.frame();
   CHECK(t.under_hovered);
   // Between two frames, the mouse moves into the overlap and presses: last frame's items say which is on top.
   t.ui.moveMouse(t.over_rect.x + 10, t.over_rect.y + 10);
   t.ui.pressLeft(true);
   t.frame();
   CHECK(t.over_hovered && !t.under_hovered && t.ui.ctx->active_id == t.ui.ctx->hovered_id);
   t.ui.pressLeft(false);
   t.frame();
   CHECK(t.over_clicks == 1 && t.under_clicks == 0);
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
   t.frame();
   CHECK(t.over_clicks == 1 && t.under_clicks == 0);
   for (int i = 0; i < 3; i++)
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

TEST(a_tap_where_an_item_is_clipped_away_does_not_reach_it)
{
   Ui ui;
   MonstroRect rect = {};
   int clicks = 0, behind_clicks = 0;
   auto draw = [&]()
   {
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 300, 200, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      MonstroVec2 at = monstro_get_cursor_pos(ui.ctx);
      // A button behind, then one over it of which only the left half is drawn.
      if (monstro_button_sized(ui.ctx, "Behind", 120, 40, nullptr))
         behind_clicks++;
      monstro_set_cursor_pos(ui.ctx, at.x, at.y);
      MonstroVec2 screen = monstro_get_cursor_screen_pos(ui.ctx);
      MonstroCanvas *canvas = monstro_get_window_canvas(ui.ctx);
      monstro_push_clip_rect(canvas, {screen.x, screen.y, 60, 40});
      if (monstro_button_sized(ui.ctx, "Half", 120, 40, nullptr))
         clicks++;
      rect = monstro_get_item_rect(ui.ctx);
      monstro_pop_clip_rect(canvas);
      monstro_end_window(ui.ctx);
   };
   frame(ui, draw);
   // A tap where the half button is clipped away reaches the button behind it.
   ui.moveMouse(rect.x + 90, rect.y + 20);
   ui.pressLeft(true);
   ui.pressLeft(false);
   frame(ui, draw);
   CHECK(clicks == 0 && behind_clicks == 1);
   // A tap where it shows reaches it, and not the button behind.
   ui.moveMouse(rect.x + 30, rect.y + 20);
   ui.pressLeft(true);
   ui.pressLeft(false);
   frame(ui, draw);
   CHECK(clicks == 1 && behind_clicks == 1);
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

/* ------------------------------------------------------------------------- */
/* Activated and deactivated edges                                           */
/* ------------------------------------------------------------------------- */

TEST(a_slider_drag_reports_one_activation_and_one_deactivation_after_edit)
{
   Ui ui;
   float value = 0.5f;
   MonstroRect slider = {};
   Edges e;
   int activations = 0, deactivations = 0, commits = 0, edits = 0;
   auto draw = [&]()
   {
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 300, 200, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      monstro_slider_float(ui.ctx, "Throttle", &value, 0.0f, 1.0f, "%.2f", nullptr);
      slider = monstro_get_item_rect(ui.ctx);
      e.read(ui.ctx);
      activations += e.activated;
      deactivations += e.deactivated;
      commits += e.after_edit;
      edits += e.edited;
      monstro_end_window(ui.ctx);
   };
   frame(ui, draw);
   ui.moveMouse(slider.x + slider.w * 0.5f, slider.y + slider.h * 0.5f);
   frame(ui, draw);
   CHECK(e.none() && !e.active);
   ui.pressLeft(true);
   frame(ui, draw);
   CHECK(e.activated && e.active && !e.deactivated);
   for (int i = 1; i <= 5; i++)
   {
      ui.moveMouse(slider.x + slider.w * 0.5f + (float)i * 10.0f, slider.y + slider.h * 0.5f);
      frame(ui, draw);
      CHECK(e.active && e.none() && e.edited);
   }
   ui.pressLeft(false);
   frame(ui, draw);
   CHECK(e.deactivated && e.after_edit && !e.active);
   frame(ui, draw);
   CHECK(e.none());
   CHECK(activations == 1 && deactivations == 1 && commits == 1 && edits >= 5);
}

TEST(a_button_press_activates_and_its_release_deactivates_without_an_edit)
{
   OneButton t;
   t.frame();
   t.ui.moveMouse(t.rect.x + 5, t.rect.y + 5);
   t.frame();
   t.ui.pressLeft(true);
   t.frame();
   CHECK(t.edges.activated && !t.edges.deactivated);
   t.ui.pressLeft(false);
   t.frame();
   CHECK(t.edges.deactivated && !t.edges.after_edit && !t.edges.activated && t.clicks == 1);
   // A tap reports both in its frame.
   t.ui.pressLeft(true);
   t.ui.pressLeft(false);
   t.frame();
   CHECK(t.edges.activated && t.edges.deactivated && !t.edges.after_edit && t.clicks == 2);
   t.frame();
   CHECK(t.edges.none());
}

TEST(a_tap_on_a_slider_sets_it_and_reports_both_edges_in_its_frame)
{
   Ui ui;
   float value = 0.5f;
   MonstroRect slider = {};
   Edges e;
   auto draw = [&]()
   {
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 300, 200, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      monstro_slider_float_ex(ui.ctx, "Throttle", &value, 0.0f, 1.0f, "%.2f", 120, 0, MONSTRO_SLIDER_NONE, nullptr);
      slider = monstro_get_item_rect(ui.ctx);
      e.read(ui.ctx);
      monstro_end_window(ui.ctx);
   };
   frame(ui, draw);
   // The bar is the slider's right 120 pixels; a quarter of the way along it.
   MonstroRect bar = slider;
   bar.x = slider.x + slider.w - 120.0f;
   bar.w = 120.0f;
   CHECK(bar.x > slider.x);
   ui.moveMouse(bar.x + bar.w * 0.25f, bar.y + bar.h * 0.5f);
   ui.pressLeft(true);
   ui.pressLeft(false);
   frame(ui, draw);
   CHECK(value > 0.2f && value < 0.3f);
   CHECK(e.activated && e.deactivated && e.after_edit && e.edited);
   CHECK(ui.ctx->active_id == 0);
   frame(ui, draw);
   CHECK(e.none() && !e.active && value > 0.2f && value < 0.3f);
}

TEST(a_text_field_reports_its_edit_when_editing_ends)
{
   Ui ui;
   char name[32] = "Kestrel";
   MonstroRect field = {};
   Edges e;
   auto draw = [&]()
   {
      monstro_input_text(ui.ctx, "Name", name, sizeof(name), MONSTRO_INPUT_TEXT_NONE, nullptr);
      field = monstro_get_item_rect(ui.ctx);
      e.read(ui.ctx);
   };
   frame(ui, draw);
   ui.moveMouse(field.x + field.w - 6, field.y + field.h * 0.5f);
   frame(ui, draw);
   ui.pressLeft(true);
   ui.pressLeft(false);
   frame(ui, draw);
   CHECK(e.activated);
   ui.type("!");
   frame(ui, draw);
   CHECK(e.edited && e.none());
   ui.pressKey(MONSTRO_KEY_ENTER, true);
   frame(ui, draw);
   CHECK(e.deactivated && e.after_edit);
   CHECK(std::strcmp(name, "Kestrel!") == 0);
}

/* ------------------------------------------------------------------------- */
/* Label IDs                                                                 */
/* ------------------------------------------------------------------------- */

TEST(a_slider_whose_label_shows_its_value_keeps_its_drag)
{
   Ui ui;
   float speed = 10.0f;
   MonstroRect slider = {};
   Edges e;
   int activations = 0;
   auto draw = [&]()
   {
      monstro_set_next_window_pos(ui.ctx, 20, 20, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ui.ctx, 400, 200, MONSTRO_COND_ALWAYS);
      monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
      char label[64];
      std::snprintf(label, sizeof(label), "speed##Speed: %.1f m/s", speed);
      monstro_slider_float(ui.ctx, label, &speed, 0.0f, 100.0f, "%.1f", nullptr);
      slider = monstro_get_item_rect(ui.ctx);
      e.read(ui.ctx);
      activations += e.activated;
      monstro_end_window(ui.ctx);
   };
   frame(ui, draw);
   float x = slider.x + slider.w - 20, y = slider.y + slider.h * 0.5f;
   ui.moveMouse(x, y);
   frame(ui, draw);
   ui.pressLeft(true);
   frame(ui, draw);
   for (int i = 1; i <= 6; i++)
   {
      float before = speed;
      ui.moveMouse(x - (float)i * 15.0f, y);
      frame(ui, draw);
      // The label changed with the value, and the drag goes on.
      CHECK(e.active && speed != before);
   }
   ui.pressLeft(false);
   frame(ui, draw);
   CHECK(activations == 1 && e.deactivated && e.after_edit);
}

TEST(a_tree_node_whose_label_shows_a_count_stays_open)
{
   Ui ui;
   int contacts = 3;
   MonstroRect row = {};
   bool open = false;
   auto draw = [&]()
   {
      char label[64];
      std::snprintf(label, sizeof(label), "contacts##Contacts (%d)", contacts);
      open = monstro_begin_tree_node(ui.ctx, label, MONSTRO_TREE_NONE);
      row = monstro_get_item_rect(ui.ctx);
      if (open)
         monstro_end_tree_node(ui.ctx);
   };
   frame(ui, draw);
   CHECK(!open);
   ui.moveMouse(row.x + 10, row.y + row.h * 0.5f);
   frame(ui, draw);
   ui.pressLeft(true);
   frame(ui, draw);
   ui.pressLeft(false);
   frame(ui, draw);
   CHECK(open);
   for (int i = 0; i < 5; i++)
   {
      contacts++;
      frame(ui, draw);
      CHECK(open);
   }
}

TEST(the_text_before_hashes_is_the_id_and_the_text_after_is_shown)
{
   Ui ui;
   uint32_t plain = 0, hidden_id = 0;
   uint64_t first = 0, second = 0;
   frame(ui,
         [&]()
         {
            uint32_t before = ui.vertexCount();
            monstro_button(ui.ctx, "Reset", nullptr);
            plain = ui.vertexCount() - before;
            before = ui.vertexCount();
            monstro_button(ui.ctx, "position##Reset", nullptr);
            hidden_id = ui.vertexCount() - before;
            first = ui.ctx->last_item_id;
            monstro_button(ui.ctx, "rotation##Reset", nullptr);
            second = ui.ctx->last_item_id;
         });
   // The same text is drawn, under two IDs; the ID is the text before the ##.
   CHECK(plain == hidden_id && plain > 0);
   CHECK(first != second);
   frame(ui,
         [&]()
         {
            monstro_button(ui.ctx, "position##Something else", nullptr);
            CHECK(ui.ctx->last_item_id == first);
         });
}

FATAL_TEST(a_label_with_nothing_before_its_hashes_aborts, "label '##cmd' has no ID",
           "write 'id##cmd', or 'cmd##' to show nothing")
{
   Ui ui;
   char text[16] = "";
   ui.begin();
   monstro_input_text(ui.ctx, "##cmd", text, sizeof(text), MONSTRO_INPUT_TEXT_NONE, nullptr);
}

FATAL_TEST(a_window_with_nothing_before_its_hashes_aborts, "window '##Toast' has no name", "write 'name##Toast'")
{
   Ui ui;
   ui.begin();
   monstro_begin_window(ui.ctx, "##Toast", nullptr, MONSTRO_WINDOW_DEFAULT);
}

/* ------------------------------------------------------------------------- */
/* Popups, child panels and windows scoped by ID                             */
/* ------------------------------------------------------------------------- */

TEST(popups_named_alike_in_two_windows_are_two_popups)
{
   Ui ui;
   monstro::Window *popups[2] = {};
   MonstroRect items[2] = {};
   int picks[2] = {};
   auto draw = [&](bool open)
   {
      const char *names[2] = {"Left", "Right"};
      for (int i = 0; i < 2; i++)
      {
         monstro_set_next_window_pos(ui.ctx, 20.0f + 300.0f * (float)i, 20, MONSTRO_COND_ALWAYS);
         monstro_set_next_window_size(ui.ctx, 250, 200, MONSTRO_COND_ALWAYS);
         monstro_begin_window(ui.ctx, names[i], nullptr, MONSTRO_WINDOW_DEFAULT);
         if (open)
            monstro_open_popup(ui.ctx, "menu");
         popups[i] = nullptr;
         if (monstro_is_popup_open(ui.ctx, "menu"))
            monstro_set_next_window_pos(ui.ctx, 40.0f + 300.0f * (float)i, 200, MONSTRO_COND_ALWAYS);
         if (monstro_begin_popup(ui.ctx, "menu", MONSTRO_WINDOW_NONE))
         {
            popups[i] = ui.ctx->current_window;
            if (monstro_button(ui.ctx, names[i], nullptr))
               picks[i]++;
            items[i] = monstro_get_item_rect(ui.ctx);
            monstro_end_popup(ui.ctx);
         }
         monstro_end_window(ui.ctx);
      }
   };
   frame(ui, [&]() { draw(true); });
   frame(ui, [&]() { draw(false); });
   CHECK(ui.ctx->popup_stack.size() == 2);
   CHECK(popups[0] && popups[1] && popups[0] != popups[1]);
   CHECK(popups[0]->pos.x == 40.0f && popups[1]->pos.x == 340.0f);
   CHECK(std::strcmp(popups[0]->title, "menu") == 0 && std::strcmp(popups[1]->title, "menu") == 0);

   // Picking from the right one closes it; the left one stays open.
   MonstroVec2 p = middle(items[1]);
   ui.moveMouse(p.x, p.y);
   frame(ui, [&]() { draw(false); });
   ui.pressLeft(true);
   frame(ui, [&]() { draw(false); });
   ui.pressLeft(false);
   frame(ui, [&]() { draw(false); });
   CHECK(picks[1] == 1 && picks[0] == 0);
   CHECK(ui.ctx->popup_stack.size() == 1 && ui.ctx->popup_stack[0].parent_window == ui.ctx->findWindow("Left"));
}

TEST(child_panels_named_alike_in_two_id_scopes_are_two_panels)
{
   Ui ui;
   monstro::Window *panels[2] = {};
   frame(ui,
         [&]()
         {
            monstro_begin_window(ui.ctx, "Panel", nullptr, MONSTRO_WINDOW_DEFAULT);
            for (int i = 0; i < 2; i++)
            {
               monstro_push_id(ui.ctx, i == 0 ? "port" : "starboard");
               monstro_begin_child(ui.ctx, "engine", 100, 40, MONSTRO_WINDOW_BORDERS);
               panels[i] = ui.ctx->current_window;
               monstro_end_child(ui.ctx);
               monstro_pop_id(ui.ctx);
            }
            monstro_end_window(ui.ctx);
         });
   CHECK(panels[0] && panels[1] && panels[0] != panels[1]);
}

TEST(windows_named_alike_in_two_contexts_keep_their_own_state)
{
   MonstroEnv *env = monstro_init(nullptr);
   load_test_font(env);
   {
      Ui a(env), b(env);
      for (Ui *ui : {&a, &b})
      {
         ui->begin();
         float x = ui == &a ? 10.0f : 400.0f;
         monstro_set_next_window_pos(ui->ctx, x, 30, MONSTRO_COND_FIRST_USE_EVER);
         monstro_set_next_window_size(ui->ctx, 200, 100, MONSTRO_COND_FIRST_USE_EVER);
         monstro_begin_window(ui->ctx, "Status", nullptr, MONSTRO_WINDOW_DEFAULT);
         monstro_end_window(ui->ctx);
         ui->end();
      }
      monstro::Window *wa = a.ctx->findWindow("Status");
      monstro::Window *wb = b.ctx->findWindow("Status");
      CHECK(wa && wb && wa != wb);
      CHECK(wa->pos.x == 10.0f && wb->pos.x == 400.0f);
   }
   monstro_shutdown(env);
}
