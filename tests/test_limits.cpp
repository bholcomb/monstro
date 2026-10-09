// Capacities are fixed and fail hard; misuse and unbalanced pairs fail hard.
#include "harness.h"
#include <cstdio>
#include <cstring>

static MonstroCanvas *root_canvas(Ui &ui) { return monstro_get_window_canvas(ui.ctx); }

static void fill_rects(MonstroCanvas *canvas, int count)
{
   for (int i = 0; i < count; i++)
   {
      MonstroRect rect = {(float)(i % 100), (float)(i / 100), 1.0f, 1.0f};
      monstro_draw_rect_filled(canvas, rect, 0xffffffff, 0.0f, MONSTRO_CORNER_NONE);
   }
}

TEST(config_defaults_match_documented_capacities)
{
   MonstroContextConfig config = monstro_context_config_default();
   CHECK(config.max_vertices == 65536);
   CHECK(config.max_indices == 65536 * 3);
   CHECK(config.max_draw_commands == 4096);
   CHECK(config.max_path_points == 1024);
}

TEST(drawing_up_to_capacity_succeeds)
{
   MonstroContextConfig config = monstro_context_config_default();
   config.max_vertices = 4096;
   Ui ui(false, &config);
   ui.begin();
   int used = (int)ui.vertexCount();
   fill_rects(root_canvas(ui), (4096 - used) / 4);
   ui.end();
   CHECK(ui.vertexCount() <= 4096);
   CHECK(ui.vertexCount() > 4000);
}

FATAL_TEST(vertex_buffer_overflow_aborts, "vertex buffer overflow: capacity 65536, needed 65540",
           "(window 'root', frame 1)", "raise MonstroContextConfig.max_vertices")
{
   Ui ui(false);
   ui.begin();
   fill_rects(root_canvas(ui), 65536 / 4 + 1);
}

FATAL_TEST(configured_vertex_capacity_is_enforced, "vertex buffer overflow: capacity 100", "(window 'Status', frame 1)")
{
   MonstroContextConfig config = monstro_context_config_default();
   config.max_vertices = 100;
   Ui ui(false, &config);
   ui.begin();
   monstro_begin_window(ui.ctx, "Status", nullptr, MONSTRO_WINDOW_DEFAULT);
   fill_rects(monstro_get_window_canvas(ui.ctx), 100);
}

FATAL_TEST(caller_vertex_buffer_overflow_aborts, "vertex buffer overflow: capacity 16, needed 20",
           "pass a larger buffer to monstro_draw_buffer_set_vertex_buffer")
{
   Ui ui(false);
   static MonstroVertex vertices[16];
   monstro_draw_buffer_set_vertex_buffer(monstro_context_get_draw_buffer(ui.ctx), vertices, 16);
   ui.begin();
   fill_rects(root_canvas(ui), 5);
}

FATAL_TEST(caller_index_buffer_overflow_aborts, "index buffer overflow: capacity 12, needed 18",
           "pass a larger buffer to monstro_draw_buffer_set_index_buffer")
{
   Ui ui(false);
   static uint32_t indices[12];
   monstro_draw_buffer_set_index_buffer(monstro_context_get_draw_buffer(ui.ctx), indices, 12);
   ui.begin();
   fill_rects(root_canvas(ui), 3);
}

TEST(clearing_a_caller_buffer_restores_the_context_buffer)
{
   Ui ui(false);
   MonstroDrawBuffer *buffer = monstro_context_get_draw_buffer(ui.ctx);
   static MonstroVertex vertices[64];
   monstro_draw_buffer_set_vertex_buffer(buffer, vertices, 64);
   monstro_draw_buffer_set_vertex_buffer(buffer, nullptr, 0);
   ui.begin();
   fill_rects(root_canvas(ui), 100);
   ui.end();
   CHECK(ui.vertexCount() >= 400);
}

FATAL_TEST(draw_command_overflow_aborts, "draw command list overflow: capacity 8, needed 9", "window 'root'",
           "raise MonstroContextConfig.max_draw_commands")
{
   MonstroContextConfig config = monstro_context_config_default();
   config.max_draw_commands = 8;
   Ui ui(false, &config);
   ui.begin();
   MonstroCanvas *canvas = root_canvas(ui);
   for (int i = 0; i < 9; i++)
   {
      monstro_push_clip_rect(canvas, {(float)i, 0.0f, 10.0f, 10.0f});
      fill_rects(canvas, 1);
      monstro_pop_clip_rect(canvas);
   }
}

