/**
 * Interaction rules, to click through: one item hovered at a time (the one on
 * top), no hover where an item is clipped away, nothing else hovered while an
 * item is active, activated and deactivated edges, IDs that stay put while a
 * label shows a live value, popups named alike in two windows, and clicks
 * that land between frames.
 */
#include "../common/example_app.h"
#include <cstdio>

/* -- an event log for the edges ------------------------------------------- */

static const int LOG_LINES = 10;

struct EventLog
{
   char lines[LOG_LINES][96];
   int count;
};

static EventLog g_log = {};
static int g_frame = 0;

static void log_event(const char *what, const char *item)
{
   char *line = g_log.lines[g_log.count % LOG_LINES];
   snprintf(line, sizeof(g_log.lines[0]), "frame %d: %s %s", g_frame, item, what);
   g_log.count++;
}

// Logs the edges of the item just built.
static void log_edges(MonstroContext *ctx, const char *item)
{
   if (monstro_is_item_activated(ctx))
      log_event("activated", item);
   if (monstro_is_item_deactivated_after_edit(ctx))
      log_event("deactivated after an edit", item);
   else if (monstro_is_item_deactivated(ctx))
      log_event("deactivated", item);
}

/* -- the page ------------------------------------------------------------- */

static int g_under_fired = 0;
static int g_over_fired = 0;
static int g_taps = 0;
static int g_list_fired[12] = {};
static int g_outside_fired = 0;
static float g_throttle = 0.5f;
static float g_trim = 0.0f;
static char g_callsign[32] = "Kestrel";
static int g_contacts = 3;
static float g_contact_timer = 0.0f;

static void overlap_window(MonstroContext *ctx)
{
   monstro_set_next_window_pos(ctx, 20, 20, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 380, 300, MONSTRO_COND_FIRST_USE_EVER);
   if (!monstro_begin_window(ctx, "overlap##One item at a time", nullptr, MONSTRO_WINDOW_DEFAULT))
      return;

   monstro_label(ctx, "Only the button on top lights up and fires.");
   MonstroVec2 at = monstro_get_cursor_pos(ctx);
   if (monstro_button_sized(ctx, "Under", 160, 50, nullptr))
      g_under_fired++;
   monstro_set_cursor_pos(ctx, at.x + 80, at.y + 25);
   if (monstro_button_sized(ctx, "Over", 160, 50, nullptr))
      g_over_fired++;

   char text[64];
   snprintf(text, sizeof(text), "Under fired %d, Over fired %d", g_under_fired, g_over_fired);
   monstro_label(ctx, text);

   monstro_separator(ctx);
   monstro_label(ctx, "Click fast: every click counts, in the next frame.");
   char tap_label[64];
   snprintf(tap_label, sizeof(tap_label), "tap##Tapped %d times", g_taps);
   if (monstro_button_sized(ctx, tap_label, 200, 40, nullptr))
      g_taps++;
   log_edges(ctx, "tap");
   monstro_end_window(ctx);
}

static void clipping_window(MonstroContext *ctx)
{
   monstro_set_next_window_pos(ctx, 420, 20, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 300, 360, MONSTRO_COND_FIRST_USE_EVER);
   if (!monstro_begin_window(ctx, "clipping##Clipped items", nullptr, MONSTRO_WINDOW_DEFAULT))
      return;

   monstro_label(ctx, "Buttons take the mouse where they show.");
   if (monstro_button_sized(ctx, "above##Above the list", 200, 26, nullptr))
      g_outside_fired++;
   if (monstro_begin_child(ctx, "list##", 220, 150, MONSTRO_WINDOW_BORDERS))
   {
      // Start scrolled a little, so buttons are cut off at both edges.
      if (g_frame == 1)
         monstro_set_scroll_y(ctx, 20);
      for (int i = 0; i < 12; i++)
      {
         char label[48];
         snprintf(label, sizeof(label), "item%d##Item %d (fired %d)", i, i, g_list_fired[i]);
         if (monstro_button_sized(ctx, label, 180, 26, nullptr))
            g_list_fired[i]++;
      }
      monstro_end_child(ctx);
   }
   if (monstro_button_sized(ctx, "below##Below the list", 200, 26, nullptr))
      g_outside_fired++;
   char text[64];
   snprintf(text, sizeof(text), "Above and below fired %d", g_outside_fired);
   monstro_label(ctx, text);
   monstro_end_window(ctx);
}

