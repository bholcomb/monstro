#ifndef MONSTRO_INTERNAL_UISTATE_H
#define MONSTRO_INTERNAL_UISTATE_H

#include "constants.h"
#include "monstro.h"

/*
 * State records of a UI. The persistent ones (tree nodes, tab bars, tables)
 * live in the context between frames; the per-frame ones (style backups,
 * column sets, the open table's columns) live in the canvas lent for a frame.
 */

enum MonstroStylePushedType
{
   MONSTRO_PUSHED_TEXT,
   MONSTRO_PUSHED_BUTTON,
   MONSTRO_PUSHED_CONTEXTUAL_BUTTON,
   MONSTRO_PUSHED_MENU_BUTTON,
   MONSTRO_PUSHED_OPTION,
   MONSTRO_PUSHED_CHECKBOX,
   MONSTRO_PUSHED_SELECTABLE,
   MONSTRO_PUSHED_SLIDER,
   MONSTRO_PUSHED_KNOB,
   MONSTRO_PUSHED_PROGRESS,
   MONSTRO_PUSHED_PROPERTY,
   MONSTRO_PUSHED_EDIT,
   MONSTRO_PUSHED_TREE,
   MONSTRO_PUSHED_TAB,
   MONSTRO_PUSHED_TABLE,
   MONSTRO_PUSHED_COMBO,
   MONSTRO_PUSHED_MENU,
   MONSTRO_PUSHED_SEPARATOR,
   MONSTRO_PUSHED_TOOLTIP,
   MONSTRO_PUSHED_CHART,
   MONSTRO_PUSHED_SCROLLH,
   MONSTRO_PUSHED_SCROLLV,
   MONSTRO_PUSHED_WINDOW
};

struct MonstroStyleBackup
{
   MonstroStylePushedType type;
   union
   {
      MonstroStyleText text;
      MonstroStyleButton button;
      MonstroStyleToggle toggle;
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
      MonstroStyleScrollbar scrollbar;
      MonstroStyleWindow window;
   } data;
};

struct MonstroTreeNodeState
{
   uint64_t id = 0;
   bool open = false;
   int last_frame_active = 0;
};

// What a table keeps between frames: its column widths, which the user may resize.
struct MonstroTableState
{
   uint64_t id = 0;
   int column_count = 0;
   int last_frame_active = 0;
   float widths[MONSTRO_MAX_TABLE_COLUMNS] = {};
};

struct MonstroTableColumn
{
   char label[128] = {};
   MonstroTableColumnFlags flags = MONSTRO_TABLE_COLUMN_NONE;
   float init_width = 0.0f;
};

// The open table during a frame. Tables do not nest, so a canvas holds one.
struct MonstroTableFrame
{
   MonstroTableState *state = nullptr;
   MonstroTableFlags flags = MONSTRO_TABLE_NONE;
   int setup_column_count = 0;
   int current_column = -1;
   int current_row = -1;
   float table_x = 0.0f, table_y = 0.0f;
   float table_w = 0.0f;
   float row_y = 0.0f;
   // The layout's room before the table, which each cell narrows to its own and monstro_end_table gives back.
   float outer_available_width = 0.0f;
   float outer_visible_max_screen_x = 0.0f;
   MonstroTableColumn columns[MONSTRO_MAX_TABLE_COLUMNS] = {};
};

struct MonstroColumnsState
{
   uint64_t id = 0;
   int column_count = 0;
   int current_column = 0;
   float columns_x = 0.0f;
   float start_y = 0.0f;
   float row_y = 0.0f;
   float columns_w = 0.0f;
   float row_max_h = 0.0f;
   float prev_visible_max_screen_x = 0.0f;
   float widths[MONSTRO_MAX_COLUMNS] = {};
   float offsets[MONSTRO_MAX_COLUMNS] = {};
};

struct MonstroTabBarState
{
   uint64_t id = 0;
   uint64_t selected_tab = 0;
   int last_frame_active = 0;
   float header_start_x = 0.0f;
   float header_start_y = 0.0f;
   float header_height = 0.0f;
   MonstroTabBarFlags flags = MONSTRO_TAB_BAR_NONE;
   uint64_t first_tab = 0;
   bool selected_tab_present = false;
   bool content_active = false;
};

#endif
