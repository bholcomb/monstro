#include "../include/monstro/style.h"
#include <string.h>

static inline MonstroColor rgba(int r, int g, int b, int a) { return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f}; }

static inline MonstroStyleItem styleItemColor(MonstroColor c)
{
   MonstroStyleItem item;
   item.type = MONSTRO_STYLE_ITEM_COLOR;
   item.data.color = c;
   return item;
}

static inline MonstroStyleItem styleItemHide() { return styleItemColor(rgba(0, 0, 0, 0)); }

void monstro_style_set_default(MonstroStyle *style)
{
   if (!style)
      return;

   memset(style, 0, sizeof(MonstroStyle));

   // Midnight Blue Next theme palette
   MonstroColor TEXT = rgba(230, 230, 230, 255);
   MonstroColor TEXT_SEL = rgba(255, 255, 255, 255);
   MonstroColor BACKGROUND = rgba(230, 230, 230, 255);
   MonstroColor WINDOW = rgba(28, 35, 45, 255);
   MonstroColor HEADER = rgba(33, 40, 51, 255);
   MonstroColor BORDER = rgba(19, 24, 29, 255);
   MonstroColor BUTTON = rgba(14, 20, 26, 255);
   MonstroColor BUTTON_HOVER = rgba(62, 84, 113, 204);
   MonstroColor BUTTON_ACTIVE = rgba(53, 90, 139, 255);
   MonstroColor TOGGLE = rgba(17, 21, 27, 255);
   MonstroColor TOGGLE_HOVER = rgba(39, 48, 61, 255);
   MonstroColor TOGGLE_CURSOR = rgba(71, 114, 179, 255);
   MonstroColor SELECT = rgba(34, 63, 94, 0);
   MonstroColor SELECT_ACTIVE = rgba(55, 87, 118, 255);
   MonstroColor SLIDER = rgba(18, 22, 25, 255);
   MonstroColor SLIDER_CURSOR = rgba(63, 78, 100, 230);
   MonstroColor SLIDER_CURSOR_HOVER = rgba(87, 108, 139, 255);
   MonstroColor SLIDER_CURSOR_ACTIVE = rgba(71, 114, 179, 255);
   MonstroColor PROPERTY = rgba(18, 22, 25, 255);
   MonstroColor EDIT = rgba(11, 14, 20, 255);
   MonstroColor COMBO = rgba(14, 20, 26, 255);
   MonstroColor CHART = rgba(18, 26, 34, 255);
   MonstroColor CHART_COLOR = rgba(86, 128, 194, 255);
   MonstroColor CHART_COLOR_HL = rgba(255, 175, 35, 255);
   MonstroColor SCROLLBAR = rgba(12, 20, 26, 128);
   MonstroColor SCROLLBAR_CURSOR = rgba(53, 64, 78, 255);
   MonstroColor SCROLLBAR_CURSOR_HOVER = rgba(30, 47, 65, 255);
   MonstroColor SCROLLBAR_CURSOR_ACTIVE = rgba(71, 114, 179, 255);
   MonstroColor TAB_HEADER = rgba(10, 12, 14, 255);
   MonstroColor ACCENT = rgba(71, 114, 179, 255);
   MonstroColor MENU_BG = rgba(24, 32, 43, 255);
   MonstroColor MENU_ITEM_SEL = rgba(39, 70, 107, 255);
   MonstroColor TOOLTIP_BG = rgba(41, 51, 68, 255);
   MonstroColor TOOLTIP_BORDER = rgba(59, 74, 94, 255);
   MonstroColor EDIT_CURSOR = rgba(51, 154, 230, 255);
   MonstroColor SEPARATOR_COL = rgba(59, 74, 94, 77);

   // Text
   style->text.color = TEXT;
   style->text.background = BACKGROUND;
   style->text.padding[0] = 4.0f;
   style->text.padding[1] = 4.0f;
   style->text.alignment = MONSTRO_ALIGN_LEFT;

   // Button
   style->button.normal = styleItemColor(BUTTON);
   style->button.hover = styleItemColor(BUTTON_HOVER);
   style->button.active = styleItemColor(BUTTON_ACTIVE);
   style->button.border_color = BORDER;
   style->button.text_background = BUTTON;
   style->button.text_normal = TEXT;
   style->button.text_hover = TEXT_SEL;
   style->button.text_active = TEXT_SEL;
   style->button.text_alignment = MONSTRO_ALIGN_CENTER;
   style->button.border = 1.0f;
   style->button.rounding = 4.0f;
   style->button.padding[0] = 4.0f;
   style->button.padding[1] = 4.0f;
   style->button.image_padding[0] = 0.0f;
   style->button.image_padding[1] = 0.0f;
   style->button.touch_padding[0] = 0.0f;
   style->button.touch_padding[1] = 0.0f;

   // Contextual Button (menu item style)
   style->contextual_button.normal = styleItemHide();
   style->contextual_button.hover = styleItemColor(MENU_ITEM_SEL);
   style->contextual_button.active = styleItemColor(ACCENT);
   style->contextual_button.border_color = rgba(0, 0, 0, 0);
   style->contextual_button.text_background = MENU_BG;
   style->contextual_button.text_normal = TEXT;
   style->contextual_button.text_hover = TEXT_SEL;
   style->contextual_button.text_active = TEXT_SEL;
   style->contextual_button.text_alignment = MONSTRO_ALIGN_CENTER;
   style->contextual_button.border = 0.0f;
   style->contextual_button.rounding = 0.0f;
   style->contextual_button.padding[0] = 2.0f;
   style->contextual_button.padding[1] = 2.0f;
   style->contextual_button.touch_padding[0] = 0.0f;
   style->contextual_button.touch_padding[1] = 0.0f;

   // Menu Button
   style->menu_button.normal = styleItemColor(HEADER);
   style->menu_button.hover = styleItemColor(MENU_ITEM_SEL);
   style->menu_button.active = styleItemColor(ACCENT);
   style->menu_button.border_color = rgba(0, 0, 0, 0);
   style->menu_button.text_background = HEADER;
   style->menu_button.text_normal = TEXT;
   style->menu_button.text_hover = TEXT_SEL;
   style->menu_button.text_active = TEXT_SEL;
   style->menu_button.text_alignment = MONSTRO_ALIGN_CENTER;
   style->menu_button.border = 0.0f;
   style->menu_button.rounding = 1.0f;
   style->menu_button.padding[0] = 2.0f;
   style->menu_button.padding[1] = 2.0f;
   style->menu_button.touch_padding[0] = 0.0f;
   style->menu_button.touch_padding[1] = 0.0f;

   // Checkbox
   style->checkbox.normal = styleItemColor(TOGGLE);
   style->checkbox.hover = styleItemColor(TOGGLE_HOVER);
   style->checkbox.active = styleItemColor(TOGGLE_HOVER);
   style->checkbox.cursor_normal = styleItemColor(TOGGLE_CURSOR);
   style->checkbox.cursor_hover = styleItemColor(TOGGLE_CURSOR);
   style->checkbox.border_color = rgba(55, 68, 87, 179);
   style->checkbox.text_background = WINDOW;
   style->checkbox.text_normal = TEXT;
   style->checkbox.text_hover = TEXT_SEL;
   style->checkbox.text_active = TEXT_SEL;
   style->checkbox.text_alignment = MONSTRO_ALIGN_LEFT;
   style->checkbox.padding[0] = 2.0f;
   style->checkbox.padding[1] = 2.0f;
   style->checkbox.touch_padding[0] = 0.0f;
   style->checkbox.touch_padding[1] = 0.0f;
   style->checkbox.border = 0.0f;
   style->checkbox.spacing = 4.0f;

   // Option (radio button)
   style->option.normal = styleItemColor(TOGGLE);
   style->option.hover = styleItemColor(TOGGLE_HOVER);
   style->option.active = styleItemColor(rgba(45, 60, 80, 255));
   style->option.cursor_normal = styleItemColor(TOGGLE_CURSOR);
   style->option.cursor_hover = styleItemColor(TOGGLE_CURSOR);
   style->option.border_color = rgba(80, 100, 120, 220);
   style->option.text_background = WINDOW;
   style->option.text_normal = TEXT;
   style->option.text_hover = TEXT_SEL;
   style->option.text_active = TEXT_SEL;
   style->option.text_alignment = MONSTRO_ALIGN_LEFT;
   style->option.padding[0] = 3.0f;
   style->option.padding[1] = 3.0f;
   style->option.touch_padding[0] = 0.0f;
   style->option.touch_padding[1] = 0.0f;
   style->option.border = 1.5f;
   style->option.spacing = 4.0f;

   // Selectable
   style->selectable.normal = styleItemColor(SELECT);
   style->selectable.hover = styleItemColor(SELECT);
   style->selectable.pressed = styleItemColor(SELECT);
   style->selectable.normal_active = styleItemColor(SELECT_ACTIVE);
   style->selectable.hover_active = styleItemColor(SELECT_ACTIVE);
   style->selectable.pressed_active = styleItemColor(SELECT_ACTIVE);
   style->selectable.text_normal = TEXT;
   style->selectable.text_hover = TEXT_SEL;
   style->selectable.text_pressed = TEXT_SEL;
   style->selectable.text_normal_active = TEXT_SEL;
   style->selectable.text_hover_active = TEXT_SEL;
   style->selectable.text_pressed_active = TEXT_SEL;
   style->selectable.text_background = WINDOW;
   style->selectable.text_alignment = MONSTRO_ALIGN_LEFT;
   style->selectable.rounding = 0.0f;
   style->selectable.padding[0] = 4.0f;
   style->selectable.padding[1] = 4.0f;
   style->selectable.image_padding[0] = 0.0f;
   style->selectable.image_padding[1] = 0.0f;
   style->selectable.touch_padding[0] = 0.0f;
   style->selectable.touch_padding[1] = 0.0f;

   // Slider
   style->slider.normal = styleItemHide();
   style->slider.hover = styleItemHide();
   style->slider.active = styleItemHide();
   style->slider.border_color = BORDER;
   style->slider.bar_normal = rgba(30, 37, 47, 255);
   style->slider.bar_hover = rgba(40, 50, 62, 255);
   style->slider.bar_active = rgba(40, 50, 62, 255);
   style->slider.bar_filled = ACCENT;
   style->slider.cursor_normal = styleItemColor(SLIDER_CURSOR);
   style->slider.cursor_hover = styleItemColor(SLIDER_CURSOR_HOVER);
   style->slider.cursor_active = styleItemColor(SLIDER_CURSOR_ACTIVE);
   style->slider.cursor_size[0] = 16.0f;
   style->slider.cursor_size[1] = 16.0f;
   style->slider.padding[0] = 4.0f;
   style->slider.padding[1] = 4.0f;
   style->slider.spacing[0] = 4.0f;
   style->slider.spacing[1] = 4.0f;
   style->slider.show_buttons = false;
   style->slider.bar_height = 8.0f;
   style->slider.rounding = 0.0f;
   style->slider.border = 1.0f;

   // Slider buttons
   style->slider.inc_button.normal = styleItemColor(HEADER);
   style->slider.inc_button.hover = styleItemColor(BUTTON_HOVER);
   style->slider.inc_button.active = styleItemColor(BUTTON_ACTIVE);
   style->slider.inc_button.border_color = BORDER;
   style->slider.inc_button.text_background = HEADER;
   style->slider.inc_button.text_normal = TEXT;
   style->slider.inc_button.text_hover = TEXT_SEL;
   style->slider.inc_button.text_active = TEXT_SEL;
   style->slider.inc_button.text_alignment = MONSTRO_ALIGN_CENTER;
   style->slider.inc_button.padding[0] = 4.0f;
   style->slider.inc_button.padding[1] = 4.0f;
   style->slider.inc_button.border = 1.0f;
   style->slider.inc_button.rounding = 0.0f;
   style->slider.dec_button = style->slider.inc_button;

   // Progress
   style->progress.normal = styleItemColor(SLIDER);
   style->progress.hover = styleItemColor(SLIDER);
   style->progress.active = styleItemColor(SLIDER);
   style->progress.cursor_normal = styleItemColor(ACCENT);
   style->progress.cursor_hover = styleItemColor(ACCENT);
   style->progress.cursor_active = styleItemColor(ACCENT);
   style->progress.border_color = rgba(0, 0, 0, 0);
   style->progress.cursor_border_color = rgba(0, 0, 0, 0);
   style->progress.padding[0] = 4.0f;
   style->progress.padding[1] = 4.0f;
   style->progress.rounding = 0.0f;
   style->progress.border = 0.0f;
   style->progress.cursor_rounding = 0.0f;
   style->progress.cursor_border = 0.0f;

   // Scrollbars
   style->scrollh.normal = styleItemColor(SCROLLBAR);
   style->scrollh.hover = styleItemColor(SCROLLBAR);
   style->scrollh.active = styleItemColor(SCROLLBAR);
   style->scrollh.cursor_normal = styleItemColor(SCROLLBAR_CURSOR);
   style->scrollh.cursor_hover = styleItemColor(SCROLLBAR_CURSOR_HOVER);
   style->scrollh.cursor_active = styleItemColor(SCROLLBAR_CURSOR_ACTIVE);
   style->scrollh.border_color = SCROLLBAR;
   style->scrollh.cursor_border_color = SCROLLBAR;
   style->scrollh.padding[0] = 4.0f;
   style->scrollh.padding[1] = 4.0f;
   style->scrollh.show_buttons = false;
   style->scrollh.border = 0.0f;
   style->scrollh.rounding = 0.0f;
   style->scrollh.border_cursor = 0.0f;
   style->scrollh.rounding_cursor = 0.0f;
   style->scrollv = style->scrollh;

   // Scrollbar buttons
   MonstroStyleButton scroll_btn;
   memset(&scroll_btn, 0, sizeof(scroll_btn));
   scroll_btn.normal = styleItemColor(HEADER);
   scroll_btn.hover = styleItemColor(BUTTON_HOVER);
   scroll_btn.active = styleItemColor(BUTTON_ACTIVE);
   scroll_btn.border_color = BORDER;
   scroll_btn.text_background = HEADER;
   scroll_btn.text_normal = TEXT;
   scroll_btn.text_hover = TEXT_SEL;
   scroll_btn.text_active = TEXT_SEL;
   scroll_btn.text_alignment = MONSTRO_ALIGN_CENTER;
   scroll_btn.padding[0] = 4.0f;
   scroll_btn.padding[1] = 4.0f;
   scroll_btn.border = 1.0f;
   scroll_btn.rounding = 0.0f;
   style->scrollh.inc_button = scroll_btn;
   style->scrollh.dec_button = scroll_btn;
   style->scrollv.inc_button = scroll_btn;
   style->scrollv.dec_button = scroll_btn;

   // Edit
   style->edit.normal = styleItemColor(EDIT);
   style->edit.hover = styleItemColor(EDIT);
   style->edit.active = styleItemColor(EDIT);
   style->edit.border_color = BORDER;
   style->edit.cursor_normal = EDIT_CURSOR;
   style->edit.cursor_hover = EDIT_CURSOR;
   style->edit.cursor_text_normal = EDIT;
   style->edit.cursor_text_hover = EDIT;
   style->edit.text_normal = TEXT_SEL;
   style->edit.text_hover = TEXT_SEL;
   style->edit.text_active = TEXT_SEL;
   style->edit.selected_normal = ACCENT;
   style->edit.selected_hover = ACCENT;
   style->edit.selected_text_normal = TEXT_SEL;
   style->edit.selected_text_hover = TEXT_SEL;
   style->edit.scrollbar_size[0] = 10.0f;
   style->edit.scrollbar_size[1] = 10.0f;
   style->edit.scrollbar = style->scrollv;
   style->edit.padding[0] = 4.0f;
   style->edit.padding[1] = 4.0f;
   style->edit.row_padding = 2.0f;
   style->edit.cursor_size = 4.0f;
   style->edit.border = 1.0f;
   style->edit.rounding = 0.0f;

   // Property
   style->property.normal = styleItemColor(PROPERTY);
   style->property.hover = styleItemColor(PROPERTY);
   style->property.active = styleItemColor(PROPERTY);
   style->property.border_color = BORDER;
   style->property.label_normal = TEXT;
   style->property.label_hover = TEXT;
   style->property.label_active = TEXT;
   style->property.padding[0] = 4.0f;
   style->property.padding[1] = 4.0f;
   style->property.border = 1.0f;
   style->property.rounding = 4.0f;

   // Property buttons
   MonstroStyleButton prop_btn;
   memset(&prop_btn, 0, sizeof(prop_btn));
   prop_btn.normal = styleItemColor(PROPERTY);
   prop_btn.hover = styleItemColor(BUTTON_HOVER);
   prop_btn.active = styleItemColor(BUTTON_ACTIVE);
   prop_btn.border_color = rgba(0, 0, 0, 0);
   prop_btn.text_background = PROPERTY;
   prop_btn.text_normal = TEXT;
   prop_btn.text_hover = TEXT_SEL;
   prop_btn.text_active = TEXT_SEL;
   prop_btn.text_alignment = MONSTRO_ALIGN_CENTER;
   prop_btn.padding[0] = 4.0f;
   prop_btn.padding[1] = 4.0f;
   prop_btn.border = 0.0f;
   prop_btn.rounding = 0.0f;
   style->property.dec_button = prop_btn;
   style->property.inc_button = prop_btn;

   // Property edit
   style->property.edit.normal = styleItemColor(PROPERTY);
   style->property.edit.hover = styleItemColor(PROPERTY);
   style->property.edit.active = styleItemColor(PROPERTY);
   style->property.edit.border_color = rgba(0, 0, 0, 0);
   style->property.edit.cursor_normal = EDIT_CURSOR;
   style->property.edit.cursor_hover = EDIT_CURSOR;
   style->property.edit.cursor_text_normal = EDIT;
   style->property.edit.cursor_text_hover = EDIT;
   style->property.edit.text_normal = TEXT;
   style->property.edit.text_hover = TEXT_SEL;
   style->property.edit.text_active = TEXT_SEL;
   style->property.edit.selected_normal = ACCENT;
   style->property.edit.selected_hover = ACCENT;
   style->property.edit.selected_text_normal = TEXT_SEL;
   style->property.edit.selected_text_hover = TEXT_SEL;
   style->property.edit.padding[0] = 4.0f;
   style->property.edit.padding[1] = 4.0f;
   style->property.edit.cursor_size = 8.0f;
   style->property.edit.border = 0.0f;
   style->property.edit.rounding = 0.0f;

   // Chart
   style->chart.background = styleItemColor(CHART);
   style->chart.border_color = BORDER;
   style->chart.selected_color = CHART_COLOR_HL;
   style->chart.color = CHART_COLOR;
   style->chart.padding[0] = 4.0f;
   style->chart.padding[1] = 4.0f;
   style->chart.border = 0.0f;
   style->chart.rounding = 0.0f;

   // Combo
   style->combo.normal = styleItemColor(COMBO);
   style->combo.hover = styleItemColor(COMBO);
   style->combo.active = styleItemColor(COMBO);
   style->combo.border_color = BORDER;
   style->combo.label_normal = TEXT;
   style->combo.label_hover = TEXT_SEL;
   style->combo.label_active = TEXT_SEL;
   style->combo.symbol_normal = TEXT;
   style->combo.symbol_hover = TEXT_SEL;
   style->combo.symbol_active = TEXT_SEL;
   style->combo.content_padding[0] = 2.0f;
   style->combo.content_padding[1] = 2.0f;
   style->combo.button_padding[0] = 0.0f;
   style->combo.button_padding[1] = 4.0f;
   style->combo.spacing[0] = 4.0f;
   style->combo.spacing[1] = 0.0f;
   style->combo.border = 1.0f;
   style->combo.rounding = 0.0f;

   // Combo button
   style->combo.button.normal = styleItemColor(COMBO);
   style->combo.button.hover = styleItemColor(BUTTON_HOVER);
   style->combo.button.active = styleItemColor(BUTTON_ACTIVE);
   style->combo.button.border_color = rgba(0, 0, 0, 0);
   style->combo.button.text_background = COMBO;
   style->combo.button.text_normal = TEXT;
   style->combo.button.text_hover = TEXT_SEL;
   style->combo.button.text_active = TEXT_SEL;
   style->combo.button.text_alignment = MONSTRO_ALIGN_CENTER;
   style->combo.button.padding[0] = 4.0f;
   style->combo.button.padding[1] = 4.0f;
   style->combo.button.border = 0.0f;
   style->combo.button.rounding = 0.0f;

   // Tab
   style->tab.background = styleItemColor(TAB_HEADER);
   style->tab.border_color = rgba(0, 0, 0, 0);
   style->tab.text = rgba(152, 152, 152, 255);
   style->tab.padding[0] = 4.0f;
   style->tab.padding[1] = 4.0f;
   style->tab.spacing[0] = 4.0f;
   style->tab.spacing[1] = 4.0f;
   style->tab.indent = 10.0f;
   style->tab.border = 1.0f;
   style->tab.rounding = 0.0f;

   // Tab buttons
   MonstroStyleButton tab_btn;
   memset(&tab_btn, 0, sizeof(tab_btn));
   tab_btn.normal = styleItemColor(TAB_HEADER);
   tab_btn.hover = styleItemColor(rgba(50, 70, 94, 228));
   tab_btn.active = styleItemColor(SELECT_ACTIVE);
   tab_btn.border_color = rgba(0, 0, 0, 0);
   tab_btn.text_background = TAB_HEADER;
   tab_btn.text_normal = rgba(152, 152, 152, 255);
   tab_btn.text_hover = TEXT_SEL;
   tab_btn.text_active = TEXT_SEL;
   tab_btn.text_alignment = MONSTRO_ALIGN_CENTER;
   tab_btn.padding[0] = 4.0f;
   tab_btn.padding[1] = 4.0f;
   tab_btn.border = 0.0f;
   tab_btn.rounding = 0.0f;
   style->tab.tab_minimize_button = tab_btn;
   style->tab.tab_maximize_button = tab_btn;

   // Node buttons
   MonstroStyleButton node_btn;
   memset(&node_btn, 0, sizeof(node_btn));
   node_btn.normal = styleItemColor(WINDOW);
   node_btn.hover = styleItemColor(WINDOW);
   node_btn.active = styleItemColor(WINDOW);
   node_btn.border_color = rgba(0, 0, 0, 0);
   node_btn.text_background = TAB_HEADER;
   node_btn.text_normal = TEXT;
   node_btn.text_hover = TEXT;
   node_btn.text_active = TEXT;
   node_btn.text_alignment = MONSTRO_ALIGN_CENTER;
   node_btn.padding[0] = 4.0f;
   node_btn.padding[1] = 4.0f;
   node_btn.border = 0.0f;
   node_btn.rounding = 0.0f;
   style->tab.node_minimize_button = node_btn;
   style->tab.node_maximize_button = node_btn;

   // Separator
   style->separator.color = SEPARATOR_COL;
   style->separator.thickness = 1.0f;
   style->separator.padding[0] = 0.0f;
   style->separator.padding[1] = 4.0f;
   style->separator.text_padding[0] = 8.0f;
   style->separator.text_padding[1] = 0.0f;

   // Tooltip
   style->tooltip.background = styleItemColor(TOOLTIP_BG);
   style->tooltip.border_color = TOOLTIP_BORDER;
   style->tooltip.text = TEXT;
   style->tooltip.border = 1.0f;
   style->tooltip.rounding = 4.0f;
   style->tooltip.padding[0] = 4.0f;
   style->tooltip.padding[1] = 4.0f;

   // Menu
   style->menu.normal = styleItemColor(MENU_BG);
   style->menu.hover = styleItemColor(MENU_ITEM_SEL);
   style->menu.active = styleItemColor(ACCENT);
   style->menu.border_color = rgba(73, 96, 134, 102);
   style->menu.text_normal = TEXT;
   style->menu.text_hover = TEXT_SEL;
   style->menu.text_active = TEXT_SEL;
   style->menu.padding[0] = 4.0f;
   style->menu.padding[1] = 4.0f;
   style->menu.spacing[0] = 4.0f;
   style->menu.spacing[1] = 4.0f;
   style->menu.border = 0.0f;
   style->menu.rounding = 0.0f;

   // Tree
   style->tree.normal = styleItemColor(WINDOW);
   style->tree.hover = styleItemColor(BUTTON_HOVER);
   style->tree.active = styleItemColor(BUTTON_ACTIVE);
   style->tree.selected = styleItemColor(SELECT_ACTIVE);
   style->tree.text_normal = TEXT;
   style->tree.text_hover = TEXT_SEL;
   style->tree.text_active = TEXT_SEL;
   style->tree.text_selected = TEXT_SEL;
   style->tree.sym_closed = 0;
   style->tree.sym_open = 0;
   style->tree.indent = 10.0f;
   style->tree.padding[0] = 4.0f;
   style->tree.padding[1] = 4.0f;
   style->tree.spacing[0] = 4.0f;
   style->tree.spacing[1] = 4.0f;
   style->tree.rounding = 0.0f;

   // Table
   style->table.background = styleItemColor(WINDOW);
   style->table.border_color = BORDER;
   style->table.row_color_even = WINDOW;
   style->table.row_color_odd = rgba(37, 49, 66, 60);
   style->table.header_normal = styleItemColor(HEADER);
   style->table.header_hover = styleItemColor(BUTTON_HOVER);
   style->table.header_active = styleItemColor(BUTTON_ACTIVE);
   style->table.header_text = TEXT;
   style->table.border = 1.0f;
   style->table.cell_padding[0] = 4.0f;
   style->table.cell_padding[1] = 4.0f;
   style->table.row_height = 24.0f;
   style->table.header_height = 28.0f;
   style->table.rounding = 0.0f;

   // Knob
   style->knob.normal = styleItemColor(SLIDER);
   style->knob.hover = styleItemColor(SLIDER_CURSOR_HOVER);
   style->knob.active = styleItemColor(SLIDER_CURSOR_ACTIVE);
   style->knob.border_color = BORDER;
   style->knob.track_normal = rgba(30, 47, 65, 255);
   style->knob.track_filled = ACCENT;
   style->knob.indicator_normal = styleItemColor(TEXT);
   style->knob.indicator_hover = styleItemColor(TEXT);
   style->knob.indicator_active = styleItemColor(TEXT);
   style->knob.border = 0.0f;
   style->knob.diameter = 32.0f;
   style->knob.indicator_length = 8.0f;
   style->knob.indicator_width = 2.0f;
   style->knob.track_width = 3.0f;
   style->knob.padding[0] = 4.0f;
   style->knob.padding[1] = 4.0f;

   // Window header
   style->window.header.normal = styleItemColor(HEADER);
   style->window.header.hover = styleItemColor(HEADER);
   style->window.header.active = styleItemColor(rgba(42, 52, 66, 255));
   style->window.header.label_normal = rgba(238, 238, 238, 255);
   style->window.header.label_hover = TEXT_SEL;
   style->window.header.label_active = TEXT_SEL;
   style->window.header.padding[0] = 4.0f;
   style->window.header.padding[1] = 4.0f;
   style->window.header.label_padding[0] = 4.0f;
   style->window.header.label_padding[1] = 4.0f;
   style->window.header.spacing[0] = 0.0f;
   style->window.header.spacing[1] = 0.0f;

   // Window header close button
   MonstroStyleButton hdr_btn;
   memset(&hdr_btn, 0, sizeof(hdr_btn));
   hdr_btn.normal = styleItemColor(HEADER);
   hdr_btn.hover = styleItemColor(BUTTON_HOVER);
   hdr_btn.active = styleItemColor(BUTTON_ACTIVE);
   hdr_btn.border_color = rgba(0, 0, 0, 0);
   hdr_btn.text_background = HEADER;
   hdr_btn.text_normal = TEXT;
   hdr_btn.text_hover = TEXT_SEL;
   hdr_btn.text_active = TEXT_SEL;
   hdr_btn.text_alignment = MONSTRO_ALIGN_CENTER;
   hdr_btn.padding[0] = 4.0f;
   hdr_btn.padding[1] = 4.0f;
   hdr_btn.border = 0.0f;
   hdr_btn.rounding = 0.0f;
   style->window.header.close_button = hdr_btn;
   style->window.header.minimize_button = hdr_btn;

   // Window
   style->window.background = WINDOW;
   style->window.fixed_background = styleItemColor(WINDOW);
   style->window.border_style = styleItemColor(rgba(18, 22, 27, 255));
   style->window.border_color = rgba(18, 22, 27, 255);
   style->window.popup_border_color = rgba(73, 96, 134, 102);
   style->window.combo_border_color = BORDER;
   style->window.contextual_border_color = rgba(73, 96, 134, 102);
   style->window.menu_border_color = rgba(73, 96, 134, 102);
   style->window.group_background = styleItemColor(rgba(35, 45, 55, 255));
   style->window.group_border_color = rgba(55, 74, 92, 153);
   style->window.tooltip_border_color = TOOLTIP_BORDER;
   style->window.scaler = styleItemColor(TEXT);
   style->window.modal_dim_color = rgba(0, 0, 0, 128);

   style->window.border = 1.0f;
   style->window.combo_border = 2.0f;
   style->window.contextual_border = 2.0f;
   style->window.menu_border = 2.0f;
   style->window.group_border = 2.0f;
   style->window.tooltip_border = 2.0f;
   style->window.popup_border = 2.0f;

   style->window.rounding = 4.0f;
   style->window.spacing[0] = 4.0f;
   style->window.spacing[1] = 4.0f;
   style->window.scrollbar_size[0] = 10.0f;
   style->window.scrollbar_size[1] = 10.0f;
   style->window.min_size[0] = 64.0f;
   style->window.min_size[1] = 64.0f;

   style->window.padding[0] = 4.0f;
   style->window.padding[1] = 4.0f;
   style->window.group_padding[0] = 4.0f;
   style->window.group_padding[1] = 4.0f;
   style->window.popup_padding[0] = 4.0f;
   style->window.popup_padding[1] = 4.0f;
   style->window.combo_padding[0] = 4.0f;
   style->window.combo_padding[1] = 4.0f;
   style->window.contextual_padding[0] = 4.0f;
   style->window.contextual_padding[1] = 4.0f;
   style->window.menu_padding[0] = 4.0f;
   style->window.menu_padding[1] = 4.0f;
   style->window.tooltip_padding[0] = 4.0f;
   style->window.tooltip_padding[1] = 4.0f;
}

