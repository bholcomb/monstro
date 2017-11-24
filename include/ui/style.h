/*-----------------------------------------------------------------------------
Copyright 2017
Bob Holcomb
-----------------------------------------------------------------------------*/

#include "ui/util.h"

namespace UI
{
   enum StyleItemType {COLOR, IMAGE};
   const int MAX_CURSOR_COUNT = 64;

   enum Alignment
   {
      LEFT = 1 << 0,
      RIGHT = 1 << 1,
      TOP = 1 << 2,
      BOTTOM = 1 << 3,
      HCENTER = LEFT | RIGHT,
      VCETNER = TOP | BOTTOM,
      MIDDLE = HCENTER | VCENTER,
      DEFAULT = LEFT | HCENTER
   };

   struct Cursor
   {
      UInt32 image;
      Vec2 size;
      Vec2 offset;
   };

   union StyleItemData
   {
      UInt32 texture;
      UInt32 color;
   };

   struct StyleItem
   {
      StyleItemType type;
      StyleItemData data;
   };

   struct StyleText
   {
      Color color;
      Vec2 padding;
   };

   struct StyleButton
   {
      /* background */
      StyleItem normal;
      StyleItem hover;
      StyleItem active;
      Color border_color;

      /* text */
      Color text_background;
      Color text_normal;
      Color text_hover;
      Color text_active;
      Alignment text_alignment;

      /* properties */
      float border;
      float rounding;
      Vec2 padding;
      Vec2 image_padding;
      Vec2 touch_padding;
   };

   struct StyleToggle
   {
      /* background */
      StyleItem normal;
      StyleItem hover;
      StyleItem active;
      Color border_color;

      /* cursor */
      StyleItem cursor_normal;
      StyleItem cursor_hover;

      /* text */
      Color text_normal;
      Color text_hover;
      Color text_active;
      Color text_background;
      Alignment text_alignment;

      /* properties */
      Vec2 padding;
      Vec2 touch_padding;
      float spacing;
      float border;
   };

   struct StyleSelectable
   {
      /* background (inactive) */
      StyleItem normal;
      StyleItem hover;
      StyleItem pressed;

      /* background (active) */
      StyleItem normal_active;
      StyleItem hover_active;
      StyleItem pressed_active;

      /* text color (inactive) */
      Color text_normal;
      Color text_hover;
      Color text_pressed;

      /* text color (active) */
      Color text_normal_active;
      Color text_hover_active;
      Color text_pressed_active;
      Color text_background;
      Alignment text_alignment;

      /* properties */
      float rounding;
      Vec2 padding;
      Vec2 touch_padding;
      Vec2 image_padding;
   };

   struct StyleSlider
   {
      /* background */
      StyleItem normal;
      StyleItem hover;
      StyleItem active;
      Color border_color;

      /* background bar */
      Color bar_normal;
      Color bar_hover;
      Color bar_active;
      Color bar_filled;

      /* cursor */
      StyleItem cursor_normal;
      StyleItem cursor_hover;
      StyleItem cursor_active;

      /* properties */
      float border;
      float rounding;
      float bar_height;
      Vec2 padding;
      Vec2 spacing;
      Vec2 cursor_size;

      /* optional buttons */
      int show_buttons;
      StyleButton inc_button;
      StyleButton dec_button;
      UInt32 inc_symbol;
      UInt32 dec_symbol;
   };

   struct StyleProgress
   {
      /* background */
      StyleItem normal;
      StyleItem hover;
      StyleItem active;
      Color border_color;

      /* cursor */
      StyleItem cursor_normal;
      StyleItem cursor_hover;
      StyleItem cursor_active;
      Color cursor_border_color;

      /* properties */
      float rounding;
      float border;
      float cursor_border;
      float cursor_rounding;
      Vec2 padding;
   };

   struct StyleScrollbar
   {
      /* background */
      StyleItem normal;
      StyleItem hover;
      StyleItem active;
      Color border_color;

      /* cursor */
      StyleItem cursor_normal;
      StyleItem cursor_hover;
      StyleItem cursor_active;
      Color cursor_border_color;

      /* properties */
      float border;
      float rounding;
      float border_cursor;
      float rounding_cursor;
      Vec2 padding;

      /* optional buttons */
      int show_buttons;
      StyleButton inc_button;
      StyleButton dec_button;
      UInt32 inc_symbol;
      UInt32 dec_symbol;
   };

