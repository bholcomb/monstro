#ifndef MONSTRO_WIDGETS_H
#define MONSTRO_WIDGETS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Flags                                                                     */
/* ------------------------------------------------------------------------- */

typedef MONSTRO_ENUM(MonstroSelectableFlags) {
    MONSTRO_SELECTABLE_NONE             = 0,
    MONSTRO_SELECTABLE_DONT_CLOSE_POPUP = 1 << 0,
    MONSTRO_SELECTABLE_SPAN_ALL_COLUMNS = 1 << 1,
    MONSTRO_SELECTABLE_MENU             = 1 << 2,
    MONSTRO_SELECTABLE_MENU_ITEM        = 1 << 3,
    MONSTRO_SELECTABLE_DISABLED         = 1 << 4,
    MONSTRO_SELECTABLE_DRAW_FILL_WIDTH  = 1 << 5,
    MONSTRO_SELECTABLE_HAS_TOGGLE       = 1 << 6
} MonstroSelectableFlags;

/* A vertical slider runs from its minimum at the bottom to its maximum at the top. */
typedef MONSTRO_ENUM(MonstroSliderFlags) {
    MONSTRO_SLIDER_NONE     = 0,
    MONSTRO_SLIDER_VERTICAL = 1 << 0
} MonstroSliderFlags;

/*
 * A knob turns by dragging up and down, relative to where it was; an absolute
 * knob points where the pointer is, about its centre, from a press on.
 */
typedef MONSTRO_ENUM(MonstroKnobFlags) {
    MONSTRO_KNOB_NONE     = 0,
    MONSTRO_KNOB_ABSOLUTE = 1 << 0
} MonstroKnobFlags;

typedef MONSTRO_ENUM(MonstroInputTextFlags) {
    MONSTRO_INPUT_TEXT_NONE              = 0,
    MONSTRO_INPUT_TEXT_READ_ONLY         = 1 << 0,
    MONSTRO_INPUT_TEXT_PASSWORD          = 1 << 1,
    MONSTRO_INPUT_TEXT_AUTO_SELECT_ALL   = 1 << 2,
    MONSTRO_INPUT_TEXT_ENTER_RETURNS     = 1 << 3,
    MONSTRO_INPUT_TEXT_CHARS_DECIMAL     = 1 << 4,
    MONSTRO_INPUT_TEXT_CHARS_HEXADECIMAL = 1 << 5,
    MONSTRO_INPUT_TEXT_CHARS_NO_BLANK    = 1 << 6
} MonstroInputTextFlags;

/* A vertical progress bar fills from the bottom up. */
typedef MONSTRO_ENUM(MonstroProgressFlags) {
    MONSTRO_PROGRESS_NONE     = 0,
    MONSTRO_PROGRESS_VERTICAL = 1 << 0
} MonstroProgressFlags;

typedef MONSTRO_ENUM(MonstroTreeFlags) {
    MONSTRO_TREE_NONE         = 0,
    MONSTRO_TREE_DEFAULT_OPEN = 1 << 0,
    MONSTRO_TREE_SELECTED     = 1 << 2
} MonstroTreeFlags;

typedef MONSTRO_ENUM(MonstroTabBarFlags) {
    MONSTRO_TAB_BAR_NONE        = 0,
    MONSTRO_TAB_BAR_REORDERABLE = 1 << 0,
    MONSTRO_TAB_BAR_AUTO_SELECT = 1 << 1
} MonstroTabBarFlags;

typedef MONSTRO_ENUM(MonstroButtonFlags) {
    MONSTRO_BUTTON_NONE               = 0,
    MONSTRO_BUTTON_REPEAT             = 1 << 0,
    MONSTRO_BUTTON_PRESSED_ON_CLICK   = 1 << 1,
    MONSTRO_BUTTON_PRESSED_ON_RELEASE = 1 << 2,
    MONSTRO_BUTTON_FLATTEN_CHILDREN   = 1 << 3,
    MONSTRO_BUTTON_DONT_CLOSE_POPUPS  = 1 << 4,
    MONSTRO_BUTTON_DISABLED           = 1 << 5,
    MONSTRO_BUTTON_ALIGN_TEXT_BASELINE= 1 << 6,
    MONSTRO_BUTTON_NO_KEY_MODIFIERS   = 1 << 7
} MonstroButtonFlags;