// The colours of a built-in theme besides the default. Every colour a widget draws comes from one of these.
struct Palette
{
   MonstroColor text, text_strong, text_dim;
   MonstroColor window, header, title, border;
   MonstroColor button, button_hover, button_active;
   MonstroColor field, toggle, toggle_hover, accent;
   MonstroColor slider_bar, slider_bar_hover, slider_cursor, slider_cursor_hover;
   MonstroColor scroll_track, scroll_thumb, scroll_thumb_hover;
   MonstroColor menu, row_hover, row_selected;
   MonstroColor tab_bar, tab, tab_hover, tab_selected;
   MonstroColor tooltip, group, group_border, table_odd_row, separator;
   MonstroColor chart, chart_highlight, edit_cursor, selection, selection_text, modal_dim;
};

static MonstroStyleButton paletteButton(const MonstroStyleButton &base, const Palette &p, MonstroColor normal,
                                        MonstroColor border)
{
   MonstroStyleButton b = base;
   b.normal = styleItemColor(normal);
   b.hover = styleItemColor(p.button_hover);
   b.active = styleItemColor(p.button_active);
   b.border_color = border;
   b.text_background = normal;
   b.text_normal = p.text;
   b.text_hover = p.text_strong;
   b.text_active = p.text_strong;
   return b;
}

