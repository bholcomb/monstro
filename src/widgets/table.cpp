#include "../context.h"
#include <cstdio>
#include <cstring>

static MonstroTableState *find_or_create_table(MonstroContext *ctx, uint64_t id)
{
   for (MonstroTableState &existing : ctx->tables)
   {
      if (existing.id == id)
      {
         return &existing;
      }
   }

   MonstroTableState *state = &ctx->tables.add();
   state->id = id;
   return state;
}

static float column_offset(MonstroTableFrame *table, int col)
{
   float x = 0.0f;
   for (int i = 0; i < col && i < table->state->column_count; i++)
   {
      x += table->state->widths[i];
   }
   return x;
}

// Put the cursor in a cell of the current row and narrow the layout's room to the cell, so that widgets which fill
// the room they are given (text and number fields, sliders, combos) fill the cell, not the rest of the window.
static void enter_cell(MonstroContext *ctx, MonstroTableFrame *table, int column)
{
   MonstroStyleTable *ts = &ctx->style().table;
   float cell_x = table->table_x + column_offset(table, column) + ts->cell_padding[0];
   float cell_w = table->state->widths[column] - ts->cell_padding[0] * 2.0f;
   if (cell_w < 0.0f)
      cell_w = 0.0f;

   monstro_set_cursor_screen_pos(ctx, cell_x, table->row_y + ts->cell_padding[1]);
   monstro::Layout *layout = ctx->getCurrentLayout();
   if (layout)
      layout->myContentRight = layout->myPosX + layout->myCursorX + cell_w;
}

// The open table, for calls that belong between monstro_begin_table and monstro_end_table.
static MonstroTableFrame *require_table(MonstroContext *ctx, const char *function)
{
   MonstroTableFrame *table = &ctx->canvas->table;
   MONSTRO_REQUIRE(table->state, "%s outside a table: call it between monstro_begin_table and monstro_end_table",
                   function);
   return table;
}

bool monstro_begin_table(MonstroContext *ctx, const char *str_id, int column_count, MonstroTableFlags flags,
                         float width, float height)
{
   monstro::Window *win = monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(str_id)});
   MONSTRO_REQUIRE(column_count >= 1 && column_count <= MONSTRO_MAX_TABLE_COLUMNS,
                   "monstro_begin_table: column_count %d is outside 1..%d", column_count, MONSTRO_MAX_TABLE_COLUMNS);
   MONSTRO_REQUIRE(!ctx->canvas->table.state, "monstro_begin_table: tables do not nest; end the open table first");
   (void)height;

   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style();
   uint64_t id = win->getChildId(str_id);

   MonstroTableState *state = find_or_create_table(ctx, id);

   monstro_add_item(ctx, style->table.cell_padding[0], style->table.cell_padding[1]);

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);

   float table_w = width;
   if (table_w <= 0.0f)
   {
      monstro::Layout *layout = win->getCurrentLayout();
      if (layout && layout->fillWidth() > 0.0f)
      {
         table_w = layout->fillWidth();
      }
      else
      {
         table_w = win->size.x - style->window.padding[0] * 2.0f;
      }
   }

   if (state->last_frame_active == 0 || state->column_count != column_count)
   {
      float col_w = table_w / (float)column_count;
      for (int i = 0; i < column_count; i++)
         state->widths[i] = col_w;
   }
   state->column_count = column_count;
   state->last_frame_active = ctx->frame;

   MonstroTableFrame *table = &ctx->canvas->table;
   table->state = state;
   table->flags = flags;
   table->setup_column_count = 0;
   table->current_column = -1;
   table->current_row = -1;
   table->table_x = pos.x;
   table->table_y = pos.y;
   table->table_w = table_w;
   table->row_y = pos.y;
   monstro::Layout *layout = win->getCurrentLayout();
   table->outer_content_right = layout ? layout->myContentRight : 0.0f;
   for (int i = 0; i < column_count; i++)
      table->columns[i] = MonstroTableColumn{};

   ctx->ids().push(str_id);

   return true;
}

