// The built-in themes.
#include "harness.h"
#include <cstring>

// How light a colour looks, 0..1, from its channels as they are.
static float lightness(MonstroColor c) { return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b; }

// Every background a widget draws in the theme: the style items behind widgets and the plain colours that fill one
// (a slider's bar, the menu bar, table rows). A transparent colour draws nothing and is left out.
static int backgrounds(const MonstroStyle &s, MonstroColor *out, int capacity)
{
   const MonstroStyleItem *items[] = {
       &s.button.normal,
       &s.button.hover,
       &s.button.active,
       &s.checkbox.normal,
       &s.checkbox.hover,
       &s.checkbox.active,
       &s.option.normal,
       &s.option.hover,
       &s.option.active,
       &s.selectable.hover,
       &s.selectable.pressed,
       &s.selectable.normal_active,
       &s.selectable.hover_active,
       &s.selectable.pressed_active,
       &s.slider.normal,
       &s.slider.hover,
       &s.slider.active,
       &s.progress.normal,
       &s.scrollh.normal,
       &s.scrollv.normal,
       &s.edit.normal,
       &s.edit.hover,
       &s.edit.active,
       &s.property.edit.normal,
       &s.property.edit.hover,
       &s.property.edit.active,
       &s.property.inc_button.normal,
       &s.property.inc_button.hover,
       &s.property.inc_button.active,
       &s.combo.normal,
       &s.combo.hover,
       &s.combo.active,
       &s.tree.hover,
       &s.tree.active,
       &s.tree.selected,
       &s.tab.background,
       &s.tab.tab_maximize_button.normal,
       &s.tab.tab_maximize_button.hover,
       &s.tab.tab_maximize_button.active,
       &s.table.header_normal,
       &s.knob.normal,
       &s.knob.hover,
       &s.knob.active,
       &s.window.fixed_background,
       &s.window.header.active,
       &s.window.header.hover,
       &s.window.group_background,
   };
   const MonstroColor colors[] = {
       s.slider.bar_normal, s.slider.bar_hover,         s.slider.bar_active,
       s.knob.track_normal, s.window.menu_border_color, s.table.row_color_even,
   };
   int count = 0;
   for (const MonstroStyleItem *item : items)
   {
      CHECK(item->type == MONSTRO_STYLE_ITEM_COLOR);
      if (item->data.color.a > 0.0f && count < capacity)
         out[count++] = item->data.color;
   }
   for (MonstroColor c : colors)
      if (c.a > 0.0f && count < capacity)
         out[count++] = c;
   return count;
}

// The text colours of the widgets drawn on those backgrounds.
static int texts(const MonstroStyle &s, MonstroColor *out)
{
   const MonstroColor colors[] = {
       s.text.color,
       s.button.text_normal,
       s.checkbox.text_normal,
       s.option.text_normal,
       s.selectable.text_normal,
       s.edit.text_normal,
       s.property.edit.text_normal,
       s.combo.label_normal,
       s.tree.text_normal,
       s.table.header_text,
       s.tab.tab_maximize_button.text_active,
       s.window.header.label_normal,
       s.tooltip.text,
   };
   int count = 0;
   for (MonstroColor c : colors)
      out[count++] = c;
   return count;
}

TEST(the_light_theme_is_light_across_every_widget)
{
   MonstroStyle light;
   monstro_style_set_light(&light);
   MonstroColor found[64];
   int count = backgrounds(light, found, 64);
   CHECK(count > 40);
   for (int i = 0; i < count; i++)
      CHECK(lightness(found[i]) > 0.6f);
   count = texts(light, found);
   for (int i = 0; i < count; i++)
      CHECK(lightness(found[i]) < 0.3f);
}

TEST(the_dark_theme_is_dark_and_not_the_default)
{
   MonstroStyle dark, standard;
   monstro_style_set_dark(&dark);
   monstro_style_set_default(&standard);
   CHECK(memcmp(&dark, &standard, sizeof(MonstroStyle)) != 0);
   CHECK(memcmp(&dark.window.fixed_background, &standard.window.fixed_background, sizeof(MonstroStyleItem)) != 0);
   CHECK(memcmp(&dark.button.normal, &standard.button.normal, sizeof(MonstroStyleItem)) != 0);

   MonstroColor found[64];
   int count = backgrounds(dark, found, 64);
   CHECK(count > 40);
   for (int i = 0; i < count; i++)
      CHECK(lightness(found[i]) < 0.45f);
   count = texts(dark, found);
   for (int i = 0; i < count; i++)
      CHECK(lightness(found[i]) > 0.7f);
}