static void edges_window(MonstroContext *ctx, float dt)
{
   monstro_set_next_window_pos(ctx, 20, 340, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 380, 440, MONSTRO_COND_FIRST_USE_EVER);
   if (!monstro_begin_window(ctx, "edges##Edges and live labels", nullptr, MONSTRO_WINDOW_DEFAULT))
      return;

   monstro_label(ctx, "Drag across other buttons: they stay dark.");
   monstro_label(ctx, "The labels show live values.");
   char label[64];
   snprintf(label, sizeof(label), "throttle##Throttle %.0f%%", g_throttle * 100.0f);
   monstro_slider_float(ctx, label, &g_throttle, 0.0f, 1.0f, "%.2f", nullptr);
   log_edges(ctx, "throttle");
   snprintf(label, sizeof(label), "trim##Trim %+.1f deg", g_trim);
   monstro_knob(ctx, label, &g_trim, -10.0f, 10.0f, "%.1f", nullptr);
   log_edges(ctx, "trim");
   monstro_input_text(ctx, "callsign##Callsign", g_callsign, sizeof(g_callsign), MONSTRO_INPUT_TEXT_NONE, nullptr);
   log_edges(ctx, "callsign");

   // A count that changes on its own; the node stays open as its label changes.
   g_contact_timer += dt;
   if (g_contact_timer > 1.0f)
   {
      g_contact_timer = 0.0f;
      g_contacts = g_contacts % 9 + 1;
   }
   snprintf(label, sizeof(label), "contacts##Contacts (%d)", g_contacts);
   if (monstro_begin_tree_node(ctx, label, MONSTRO_TREE_NONE))
   {
      for (int i = 0; i < g_contacts; i++)
      {
         char contact[32];
         snprintf(contact, sizeof(contact), "Contact %d", i + 1);
         monstro_label(ctx, contact);
      }
      monstro_end_tree_node(ctx);
   }

   monstro_separator(ctx);
   int first = g_log.count > LOG_LINES ? g_log.count - LOG_LINES : 0;
   for (int i = g_log.count - 1; i >= first; i--)
      monstro_label(ctx, g_log.lines[i % LOG_LINES]);
   monstro_end_window(ctx);
}

// A window with a popup called "menu"; the other window has one too, and they are two popups.
static bool g_open_both = false;

static void menu_window(MonstroContext *ctx, const char *name, const char *side, float x, bool opens_both)
{
   monstro_set_next_window_pos(ctx, x, 420, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 220, 160, MONSTRO_COND_FIRST_USE_EVER);
   if (!monstro_begin_window(ctx, name, nullptr, MONSTRO_WINDOW_DEFAULT))
      return;
   monstro_label(ctx, "A popup named \"menu\":");
   bool open = monstro_button(ctx, "open##Open menu", nullptr);
   MonstroRect below = monstro_get_item_rect(ctx);
   if (opens_both && monstro_button(ctx, "both##Open both menus", nullptr))
      g_open_both = true;
   if (open || g_open_both)
      monstro_open_popup(ctx, "menu");
   if (!opens_both)
      g_open_both = false;

   if (monstro_is_popup_open(ctx, "menu"))
      monstro_set_next_window_pos(ctx, below.x, below.y + below.h + 30, MONSTRO_COND_APPEARING);
   if (monstro_begin_popup(ctx, "menu", MONSTRO_WINDOW_NONE))
   {
      char text[64];
      snprintf(text, sizeof(text), "The %s window's menu", side);
      monstro_label(ctx, text);
      monstro_menu_item(ctx, "Engage", nullptr, true);
      monstro_menu_item(ctx, "Stand down", nullptr, true);
      monstro_end_popup(ctx);
   }
   monstro_end_window(ctx);
}

static void on_update(ExampleApp *app, void *, float dt)
{
   MonstroContext *ctx = app->ui;
   g_frame++;
   overlap_window(ctx);
   clipping_window(ctx);
   edges_window(ctx, dt);
   menu_window(ctx, "left##Left", "left", 420, true);
   menu_window(ctx, "right##Right", "right", 660, false);
}

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   ExampleAppDesc desc = {};
   desc.title = "Monstro Interaction Example";
   desc.width = 900;
   desc.height = 800;
   desc.update = on_update;
   return example_app_run(desc);
}