void monstro_end_table(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   MONSTRO_REQUIRE(ctx->canvas->table.state, "monstro_end_table without monstro_begin_table");

   MonstroTableFrame *table = &ctx->canvas->table;
   MonstroStyle *style = &ctx->style();
   monstro::Canvas *canvas = ctx->canvas;

   float total_rows = (float)(table->current_row + 1);
   float header_h = style->table.header_height;
   float row_h = style->table.row_height;
   float table_h = header_h + total_rows * row_h;

   if (table->flags & MONSTRO_TABLE_BORDERS)
   {
      MonstroRect outer = {table->table_x, table->table_y, table->table_w, table_h};
      uint32_t border_col = monstro_color_to_u32(style->table.border_color);
      float border_thick = style->table.border;

      canvas->addRect(outer, border_col, style->table.rounding, MONSTRO_CORNER_ALL, border_thick);

      for (int r = 0; r <= (int)total_rows; r++)
      {
         float ly = table->table_y + header_h + (float)r * row_h;
         if (ly < table->table_y + table_h)
         {
            MonstroVec2 p1 = {table->table_x, ly};
            MonstroVec2 p2 = {table->table_x + table->table_w, ly};
            canvas->addLine(p1, p2, border_col, border_thick);
         }
      }

      for (int c = 1; c < table->state->column_count; c++)
      {
         float cx = table->table_x + column_offset(table, c);
         MonstroVec2 p1 = {cx, table->table_y};
         MonstroVec2 p2 = {cx, table->table_y + table_h};
         canvas->addLine(p1, p2, border_col, border_thick);
      }
   }

   monstro::Layout *layout = ctx->getCurrentLayout();
   if (layout)
   {
      layout->myContentRight = table->outer_content_right;
   }
   monstro_set_cursor_screen_pos(ctx, table->table_x, table->table_y);
   monstro_add_item(ctx, table->table_w, table_h);

   ctx->ids().pop();
   table->state = nullptr;
}

void monstro_table_setup_column(MonstroContext *ctx, const char *label, MonstroTableColumnFlags flags, float init_width)
{
   monstro::requireWindow(ctx, __func__);
   MonstroTableFrame *table = require_table(ctx, __func__);
   int idx = table->setup_column_count;
   if (idx >= table->state->column_count)
      return;

   MonstroTableColumn *col = &table->columns[idx];
   if (label)
   {
      monstro_utf8_copy(col->label, sizeof(col->label), label);
   }
   else
   {
      col->label[0] = '\0';
   }
   col->flags = flags;
   col->init_width = init_width;

   if (init_width > 0.0f && (flags & MONSTRO_TABLE_COLUMN_WIDTH_FIXED))
   {
      table->state->widths[idx] = init_width;
   }

   table->setup_column_count++;
}

