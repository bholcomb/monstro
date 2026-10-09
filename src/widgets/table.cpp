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

   MonstroTableState *state = &ctx->tables.allocate();
   *state = MonstroTableState{};
   state->id = id;
   return state;
}

static float column_offset(MonstroTableState *table, int col)
{
   float x = 0.0f;
   for (int i = 0; i < col && i < table->column_count; i++)
   {
      x += table->columns[i].width;
   }
   return x;
}

bool monstro_begin_table(MonstroContext *ctx, const char *str_id, int column_count, MonstroTableFlags flags,
                         float width, float height)
{
   monstro::requireWindow(ctx, __func__);
   MONSTRO_REQUIRE(column_count >= 1 && column_count <= MONSTRO_MAX_TABLE_COLUMNS,
                   "monstro_begin_table: column_count %d is outside 1..%d", column_count, MONSTRO_MAX_TABLE_COLUMNS);
   MONSTRO_REQUIRE(!ctx->current_table, "monstro_begin_table: tables do not nest; end the open table first");
   if (!str_id)
      return false;
   (void)height;

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style;
   uint64_t id = win->getChildId(str_id);

   MonstroTableState *table = find_or_create_table(ctx, id);
   if (!table)
      return false;

   monstro_add_item(ctx, style->table.cell_padding[0], style->table.cell_padding[1]);

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);

   float table_w = width;
   if (table_w <= 0.0f)
   {
      monstro::Layout *layout = win->getCurrentLayout();
      if (layout && layout->myAvailableWidth > 0.0f)
      {
         table_w = layout->myAvailableWidth;
      }
      else
      {
         table_w = win->size.x - style->window.padding[0] * 2.0f;
      }
   }

   bool first_frame = (table->last_frame_active == 0 || table->column_count != column_count);

   table->flags = flags;
   table->column_count = column_count;
   table->setup_column_count = 0;
   table->current_column = -1;
   table->current_row = -1;
   table->table_x = pos.x;
   table->table_y = pos.y;
   table->table_w = table_w;
   table->row_y = pos.y;
   table->last_frame_active = ctx->frame;

   if (first_frame)
   {
      float col_w = table_w / (float)column_count;
      for (int i = 0; i < column_count; i++)
      {
         table->columns[i].width = col_w;
         table->columns[i].init_width = 0.0f;
         table->columns[i].flags = MONSTRO_TABLE_COLUMN_NONE;
         table->columns[i].label[0] = '\0';
      }
   }

   ctx->current_table = table;
   ctx->id_stack.push(str_id);

   return true;
}

void monstro_end_table(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   MONSTRO_REQUIRE(ctx->current_table, "monstro_end_table without monstro_begin_table");

   monstro::Window *win = ctx->current_window;
   MonstroTableState *table = ctx->current_table;
   MonstroStyle *style = &ctx->style;
   monstro::Canvas *canvas = &win->canvas;

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

      for (int c = 1; c < table->column_count; c++)
      {
         float cx = table->table_x + column_offset(table, c);
         MonstroVec2 p1 = {cx, table->table_y};
         MonstroVec2 p2 = {cx, table->table_y + table_h};
         canvas->addLine(p1, p2, border_col, border_thick);
      }
   }

   monstro_set_cursor_screen_pos(ctx, table->table_x, table->table_y);
   monstro_add_item(ctx, table->table_w, table_h);

   ctx->id_stack.pop();
   ctx->current_table = nullptr;
}

void monstro_table_setup_column(MonstroContext *ctx, const char *label, MonstroTableColumnFlags flags, float init_width)
{
   if (!ctx || !ctx->current_table)
      return;

   MonstroTableState *table = ctx->current_table;
   int idx = table->setup_column_count;
   if (idx >= table->column_count)
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
      col->width = init_width;
   }

   table->setup_column_count++;
}