// The default theme's sizes and shapes in the palette's colours.
static void style_from_palette(MonstroStyle *style, const Palette &p)
{
   monstro_style_set_default(style);
   MonstroColor none = {0.0f, 0.0f, 0.0f, 0.0f};

   style->text.color = p.text;
   style->text.background = p.window;

   style->button = paletteButton(style->button, p, p.button, p.border);

   MonstroStyleButton *contextual = &style->contextual_button;
   contextual->hover = styleItemColor(p.row_hover);
   contextual->active = styleItemColor(p.row_selected);
   contextual->text_background = p.menu;
   contextual->text_normal = p.text;
   contextual->text_hover = p.text_strong;
   contextual->text_active = p.text_strong;

   MonstroStyleButton *menu_button = &style->menu_button;
   menu_button->normal = styleItemColor(p.header);
   menu_button->hover = styleItemColor(p.row_hover);
   menu_button->active = styleItemColor(p.row_selected);
   menu_button->text_background = p.header;
   menu_button->text_normal = p.text;
   menu_button->text_hover = p.text_strong;
   menu_button->text_active = p.text_strong;

   MonstroStyleToggle *toggles[] = {&style->checkbox, &style->option};
   for (MonstroStyleToggle *t : toggles)
   {
      t->normal = styleItemColor(p.toggle);
      t->hover = styleItemColor(p.toggle_hover);
      t->active = styleItemColor(t == &style->option ? p.toggle : p.toggle_hover);
      t->cursor_normal = styleItemColor(p.accent);
      t->cursor_hover = styleItemColor(p.accent);
      t->border_color = p.border;
      t->text_background = p.window;
      t->text_normal = p.text;
      t->text_hover = p.text_strong;
      t->text_active = p.text_strong;
   }

   MonstroStyleSelectable *sel = &style->selectable;
   sel->normal = styleItemColor(none);
   sel->hover = styleItemColor(p.row_hover);
   sel->pressed = styleItemColor(p.row_selected);
   sel->normal_active = styleItemColor(p.row_selected);
   sel->hover_active = styleItemColor(p.row_selected);
   sel->pressed_active = styleItemColor(p.row_selected);
   sel->text_normal = p.text;
   sel->text_hover = p.text_strong;
   sel->text_pressed = p.text_strong;
   sel->text_normal_active = p.text_strong;
   sel->text_hover_active = p.text_strong;
   sel->text_pressed_active = p.text_strong;
   sel->text_background = p.window;

   MonstroStyleSlider *slider = &style->slider;
   slider->border_color = p.border;
   slider->bar_normal = p.slider_bar;
   slider->bar_hover = p.slider_bar_hover;
   slider->bar_active = p.slider_bar_hover;
   slider->bar_filled = p.accent;
   slider->cursor_normal = styleItemColor(p.slider_cursor);
   slider->cursor_hover = styleItemColor(p.slider_cursor_hover);
   slider->cursor_active = styleItemColor(p.accent);
   slider->inc_button = paletteButton(slider->inc_button, p, p.header, p.border);
   slider->dec_button = slider->inc_button;

   MonstroStyleProgress *progress = &style->progress;
   progress->normal = styleItemColor(p.slider_bar);
   progress->hover = styleItemColor(p.slider_bar);
   progress->active = styleItemColor(p.slider_bar);
   progress->cursor_normal = styleItemColor(p.accent);
   progress->cursor_hover = styleItemColor(p.accent);
   progress->cursor_active = styleItemColor(p.accent);

   MonstroStyleScrollbar *scroll = &style->scrollh;
   scroll->normal = styleItemColor(p.scroll_track);
   scroll->hover = styleItemColor(p.scroll_track);
   scroll->active = styleItemColor(p.scroll_track);
   scroll->cursor_normal = styleItemColor(p.scroll_thumb);
   scroll->cursor_hover = styleItemColor(p.scroll_thumb_hover);
   scroll->cursor_active = styleItemColor(p.accent);
   scroll->border_color = p.scroll_track;
   scroll->cursor_border_color = p.scroll_track;
   scroll->inc_button = paletteButton(scroll->inc_button, p, p.header, p.border);
   scroll->dec_button = scroll->inc_button;
   style->scrollv = style->scrollh;

   MonstroStyleEdit *edits[] = {&style->edit, &style->property.edit};
   for (MonstroStyleEdit *e : edits)
   {
      e->normal = styleItemColor(p.field);
      e->hover = styleItemColor(p.field);
      e->active = styleItemColor(p.field);
      e->border_color = e == &style->edit ? p.border : none;
      e->cursor_normal = p.edit_cursor;
      e->cursor_hover = p.edit_cursor;
      e->cursor_text_normal = p.field;
      e->cursor_text_hover = p.field;
      e->text_normal = p.text;
      e->text_hover = p.text_strong;
      e->text_active = p.text_strong;
      e->selected_normal = p.selection;
      e->selected_hover = p.selection;
      e->selected_text_normal = p.selection_text;
      e->selected_text_hover = p.selection_text;
   }
   style->edit.scrollbar = style->scrollv;

   MonstroStyleProperty *property = &style->property;
   property->normal = styleItemColor(p.field);
   property->hover = styleItemColor(p.field);
   property->active = styleItemColor(p.field);
   property->border_color = p.border;
   property->label_normal = p.text;
   property->label_hover = p.text;
   property->label_active = p.text;
   property->dec_button = paletteButton(property->dec_button, p, p.field, none);
   property->inc_button = property->dec_button;

   style->chart.background = styleItemColor(p.chart);
   style->chart.border_color = p.border;
   style->chart.selected_color = p.chart_highlight;
   style->chart.color = p.accent;

   MonstroStyleCombo *combo = &style->combo;
   combo->normal = styleItemColor(p.button);
   combo->hover = styleItemColor(p.button_hover);
   combo->active = styleItemColor(p.button_hover);
   combo->border_color = p.border;
   combo->label_normal = p.text;
   combo->label_hover = p.text_strong;
   combo->label_active = p.text_strong;
   combo->symbol_normal = p.text;
   combo->symbol_hover = p.text_strong;
   combo->symbol_active = p.text_strong;
   combo->button = paletteButton(combo->button, p, p.button, none);

   MonstroStyleTab *tab = &style->tab;
   tab->background = styleItemColor(p.tab_bar);
   tab->border_color = p.accent;
   tab->text = p.text_dim;
   MonstroStyleButton tab_button = paletteButton(tab->tab_maximize_button, p, p.tab, none);
   tab_button.hover = styleItemColor(p.tab_hover);
   tab_button.active = styleItemColor(p.tab_selected);
   tab_button.text_background = p.tab_bar;
   tab_button.text_normal = p.text_dim;
   tab->tab_maximize_button = tab_button;
   tab->tab_minimize_button = tab_button;
   MonstroStyleButton node_button = tab->node_maximize_button;
   node_button.normal = styleItemColor(p.window);
   node_button.hover = styleItemColor(p.window);
   node_button.active = styleItemColor(p.window);
   node_button.text_background = p.tab_bar;
   node_button.text_normal = p.text;
   node_button.text_hover = p.text;
   node_button.text_active = p.text;
   tab->node_maximize_button = node_button;
   tab->node_minimize_button = node_button;

   style->separator.color = p.separator;

   style->tooltip.background = styleItemColor(p.tooltip);
   style->tooltip.border_color = p.border;
   style->tooltip.text = p.text;

   MonstroStyleMenu *menu = &style->menu;
   menu->normal = styleItemColor(p.menu);
   menu->hover = styleItemColor(p.row_hover);
   menu->active = styleItemColor(p.row_selected);
   menu->border_color = p.border;
   menu->text_normal = p.text;
   menu->text_hover = p.text_strong;
   menu->text_active = p.text_strong;

   // A collapsing header is a bar in the tree's active colour.
   MonstroStyleTree *tree = &style->tree;
   tree->normal = styleItemColor(p.window);
   tree->hover = styleItemColor(p.row_hover);
   tree->active = styleItemColor(p.button);
   tree->selected = styleItemColor(p.row_selected);
   tree->text_normal = p.text;
   tree->text_hover = p.text_strong;
   tree->text_active = p.text_strong;
   tree->text_selected = p.text_strong;

   MonstroStyleTable *table = &style->table;
   table->background = styleItemColor(p.window);
   table->border_color = p.border;
   table->row_color_even = p.window;
   table->row_color_odd = p.table_odd_row;
   table->header_normal = styleItemColor(p.header);
   table->header_hover = styleItemColor(p.button_hover);
   table->header_active = styleItemColor(p.button_active);
   table->header_text = p.text;

   MonstroStyleKnob *knob = &style->knob;
   knob->normal = styleItemColor(p.button);
   knob->hover = styleItemColor(p.button_hover);
   knob->active = styleItemColor(p.button_active);
   knob->border_color = p.border;
   knob->track_normal = p.slider_bar;
   knob->track_filled = p.accent;
   knob->indicator_normal = styleItemColor(p.text);
   knob->indicator_hover = styleItemColor(p.text_strong);
   knob->indicator_active = styleItemColor(p.text_strong);

   MonstroStyleWindowHeader *header = &style->window.header;
   header->normal = styleItemColor(p.title);
   header->hover = styleItemColor(p.button_hover);
   header->active = styleItemColor(p.title);
   header->label_normal = p.text;
   header->label_hover = p.text_strong;
   header->label_active = p.text_strong;
   header->close_button = paletteButton(header->close_button, p, p.title, none);
   header->minimize_button = header->close_button;

   MonstroStyleWindow *window = &style->window;
   window->background = p.window;
   window->fixed_background = styleItemColor(p.window);
   window->border_style = styleItemColor(p.border);
   window->border_color = p.border;
   window->popup_border_color = p.border;
   window->combo_border_color = p.border;
   window->contextual_border_color = p.border;
   window->menu_border_color = p.header; // the menu bar's background
   window->group_background = styleItemColor(p.group);
   window->group_border_color = p.group_border;
   window->tooltip_border_color = p.border;
   window->scaler = styleItemColor(p.text_dim);
   window->modal_dim_color = p.modal_dim;
}

