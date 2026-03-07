#ifndef MONSTRO_WIDGETS_H
#define MONSTRO_WIDGETS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Flags                                                                     */
/* ------------------------------------------------------------------------- */

typedef enum MonstroSelectableFlags {
    MONSTRO_SELECTABLE_NONE             = 0,
    MONSTRO_SELECTABLE_DONT_CLOSE_POPUP = 1 << 0,
    MONSTRO_SELECTABLE_SPAN_ALL_COLUMNS = 1 << 1,
    MONSTRO_SELECTABLE_MENU             = 1 << 2,
    MONSTRO_SELECTABLE_MENU_ITEM        = 1 << 3,
    MONSTRO_SELECTABLE_DISABLED         = 1 << 4,
    MONSTRO_SELECTABLE_DRAW_FILL_WIDTH  = 1 << 5,
    MONSTRO_SELECTABLE_HAS_TOGGLE       = 1 << 6
} MonstroSelectableFlags;

typedef enum MonstroSliderFlags {
    MONSTRO_SLIDER_NONE     = 0,
    MONSTRO_SLIDER_VERTICAL = 1 << 0
} MonstroSliderFlags;

typedef enum MonstroTextboxFlags {
    MONSTRO_TEXTBOX_NONE            = 0,
    MONSTRO_TEXTBOX_READ_ONLY       = 1 << 0,
    MONSTRO_TEXTBOX_PASSWORD        = 1 << 1,
    MONSTRO_TEXTBOX_MULTILINE       = 1 << 2,
    MONSTRO_TEXTBOX_AUTO_SELECT_ALL = 1 << 3,
    MONSTRO_TEXTBOX_ENTER_RETURNS   = 1 << 4
} MonstroTextboxFlags;

typedef enum MonstroTreeFlags {
    MONSTRO_TREE_NONE         = 0,
    MONSTRO_TREE_DEFAULT_OPEN = 1 << 0,
    MONSTRO_TREE_LEAF         = 1 << 1,
    MONSTRO_TREE_SELECTED     = 1 << 2
} MonstroTreeFlags;

typedef enum MonstroTabBarFlags {
    MONSTRO_TAB_BAR_NONE        = 0,
    MONSTRO_TAB_BAR_REORDERABLE = 1 << 0,
    MONSTRO_TAB_BAR_AUTO_SELECT = 1 << 1
} MonstroTabBarFlags;

