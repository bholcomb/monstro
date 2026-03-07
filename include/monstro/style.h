#ifndef MONSTRO_STYLE_H
#define MONSTRO_STYLE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MonstroStyleText {
    MonstroColor color;
    float padding[2];
} MonstroStyleText;

typedef struct MonstroStyleButton {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroColor border_color;

    MonstroColor text_background;
    MonstroColor text_normal;
    MonstroColor text_hover;
    MonstroColor text_active;
    MonstroAlign text_alignment;

    float border;
    float rounding;
    float padding[2];
    float image_padding[2];
    float touch_padding[2];
} MonstroStyleButton;

typedef struct MonstroStyleToggle {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroColor border_color;

    MonstroStyleItem cursor_normal;
    MonstroStyleItem cursor_hover;

    MonstroColor text_normal;
    MonstroColor text_hover;
    MonstroColor text_active;
    MonstroColor text_background;
    MonstroAlign text_alignment;

    float padding[2];
    float touch_padding[2];
    float spacing;
    float border;
    float rounding;
} MonstroStyleToggle;

typedef struct MonstroStyleSelectable {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem pressed;

    MonstroStyleItem normal_active;
    MonstroStyleItem hover_active;
    MonstroStyleItem pressed_active;

    MonstroColor text_normal;
    MonstroColor text_hover;
    MonstroColor text_pressed;

    MonstroColor text_normal_active;
    MonstroColor text_hover_active;
    MonstroColor text_pressed_active;
    MonstroColor text_background;
    MonstroAlign text_alignment;

    float rounding;
    float padding[2];
    float touch_padding[2];
    float image_padding[2];
} MonstroStyleSelectable;

typedef struct MonstroStyleSlider {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroColor border_color;

    MonstroColor bar_normal;
    MonstroColor bar_hover;
    MonstroColor bar_active;
    MonstroColor bar_filled;

    MonstroStyleItem cursor_normal;
    MonstroStyleItem cursor_hover;
    MonstroStyleItem cursor_active;

    float border;
    float rounding;
    float bar_height;
    float padding[2];
    float spacing[2];
    float cursor_size[2];

    bool show_buttons;
    MonstroStyleButton inc_button;
    MonstroStyleButton dec_button;
    uint32_t inc_symbol;
    uint32_t dec_symbol;
} MonstroStyleSlider;

typedef struct MonstroStyleProgress {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroColor border_color;

    MonstroStyleItem cursor_normal;
    MonstroStyleItem cursor_hover;
    MonstroStyleItem cursor_active;
    MonstroColor cursor_border_color;

    float rounding;
    float border;
    float cursor_border;
    float cursor_rounding;
    float padding[2];
} MonstroStyleProgress;

typedef struct MonstroStyleScrollbar {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroColor border_color;

    MonstroStyleItem cursor_normal;
    MonstroStyleItem cursor_hover;
    MonstroStyleItem cursor_active;
    MonstroColor cursor_border_color;

    float border;
    float rounding;
    float border_cursor;
    float rounding_cursor;
    float padding[2];

    bool show_buttons;
    MonstroStyleButton inc_button;
    MonstroStyleButton dec_button;
    uint32_t inc_symbol;
    uint32_t dec_symbol;
} MonstroStyleScrollbar;

typedef struct MonstroStyleEdit {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroColor border_color;
    MonstroStyleScrollbar scrollbar;

    MonstroColor cursor_normal;
    MonstroColor cursor_hover;
    MonstroColor cursor_text_normal;
    MonstroColor cursor_text_hover;

    MonstroColor text_normal;
    MonstroColor text_hover;
    MonstroColor text_active;

    MonstroColor selected_normal;
    MonstroColor selected_hover;
    MonstroColor selected_text_normal;
    MonstroColor selected_text_hover;

    float border;
    float rounding;
    float cursor_size;
    float scrollbar_size[2];
    float padding[2];
    float row_padding;
} MonstroStyleEdit;

typedef struct MonstroStyleProperty {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroColor border_color;

    MonstroColor label_normal;
    MonstroColor label_hover;
    MonstroColor label_active;

    uint32_t sym_left;
    uint32_t sym_right;

    float border;
    float rounding;
    float padding[2];

    MonstroStyleEdit edit;
    MonstroStyleButton inc_button;
    MonstroStyleButton dec_button;
} MonstroStyleProperty;

typedef struct MonstroStyleKnob {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroColor border_color;

    MonstroColor track_normal;
    MonstroColor track_filled;

    MonstroStyleItem indicator_normal;
    MonstroStyleItem indicator_hover;
    MonstroStyleItem indicator_active;

    float border;
    float diameter;
    float indicator_length;
    float indicator_width;
    float track_width;
    float padding[2];
} MonstroStyleKnob;

typedef struct MonstroStyleTree {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroStyleItem selected;

    MonstroColor text_normal;
    MonstroColor text_hover;
    MonstroColor text_active;
    MonstroColor text_selected;

    uint32_t sym_closed;
    uint32_t sym_open;

    float indent;
    float padding[2];
    float spacing[2];
    float rounding;
} MonstroStyleTree;