   struct StyleEdit
   {
      /* background */
      StyleItem normal;
      StyleItem hover;
      StyleItem active;
      Color border_color;
      StyleScrollbar scrollbar;

      /* cursor  */
      Color cursor_normal;
      Color cursor_hover;
      Color cursor_text_normal;
      Color cursor_text_hover;

      /* text (unselected) */
      Color text_normal;
      Color text_hover;
      Color text_active;

      /* text (selected) */
      Color selected_normal;
      Color selected_hover;
      Color selected_text_normal;
      Color selected_text_hover;

      /* properties */
      float border;
      float rounding;
      float cursor_size;
      Vec2 scrollbar_size;
      Vec2 padding;
      float row_padding;
   };

   struct StyleProperty
   {
      /* background */
      StyleItem normal;
      StyleItem hover;
      StyleItem active;
      Color border_color;

      /* text */
      Color label_normal;
      Color label_hover;
      Color label_active;

      /* symbols */
      UInt32 sym_left;
      UInt32 sym_right;

      /* properties */
      float border;
      float rounding;
      Vec2 padding;

      StyleEdit edit;
      StyleButton inc_button;
      StyleButton dec_button;
   };

   struct StyleChart
   {
      /* colors */
      StyleItem background;
      Color border_color;
      Color selected_color;
      Color color;

      /* properties */
      float border;
      float rounding;
      Vec2 padding;
   };

   struct StyleCombo
   {
      /* background */
      StyleItem normal;
      StyleItem hover;
      StyleItem active;
      Color border_color;

      /* label */
      Color label_normal;
      Color label_hover;
      Color label_active;

      /* symbol */
      Color symbol_normal;
      Color symbol_hover;
      Color symbol_active;

      /* button */
      StyleButton button;
      UInt32 sym_normal;
      UInt32 sym_hover;
      UInt32 sym_active;

      /* properties */
      float border;
      float rounding;
      Vec2 content_padding;
      Vec2 button_padding;
      Vec2 spacing;
   };

   struct StyleTab
   {
      /* background */
      StyleItem background;
      Color border_color;
      Color text;

      /* button */
      StyleButton tab_maximize_button;
      StyleButton tab_minimize_button;
      StyleButton node_maximize_button;
      StyleButton node_minimize_button;
      UInt32 sym_minimize;
      UInt32 sym_maximize;

      /* properties */
      float border;
      float rounding;
      float indent;
      Vec2 padding;
      Vec2 spacing;
   };

   struct StyleWindowHeader
   {
      /* background */
      StyleItem normal;
      StyleItem hover;
      StyleItem active;

      /* button */
      StyleButton close_button;
      StyleButton minimize_button;
      UInt32 close_symbol;
      UInt32 minimize_symbol;
      UInt32 maximize_symbol;

      /* title */
      Color label_normal;
      Color label_hover;
      Color label_active;

      /* properties */
      Alignment align;
      Vec2 padding;
      Vec2 label_padding;
      Vec2 spacing;
   };

   struct StyleWindow
   {
      StyleWindowHeader header;
      StyleItem fixed_background;
      Color background;

      Color border_color;
      Color popup_border_color;
      Color combo_border_color;
      Color contextual_border_color;
      Color menu_border_color;
      Color group_border_color;
      Color tooltip_border_color;
      StyleItem scaler;

      float border;
      float combo_border;
      float contextual_border;
      float menu_border;
      float group_border;
      float tooltip_border;
      float popup_border;

      float rounding;
      Vec2 spacing;
      Vec2 scrollbar_size;
      Vec2 min_size;

      Vec2 padding;
      Vec2 group_padding;
      Vec2 popup_padding;
      Vec2 combo_padding;
      Vec2 contextual_padding;
      Vec2 menu_padding;
      Vec2 tooltip_padding;
   };

   struct Style
   {
      Font* font;
      Cursor* cursors[MAX_CURSOR_COUNT];
      Cursor* cursor_active;
      Cursor* cursor_last;
      bool cursor_visible;

      StyleText text;
      StyleButton button;
      StyleButton contextual_button;
      StyleButton menu_button;
      StyleToggle option;
      StyleToggle checkbox;
      StyleSelectable selectable;
      StyleSlider slider;
      StyleProgress progress;
      StyleProperty property;
      StyleEdit edit;
      StyleChart chart;
      StyleScrollbar scrollh;
      StyleScrollbar scrollv;
      StyleTab tab;
      StyleCombo combo;
      StyleWindow window;
   };
}