FATAL_TEST(frame_draw_command_overflow_aborts, "draw command overflow at frame 1: merging window")
{
   MonstroContextConfig config = monstro_context_config_default();
   config.max_draw_commands = 4;
   Ui ui(false, &config);
   ui.begin();
   const char *names[] = {"A", "B", "C", "D", "E"};
   for (const char *name : names)
   {
      monstro_begin_window(ui.ctx, name, nullptr, MONSTRO_WINDOW_BACKGROUND);
      monstro_end_window(ui.ctx);
   }
   ui.end();
}

FATAL_TEST(path_overflow_aborts, "path overflow: capacity 8, needed 9", "max_path_points")
{
   MonstroContextConfig config = monstro_context_config_default();
   config.max_path_points = 8;
   Ui ui(false, &config);
   ui.begin();
   MonstroCanvas *canvas = root_canvas(ui);
   monstro_path_move_to(canvas, {0, 0});
   for (int i = 0; i < 8; i++)
      monstro_path_line_to(canvas, {(float)i, (float)i});
}

FATAL_TEST(config_too_small_aborts, "MonstroContextConfig capacities too small")
{
   MonstroContextConfig config = monstro_context_config_default();
   config.max_draw_commands = 0;
   monstro_context_create_ex(&config);
}

FATAL_TEST(id_stack_overflow_aborts, "ID stack overflow: capacity 64")
{
   Ui ui(false);
   ui.begin();
   for (int i = 0; i < 64; i++)
      monstro_push_id(ui.ctx, "deep");
}

FATAL_TEST(id_stack_underflow_aborts, "ID stack underflow")
{
   Ui ui(false);
   monstro_pop_id(ui.ctx);
}

FATAL_TEST(unpopped_id_aborts_at_end_frame, "unbalanced begin/end or push/pop pairs at monstro_end_frame", "1 IDs")
{
   Ui ui(false);
   ui.begin();
   monstro_push_id(ui.ctx, "leak");
   ui.end();
}

FATAL_TEST(begin_frame_twice_aborts, "monstro_begin_frame called during frame 1")
{
   Ui ui(false);
   ui.begin();
   ui.begin();
}

FATAL_TEST(end_frame_without_begin_aborts, "monstro_end_frame called without monstro_begin_frame")
{
   Ui ui(false);
   ui.end();
}

FATAL_TEST(unended_window_aborts_at_end_frame, "window 'Open' is still open at the end of frame 1")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_window(ui.ctx, "Open", nullptr, MONSTRO_WINDOW_DEFAULT);
   ui.end();
}

FATAL_TEST(end_window_without_begin_aborts, "monstro_end_window without a matching monstro_begin_window")
{
   Ui ui(false);
   ui.begin();
   monstro_end_window(ui.ctx);
}

FATAL_TEST(window_begun_twice_while_open_aborts, "window 'Twice' is begun again while it is still open")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_window(ui.ctx, "Twice", nullptr, MONSTRO_WINDOW_DEFAULT);
   monstro_begin_window(ui.ctx, "Twice", nullptr, MONSTRO_WINDOW_DEFAULT);
}

FATAL_TEST(widget_outside_frame_aborts, "monstro_button_ex called outside a frame")
{
   Ui ui(false);
   monstro_button(ui.ctx, "early", nullptr);
}

FATAL_TEST(null_context_aborts, "monstro_label: the context must not be null") { monstro_label(nullptr, "text"); }

FATAL_TEST(is_popup_open_outside_frame_aborts, "monstro_is_popup_open called outside a frame")
{
   Ui ui(false);
   monstro_is_popup_open(ui.ctx, "menu");
}

FATAL_TEST(style_stack_overflow_aborts, "style stack overflow: capacity 64")
{
   Ui ui(false);
   ui.begin();
   MonstroStyleButton button = *monstro_get_button_style(ui.ctx);
   for (int i = 0; i < 65; i++)
      monstro_push_button_style(ui.ctx, &button);
}

FATAL_TEST(style_stack_underflow_aborts, "style stack underflow")
{
   Ui ui(false);
   ui.begin();
   MonstroStyleButton button = *monstro_get_button_style(ui.ctx);
   monstro_push_button_style(ui.ctx, &button);
   monstro_pop_style(ui.ctx, 2);
}

TEST(style_push_and_pop_restore_in_order)
{
   Ui ui(false);
   ui.begin();
   MonstroStyleButton original = *monstro_get_button_style(ui.ctx);
   MonstroStyleButton first = original;
   first.rounding = 11.0f;
   MonstroStyleButton second = original;
   second.rounding = 22.0f;
   monstro_push_button_style(ui.ctx, &first);
   monstro_push_button_style(ui.ctx, &second);
   CHECK(monstro_get_button_style(ui.ctx)->rounding == 22.0f);
   monstro_pop_style(ui.ctx, 1);
   CHECK(monstro_get_button_style(ui.ctx)->rounding == 11.0f);
   monstro_pop_style(ui.ctx, 1);
   CHECK(monstro_get_button_style(ui.ctx)->rounding == original.rounding);
   ui.end();
}