typedef struct MonstroStyleTable {
    MonstroStyleItem background;
    MonstroColor border_color;
    MonstroColor row_color_even;
    MonstroColor row_color_odd;

    MonstroStyleItem header_normal;
    MonstroStyleItem header_hover;
    MonstroStyleItem header_active;
    MonstroColor header_text;

    float border;
    float cell_padding[2];
    float row_height;
    float header_height;
    float rounding;
} MonstroStyleTable;

typedef struct MonstroStyleSeparator {
    MonstroColor color;
    float thickness;
    float padding[2];
    float text_padding[2];
} MonstroStyleSeparator;

typedef struct MonstroStyleTooltip {
    MonstroStyleItem background;
    MonstroColor border_color;
    MonstroColor text;

    float border;
    float rounding;
    float padding[2];
} MonstroStyleTooltip;

typedef struct MonstroStyleMenu {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroColor border_color;

    MonstroColor text_normal;
    MonstroColor text_hover;
    MonstroColor text_active;

    float border;
    float rounding;
    float padding[2];
    float spacing[2];
} MonstroStyleMenu;

typedef struct MonstroStyleChart {
    MonstroStyleItem background;
    MonstroColor border_color;
    MonstroColor selected_color;
    MonstroColor color;

    float border;
    float rounding;
    float padding[2];
} MonstroStyleChart;

typedef struct MonstroStyleCombo {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;
    MonstroColor border_color;

    MonstroColor label_normal;
    MonstroColor label_hover;
    MonstroColor label_active;

    MonstroColor symbol_normal;
    MonstroColor symbol_hover;
    MonstroColor symbol_active;

    MonstroStyleButton button;
    uint32_t sym_normal;
    uint32_t sym_hover;
    uint32_t sym_active;

    float border;
    float rounding;
    float content_padding[2];
    float button_padding[2];
    float spacing[2];
} MonstroStyleCombo;

typedef struct MonstroStyleTab {
    MonstroStyleItem background;
    MonstroColor border_color;
    MonstroColor text;

    MonstroStyleButton tab_maximize_button;
    MonstroStyleButton tab_minimize_button;
    MonstroStyleButton node_maximize_button;
    MonstroStyleButton node_minimize_button;
    uint32_t sym_minimize;
    uint32_t sym_maximize;

    float border;
    float rounding;
    float indent;
    float padding[2];
    float spacing[2];
} MonstroStyleTab;

typedef struct MonstroStyleWindowHeader {
    MonstroStyleItem normal;
    MonstroStyleItem hover;
    MonstroStyleItem active;

    MonstroStyleButton close_button;
    MonstroStyleButton minimize_button;
    uint32_t close_symbol;
    uint32_t minimize_symbol;
    uint32_t maximize_symbol;

    MonstroColor label_normal;
    MonstroColor label_hover;
    MonstroColor label_active;

    MonstroAlign align;
    float padding[2];
    float label_padding[2];
    float spacing[2];
} MonstroStyleWindowHeader;

typedef struct MonstroStyleWindow {
    MonstroStyleWindowHeader header;
    MonstroStyleItem fixed_background;
    MonstroColor background;

    MonstroColor border_color;
    MonstroColor popup_border_color;
    MonstroColor combo_border_color;
    MonstroColor contextual_border_color;
    MonstroColor menu_border_color;
    MonstroColor group_border_color;
    MonstroColor tooltip_border_color;
    MonstroStyleItem scaler;

    float border;
    float combo_border;
    float contextual_border;
    float menu_border;
    float group_border;
    float tooltip_border;
    float popup_border;

    float rounding;
    float spacing[2];
    float scrollbar_size[2];
    float min_size[2];

    float padding[2];
    float group_padding[2];
    float popup_padding[2];
    float combo_padding[2];
    float contextual_padding[2];
    float menu_padding[2];
    float tooltip_padding[2];
} MonstroStyleWindow;

typedef struct MonstroStyle {
    MonstroFont* font;

    MonstroStyleText text;
    MonstroStyleButton button;
    MonstroStyleButton contextual_button;
    MonstroStyleButton menu_button;
    MonstroStyleToggle option;
    MonstroStyleToggle checkbox;
    MonstroStyleSelectable selectable;
    MonstroStyleSlider slider;
    MonstroStyleKnob knob;
    MonstroStyleProgress progress;
    MonstroStyleProperty property;
    MonstroStyleEdit edit;
    MonstroStyleTree tree;
    MonstroStyleTab tab;
    MonstroStyleTable table;
    MonstroStyleCombo combo;
    MonstroStyleMenu menu;
    MonstroStyleSeparator separator;
    MonstroStyleTooltip tooltip;
    MonstroStyleChart chart;
    MonstroStyleScrollbar scrollh;
    MonstroStyleScrollbar scrollv;
    MonstroStyleWindow window;
} MonstroStyle;

MonstroStyle* monstro_get_style(MonstroContext* ctx);
void monstro_set_style(MonstroContext* ctx, const MonstroStyle* style);

void monstro_push_style(MonstroContext* ctx);
void monstro_pop_style(MonstroContext* ctx);

void monstro_style_set_default(MonstroStyle* style);
void monstro_style_set_dark(MonstroStyle* style);
void monstro_style_set_light(MonstroStyle* style);

#ifdef __cplusplus
}
#endif

#endif
