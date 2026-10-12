#include "themes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The theme's own copy of the PNG reader, private to this file.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#define SLOT(member) offsetof(MonstroStyle, member)

static MonstroColor rgba(float r, float g, float b, float a) { return monstro_color_rgba(r, g, b, a); }

static MonstroStyleItem color_item(MonstroColor c)
{
   MonstroStyleItem item;
   item.type = MONSTRO_STYLE_ITEM_COLOR;
   item.data.color = c;
   return item;
}

/* -- shared base: text colours and borders ---------------------------------------------------------------------- */

// Every widget's text in one colour; the art supplies the contrast.
static void set_text_color(MonstroStyle *s, MonstroColor text)
{
   s->text.color = text;
   MonstroStyleButton *buttons[] = {&s->button,       &s->property.inc_button,     &s->property.dec_button,
                                    &s->combo.button, &s->tab.tab_maximize_button, &s->tab.tab_minimize_button};
   for (MonstroStyleButton *b : buttons)
   {
      b->text_normal = text;
      b->text_hover = text;
      b->text_active = text;
   }
   MonstroStyleToggle *toggles[] = {&s->checkbox, &s->option};
   for (MonstroStyleToggle *t : toggles)
   {
      t->text_normal = text;
      t->text_hover = text;
      t->text_active = text;
   }
   MonstroStyleSelectable *sel = &s->selectable;
   sel->text_normal = sel->text_hover = sel->text_pressed = text;
   sel->text_normal_active = sel->text_hover_active = sel->text_pressed_active = text;
   MonstroStyleEdit *edits[] = {&s->edit, &s->property.edit};
   for (MonstroStyleEdit *e : edits)
   {
      e->text_normal = e->text_hover = e->text_active = text;
      e->cursor_normal = e->cursor_hover = text;
      e->selected_text_normal = e->selected_text_hover = text;
   }
   s->property.label_normal = s->property.label_hover = s->property.label_active = text;
   s->combo.label_normal = s->combo.label_hover = s->combo.label_active = text;
   s->combo.symbol_normal = s->combo.symbol_hover = s->combo.symbol_active = text;
   s->tree.text_normal = s->tree.text_hover = s->tree.text_active = s->tree.text_selected = text;
   s->table.header_text = text;
   s->tab.text = text;
   s->tooltip.text = text;
   s->knob.indicator_normal = s->knob.indicator_hover = s->knob.indicator_active = color_item(text);
   s->window.header.label_normal = s->window.header.label_hover = s->window.header.label_active = text;
}

// Art has its own edges: no outlines drawn over it.
static void clear_borders(MonstroStyle *s)
{
   MonstroColor none = rgba(0, 0, 0, 0);
   s->button.border = 0.0f;
   s->button.border_color = none;
   s->property.inc_button.border = s->property.dec_button.border = 0.0f;
   s->checkbox.border = 0.0f;
   s->checkbox.border_color = none;
   s->option.border_color = none; // a radio button always outlines its circle, in this colour
   s->slider.border = 0.0f;
   s->progress.border = s->progress.cursor_border = 0.0f;
   s->scrollh.border = s->scrollh.border_cursor = 0.0f;
   s->scrollv.border = s->scrollv.border_cursor = 0.0f;
   s->edit.border = s->property.edit.border = s->property.border = 0.0f;
   s->combo.border = 0.0f;
   s->tab.border = 0.0f;
   s->tab.background = color_item(none);
   s->knob.border = 0.0f;
   s->window.border = 0.0f;
   s->window.border_style = color_item(none);
   s->window.group_border = 0.0f;
}

// Room for the art's rims, and the scrollbars as wide as their art. The padding across stays small: widgets that
// fill the window's width start a padding in from its left edge and run its full width, past the right one.
static void set_sizes(MonstroStyle *s, float padding_down, float scrollbar)
{
   s->window.padding[0] = 6.0f;
   s->window.padding[1] = padding_down;
   s->window.popup_padding[0] = s->window.popup_padding[1] = 6.0f;
   s->window.combo_padding[0] = s->window.combo_padding[1] = 6.0f;
   s->window.menu_padding[0] = s->window.menu_padding[1] = 6.0f;
   s->window.tooltip_padding[0] = s->window.tooltip_padding[1] = 6.0f;
   s->window.group_padding[0] = s->window.group_padding[1] = 8.0f; // also the title bar's height over its text
   s->window.scrollbar_size[0] = s->window.scrollbar_size[1] = scrollbar;
   s->button.padding[0] = 10.0f;
   s->button.padding[1] = 6.0f;
   s->edit.padding[0] = s->property.edit.padding[0] = 6.0f;
   s->combo.content_padding[0] = 8.0f;
   s->combo.content_padding[1] = 5.0f;
   s->tab.padding[0] = 10.0f;
   s->tab.padding[1] = 6.0f;
   s->tab.spacing[0] = 2.0f;
   s->tree.padding[1] = 5.0f;
   s->table.header_height = 30.0f;
   s->slider.bar_height = 10.0f;
   s->slider.rounding = 5.0f;
}