void monstro_table_headers_row(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   if (!ctx->current_table)
      return;

   monstro::Window *win = ctx->current_window;
   MonstroTableState *table = ctx->current_table;
   MonstroStyle *style = &ctx->style;
   MonstroStyleTable *ts = &style->table;
   monstro::Canvas *canvas = &win->canvas;
   MonstroFont *font = style->font;

   float fixed_total = 0.0f;
   int flex_count = 0;
   for (int i = 0; i < table->column_count; i++)
   {
      if ((table->columns[i].flags & MONSTRO_TABLE_COLUMN_WIDTH_FIXED) && table->columns[i].init_width > 0.0f)
      {
         fixed_total += table->columns[i].width;
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
      for (int i = 0; i < table->column_count; i++)
      {
         if (!((table->columns[i].flags & MONSTRO_TABLE_COLUMN_WIDTH_FIXED) && table->columns[i].init_width > 0.0f))
         {
            table->columns[i].width = flex_w;
         }
      }
   }

   float header_h = ts->header_height;
   MonstroRect header_rect = {table->table_x, table->table_y, table->table_w, header_h};

   if (ts->header_normal.type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addRectFilled(header_rect, monstro_color_to_u32(ts->header_normal.data.color), ts->rounding,
                            MONSTRO_CORNER_TOP);
   }

   float cx = table->table_x;
   uint32_t text_col = monstro_color_to_u32(ts->header_text);
   for (int i = 0; i < table->column_count; i++)
   {
      float col_w = table->columns[i].width;
      if (font && table->columns[i].label[0] != '\0')
      {
         MonstroRect cell_rect = {cx + ts->cell_padding[0], table->table_y + ts->cell_padding[1],
                                  col_w - ts->cell_padding[0] * 2.0f, header_h - ts->cell_padding[1] * 2.0f};
         canvas->addTextEx(font, cell_rect, text_col, table->columns[i].label, MONSTRO_ALIGN_DEFAULT);
      }

      if (table->flags & MONSTRO_TABLE_RESIZABLE)
      {
         if (i < table->column_count - 1 && !(table->columns[i].flags & MONSTRO_TABLE_COLUMN_NO_RESIZE))
         {
            float grip_x = cx + col_w - 3.0f;
            MonstroRect grip_rect = {grip_x, table->table_y, 6.0f, header_h};

            uint64_t grip_id = ctx->id_stack.getId("##col_resize") ^ (uint64_t)(i + 1);
            ctx->keepAliveId(grip_id);

            bool grip_hovered = false;
            if (win == ctx->hovered_window)
            {
               float mx = ctx->mouse.pos_x;
               float my = ctx->mouse.pos_y;
               grip_hovered = mx >= grip_rect.x && mx < grip_rect.x + grip_rect.w && my >= grip_rect.y &&
                              my < grip_rect.y + grip_rect.h;
               if (grip_hovered && ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
               {
                  ctx->setActiveId(grip_id);
               }
            }

            if (ctx->active_id == grip_id)
            {
               if (ctx->mouse.isDown(MONSTRO_MOUSE_BUTTON_LEFT))
               {
                  float delta = ctx->mouse.delta_x;
                  float min_w = ts->cell_padding[0] * 2.0f + 10.0f;
                  float new_w = table->columns[i].width + delta;
                  float next_w = table->columns[i + 1].width - delta;
                  if (new_w >= min_w && next_w >= min_w)
                  {
                     table->columns[i].width = new_w;
                     table->columns[i + 1].width = next_w;
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
   if (!ctx->current_table)
      return;

   MonstroTableState *table = ctx->current_table;
   MonstroStyle *style = &ctx->style;
   MonstroStyleTable *ts = &style->table;
   monstro::Canvas *canvas = &ctx->current_window->canvas;

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
   if (!ctx->current_table)
      return false;

   MonstroTableState *table = ctx->current_table;
   table->current_column++;

   if (table->current_column >= table->column_count)
      return false;

   MonstroStyle *style = &ctx->style;
   MonstroStyleTable *ts = &style->table;

   float cx = table->table_x + column_offset(table, table->current_column) + ts->cell_padding[0];
   float cy = table->row_y + ts->cell_padding[1];

   monstro_set_cursor_screen_pos(ctx, cx, cy);

   return true;
}

bool monstro_table_set_column_index(MonstroContext *ctx, int column)
{
   monstro::requireWindow(ctx, __func__);
   if (!ctx->current_table)
      return false;

   MonstroTableState *table = ctx->current_table;
   if (column < 0 || column >= table->column_count)
      return false;

   table->current_column = column;

   MonstroStyle *style = &ctx->style;
   MonstroStyleTable *ts = &style->table;

   float cx = table->table_x + column_offset(table, column) + ts->cell_padding[0];
   float cy = table->row_y + ts->cell_padding[1];

   monstro_set_cursor_screen_pos(ctx, cx, cy);

   return true;
}
