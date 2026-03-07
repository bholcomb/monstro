#include "../include/monstro/style.h"
#include <string.h>

static inline MonstroColor rgba(int r, int g, int b, int a) {
    return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
}

static inline MonstroStyleItem styleItemColor(MonstroColor c) {
    MonstroStyleItem item;
    item.type = MONSTRO_STYLE_ITEM_COLOR;
    item.data.color = c;
    return item;
}

static inline MonstroStyleItem styleItemHide() {
    return styleItemColor(rgba(0, 0, 0, 0));
}

void monstro_style_set_default(MonstroStyle* style) {
    if (!style) return;
    
    memset(style, 0, sizeof(MonstroStyle));

    MonstroColor TEXT           = rgba(175, 175, 175, 255);
    MonstroColor BACKGROUND     = rgba(175, 175, 175, 255);
    MonstroColor WINDOW         = rgba( 45,  45,  45, 255);
    MonstroColor HEADER         = rgba( 40,  40,  40, 255);
    MonstroColor BORDER         = rgba( 65,  65,  65, 255);
    MonstroColor BUTTON         = rgba( 50,  50,  50, 255);
    MonstroColor BUTTON_HOVER   = rgba( 70,  70,  70, 255);
    MonstroColor BUTTON_ACTIVE  = rgba( 35,  35,  35, 255);
    MonstroColor TOGGLE         = rgba(100, 100, 100, 255);
    MonstroColor TOGGLE_HOVER   = rgba(120, 120, 120, 255);
    MonstroColor TOGGLE_CURSOR  = rgba( 45,  45,  45, 255);
    MonstroColor SELECT         = rgba( 45,  45,  45, 255);
    MonstroColor SELECT_ACTIVE  = rgba( 35,  35,  35, 255);
    MonstroColor SLIDER         = rgba( 38,  38,  38, 255);
    MonstroColor SLIDER_CURSOR  = rgba(100, 100, 100, 255);
    MonstroColor SLIDER_CURSOR_HOVER  = rgba(120, 120, 120, 255);
    MonstroColor SLIDER_CURSOR_ACTIVE = rgba(150, 150, 150, 255);
    MonstroColor PROPERTY       = rgba( 38,  38,  38, 255);
    MonstroColor EDIT           = rgba( 38,  38,  38, 255);
    MonstroColor COMBO          = rgba( 45,  45,  45, 255);
    MonstroColor CHART          = rgba(120, 120, 120, 255);
    MonstroColor CHART_COLOR    = rgba( 45,  45,  45, 255);
    MonstroColor CHART_COLOR_HL = rgba(255,   0,   0, 255);
    MonstroColor SCROLLBAR      = rgba( 40,  40,  40, 255);
    MonstroColor SCROLLBAR_CURSOR        = rgba(100, 100, 100, 255);
    MonstroColor SCROLLBAR_CURSOR_HOVER  = rgba(120, 120, 120, 255);
    MonstroColor SCROLLBAR_CURSOR_ACTIVE = rgba(150, 150, 150, 255);
    MonstroColor TAB_HEADER     = rgba( 40,  40,  40, 255);

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
    style->button.text_hover = TEXT;
    style->button.text_active = TEXT;
    style->button.text_alignment = MONSTRO_ALIGN_CENTER;
    style->button.border = 1.0f;
    style->button.rounding = 4.0f;
    style->button.padding[0] = 4.0f;
    style->button.padding[1] = 4.0f;
    style->button.image_padding[0] = 0.0f;
    style->button.image_padding[1] = 0.0f;
    style->button.touch_padding[0] = 0.0f;
    style->button.touch_padding[1] = 0.0f;

    // Contextual Button
    style->contextual_button.normal = styleItemColor(WINDOW);
    style->contextual_button.hover = styleItemColor(BUTTON_HOVER);
    style->contextual_button.active = styleItemColor(BUTTON_ACTIVE);
    style->contextual_button.border_color = WINDOW;
    style->contextual_button.text_background = WINDOW;
    style->contextual_button.text_normal = TEXT;
    style->contextual_button.text_hover = TEXT;
    style->contextual_button.text_active = TEXT;
    style->contextual_button.text_alignment = MONSTRO_ALIGN_CENTER;
    style->contextual_button.border = 0.0f;
    style->contextual_button.rounding = 0.0f;
    style->contextual_button.padding[0] = 2.0f;
    style->contextual_button.padding[1] = 2.0f;
    style->contextual_button.touch_padding[0] = 0.0f;
    style->contextual_button.touch_padding[1] = 0.0f;

    // Menu Button
    style->menu_button.normal = styleItemColor(WINDOW);
    style->menu_button.hover = styleItemColor(WINDOW);
    style->menu_button.active = styleItemColor(WINDOW);
    style->menu_button.border_color = WINDOW;
    style->menu_button.text_background = WINDOW;
    style->menu_button.text_normal = TEXT;
    style->menu_button.text_hover = TEXT;
    style->menu_button.text_active = TEXT;
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
    style->checkbox.border_color = rgba(0, 0, 0, 0);
    style->checkbox.text_background = WINDOW;
    style->checkbox.text_normal = TEXT;
    style->checkbox.text_hover = TEXT;
    style->checkbox.text_active = TEXT;
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
    style->option.active = styleItemColor(TOGGLE_HOVER);
    style->option.cursor_normal = styleItemColor(TOGGLE_CURSOR);
    style->option.cursor_hover = styleItemColor(TOGGLE_CURSOR);
    style->option.border_color = rgba(0, 0, 0, 0);
    style->option.text_background = WINDOW;
    style->option.text_normal = TEXT;
    style->option.text_hover = TEXT;
    style->option.text_active = TEXT;
    style->option.text_alignment = MONSTRO_ALIGN_LEFT;
    style->option.padding[0] = 3.0f;
    style->option.padding[1] = 3.0f;
    style->option.touch_padding[0] = 0.0f;
    style->option.touch_padding[1] = 0.0f;
    style->option.border = 0.0f;
    style->option.spacing = 4.0f;

    // Selectable
    style->selectable.normal = styleItemColor(SELECT);
    style->selectable.hover = styleItemColor(SELECT);
    style->selectable.pressed = styleItemColor(SELECT);
    style->selectable.normal_active = styleItemColor(SELECT_ACTIVE);
    style->selectable.hover_active = styleItemColor(SELECT_ACTIVE);
    style->selectable.pressed_active = styleItemColor(SELECT_ACTIVE);
    style->selectable.text_normal = TEXT;
    style->selectable.text_hover = TEXT;
    style->selectable.text_pressed = TEXT;
    style->selectable.text_normal_active = TEXT;
    style->selectable.text_hover_active = TEXT;
    style->selectable.text_pressed_active = TEXT;
    style->selectable.text_background = BACKGROUND;
    style->selectable.text_alignment = MONSTRO_ALIGN_CENTER;
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
    style->slider.bar_normal = SLIDER;
    style->slider.bar_hover = SLIDER;
    style->slider.bar_active = SLIDER;
    style->slider.bar_filled = SLIDER_CURSOR;
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
    style->slider.border = 0.0f;

    // Slider buttons
    style->slider.inc_button.normal = styleItemColor(rgba(40, 40, 40, 255));
    style->slider.inc_button.hover = styleItemColor(rgba(42, 42, 42, 255));
    style->slider.inc_button.active = styleItemColor(rgba(44, 44, 44, 255));
    style->slider.inc_button.border_color = rgba(65, 65, 65, 255);
    style->slider.inc_button.text_background = rgba(40, 40, 40, 255);
    style->slider.inc_button.text_normal = rgba(175, 175, 175, 255);
    style->slider.inc_button.text_hover = rgba(175, 175, 175, 255);
    style->slider.inc_button.text_active = rgba(175, 175, 175, 255);
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
    style->progress.cursor_normal = styleItemColor(SLIDER_CURSOR);
    style->progress.cursor_hover = styleItemColor(SLIDER_CURSOR_HOVER);
    style->progress.cursor_active = styleItemColor(SLIDER_CURSOR_ACTIVE);
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
    scroll_btn.normal = styleItemColor(rgba(40, 40, 40, 255));
    scroll_btn.hover = styleItemColor(rgba(42, 42, 42, 255));
    scroll_btn.active = styleItemColor(rgba(44, 44, 44, 255));
    scroll_btn.border_color = rgba(65, 65, 65, 255);
    scroll_btn.text_background = rgba(40, 40, 40, 255);
    scroll_btn.text_normal = rgba(175, 175, 175, 255);
    scroll_btn.text_hover = rgba(175, 175, 175, 255);
    scroll_btn.text_active = rgba(175, 175, 175, 255);
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
    style->edit.cursor_normal = TEXT;
    style->edit.cursor_hover = TEXT;
    style->edit.cursor_text_normal = EDIT;
    style->edit.cursor_text_hover = EDIT;
    style->edit.text_normal = TEXT;
    style->edit.text_hover = TEXT;
    style->edit.text_active = TEXT;
    style->edit.selected_normal = TEXT;
    style->edit.selected_hover = TEXT;
    style->edit.selected_text_normal = EDIT;
    style->edit.selected_text_hover = EDIT;
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
    style->property.rounding = 10.0f;

    // Property buttons
    MonstroStyleButton prop_btn;
    memset(&prop_btn, 0, sizeof(prop_btn));
    prop_btn.normal = styleItemColor(PROPERTY);
    prop_btn.hover = styleItemColor(PROPERTY);
    prop_btn.active = styleItemColor(PROPERTY);
    prop_btn.border_color = rgba(0, 0, 0, 0);
    prop_btn.text_background = PROPERTY;
    prop_btn.text_normal = TEXT;
    prop_btn.text_hover = TEXT;
    prop_btn.text_active = TEXT;
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
    style->property.edit.cursor_normal = TEXT;
    style->property.edit.cursor_hover = TEXT;
    style->property.edit.cursor_text_normal = EDIT;
    style->property.edit.cursor_text_hover = EDIT;
    style->property.edit.text_normal = TEXT;
    style->property.edit.text_hover = TEXT;
    style->property.edit.text_active = TEXT;
    style->property.edit.selected_normal = TEXT;
    style->property.edit.selected_hover = TEXT;
    style->property.edit.selected_text_normal = EDIT;
    style->property.edit.selected_text_hover = EDIT;
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
    style->combo.label_hover = TEXT;
    style->combo.label_active = TEXT;
    style->combo.symbol_normal = TEXT;
    style->combo.symbol_hover = TEXT;
    style->combo.symbol_active = TEXT;
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
    style->combo.button.hover = styleItemColor(COMBO);
    style->combo.button.active = styleItemColor(COMBO);
    style->combo.button.border_color = rgba(0, 0, 0, 0);
    style->combo.button.text_background = COMBO;
    style->combo.button.text_normal = TEXT;
    style->combo.button.text_hover = TEXT;
    style->combo.button.text_active = TEXT;
    style->combo.button.text_alignment = MONSTRO_ALIGN_CENTER;
    style->combo.button.padding[0] = 4.0f;
    style->combo.button.padding[1] = 4.0f;
    style->combo.button.border = 0.0f;
    style->combo.button.rounding = 0.0f;

    // Tab
    style->tab.background = styleItemColor(TAB_HEADER);
    style->tab.border_color = BORDER;
    style->tab.text = TEXT;
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
    tab_btn.hover = styleItemColor(TAB_HEADER);
    tab_btn.active = styleItemColor(TAB_HEADER);
    tab_btn.border_color = rgba(0, 0, 0, 0);
    tab_btn.text_background = TAB_HEADER;
    tab_btn.text_normal = TEXT;
    tab_btn.text_hover = TEXT;
    tab_btn.text_active = TEXT;
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
    style->separator.color = rgba(80, 80, 80, 255);
    style->separator.thickness = 1.0f;
    style->separator.padding[0] = 0.0f;
    style->separator.padding[1] = 4.0f;
    style->separator.text_padding[0] = 8.0f;
    style->separator.text_padding[1] = 0.0f;

    // Tooltip
    style->tooltip.background = styleItemColor(WINDOW);
    style->tooltip.border_color = BORDER;
    style->tooltip.text = TEXT;
    style->tooltip.border = 1.0f;
    style->tooltip.rounding = 4.0f;
    style->tooltip.padding[0] = 4.0f;
    style->tooltip.padding[1] = 4.0f;

    // Menu
    style->menu.normal = styleItemColor(WINDOW);
    style->menu.hover = styleItemColor(BUTTON_HOVER);
    style->menu.active = styleItemColor(BUTTON_ACTIVE);
    style->menu.border_color = BORDER;
    style->menu.text_normal = TEXT;
    style->menu.text_hover = TEXT;
    style->menu.text_active = TEXT;
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
    style->tree.text_hover = TEXT;
    style->tree.text_active = TEXT;
    style->tree.text_selected = TEXT;
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
    style->table.row_color_odd = rgba(50, 50, 50, 255);
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
    style->knob.track_normal = rgba(60, 60, 60, 255);
    style->knob.track_filled = SLIDER_CURSOR;
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
    style->window.header.active = styleItemColor(HEADER);
    style->window.header.label_normal = TEXT;
    style->window.header.label_hover = TEXT;
    style->window.header.label_active = TEXT;
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
    hdr_btn.hover = styleItemColor(HEADER);
    hdr_btn.active = styleItemColor(HEADER);
    hdr_btn.border_color = rgba(0, 0, 0, 0);
    hdr_btn.text_background = HEADER;
    hdr_btn.text_normal = TEXT;
    hdr_btn.text_hover = TEXT;
    hdr_btn.text_active = TEXT;
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
    style->window.border_style = styleItemColor(BORDER);
    style->window.border_color = BORDER;
    style->window.popup_border_color = BORDER;
    style->window.combo_border_color = BORDER;
    style->window.contextual_border_color = BORDER;
    style->window.menu_border_color = BORDER;
    style->window.group_border_color = BORDER;
    style->window.tooltip_border_color = BORDER;
    style->window.scaler = styleItemColor(TEXT);

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

void monstro_style_set_dark(MonstroStyle* style) {
    monstro_style_set_default(style);
}

void monstro_style_set_light(MonstroStyle* style) {
    if (!style) return;
    
    monstro_style_set_default(style);
    
    MonstroColor TEXT_L = rgba(40, 40, 40, 255);
    MonstroColor WINDOW_L = rgba(240, 240, 240, 255);
    MonstroColor HEADER_L = rgba(220, 220, 220, 255);
    MonstroColor BORDER_L = rgba(180, 180, 180, 255);
    MonstroColor BUTTON_L = rgba(220, 220, 220, 255);
    MonstroColor BUTTON_HOVER_L = rgba(200, 200, 200, 255);
    MonstroColor BUTTON_ACTIVE_L = rgba(180, 180, 180, 255);

    style->text.color = TEXT_L;
    
    style->button.normal = styleItemColor(BUTTON_L);
    style->button.hover = styleItemColor(BUTTON_HOVER_L);
    style->button.active = styleItemColor(BUTTON_ACTIVE_L);
    style->button.border_color = BORDER_L;
    style->button.text_normal = TEXT_L;
    style->button.text_hover = rgba(20, 20, 20, 255);
    style->button.text_active = rgba(60, 60, 60, 255);
    
    style->window.background = WINDOW_L;
    style->window.fixed_background = styleItemColor(WINDOW_L);
    style->window.border_style = styleItemColor(BORDER_L);
    style->window.border_color = BORDER_L;
    style->window.header.normal = styleItemColor(HEADER_L);
    style->window.header.hover = styleItemColor(rgba(210, 210, 210, 255));
    style->window.header.active = styleItemColor(rgba(200, 200, 200, 255));
    style->window.header.label_normal = TEXT_L;
    style->window.header.label_hover = TEXT_L;
    style->window.header.label_active = TEXT_L;
}