/* -- Sci-Fi: Kenney UI Pack: Sci-fi 2.0 ------------------------------------------------------------------------- */

static void scifi_base(MonstroStyle *s)
{
   monstro_style_set_default(s);
   clear_borders(s);
   set_sizes(s, 12.0f, 14.0f);
   set_text_color(s, rgba(0.08f, 0.10f, 0.17f, 1.0f)); // dark navy on the light grey and blue art

   MonstroColor outline = rgba(0.59f, 0.59f, 0.64f, 1.0f);        // the art's own outline grey
   MonstroColor blue = rgba(0.11f, 0.62f, 0.84f, 1.0f);           // the art's blue
   MonstroColor slot = rgba(0.0f, 0.0f, 0.0f, 0.30f);             // as the art's dark slots
   s->window.menu_border_color = rgba(0.77f, 0.78f, 0.83f, 1.0f); // the menu bar
   s->window.scaler = color_item(outline);
   s->window.popup_border_color = s->window.combo_border_color = outline;
   s->window.contextual_border_color = s->window.menu_border_color;
   s->window.modal_dim_color = rgba(0.0f, 0.0f, 0.0f, 0.4f);
   s->window.rounding = 6.0f;
   s->separator.color = outline;
   s->slider.normal = s->slider.hover = s->slider.active = color_item(rgba(0, 0, 0, 0));
   s->slider.bar_normal = slot;
   s->slider.bar_hover = s->slider.bar_active = rgba(0.0f, 0.0f, 0.0f, 0.38f);
   s->slider.bar_filled = blue;
   s->slider.cursor_size[0] = s->slider.cursor_size[1] = 20.0f;
   s->knob.track_normal = slot;
   s->knob.track_filled = blue;
   s->edit.selected_normal = s->edit.selected_hover = blue;
   s->property.edit.selected_normal = s->property.edit.selected_hover = blue;
   s->table.border_color = outline;
   s->table.row_color_even = rgba(0, 0, 0, 0);
   s->table.row_color_odd = rgba(0.0f, 0.0f, 0.0f, 0.06f);
   s->selectable.normal = color_item(rgba(0, 0, 0, 0));
}