/* -- The widget tour's textured themes ------------------------------------------------------------------------- */

#include "../examples/themes/themes.h"
#include <cstddef>
#include <cstdio>
#include <set>

// Hands out texture IDs from 1000 up, as a renderer would its texture handles.
static uint64_t fake_upload(void *user, const char *, const unsigned char *rgba, int width, int height)
{
   CHECK(rgba && width > 0 && height > 0);
   return 1000 + (*(uint64_t *)user)++;
}

// The main widgets, each in its resting state, in a window they overflow (the last ones are scrolled out of view).
static void widget_frame(MonstroContext *ctx)
{
   static bool checked = true, selected = true, header_open = true;
   static float value = 0.4f, knob = 0.5f, number = 1.5f;
   static int option = 1, item = 0, page = 0;
   static char text[32] = "text";
   monstro_set_next_window_pos(ctx, 20, 20, MONSTRO_COND_ALWAYS);
   monstro_set_next_window_size(ctx, 420, 500, MONSTRO_COND_ALWAYS);
   if (monstro_begin_window(ctx, "Theme", nullptr, MONSTRO_WINDOW_DEFAULT))
   {
      monstro_begin_group(ctx, MONSTRO_LAYOUT_HORIZONTAL, MONSTRO_GROUP_BACKGROUND);
      monstro_button(ctx, "Button", nullptr);
      monstro_end_group(ctx);
      monstro_checkbox(ctx, "Checkbox", &checked, nullptr);
      monstro_radio_button(ctx, "Off", &option, 0, nullptr);
      monstro_radio_button(ctx, "On", &option, 1, nullptr);
      monstro_slider_float(ctx, "Slider", &value, 0, 1, "%.2f", nullptr);
      monstro_knob(ctx, "Knob", &knob, 0, 1, "%.2f", nullptr);
      monstro_progress_bar(ctx, 0.6f, 200, 20, nullptr);
      const char *items[] = {"One", "Two"};
      monstro_combo(ctx, "Combo", &item, items, 2, nullptr);
      monstro_input_text(ctx, "Text", text, sizeof(text), MONSTRO_INPUT_TEXT_NONE, nullptr);
      monstro_input_float(ctx, "Number", &number, 0.1f, 1.0f, "%.2f", nullptr);
      monstro_selectable(ctx, "Selected", &selected, MONSTRO_SELECTABLE_NONE, nullptr);
      if (monstro_begin_tree_node(ctx, "Tree", MONSTRO_TREE_DEFAULT_OPEN))
      {
         monstro_tree_leaf(ctx, "Selected leaf", MONSTRO_TREE_SELECTED);
         monstro_end_tree_node(ctx);
      }
      if (monstro_collapsing_header(ctx, "Header", &header_open))
         monstro_label(ctx, "Inside");
      const char *pages[] = {"First", "Second"};
      monstro_page_tabs(ctx, "pages", pages, 2, &page, 300, 0);
      if (monstro_begin_table(ctx, "table", 2, MONSTRO_TABLE_BORDERS, 0, 0))
      {
         monstro_table_setup_column(ctx, "Name", MONSTRO_TABLE_COLUMN_NONE, 0);
         monstro_table_setup_column(ctx, "Value", MONSTRO_TABLE_COLUMN_NONE, 0);
         monstro_table_headers_row(ctx);
         monstro_end_table(ctx);
      }
      for (int i = 0; i < 10; i++)
         monstro_label(ctx, "Filler");
      monstro_end_window(ctx);
   }
}

static uint64_t item_texture(const MonstroStyleItem &item)
{
   switch (item.type)
   {
   case MONSTRO_STYLE_ITEM_IMAGE:
      return item.data.texture_id;
   case MONSTRO_STYLE_ITEM_SPRITE:
      return item.data.sprite.texture_id;
   case MONSTRO_STYLE_ITEM_NINEPATCH:
      return item.data.ninepatch.texture_id;
   default:
      return 0;
   }
}