FATAL_TEST(unpopped_style_aborts_at_end_frame, "1 styles")
{
   Ui ui(false);
   ui.begin();
   MonstroStyleButton button = *monstro_get_button_style(ui.ctx);
   monstro_push_button_style(ui.ctx, &button);
   ui.end();
}

TEST(nested_disabled_scopes_restore_the_enclosing_state)
{
   Ui ui(false);
   ui.begin();
   monstro_begin_disabled(ui.ctx, false);
   CHECK(!monstro_is_disabled(ui.ctx));
   monstro_begin_disabled(ui.ctx, true);
   CHECK(monstro_is_disabled(ui.ctx));
   monstro_begin_disabled(ui.ctx, false);
   CHECK(monstro_is_disabled(ui.ctx));
   monstro_end_disabled(ui.ctx);
   CHECK(monstro_is_disabled(ui.ctx));
   monstro_end_disabled(ui.ctx);
   CHECK(!monstro_is_disabled(ui.ctx));
   monstro_end_disabled(ui.ctx);
   CHECK(!monstro_is_disabled(ui.ctx));
   ui.end();
}

FATAL_TEST(disabled_underflow_aborts, "disabled stack underflow")
{
   Ui ui(false);
   ui.begin();
   monstro_end_disabled(ui.ctx);
}

FATAL_TEST(unended_disabled_aborts_at_end_frame, "1 disabled scopes")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_disabled(ui.ctx, true);
   ui.end();
}

FATAL_TEST(layout_stack_overflow_aborts, "layout stack overflow: capacity 32")
{
   Ui ui(false);
   ui.begin();
   for (int i = 0; i < 40; i++)
      monstro_begin_layout(ui.ctx, MONSTRO_LAYOUT_VERTICAL);
}

FATAL_TEST(unended_layout_aborts_at_window_end, "window 'Layouts' ends with 3 layouts open (expected 2)")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_window(ui.ctx, "Layouts", nullptr, MONSTRO_WINDOW_DEFAULT);
   monstro_begin_layout(ui.ctx, MONSTRO_LAYOUT_HORIZONTAL);
   monstro_end_window(ui.ctx);
}

FATAL_TEST(extra_end_layout_aborts_at_window_end, "window 'Layouts' ends with 1 layouts open (expected 2)")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_window(ui.ctx, "Layouts", nullptr, MONSTRO_WINDOW_DEFAULT);
   monstro_end_layout(ui.ctx);
   monstro_end_window(ui.ctx);
}

FATAL_TEST(unended_group_aborts_at_window_end, "1 groups open")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_window(ui.ctx, "Groups", nullptr, MONSTRO_WINDOW_DEFAULT);
   monstro_begin_group(ui.ctx, MONSTRO_LAYOUT_VERTICAL, MONSTRO_GROUP_DEFAULT);
   monstro_begin_layout(ui.ctx, MONSTRO_LAYOUT_VERTICAL);
   monstro_end_layout(ui.ctx);
   monstro_end_layout(ui.ctx);
   monstro_end_window(ui.ctx);
}

FATAL_TEST(unpopped_clip_rect_aborts_at_window_end, "window 'Clips' ends with unbalanced canvas stacks: 2 clip rects")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_window(ui.ctx, "Clips", nullptr, MONSTRO_WINDOW_DEFAULT);
   monstro_push_clip_rect(monstro_get_window_canvas(ui.ctx), {0, 0, 10, 10});
   monstro_end_window(ui.ctx);
}

FATAL_TEST(clip_rect_underflow_aborts, "clip rect stack underflow")
{
   Ui ui(false);
   ui.begin();
   MonstroCanvas *canvas = root_canvas(ui);
   for (int i = 0; i < 4; i++)
      monstro_pop_clip_rect(canvas);
}

FATAL_TEST(window_table_full_aborts, "window table full: all 64 slots are in use")
{
   Ui ui(false);
   ui.begin();
   char name[32];
   for (int i = 0; i < 64; i++)
   {
      snprintf(name, sizeof(name), "Window %d", i);
      monstro_begin_window(ui.ctx, name, nullptr, MONSTRO_WINDOW_DEFAULT);
      monstro_end_window(ui.ctx);
   }
}

FATAL_TEST(tree_node_table_full_aborts, "tree node table full: all 256 slots are in use")
{
   Ui ui(false);
   ui.begin();
   char name[32];
   for (int i = 0; i < 257; i++)
   {
      snprintf(name, sizeof(name), "node %d", i);
      monstro_begin_tree_node(ui.ctx, name, MONSTRO_TREE_NONE);
   }
}