static const GameThemeArt SCIFI_ART[] = {
    // Windows, popups, menus and tooltips: a light grey panel with screws in its corners, under a blue title strip
    // (the header of a panel with a header).
    {SLOT(window.fixed_background), "panel_glass_screws.png", 14, 14, 14, 14, 0, 0, 0, 0},
    {SLOT(window.header.active), "blue_button_square_header_large_rectangle.png", 8, 8, 8, 2, 0, 0, 192, 32},
    {SLOT(window.header.hover), "red_bar_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(window.group_background), "panel_rectangle.png", 8, 8, 8, 8, 0, 0, 0, 0},

    // Buttons: raised grey, lit blue under the pointer, flat blue held.
    {SLOT(button.normal), "button_rectangle_depth.png", 8, 8, 8, 12, 0, 0, 0, 0},
    {SLOT(button.hover), "blue_bar_square_gloss_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(button.active), "blue_bar_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(property.dec_button.normal), "button_square_depth.png", 8, 8, 8, 12, 0, 0, 0, 0},
    {SLOT(property.dec_button.hover), "blue_bar_square_gloss_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(property.dec_button.active), "blue_bar_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(property.inc_button.normal), "button_square_depth.png", 8, 8, 8, 12, 0, 0, 0, 0},
    {SLOT(property.inc_button.hover), "blue_bar_square_gloss_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(property.inc_button.active), "blue_bar_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(combo.normal), "button_rectangle_depth.png", 8, 8, 8, 12, 0, 0, 0, 0},
    {SLOT(combo.hover), "blue_bar_square_gloss_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(combo.active), "button_rectangle.png", 8, 8, 8, 8, 0, 0, 0, 0},

    // Fields: dark slots in the panel.
    {SLOT(edit.normal), "bar_shadow_square_outline_large.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(edit.hover), "bar_shadow_square_outline_large.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(edit.active), "bar_shadow_square_large.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(property.edit.normal), "bar_shadow_square_outline_large.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(property.edit.hover), "bar_shadow_square_outline_large.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(property.edit.active), "bar_shadow_square_large.png", 4, 4, 4, 4, 0, 0, 0, 0},

    // Checkboxes and radio buttons: slots with a glossy light in them, yellow under the pointer.
    {SLOT(checkbox.normal), "bar_shadow_square_small_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(checkbox.hover), "bar_shadow_square_outline_small_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(checkbox.active), "bar_shadow_square_outline_small_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(checkbox.cursor_normal), "blue_bar_square_gloss_small_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(checkbox.cursor_hover), "yellow_bar_square_gloss_small_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(option.normal), "bar_shadow_round_small_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(option.hover), "bar_shadow_round_outline_small_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(option.active), "bar_shadow_round_small_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(option.cursor_normal), "blue_bar_round_gloss_small_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(option.cursor_hover), "yellow_bar_round_gloss_small_square.png", 0, 0, 0, 0, 0, 0, 0, 0},

    // Sliders and knobs: glossy round caps.
    {SLOT(slider.cursor_normal), "blue_bar_round_gloss_large_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(slider.cursor_hover), "yellow_bar_round_gloss_large_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(slider.cursor_active), "yellow_bar_round_gloss_large_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(knob.normal), "grey_bar_round_gloss_large_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(knob.hover), "blue_bar_round_gloss_large_square.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(knob.active), "blue_bar_round_gloss_large_square.png", 0, 0, 0, 0, 0, 0, 0, 0},

    // Progress bars and scrollbars: a dark slot holding a yellow or blue bar.
    {SLOT(progress.normal), "bar_shadow_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(progress.cursor_normal), "yellow_bar_square_gloss_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(scrollv.normal), "bar_shadow_square_small.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollv.cursor_normal), "blue_bar_square_small.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollv.cursor_hover), "yellow_bar_square_small.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollv.cursor_active), "yellow_bar_square_small.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollh.normal), "bar_shadow_square_small.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollh.cursor_normal), "blue_bar_square_small.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollh.cursor_hover), "yellow_bar_square_small.png", 4, 4, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollh.cursor_active), "yellow_bar_square_small.png", 4, 4, 4, 4, 0, 0, 0, 0},

    // Tabs: grey plates, the selected one blue.
    {SLOT(tab.tab_maximize_button.normal), "button_rectangle.png", 8, 8, 8, 8, 0, 0, 0, 0},
    {SLOT(tab.tab_maximize_button.hover), "button_rectangle_depth.png", 8, 8, 8, 12, 0, 0, 0, 0},
    {SLOT(tab.tab_maximize_button.active), "blue_bar_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},

    // Rows: blue under the pointer, glossy blue selected; a collapsing header is a raised plate.
    {SLOT(selectable.hover), "blue_bar_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(selectable.pressed), "blue_bar_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(selectable.normal_active), "blue_bar_square_gloss_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(selectable.hover_active), "blue_bar_square_gloss_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(selectable.pressed_active), "blue_bar_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(tree.hover), "blue_bar_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(tree.active), "button_rectangle_depth.png", 8, 8, 8, 12, 0, 0, 0, 0},
    {SLOT(tree.selected), "blue_bar_square_gloss_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(table.header_normal), "blue_bar_square_large.png", 6, 6, 6, 6, 0, 0, 0, 0},
};

const GameTheme GAME_THEME_SCIFI = {
    "scifi", "KenneyFuture.ttf", 16.0f, scifi_base, SCIFI_ART, (int)(sizeof(SCIFI_ART) / sizeof(SCIFI_ART[0])),
};

/* -- RPG: Kenney UI pack: RPG extension ----------------------------------------------------------------------- */

static void rpg_base(MonstroStyle *s)
{
   monstro_style_set_default(s);
   clear_borders(s);
   set_sizes(s, 10.0f, 18.0f);
   MonstroColor ink = rgba(0.23f, 0.16f, 0.10f, 1.0f);  // dark brown on the parchment and beige art
   MonstroColor cream = rgba(1.0f, 0.97f, 0.91f, 1.0f); // on the brown and slate art
   set_text_color(s, ink);
   s->button.text_normal = cream; // a brown button; beige under the pointer and held
   s->tab.tab_maximize_button.text_active = cream;
   s->selectable.text_normal_active = s->selectable.text_hover_active = s->selectable.text_pressed_active = cream;
   s->tree.text_selected = cream;
   s->table.header_text = cream;
   s->window.header.label_normal = s->window.header.label_hover = s->window.header.label_active = cream;
   s->knob.indicator_normal = s->knob.indicator_hover = s->knob.indicator_active = color_item(cream);

   MonstroColor tan = rgba(0.69f, 0.63f, 0.47f, 1.0f);    // the beige art's rim
   MonstroColor orange = rgba(0.91f, 0.42f, 0.09f, 1.0f); // the red bar's fill
   MonstroColor slot = rgba(0.0f, 0.0f, 0.0f, 0.12f);     // as the bars' backs
   s->window.menu_border_color = tan;                     // the menu bar
   s->window.scaler = color_item(rgba(0.53f, 0.40f, 0.27f, 1.0f));
   s->window.popup_border_color = s->window.combo_border_color = tan;
   s->window.modal_dim_color = rgba(0.0f, 0.0f, 0.0f, 0.4f);
   s->window.rounding = 4.0f;
   s->separator.color = tan;
   s->slider.normal = s->slider.hover = s->slider.active = color_item(rgba(0, 0, 0, 0));
   s->slider.bar_normal = slot;
   s->slider.bar_hover = s->slider.bar_active = rgba(0.0f, 0.0f, 0.0f, 0.18f);
   s->slider.bar_filled = orange;
   s->slider.cursor_size[0] = 18.0f;
   s->slider.cursor_size[1] = 20.0f;
   s->knob.track_normal = slot;
   s->knob.track_filled = orange;
   MonstroColor selection = rgba(0.59f, 0.44f, 0.29f, 1.0f); // the brown art
   s->edit.selected_normal = s->edit.selected_hover = selection;
   s->edit.selected_text_normal = s->edit.selected_text_hover = cream;
   s->property.edit.selected_normal = s->property.edit.selected_hover = selection;
   s->property.edit.selected_text_normal = s->property.edit.selected_text_hover = cream;
   s->table.border_color = tan;
   s->table.row_color_even = rgba(0, 0, 0, 0);
   s->table.row_color_odd = rgba(0.0f, 0.0f, 0.0f, 0.05f);
   s->selectable.normal = color_item(rgba(0, 0, 0, 0));
   s->property.dec_button.text_normal = s->property.inc_button.text_normal = cream;
}

static const GameThemeArt RPG_ART[] = {
    // Windows, popups, menus and tooltips: a beige panel under a brown title plank. The borders keep the panel's
    // edge notches in its corners, and the planks' in their left ends, where they are not stretched.
    {SLOT(window.fixed_background), "panel_beige.png", 20, 8, 24, 33, 0, 0, 0, 0},
    {SLOT(window.header.active), "buttonLong_brown_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},
    {SLOT(window.header.hover), "buttonLong_beige_pressed.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(window.group_background), "panel_beigeLight.png", 6, 6, 6, 6, 0, 0, 0, 0},

    // Buttons: raised brown, beige under the pointer, pressed beige held.
    {SLOT(button.normal), "buttonLong_brown.png", 6, 6, 6, 10, 0, 0, 0, 0},
    {SLOT(button.hover), "buttonLong_beige.png", 6, 6, 6, 10, 0, 0, 0, 0},
    {SLOT(button.active), "buttonLong_beige_pressed.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(property.dec_button.normal), "buttonSquare_brown.png", 6, 6, 6, 10, 0, 0, 0, 0},
    {SLOT(property.dec_button.hover), "buttonLong_beige.png", 6, 6, 6, 10, 0, 0, 0, 0},
    {SLOT(property.dec_button.active), "buttonLong_beige_pressed.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(property.inc_button.normal), "buttonSquare_brown.png", 6, 6, 6, 10, 0, 0, 0, 0},
    {SLOT(property.inc_button.hover), "buttonLong_beige.png", 6, 6, 6, 10, 0, 0, 0, 0},
    {SLOT(property.inc_button.active), "buttonLong_beige_pressed.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(combo.normal), "buttonLong_beige.png", 43, 92, 6, 10, 0, 0, 0, 0},
    {SLOT(combo.hover), "buttonLong_grey_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},
    {SLOT(combo.active), "buttonLong_beige_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},

    // Fields: light parchment.
    {SLOT(edit.normal), "panel_beigeLight.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(edit.hover), "panel_beigeLight.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(edit.active), "panel_beigeLight.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(property.edit.normal), "panel_beigeLight.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(property.edit.hover), "panel_beigeLight.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(property.edit.active), "panel_beigeLight.png", 6, 6, 6, 6, 0, 0, 0, 0},

    // Checkboxes and radio buttons: brown buttons, slate under the pointer, with a cream check or dot.
    {SLOT(checkbox.normal), "buttonSquare_brown.png", 6, 6, 6, 10, 0, 0, 0, 0},
    {SLOT(checkbox.hover), "buttonSquare_blue.png", 6, 6, 6, 10, 0, 0, 0, 0},
    {SLOT(checkbox.active), "buttonSquare_brown_pressed.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(checkbox.cursor_normal), "iconCheck_beige.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(checkbox.cursor_hover), "iconCheck_beige.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(option.normal), "buttonRound_brown.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(option.hover), "buttonRound_blue.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(option.active), "buttonRound_brown.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(option.cursor_normal), "iconCircle_beige.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(option.cursor_hover), "iconCircle_beige.png", 0, 0, 0, 0, 0, 0, 0, 0},

    // Sliders and knobs: round brown buttons, slate under the pointer.
    {SLOT(slider.cursor_normal), "buttonRound_brown.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(slider.cursor_hover), "buttonRound_blue.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(slider.cursor_active), "buttonRound_blue.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(knob.normal), "buttonRound_brown.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(knob.hover), "buttonRound_blue.png", 0, 0, 0, 0, 0, 0, 0, 0},
    {SLOT(knob.active), "buttonRound_blue.png", 0, 0, 0, 0, 0, 0, 0, 0},

    // Progress bars and scrollbars: rounded bars in their backs, each joined from the pack's three pieces.
    {SLOT(progress.normal), "barBack_horizontal.png", 9, 9, 4, 4, 0, 0, 0, 0},
    {SLOT(progress.cursor_normal), "barRed_horizontal.png", 9, 9, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollv.normal), "barBack_vertical.png", 4, 4, 9, 9, 0, 0, 0, 0},
    {SLOT(scrollv.cursor_normal), "barYellow_vertical.png", 4, 4, 9, 9, 0, 0, 0, 0},
    {SLOT(scrollv.cursor_hover), "barYellow_vertical.png", 4, 4, 9, 9, 0, 0, 0, 0},
    {SLOT(scrollv.cursor_active), "barYellow_vertical.png", 4, 4, 9, 9, 0, 0, 0, 0},
    {SLOT(scrollh.normal), "barBack_horizontal.png", 9, 9, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollh.cursor_normal), "barYellow_horizontal.png", 9, 9, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollh.cursor_hover), "barYellow_horizontal.png", 9, 9, 4, 4, 0, 0, 0, 0},
    {SLOT(scrollh.cursor_active), "barYellow_horizontal.png", 9, 9, 4, 4, 0, 0, 0, 0},

    // Tabs: pressed beige, raised beige under the pointer, brown selected.
    {SLOT(tab.tab_maximize_button.normal), "buttonLong_beige_pressed.png", 6, 6, 6, 6, 0, 0, 0, 0},
    {SLOT(tab.tab_maximize_button.hover), "buttonLong_beige.png", 6, 6, 6, 10, 0, 0, 0, 0},
    {SLOT(tab.tab_maximize_button.active), "buttonLong_brown.png", 6, 6, 6, 10, 0, 0, 0, 0},

    // Rows: stone under the pointer, brown selected; a collapsing header is a beige plank.
    {SLOT(selectable.hover), "buttonLong_grey_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},
    {SLOT(selectable.pressed), "buttonLong_beige_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},
    {SLOT(selectable.normal_active), "buttonLong_brown_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},
    {SLOT(selectable.hover_active), "buttonLong_brown_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},
    {SLOT(selectable.pressed_active), "buttonLong_brown_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},
    {SLOT(tree.hover), "buttonLong_grey_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},
    {SLOT(tree.active), "buttonLong_beige.png", 43, 92, 6, 10, 0, 0, 0, 0},
    {SLOT(tree.selected), "buttonLong_brown_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},
    {SLOT(table.header_normal), "buttonLong_brown_pressed.png", 43, 92, 6, 6, 0, 0, 0, 0},
};

const GameTheme GAME_THEME_RPG = {
    "rpg", NULL, 0.0f, rpg_base, RPG_ART, (int)(sizeof(RPG_ART) / sizeof(RPG_ART[0])),
};

/* -- registration ------------------------------------------------------------------------------------------------ */

struct LoadedImage
{
   const char *file;
   uint64_t texture_id;
   int width, height;
};

MonstroTheme *game_theme_register(MonstroEnv *env, const GameTheme *theme, const char *art_root, GameThemeUpload upload,
                                  void *user)
{
   MonstroStyle style;
   theme->base(&style);

   char path[1024];
   if (theme->font)
   {
      snprintf(path, sizeof(path), "%s/%s/%s", art_root, theme->name, theme->font);
      style.font = monstro_font_load_ttf_from_file(env, theme->name, path, theme->font_size, 32, 96);
      if (!style.font)
      {
         fprintf(stderr, "theme %s: cannot load the font %s\n", theme->name, path);
         abort();
      }
   }

   // Each image is read and uploaded once, however many slots show it.
   LoadedImage images[64];
   int image_count = 0;
   for (int i = 0; i < theme->art_count; i++)
   {
      const GameThemeArt *art = &theme->art[i];
      LoadedImage *image = NULL;
      for (int j = 0; j < image_count && !image; j++)
         if (strcmp(images[j].file, art->file) == 0)
            image = &images[j];
      if (!image)
      {
         if (image_count == (int)(sizeof(images) / sizeof(images[0])))
         {
            fprintf(stderr, "theme %s: more than %d images\n", theme->name, image_count);
            abort();
         }
         snprintf(path, sizeof(path), "%s/%s/%s", art_root, theme->name, art->file);
         int width = 0, height = 0, channels = 0;
         unsigned char *rgba = stbi_load(path, &width, &height, &channels, 4);
         if (!rgba)
         {
            fprintf(stderr, "theme %s: cannot load %s: %s\n", theme->name, path, stbi_failure_reason());
            abort();
         }
         image = &images[image_count++];
         image->file = art->file;
         image->width = width;
         image->height = height;
         image->texture_id = upload(user, path, rgba, width, height);
         stbi_image_free(rgba);
      }

      int w = art->w > 0 ? art->w : image->width;
      int h = art->h > 0 ? art->h : image->height;
      if (art->x < 0 || art->y < 0 || art->x + w > image->width || art->y + h > image->height)
      {
         fprintf(stderr, "theme %s: %d,%d %dx%d lies outside %s (%dx%d)\n", theme->name, art->x, art->y, w, h,
                 art->file, image->width, image->height);
         abort();
      }
      MonstroVec2 uv_min = {(float)art->x / (float)image->width, (float)art->y / (float)image->height};
      MonstroVec2 uv_max = {(float)(art->x + w) / (float)image->width, (float)(art->y + h) / (float)image->height};

      MonstroStyleItem *item = (MonstroStyleItem *)((char *)&style + art->slot);
      if (art->left > 0 || art->right > 0 || art->top > 0 || art->bottom > 0)
      {
         item->type = MONSTRO_STYLE_ITEM_NINEPATCH;
         MonstroNinePatch *patch = &item->data.ninepatch;
         patch->texture_id = image->texture_id;
         patch->uv_min = uv_min;
         patch->uv_max = uv_max;
         patch->border_left = art->left;
         patch->border_right = art->right;
         patch->border_top = art->top;
         patch->border_bottom = art->bottom;
         patch->texture_width = (uint32_t)image->width;
         patch->texture_height = (uint32_t)image->height;
      }
      else
      {
         item->type = MONSTRO_STYLE_ITEM_SPRITE;
         item->data.sprite.texture_id = image->texture_id;
         item->data.sprite.uv_min = uv_min;
         item->data.sprite.uv_max = uv_max;
      }
   }

   // The scrollbar inside a multi-line field matches the window's.
   style.edit.scrollbar = style.scrollv;
   return monstro_theme_register(env, theme->name, &style);
}