/* ------------------------------------------------------------------------- */
/* Item State Queries (call after the widget)                                */
/* ------------------------------------------------------------------------- */

/*
 * An item is active while it is held (a pressed button, a dragged slider or
 * knob), edited (a text or number field) or open (a combo). Its edges tell a
 * host when an interaction starts and ends, so it can act once per drag or
 * edit rather than on every change. A tap, pressed and released between two
 * frames, starts and ends in one frame: both edges are reported in it.
 */
MONSTRO_API bool monstro_is_item_hovered(MonstroContext* ctx);
MONSTRO_API bool monstro_is_item_active(MonstroContext* ctx);
MONSTRO_API bool monstro_is_item_clicked(MonstroContext* ctx, MonstroMouseButton button);
/* The item changed its value this frame. */
MONSTRO_API bool monstro_is_item_edited(MonstroContext* ctx);
/* The item became active this frame. */
MONSTRO_API bool monstro_is_item_activated(MonstroContext* ctx);
/* The item stopped being active this frame. */
MONSTRO_API bool monstro_is_item_deactivated(MonstroContext* ctx);
/* The item stopped being active this frame and changed its value while it was. */
MONSTRO_API bool monstro_is_item_deactivated_after_edit(MonstroContext* ctx);
MONSTRO_API MonstroRect monstro_get_item_rect(MonstroContext* ctx);
MONSTRO_API MonstroVec2 monstro_get_item_rect_min(MonstroContext* ctx);
MONSTRO_API MonstroVec2 monstro_get_item_rect_max(MonstroContext* ctx);
MONSTRO_API MonstroVec2 monstro_get_item_rect_size(MonstroContext* ctx);

/* ------------------------------------------------------------------------- */
/* Disabled State                                                            */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_begin_disabled(MonstroContext* ctx, bool disabled);
MONSTRO_API void monstro_end_disabled(MonstroContext* ctx);
MONSTRO_API bool monstro_is_disabled(MonstroContext* ctx);

/* ------------------------------------------------------------------------- */
/* Button                                                                    */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_button_ex(MonstroContext* ctx, const char* label, float width, float height,
                       MonstroButtonFlags flags, const char* tooltip);
MONSTRO_API bool monstro_button(MonstroContext* ctx, const char* label, const char* tooltip);
MONSTRO_API bool monstro_button_sized(MonstroContext* ctx, const char* label, float width, float height, const char* tooltip);
MONSTRO_API bool monstro_image_button_ex(MonstroContext* ctx, const char* str_id, uint64_t texture_id,
                             float width, float height, MonstroButtonFlags flags, const char* tooltip);
MONSTRO_API bool monstro_image_button(MonstroContext* ctx, const char* str_id, uint64_t texture_id, float width, float height, const char* tooltip);
MONSTRO_API bool monstro_invisible_button_ex(MonstroContext* ctx, const char* str_id, float width, float height,
                                 MonstroButtonFlags flags);
MONSTRO_API bool monstro_invisible_button(MonstroContext* ctx, const char* str_id, float width, float height);

/* ------------------------------------------------------------------------- */
/* Label / Text                                                              */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_label(MonstroContext* ctx, const char* text);
MONSTRO_API void monstro_label_sized(MonstroContext* ctx, const char* text, float width, float height);
MONSTRO_API void monstro_label_wrapped(MonstroContext* ctx, const char* text, float wrap_width);
MONSTRO_API void monstro_label_colored(MonstroContext* ctx, const char* text, MonstroColor color);

/* ------------------------------------------------------------------------- */
/* Image                                                                     */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_image(MonstroContext* ctx, uint64_t texture_id, float width, float height);