// Neutral charcoal with a teal accent, where the default is blue-grey.
void monstro_style_set_dark(MonstroStyle *style)
{
   if (!style)
      return;
   Palette p;
   p.text = {0.86f, 0.86f, 0.86f, 1.0f};
   p.text_strong = {1.0f, 1.0f, 1.0f, 1.0f};
   p.text_dim = {0.60f, 0.60f, 0.60f, 1.0f};
   p.window = {0.13f, 0.13f, 0.14f, 1.0f};
   p.header = {0.09f, 0.09f, 0.10f, 1.0f};
   p.title = {0.18f, 0.18f, 0.19f, 1.0f};
   p.border = {0.05f, 0.05f, 0.05f, 1.0f};
   p.button = {0.22f, 0.22f, 0.23f, 1.0f};
   p.button_hover = {0.30f, 0.30f, 0.32f, 1.0f};
   p.button_active = {0.10f, 0.42f, 0.38f, 1.0f};
   p.field = {0.07f, 0.07f, 0.08f, 1.0f};
   p.toggle = {0.07f, 0.07f, 0.08f, 1.0f};
   p.toggle_hover = {0.20f, 0.20f, 0.21f, 1.0f};
   p.accent = {0.16f, 0.62f, 0.56f, 1.0f};
   p.slider_bar = {0.22f, 0.22f, 0.23f, 1.0f};
   p.slider_bar_hover = {0.27f, 0.27f, 0.29f, 1.0f};
   p.slider_cursor = {0.55f, 0.55f, 0.57f, 1.0f};
   p.slider_cursor_hover = {0.70f, 0.70f, 0.72f, 1.0f};
   p.scroll_track = {0.09f, 0.09f, 0.10f, 1.0f};
   p.scroll_thumb = {0.30f, 0.30f, 0.32f, 1.0f};
   p.scroll_thumb_hover = {0.40f, 0.40f, 0.42f, 1.0f};
   p.menu = {0.10f, 0.10f, 0.11f, 1.0f};
   p.row_hover = {0.24f, 0.24f, 0.26f, 1.0f};
   p.row_selected = {0.10f, 0.38f, 0.35f, 1.0f};
   p.tab_bar = {0.08f, 0.08f, 0.09f, 1.0f};
   p.tab = {0.16f, 0.16f, 0.17f, 1.0f};
   p.tab_hover = {0.26f, 0.26f, 0.28f, 1.0f};
   p.tab_selected = {0.13f, 0.13f, 0.14f, 1.0f};
   p.tooltip = {0.20f, 0.20f, 0.21f, 1.0f};
   p.group = {0.17f, 0.17f, 0.18f, 1.0f};
   p.group_border = {0.26f, 0.26f, 0.28f, 1.0f};
   p.table_odd_row = {1.0f, 1.0f, 1.0f, 0.04f};
   p.separator = {0.30f, 0.30f, 0.32f, 1.0f};
   p.chart = {0.08f, 0.08f, 0.09f, 1.0f};
   p.chart_highlight = {1.0f, 0.68f, 0.14f, 1.0f};
   p.edit_cursor = {0.40f, 0.85f, 0.78f, 1.0f};
   p.selection = {0.10f, 0.42f, 0.38f, 1.0f};
   p.selection_text = {1.0f, 1.0f, 1.0f, 1.0f};
   p.modal_dim = {0.0f, 0.0f, 0.0f, 0.5f};
   style_from_palette(style, p);
}

