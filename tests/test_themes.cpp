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