/* ------------------------------------------------------------------------- */
/* Icon -- see include/monstro/icons.h for the full icon API                 */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* Checkbox                                                                  */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_checkbox(MonstroContext* ctx, const char* label, bool* value, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Radio Button                                                              */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_radio_button(MonstroContext* ctx, const char* label, int* value, int button_value, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Slider                                                                    */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_slider_float(MonstroContext* ctx, const char* label, float* value,
                          float min, float max, const char* format, const char* tooltip);
MONSTRO_API bool monstro_slider_int(MonstroContext* ctx, const char* label, int* value,
                        int min, int max, const char* format, const char* tooltip);
MONSTRO_API bool monstro_slider_float_ex(MonstroContext* ctx, const char* label, float* value,
                             float min, float max, const char* format,
                             float width, float height, MonstroSliderFlags flags, const char* tooltip);
MONSTRO_API bool monstro_slider_int_ex(MonstroContext* ctx, const char* label, int* value,
                           int min, int max, const char* format,
                           float width, float height, MonstroSliderFlags flags, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Knob (Rotary Slider)                                                      */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_knob(MonstroContext* ctx, const char* label, float* value,
                  float min, float max, const char* format, const char* tooltip);
MONSTRO_API bool monstro_knob_sized(MonstroContext* ctx, const char* label, float* value,
                        float min, float max, const char* format, float diameter, const char* tooltip);
MONSTRO_API bool monstro_knob_ex(MonstroContext* ctx, const char* label, float* value,
                     float min, float max, const char* format, float diameter,
                     MonstroKnobFlags flags, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Progress Bar                                                              */
/* ------------------------------------------------------------------------- */

/* overlay is plain text; NULL shows the percentage and "" shows no text. */
MONSTRO_API void monstro_progress_bar(MonstroContext* ctx, float fraction, float width, float height, const char* overlay);
MONSTRO_API void monstro_progress_bar_ex(MonstroContext* ctx, float fraction, float width, float height, const char* overlay,
                             MonstroProgressFlags flags);

/* ------------------------------------------------------------------------- */
/* Text Input                                                                */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_input_text(MonstroContext* ctx, const char* label, char* buf, size_t buf_size,
                        MonstroInputTextFlags flags, const char* tooltip);
MONSTRO_API bool monstro_input_text_multiline(MonstroContext* ctx, const char* label, char* buf, size_t buf_size,
                                  float width, float height, MonstroInputTextFlags flags, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Numeric Input                                                             */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_input_int(MonstroContext* ctx, const char* label, int* value,
                       int step, int step_fast, const char* tooltip);
MONSTRO_API bool monstro_input_float(MonstroContext* ctx, const char* label, float* value,
                         float step, float step_fast, const char* format, const char* tooltip);
MONSTRO_API bool monstro_input_double(MonstroContext* ctx, const char* label, double* value,
                          double step, double step_fast, const char* format, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Selectable                                                                */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_selectable(MonstroContext* ctx, const char* label, bool* selected,
                        MonstroSelectableFlags flags, const char* tooltip);
MONSTRO_API bool monstro_selectable_sized(MonstroContext* ctx, const char* label, bool* selected,
                              float width, float height, MonstroSelectableFlags flags, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Combo Box                                                                 */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_begin_combo(MonstroContext* ctx, const char* label, const char* preview_value, const char* tooltip);
MONSTRO_API void monstro_end_combo(MonstroContext* ctx);
MONSTRO_API bool monstro_combo(MonstroContext* ctx, const char* label, int* current_item,
                   const char* const items[], int items_count, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Menu                                                                      */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_begin_menu_bar(MonstroContext* ctx);
MONSTRO_API void monstro_end_menu_bar(MonstroContext* ctx);
MONSTRO_API bool monstro_begin_menu(MonstroContext* ctx, const char* label, bool enabled);
MONSTRO_API void monstro_end_menu(MonstroContext* ctx);
MONSTRO_API bool monstro_menu_item(MonstroContext* ctx, const char* label, bool* selected, bool enabled);

/* ------------------------------------------------------------------------- */
/* Popup                                                                     */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_open_popup(MonstroContext* ctx, const char* label);
MONSTRO_API void monstro_open_popup_ex(MonstroContext* ctx, const char* label, bool modal);
MONSTRO_API void monstro_close_current_popup(MonstroContext* ctx);
MONSTRO_API bool monstro_begin_popup(MonstroContext* ctx, const char* label, MonstroWindowFlags extra_flags);
MONSTRO_API bool monstro_begin_popup_modal(MonstroContext* ctx, const char* label, bool* p_open, MonstroWindowFlags flags);
MONSTRO_API bool monstro_begin_popup_context_item(MonstroContext* ctx, const char* label, MonstroMouseButton mouse_button);
MONSTRO_API bool monstro_begin_popup_context_window(MonstroContext* ctx, const char* label, MonstroMouseButton mouse_button, bool also_over_items);
MONSTRO_API bool monstro_begin_popup_context_void(MonstroContext* ctx, const char* label, MonstroMouseButton mouse_button);
MONSTRO_API void monstro_end_popup(MonstroContext* ctx);
MONSTRO_API bool monstro_is_popup_open(MonstroContext* ctx, const char* str_id);

/* ------------------------------------------------------------------------- */
/* Tree / Collapsible                                                        */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_begin_tree_node(MonstroContext* ctx, const char* label, MonstroTreeFlags flags);
MONSTRO_API bool monstro_begin_tree_node_id(MonstroContext* ctx, uint64_t id, const char* label, MonstroTreeFlags flags);
MONSTRO_API void monstro_end_tree_node(MonstroContext* ctx);
MONSTRO_API bool monstro_tree_leaf(MonstroContext* ctx, const char* label, MonstroTreeFlags flags);
MONSTRO_API bool monstro_collapsing_header(MonstroContext* ctx, const char* label, bool* p_visible);

MONSTRO_API float monstro_get_tree_indent(MonstroContext* ctx);
MONSTRO_API void monstro_set_tree_indent(MonstroContext* ctx, float indent);

/* ------------------------------------------------------------------------- */
/* Group                                                                     */
/* ------------------------------------------------------------------------- */

typedef MONSTRO_ENUM(MonstroGroupFlags) {
    MONSTRO_GROUP_NONE       = 0,
    MONSTRO_GROUP_BORDER     = 1 << 0,
    MONSTRO_GROUP_BACKGROUND = 1 << 1,
    MONSTRO_GROUP_DEFAULT    = MONSTRO_GROUP_BORDER
} MonstroGroupFlags;

MONSTRO_API void monstro_begin_group(MonstroContext* ctx, MonstroLayoutDirection layout, MonstroGroupFlags flags);
MONSTRO_API void monstro_end_group(MonstroContext* ctx);

/* ------------------------------------------------------------------------- */
/* Tabs                                                                      */
/* ------------------------------------------------------------------------- */

MONSTRO_API bool monstro_begin_tab_bar(MonstroContext* ctx, const char* str_id, MonstroTabBarFlags flags);
MONSTRO_API void monstro_end_tab_bar(MonstroContext* ctx);
MONSTRO_API bool monstro_begin_tab_item(MonstroContext* ctx, const char* label, bool* p_open);
MONSTRO_API void monstro_end_tab_item(MonstroContext* ctx);

/*
 * Page tabs: a row of count tabs sharing width x height evenly (width 0 fills
 * the available width, height 0 fits the text), with tab *selected lit. The
 * selection is the host's, as a slider's value is: pass it every frame, and a
 * click on another tab writes that tab's index and returns true. Each tab's
 * ID is its label's within str_id. *selected must be a tab's index.
 */
MONSTRO_API bool monstro_page_tabs(MonstroContext* ctx, const char* str_id, const char* const labels[], int count,
                       int* selected, float width, float height);

/* ------------------------------------------------------------------------- */
/* Table                                                                     */
/* ------------------------------------------------------------------------- */

typedef MONSTRO_ENUM(MonstroTableFlags) {
    MONSTRO_TABLE_NONE           = 0,
    MONSTRO_TABLE_BORDERS        = 1 << 0,
    MONSTRO_TABLE_RESIZABLE      = 1 << 1,
    MONSTRO_TABLE_SORTABLE       = 1 << 2,
    MONSTRO_TABLE_ROW_BACKGROUND = 1 << 3,
    MONSTRO_TABLE_SCROLL_X       = 1 << 4,
    MONSTRO_TABLE_SCROLL_Y       = 1 << 5
} MonstroTableFlags;

typedef MONSTRO_ENUM(MonstroTableColumnFlags) {
    MONSTRO_TABLE_COLUMN_NONE           = 0,
    MONSTRO_TABLE_COLUMN_WIDTH_FIXED    = 1 << 0,
    MONSTRO_TABLE_COLUMN_NO_RESIZE      = 1 << 1,
    MONSTRO_TABLE_COLUMN_NO_SORT        = 1 << 2,
    MONSTRO_TABLE_COLUMN_DEFAULT_HIDDEN = 1 << 3
} MonstroTableColumnFlags;

MONSTRO_API bool monstro_begin_table(MonstroContext* ctx, const char* str_id, int column_count,
                         MonstroTableFlags flags, float width, float height);
MONSTRO_API void monstro_end_table(MonstroContext* ctx);

MONSTRO_API void monstro_table_setup_column(MonstroContext* ctx, const char* label,
                                MonstroTableColumnFlags flags, float init_width);
MONSTRO_API void monstro_table_headers_row(MonstroContext* ctx);

MONSTRO_API void monstro_table_next_row(MonstroContext* ctx);
MONSTRO_API bool monstro_table_next_column(MonstroContext* ctx);
MONSTRO_API bool monstro_table_set_column_index(MonstroContext* ctx, int column);

/* ------------------------------------------------------------------------- */
/* Tooltip                                                                   */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_tooltip(MonstroContext* ctx, const char* text);

/* ------------------------------------------------------------------------- */
/* Separator                                                                 */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_separator(MonstroContext* ctx);
MONSTRO_API void monstro_separator_text(MonstroContext* ctx, const char* text);

/* ------------------------------------------------------------------------- */
/* Color Picker                                                              */
/* ------------------------------------------------------------------------- */

typedef MONSTRO_ENUM(MonstroColorPickerFlags) {
    MONSTRO_COLOR_PICKER_NONE       = 0,
    MONSTRO_COLOR_PICKER_NO_ALPHA   = 1 << 0,
    MONSTRO_COLOR_PICKER_NO_SLIDERS = 1 << 1,
    MONSTRO_COLOR_PICKER_NO_HEX     = 1 << 2,
    MONSTRO_COLOR_PICKER_NO_PREVIEW = 1 << 3
} MonstroColorPickerFlags;

MONSTRO_API bool monstro_color_picker(MonstroContext* ctx, const char* label, MonstroColor* color,
                          MonstroColorPickerFlags flags);

MONSTRO_API bool monstro_color_button(MonstroContext* ctx, const char* label, MonstroColor color,
                          float width, float height);

/* ------------------------------------------------------------------------- */
/* File Dialog                                                               */
/* ------------------------------------------------------------------------- */

typedef MONSTRO_ENUM(MonstroFileDialogMode) {
    MONSTRO_FILE_DIALOG_OPEN = 0,
    MONSTRO_FILE_DIALOG_SAVE = 1
} MonstroFileDialogMode;

typedef MONSTRO_ENUM(MonstroFileDialogResult) {
    MONSTRO_FILE_DIALOG_NONE = 0,
    MONSTRO_FILE_DIALOG_OK   = 1,
    MONSTRO_FILE_DIALOG_CANCEL = 2
} MonstroFileDialogResult;

typedef struct MonstroFileDialogState MonstroFileDialogState;

MONSTRO_API MonstroFileDialogState* monstro_file_dialog_create(void);
MONSTRO_API void monstro_file_dialog_destroy(MonstroFileDialogState* state);

MONSTRO_API void monstro_file_dialog_open(MonstroFileDialogState* state,
                              const char* title,
                              const char* start_path,
                              const char* filter,
                              MonstroFileDialogMode mode);

MONSTRO_API MonstroFileDialogResult monstro_file_dialog_draw(MonstroContext* ctx,
                                                 MonstroFileDialogState* state);

MONSTRO_API const char* monstro_file_dialog_get_path(const MonstroFileDialogState* state);

MONSTRO_API bool monstro_file_dialog_is_open(const MonstroFileDialogState* state);

#ifdef __cplusplus
}
#endif

#endif