// Light grey surfaces, white fields and dark text, with a blue accent.
void monstro_style_set_light(MonstroStyle *style)
{
   if (!style)
      return;
   Palette p;
   p.text = {0.12f, 0.12f, 0.14f, 1.0f};
   p.text_strong = {0.0f, 0.0f, 0.0f, 1.0f};
   p.text_dim = {0.38f, 0.38f, 0.40f, 1.0f};
   p.window = {0.95f, 0.95f, 0.96f, 1.0f};
   p.header = {0.88f, 0.88f, 0.90f, 1.0f};
   p.title = {0.82f, 0.84f, 0.88f, 1.0f};
   p.border = {0.68f, 0.69f, 0.72f, 1.0f};
   p.button = {0.87f, 0.88f, 0.90f, 1.0f};
   p.button_hover = {0.80f, 0.85f, 0.92f, 1.0f};
   p.button_active = {0.70f, 0.79f, 0.92f, 1.0f};
   p.field = {1.0f, 1.0f, 1.0f, 1.0f};
   p.toggle = {0.84f, 0.85f, 0.88f, 1.0f}; // a checkbox has no outline: its box stands out from the window
   p.toggle_hover = {0.78f, 0.83f, 0.92f, 1.0f};
   p.accent = {0.16f, 0.44f, 0.80f, 1.0f};
   p.slider_bar = {0.82f, 0.83f, 0.86f, 1.0f};
   p.slider_bar_hover = {0.77f, 0.78f, 0.82f, 1.0f};
   p.slider_cursor = {0.52f, 0.55f, 0.61f, 1.0f};
   p.slider_cursor_hover = {0.42f, 0.45f, 0.52f, 1.0f};
   p.scroll_track = {0.90f, 0.90f, 0.92f, 1.0f};
   p.scroll_thumb = {0.68f, 0.69f, 0.72f, 1.0f};
   p.scroll_thumb_hover = {0.58f, 0.59f, 0.63f, 1.0f};
   p.menu = {0.97f, 0.97f, 0.98f, 1.0f};
   p.row_hover = {0.85f, 0.89f, 0.96f, 1.0f};
   p.row_selected = {0.74f, 0.83f, 0.95f, 1.0f};
   p.tab_bar = {0.86f, 0.86f, 0.88f, 1.0f};
   p.tab = {0.86f, 0.86f, 0.88f, 1.0f};
   p.tab_hover = {0.92f, 0.92f, 0.94f, 1.0f};
   p.tab_selected = {0.98f, 0.98f, 0.99f, 1.0f};
   p.tooltip = {1.0f, 0.99f, 0.92f, 1.0f};
   p.group = {0.90f, 0.90f, 0.92f, 1.0f};
   p.group_border = {0.78f, 0.79f, 0.82f, 1.0f};
   p.table_odd_row = {0.0f, 0.0f, 0.0f, 0.04f};
   p.separator = {0.75f, 0.76f, 0.79f, 1.0f};
   p.chart = {1.0f, 1.0f, 1.0f, 1.0f};
   p.chart_highlight = {0.90f, 0.50f, 0.10f, 1.0f};
   p.edit_cursor = {0.16f, 0.44f, 0.80f, 1.0f};
   p.selection = {0.65f, 0.78f, 0.95f, 1.0f};
   p.selection_text = {0.0f, 0.0f, 0.0f, 1.0f};
   p.modal_dim = {0.0f, 0.0f, 0.0f, 0.3f};
   style_from_palette(style, p);
}