// Registered as the widget tour registers it, each theme gives every main widget art, and every widget draws it.
static void check_game_theme(const GameTheme *game_theme)
{
   Ui ui;
   uint64_t uploaded = 0;
   MonstroTheme *theme = game_theme_register(ui.env, game_theme, "examples/themes", fake_upload, &uploaded);
   CHECK(theme == monstro_theme_find(ui.env, game_theme->name));
   CHECK(uploaded > 10);
   monstro_context_set_theme(ui.ctx, theme);

   ui.moveMouse(790, 590); // away from every widget
   const MonstroDrawData *data = nullptr;
   for (int frame = 0; frame < 3; frame++)
   {
      ui.begin();
      widget_frame(ui.ctx);
      data = ui.end();
   }
   std::set<uint64_t> drawn;
   for (uint32_t i = 0; i < data->command_count; i++)
      drawn.insert(data->commands[i].texture_id);

   const MonstroStyle *style = monstro_theme_get_style(theme);
   const size_t resting[] = {
       offsetof(MonstroStyle, window.fixed_background),
       offsetof(MonstroStyle, window.header.active),
       offsetof(MonstroStyle, window.group_background),
       offsetof(MonstroStyle, button.normal),
       offsetof(MonstroStyle, checkbox.normal),
       offsetof(MonstroStyle, checkbox.cursor_normal),
       offsetof(MonstroStyle, option.normal),
       offsetof(MonstroStyle, option.active),
       offsetof(MonstroStyle, option.cursor_normal),
       offsetof(MonstroStyle, slider.cursor_normal),
       offsetof(MonstroStyle, knob.normal),
       offsetof(MonstroStyle, progress.normal),
       offsetof(MonstroStyle, progress.cursor_normal),
       offsetof(MonstroStyle, combo.normal),
       offsetof(MonstroStyle, edit.normal),
       offsetof(MonstroStyle, property.edit.normal),
       offsetof(MonstroStyle, property.inc_button.normal),
       offsetof(MonstroStyle, selectable.normal_active),
       offsetof(MonstroStyle, tree.selected),
       offsetof(MonstroStyle, tree.active),
       offsetof(MonstroStyle, tab.tab_maximize_button.normal),
       offsetof(MonstroStyle, tab.tab_maximize_button.active),
       offsetof(MonstroStyle, table.header_normal),
       offsetof(MonstroStyle, scrollv.normal),
       offsetof(MonstroStyle, scrollv.cursor_normal),
   };
   for (size_t slot : resting)
   {
      const MonstroStyleItem &item = *(const MonstroStyleItem *)((const char *)style + slot);
      uint64_t texture = item_texture(item);
      if (texture < 1000 || !drawn.count(texture))
      {
         printf("  theme %s: the item at offset %zu (type %d) is not drawn\n", game_theme->name, slot, (int)item.type);
         CHECK(false);
      }
   }
}

TEST(the_scifi_theme_draws_art_on_every_main_widget) { check_game_theme(&GAME_THEME_SCIFI); }

TEST(the_rpg_theme_draws_art_on_every_main_widget) { check_game_theme(&GAME_THEME_RPG); }

// The frame a context draws in a game theme, set before its first frame or switched to between frames after frames in
// the default theme, as the widget tour's --theme and its Theme menu do.
static uint64_t frame_in_theme(const GameTheme *game_theme, int default_frames)
{
   Ui ui;
   uint64_t uploaded = 0;
   MonstroTheme *theme = game_theme_register(ui.env, game_theme, "examples/themes", fake_upload, &uploaded);
   ui.moveMouse(790, 590); // away from every widget
   for (int frame = 0; frame < default_frames; frame++)
   {
      ui.begin();
      widget_frame(ui.ctx);
      ui.end();
   }
   monstro_context_set_theme(ui.ctx, theme);
   const MonstroDrawData *data = nullptr;
   for (int frame = 0; frame < 2; frame++)
   {
      ui.begin();
      widget_frame(ui.ctx);
      data = ui.end();
   }
   return data->content_hash;
}

TEST(a_game_theme_switched_to_between_frames_draws_as_one_set_before_the_first)
{
   CHECK(frame_in_theme(&GAME_THEME_SCIFI, 3) == frame_in_theme(&GAME_THEME_SCIFI, 0));
   CHECK(frame_in_theme(&GAME_THEME_RPG, 3) == frame_in_theme(&GAME_THEME_RPG, 0));
   CHECK(frame_in_theme(&GAME_THEME_SCIFI, 0) != frame_in_theme(&GAME_THEME_RPG, 0));
}