typedef enum MonstroButtonFlags {
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
/* Button                                                                    */
/* ------------------------------------------------------------------------- */

bool monstro_button(MonstroContext* ctx, const char* label, const char* tooltip);
bool monstro_button_sized(MonstroContext* ctx, const char* label, float width, float height, const char* tooltip);
bool monstro_image_button(MonstroContext* ctx, const char* str_id, uint64_t texture_id, float width, float height, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Label / Text                                                              */
/* ------------------------------------------------------------------------- */

void monstro_label(MonstroContext* ctx, const char* text);
void monstro_label_ex(MonstroContext* ctx, const char* text, uint32_t color);

/* ------------------------------------------------------------------------- */
/* Image                                                                     */
/* ------------------------------------------------------------------------- */

void monstro_image(MonstroContext* ctx, uint64_t texture_id, float width, float height);
void monstro_image_ex(MonstroContext* ctx, uint64_t texture_id, float width, float height,
                      float uv0_x, float uv0_y, float uv1_x, float uv1_y, uint32_t tint_col);

/* ------------------------------------------------------------------------- */
/* Icon                                                                      */
/* ------------------------------------------------------------------------- */

void monstro_icon(MonstroContext* ctx, uint64_t texture_id, float size);
void monstro_icon_colored(MonstroContext* ctx, uint64_t texture_id, float size, uint32_t color);

/* ------------------------------------------------------------------------- */
/* Checkbox                                                                  */
/* ------------------------------------------------------------------------- */

bool monstro_checkbox(MonstroContext* ctx, const char* label, bool* value, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Radio Button                                                              */
/* ------------------------------------------------------------------------- */

bool monstro_radio_button(MonstroContext* ctx, const char* label, int* value, int button_value, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Slider                                                                    */
/* ------------------------------------------------------------------------- */

bool monstro_slider_float(MonstroContext* ctx, const char* label, float* value,
                          float min, float max, const char* format, const char* tooltip);
bool monstro_slider_int(MonstroContext* ctx, const char* label, int* value,
                        int min, int max, const char* format, const char* tooltip);
bool monstro_slider_float_ex(MonstroContext* ctx, const char* label, float* value,
                             float min, float max, const char* format,
                             float width, float height, MonstroSliderFlags flags, const char* tooltip);
bool monstro_slider_int_ex(MonstroContext* ctx, const char* label, int* value,
                           int min, int max, const char* format,
                           float width, float height, MonstroSliderFlags flags, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Knob (Rotary Slider)                                                      */
/* ------------------------------------------------------------------------- */

bool monstro_knob(MonstroContext* ctx, const char* label, float* value,
                  float min, float max, const char* format, const char* tooltip);
bool monstro_knob_sized(MonstroContext* ctx, const char* label, float* value,
                        float min, float max, const char* format, float diameter, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Progress Bar                                                              */
/* ------------------------------------------------------------------------- */

void monstro_progress_bar(MonstroContext* ctx, float fraction, float width, float height, const char* overlay);

/* ------------------------------------------------------------------------- */
/* Textbox                                                                   */
/* ------------------------------------------------------------------------- */

bool monstro_textbox(MonstroContext* ctx, const char* label, char* buf, size_t buf_size,
                     MonstroTextboxFlags flags, const char* tooltip);
bool monstro_textbox_sized(MonstroContext* ctx, const char* label, char* buf, size_t buf_size,
                           float width, float height, MonstroTextboxFlags flags, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Selectable                                                                */
/* ------------------------------------------------------------------------- */

bool monstro_selectable(MonstroContext* ctx, const char* label, bool* selected,
                        MonstroSelectableFlags flags, const char* tooltip);
bool monstro_selectable_sized(MonstroContext* ctx, const char* label, bool* selected,
                              float width, float height, MonstroSelectableFlags flags, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Combo Box                                                                 */
/* ------------------------------------------------------------------------- */

bool monstro_begin_combo(MonstroContext* ctx, const char* label, const char* preview_value, const char* tooltip);
void monstro_end_combo(MonstroContext* ctx);
bool monstro_combo(MonstroContext* ctx, const char* label, int* current_item,
                   const char* const items[], int items_count, const char* tooltip);

/* ------------------------------------------------------------------------- */
/* Menu                                                                      */
/* ------------------------------------------------------------------------- */

bool monstro_begin_menu_bar(MonstroContext* ctx);
void monstro_end_menu_bar(MonstroContext* ctx);
bool monstro_begin_menu(MonstroContext* ctx, const char* label, bool enabled);
void monstro_end_menu(MonstroContext* ctx);
bool monstro_menu_item(MonstroContext* ctx, const char* label, bool* selected, bool enabled);

/* ------------------------------------------------------------------------- */
/* Popup                                                                     */
/* ------------------------------------------------------------------------- */

void monstro_open_popup(MonstroContext* ctx, const char* label);
void monstro_close_current_popup(MonstroContext* ctx);
bool monstro_begin_popup(MonstroContext* ctx, const char* label, MonstroWindowFlags extra_flags);
bool monstro_begin_popup_modal(MonstroContext* ctx, const char* label, bool* p_open, MonstroWindowFlags flags);
bool monstro_begin_popup_context_item(MonstroContext* ctx, const char* label, MonstroMouseButton mouse_button);
bool monstro_begin_popup_context_window(MonstroContext* ctx, const char* label, MonstroMouseButton mouse_button, bool also_over_items);
bool monstro_begin_popup_context_void(MonstroContext* ctx, const char* label, MonstroMouseButton mouse_button);
void monstro_end_popup(MonstroContext* ctx);
bool monstro_is_popup_open(MonstroContext* ctx, const char* str_id);

/* ------------------------------------------------------------------------- */
/* Tree / Collapsible                                                        */
/* ------------------------------------------------------------------------- */

bool monstro_tree_node(MonstroContext* ctx, const char* label, MonstroTreeFlags flags);
bool monstro_tree_node_id(MonstroContext* ctx, uint64_t id, const char* label, MonstroTreeFlags flags);
void monstro_tree_pop(MonstroContext* ctx);
bool monstro_collapsing_header(MonstroContext* ctx, const char* label, bool* p_visible);

float monstro_get_tree_indent(MonstroContext* ctx);
void monstro_set_tree_indent(MonstroContext* ctx, float indent);

/* ------------------------------------------------------------------------- */
/* Tabs                                                                      */
/* ------------------------------------------------------------------------- */

bool monstro_begin_tab_bar(MonstroContext* ctx, const char* str_id, MonstroTabBarFlags flags);
void monstro_end_tab_bar(MonstroContext* ctx);
bool monstro_begin_tab_item(MonstroContext* ctx, const char* label, bool* p_open);
void monstro_end_tab_item(MonstroContext* ctx);

/* ------------------------------------------------------------------------- */
/* Table                                                                     */
/* ------------------------------------------------------------------------- */

typedef enum MonstroTableFlags {
    MONSTRO_TABLE_NONE           = 0,
    MONSTRO_TABLE_BORDERS        = 1 << 0,
    MONSTRO_TABLE_RESIZABLE      = 1 << 1,
    MONSTRO_TABLE_SORTABLE       = 1 << 2,
    MONSTRO_TABLE_ROW_BACKGROUND = 1 << 3,
    MONSTRO_TABLE_SCROLL_X       = 1 << 4,
    MONSTRO_TABLE_SCROLL_Y       = 1 << 5
} MonstroTableFlags;

typedef enum MonstroTableColumnFlags {
    MONSTRO_TABLE_COLUMN_NONE           = 0,
    MONSTRO_TABLE_COLUMN_WIDTH_FIXED    = 1 << 0,
    MONSTRO_TABLE_COLUMN_NO_RESIZE      = 1 << 1,
    MONSTRO_TABLE_COLUMN_NO_SORT        = 1 << 2,
    MONSTRO_TABLE_COLUMN_DEFAULT_HIDDEN = 1 << 3
} MonstroTableColumnFlags;

bool monstro_begin_table(MonstroContext* ctx, const char* str_id, int column_count,
                         MonstroTableFlags flags, float width, float height);
void monstro_end_table(MonstroContext* ctx);

void monstro_table_setup_column(MonstroContext* ctx, const char* label,
                                MonstroTableColumnFlags flags, float init_width);
void monstro_table_headers_row(MonstroContext* ctx);

void monstro_table_next_row(MonstroContext* ctx);
bool monstro_table_next_column(MonstroContext* ctx);
bool monstro_table_set_column_index(MonstroContext* ctx, int column);

/* ------------------------------------------------------------------------- */
/* Separator                                                                 */
/* ------------------------------------------------------------------------- */

void monstro_separator(MonstroContext* ctx);
void monstro_separator_text(MonstroContext* ctx, const char* text);

#ifdef __cplusplus
}
#endif

#endif