FATAL_TEST(tab_bar_table_full_aborts, "tab bar table full: all 32 slots are in use")
{
   Ui ui(false);
   ui.begin();
   char name[32];
   for (int i = 0; i < 33; i++)
   {
      snprintf(name, sizeof(name), "bar %d", i);
      if (monstro_begin_tab_bar(ui.ctx, name, MONSTRO_TAB_BAR_NONE))
         monstro_end_tab_bar(ui.ctx);
   }
}

FATAL_TEST(table_state_table_full_aborts, "table state table full: all 32 slots are in use")
{
   Ui ui(false);
   ui.begin();
   char name[32];
   for (int i = 0; i < 33; i++)
   {
      snprintf(name, sizeof(name), "table %d", i);
      if (monstro_begin_table(ui.ctx, name, 2, MONSTRO_TABLE_NONE, 0, 0))
         monstro_end_table(ui.ctx);
   }
}

FATAL_TEST(popup_stack_full_aborts, "popup stack overflow: capacity 8")
{
   Ui ui(false);
   ui.begin();
   char name[32];
   for (int i = 0; i < 9; i++)
   {
      snprintf(name, sizeof(name), "modal %d", i);
      monstro_open_popup_ex(ui.ctx, name, true);
   }
}

FATAL_TEST(columns_count_out_of_range_aborts, "monstro_begin_columns: count 17 is outside 1..16")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_columns(ui.ctx, "cols", 17);
}

FATAL_TEST(unended_columns_abort_at_end_frame, "1 column sets")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_columns(ui.ctx, "cols", 2);
   ui.end();
}

FATAL_TEST(table_column_count_above_maximum_aborts, "monstro_begin_table: column_count 129 is outside 1..128")
{
   Ui ui(false);
   ui.begin();
   monstro_begin_table(ui.ctx, "wide", 129, MONSTRO_TABLE_NONE, 0, 0);
}

TEST(table_with_maximum_columns_works)
{
   Ui ui(true);
   ui.begin();
   if (monstro_begin_table(ui.ctx, "wide", 128, (MonstroTableFlags)(MONSTRO_TABLE_BORDERS | MONSTRO_TABLE_RESIZABLE), 0,
                           0))
   {
      for (int i = 0; i < 130; i++)
         monstro_table_setup_column(ui.ctx, "col", MONSTRO_TABLE_COLUMN_NONE, 0.0f);
      monstro_table_headers_row(ui.ctx);
      monstro_table_next_row(ui.ctx);
      int columns = 0;
      while (monstro_table_next_column(ui.ctx))
         columns++;
      CHECK(columns == 128);
      CHECK(!monstro_table_set_column_index(ui.ctx, 128));
      monstro_end_table(ui.ctx);
   }
   ui.end();
}

FATAL_TEST(end_table_without_begin_aborts, "monstro_end_table without monstro_begin_table")
{
   Ui ui(false);
   ui.begin();
   monstro_end_table(ui.ctx);
}

TEST(popup_begins_and_ends_inside_its_parent)
{
   Ui ui(true);
   for (int frame = 0; frame < 3; frame++)
   {
      ui.begin();
      monstro_begin_window(ui.ctx, "Parent", nullptr, MONSTRO_WINDOW_DEFAULT);
      if (frame == 0)
         monstro_open_popup(ui.ctx, "Context");
      CHECK(monstro_is_popup_open(ui.ctx, "Context"));
      if (monstro_begin_popup(ui.ctx, "Context", MONSTRO_WINDOW_NONE))
      {
         monstro_label(ui.ctx, "inside");
         monstro_end_popup(ui.ctx);
      }
      monstro_label(ui.ctx, "after the popup, still in Parent");
      monstro_end_window(ui.ctx);
      ui.end();
   }
}

TEST(modal_closed_by_escape_leaves_the_parent_open)
{
   Ui ui(true);
   bool open = true;
   for (int frame = 0; frame < 3; frame++)
   {
      ui.pressKey(MONSTRO_KEY_ESCAPE, frame == 1);
      ui.begin();
      monstro_begin_window(ui.ctx, "Parent", nullptr, MONSTRO_WINDOW_DEFAULT);
      if (frame == 0)
         monstro_open_popup_ex(ui.ctx, "Modal", true);
      if (monstro_begin_popup_modal(ui.ctx, "Modal", &open, MONSTRO_WINDOW_NONE))
      {
         monstro_label(ui.ctx, "modal body");
         monstro_end_popup(ui.ctx);
      }
      monstro_label(ui.ctx, "parent body");
      monstro_end_window(ui.ctx);
      ui.end();
   }
   CHECK(!open);
}