void monstro_table_headers_row(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   MonstroTableFrame *table = require_table(ctx, __func__);
   int column_count = table->state->column_count;
   float *widths = table->state->widths;
   MonstroStyle *style = &ctx->style();
   MonstroStyleTable *ts = &style->table;
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;

   float fixed_total = 0.0f;
   int flex_count = 0;
   for (int i = 0; i < column_count; i++)
   {
      if ((table->columns[i].flags & MONSTRO_TABLE_COLUMN_WIDTH_FIXED) && table->columns[i].init_width > 0.0f)
      {
         fixed_total += widths[i];
      }
      else
      {
         flex_count++;
      }
   }
   if (flex_count > 0)
   {
      float remaining = table->table_w - fixed_total;
      if (remaining < 0.0f)
         remaining = 0.0f;
      float flex_w = remaining / (float)flex_count;
      for (int i = 0; i < column_count; i++)
      {
         if (!((table->columns[i].flags & MONSTRO_TABLE_COLUMN_WIDTH_FIXED) && table->columns[i].init_width > 0.0f))
         {
            widths[i] = flex_w;
         }
      }
   }

   float header_h = ts->header_height;
   MonstroRect header_rect = {table->table_x, table->table_y, table->table_w, header_h};

   canvas->addStyleItem(ts->header_normal, header_rect, ts->rounding, MONSTRO_CORNER_TOP);

   float cx = table->table_x;
   uint32_t text_col = monstro_color_to_u32(ts->header_text);
   for (int i = 0; i < column_count; i++)
   {
      float col_w = widths[i];
      if (font && table->columns[i].label[0] != '\0')
      {
         MonstroRect cell_rect = {cx + ts->cell_padding[0], table->table_y + ts->cell_padding[1],
                                  col_w - ts->cell_padding[0] * 2.0f, header_h - ts->cell_padding[1] * 2.0f};
         canvas->addTextEx(font, cell_rect, text_col, table->columns[i].label, MONSTRO_ALIGN_DEFAULT);
      }

      if (table->flags & MONSTRO_TABLE_RESIZABLE)
      {
         if (i < column_count - 1 && !(table->columns[i].flags & MONSTRO_TABLE_COLUMN_NO_RESIZE))
         {
            float grip_x = cx + col_w - 3.0f;
            MonstroRect grip_rect = {grip_x, table->table_y, 6.0f, header_h};

            uint64_t grip_id = ctx->ids().getId("##col_resize") ^ (uint64_t)(i + 1);
            ctx->keepAliveId(grip_id);

            bool grip_hovered = ctx->hoverItem(grip_id, grip_rect);
            if (grip_hovered && ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
            {
               ctx->setActiveId(grip_id);
            }

            if (ctx->active_id == grip_id)
            {
               if (ctx->mouse.isDown(MONSTRO_MOUSE_BUTTON_LEFT))
               {
                  float delta = ctx->mouse.getHeldDelta(MONSTRO_MOUSE_BUTTON_LEFT).x;
                  float min_w = ts->cell_padding[0] * 2.0f + 10.0f;
                  float new_w = widths[i] + delta;
                  float next_w = widths[i + 1] - delta;
                  if (new_w >= min_w && next_w >= min_w)
                  {
                     widths[i] = new_w;
                     widths[i + 1] = next_w;
                  }
               }
               else
               {
                  ctx->setActiveId(0);
               }
            }
         }
      }

      cx += col_w;
   }

   table->row_y = table->table_y + header_h;
}

void monstro_table_next_row(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   MonstroTableFrame *table = require_table(ctx, __func__);
   MonstroStyle *style = &ctx->style();
   MonstroStyleTable *ts = &style->table;
   monstro::Canvas *canvas = ctx->canvas;

   table->current_row++;
   table->current_column = -1;

   float row_h = ts->row_height;
   float ry = table->table_y + ts->header_height + (float)table->current_row * row_h;
   table->row_y = ry;

   if (table->flags & MONSTRO_TABLE_ROW_BACKGROUND)
   {
      MonstroColor row_col = (table->current_row % 2 == 0) ? ts->row_color_even : ts->row_color_odd;
      uint32_t col32 = monstro_color_to_u32(row_col);
      if ((col32 & 0xFF000000) != 0)
      {
         MonstroRect row_rect = {table->table_x, ry, table->table_w, row_h};
         canvas->addRectFilled(row_rect, col32, 0.0f, MONSTRO_CORNER_NONE);
      }
   }
}

bool monstro_table_next_column(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   MonstroTableFrame *table = require_table(ctx, __func__);
   table->current_column++;

   if (table->current_column >= table->state->column_count)
      return false;

   enter_cell(ctx, table, table->current_column);
   return true;
}

bool monstro_table_set_column_index(MonstroContext *ctx, int column)
{
   monstro::requireWindow(ctx, __func__);
   MonstroTableFrame *table = require_table(ctx, __func__);
   if (column < 0 || column >= table->state->column_count)
      return false;

   table->current_column = column;
   enter_cell(ctx, table, column);
   return true;
}
