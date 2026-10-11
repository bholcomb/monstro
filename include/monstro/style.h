#ifndef MONSTRO_STYLE_H
#define MONSTRO_STYLE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MonstroStyleText {
    MonstroColor color;
    MonstroColor background;
    float padding[2];
    MonstroAlign alignment;
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
    MonstroStyleItem border_style;
    MonstroColor background;

    MonstroColor border_color;
    MonstroColor popup_border_color;
    MonstroColor combo_border_color;
    MonstroColor contextual_border_color;
    MonstroColor menu_border_color;
    MonstroStyleItem group_background;
    MonstroColor group_border_color;
    MonstroColor tooltip_border_color;
    MonstroStyleItem scaler;
    MonstroColor modal_dim_color;

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

/* ------------------------------------------------------------------------- */
/* Themes                                                                    */
/* ------------------------------------------------------------------------- */

/*
 * A theme is a style registered once in the environment and immutable from
 * then on; any number of contexts reference it. Each frame starts from a
 * working copy of the context's theme, and push/pop changes apply to that copy
 * only. A theme whose font is NULL uses the environment's default font.
 * monstro_init registers three themes: "default", "dark" and "light"; a new
 * context uses "default".
 */
MONSTRO_API MonstroTheme* monstro_theme_register(MonstroEnv* env, const char* name, const MonstroStyle* style);
MONSTRO_API MonstroTheme* monstro_theme_find(MonstroEnv* env, const char* name);
MONSTRO_API const MonstroStyle* monstro_theme_get_style(MonstroTheme* theme);
MONSTRO_API const char* monstro_theme_get_name(MonstroTheme* theme);

MONSTRO_API void monstro_context_set_theme(MonstroContext* ctx, MonstroTheme* theme);
MONSTRO_API MonstroTheme* monstro_context_get_theme(MonstroContext* ctx);

MONSTRO_API void monstro_style_set_default(MonstroStyle* style);
MONSTRO_API void monstro_style_set_dark(MonstroStyle* style);
MONSTRO_API void monstro_style_set_light(MonstroStyle* style);

/* ------------------------------------------------------------------------- */
/* Current style: the frame's working copy, or the theme between frames      */
/* ------------------------------------------------------------------------- */

MONSTRO_API const MonstroStyle*           monstro_get_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleText*       monstro_get_text_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleButton*     monstro_get_button_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleButton*     monstro_get_contextual_button_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleButton*     monstro_get_menu_button_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleToggle*     monstro_get_option_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleToggle*     monstro_get_checkbox_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleSelectable* monstro_get_selectable_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleSlider*     monstro_get_slider_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleKnob*       monstro_get_knob_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleProgress*   monstro_get_progress_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleProperty*   monstro_get_property_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleEdit*       monstro_get_edit_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleTree*       monstro_get_tree_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleTab*        monstro_get_tab_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleTable*      monstro_get_table_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleCombo*      monstro_get_combo_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleMenu*       monstro_get_menu_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleSeparator*  monstro_get_separator_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleTooltip*    monstro_get_tooltip_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleChart*      monstro_get_chart_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleScrollbar*  monstro_get_scrollh_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleScrollbar*  monstro_get_scrollv_style(MonstroContext* ctx);
MONSTRO_API const MonstroStyleWindow*     monstro_get_window_style(MonstroContext* ctx);

/* ------------------------------------------------------------------------- */
/* Per-widget style push/pop (inside a frame, on the working copy)           */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_push_text_style(MonstroContext* ctx, const MonstroStyleText* style);
MONSTRO_API void monstro_push_button_style(MonstroContext* ctx, const MonstroStyleButton* style);
MONSTRO_API void monstro_push_contextual_button_style(MonstroContext* ctx, const MonstroStyleButton* style);
MONSTRO_API void monstro_push_menu_button_style(MonstroContext* ctx, const MonstroStyleButton* style);
MONSTRO_API void monstro_push_option_style(MonstroContext* ctx, const MonstroStyleToggle* style);
MONSTRO_API void monstro_push_checkbox_style(MonstroContext* ctx, const MonstroStyleToggle* style);
MONSTRO_API void monstro_push_selectable_style(MonstroContext* ctx, const MonstroStyleSelectable* style);
MONSTRO_API void monstro_push_slider_style(MonstroContext* ctx, const MonstroStyleSlider* style);
MONSTRO_API void monstro_push_knob_style(MonstroContext* ctx, const MonstroStyleKnob* style);
MONSTRO_API void monstro_push_progress_style(MonstroContext* ctx, const MonstroStyleProgress* style);
MONSTRO_API void monstro_push_property_style(MonstroContext* ctx, const MonstroStyleProperty* style);
MONSTRO_API void monstro_push_edit_style(MonstroContext* ctx, const MonstroStyleEdit* style);
MONSTRO_API void monstro_push_tree_style(MonstroContext* ctx, const MonstroStyleTree* style);
MONSTRO_API void monstro_push_tab_style(MonstroContext* ctx, const MonstroStyleTab* style);
MONSTRO_API void monstro_push_table_style(MonstroContext* ctx, const MonstroStyleTable* style);
MONSTRO_API void monstro_push_combo_style(MonstroContext* ctx, const MonstroStyleCombo* style);
MONSTRO_API void monstro_push_menu_style(MonstroContext* ctx, const MonstroStyleMenu* style);
MONSTRO_API void monstro_push_separator_style(MonstroContext* ctx, const MonstroStyleSeparator* style);
MONSTRO_API void monstro_push_tooltip_style(MonstroContext* ctx, const MonstroStyleTooltip* style);
MONSTRO_API void monstro_push_chart_style(MonstroContext* ctx, const MonstroStyleChart* style);
MONSTRO_API void monstro_push_scrollh_style(MonstroContext* ctx, const MonstroStyleScrollbar* style);
MONSTRO_API void monstro_push_scrollv_style(MonstroContext* ctx, const MonstroStyleScrollbar* style);
MONSTRO_API void monstro_push_window_style(MonstroContext* ctx, const MonstroStyleWindow* style);

MONSTRO_API void monstro_pop_style(MonstroContext* ctx, int count);

#ifdef __cplusplus
}
#endif

#endif
