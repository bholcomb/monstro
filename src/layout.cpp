#include "context.h"
#include <algorithm>

/* ------------------------------------------------------------------------- */
/* Layout member functions                                                   */
/* ------------------------------------------------------------------------- */

namespace monstro
{

void Layout::init(MonstroLayoutDirection direction, float posX, float posY)
{
   myDirection = direction;
   myPosX = posX;
   myPosY = posY;
   mySizeX = 0.0f;
   mySizeY = 0.0f;
   myCursorX = 0.0f;
   myCursorY = 0.0f;
   myMaxX = 0.0f;
   myMaxY = 0.0f;
   myScrollOffsetX = 0.0f;
   myScrollOffsetY = 0.0f;
   myAvailableWidth = 0.0f;
   myAvailableHeight = 0.0f;
   myVisibleMaxScreenX = 0.0f;
   myVisibleMaxScreenY = 0.0f;
   myLastItemX = 0.0f;
   myLastItemY = 0.0f;
   myLastItemW = 0.0f;
   myLastItemH = 0.0f;
}

void Layout::addItem(float width, float height)
{
   myLastItemX = myCursorX;
   myLastItemY = myCursorY;
   myLastItemW = width;
   myLastItemH = height;
   mySizeX = std::max(mySizeX, myCursorX + width);
   mySizeY = std::max(mySizeY, myCursorY + height);

   if (myDirection == MONSTRO_LAYOUT_VERTICAL)
   {
      myCursorY += height;
   }
   else
   {
      myCursorX += width;
   }
}


} // namespace monstro

/* ------------------------------------------------------------------------- */
/* Layout C API - delegates to window methods (matching C# architecture)     */
/* ------------------------------------------------------------------------- */

void monstro_begin_layout(MonstroContext *ctx, MonstroLayoutDirection direction)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->beginLayout(direction);
}

void monstro_begin_layout_at(MonstroContext *ctx, float x, float y, MonstroLayoutDirection direction)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->beginLayout(x, y, direction);
}

void monstro_end_layout(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->endLayout();
}

void monstro_add_item(MonstroContext *ctx, float width, float height)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->addItem(width, height);
}

const char *monstro_display_text(const char *label)
{
   if (!label)
      return label;
   for (const char *p = label; *p; p++)
   {
      if (p[0] == '#' && p[1] == '#')
         return p + 2;
   }
   return label;
}

void monstro_copy_id_text(const char *label, char *out, size_t out_size)
{
   if (!out || out_size == 0)
      return;

   out[0] = '\0';
   if (!label)
      return;

   size_t len = 0;
   while (label[len] != '\0')
   {
      if (label[len] == '#' && label[len + 1] == '#')
         break;
      if (len >= out_size - 1)
         break;
      out[len] = label[len];
      len++;
   }
   out[len] = '\0';
}

MonstroVec2 monstro_get_cursor_pos(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return {0.0f, 0.0f};
   return ctx->current_window->getCursorPos();
}

void monstro_set_cursor_pos(MonstroContext *ctx, float x, float y)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->setCursorPos(x, y);
}

MonstroVec2 monstro_get_cursor_screen_pos(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return {0.0f, 0.0f};
   return ctx->current_window->getCursorScreenPos();
}

void monstro_set_cursor_screen_pos(MonstroContext *ctx, float x, float y)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->setCursorScreenPos(x, y);
}

MonstroVec2 monstro_get_content_region_avail(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return {0.0f, 0.0f};

   monstro::Layout *layout = ctx->getCurrentLayout();
   if (!layout)
      return {0.0f, 0.0f};

   MonstroVec2 cursor = ctx->current_window->getCursorScreenPos();
   float avail_w = layout->myVisibleMaxScreenX - cursor.x;
   float avail_h = layout->myVisibleMaxScreenY - cursor.y;

   if (avail_w < 0.0f)
      avail_w = 0.0f;
   if (avail_h < 0.0f)
      avail_h = 0.0f;

   return {avail_w, avail_h};
}

void monstro_indent(MonstroContext *ctx, float indent_width)
{
   monstro::Layout *layout = ctx ? ctx->getCurrentLayout() : nullptr;
   if (!layout)
      return;

   if (indent_width <= 0.0f)
   {
      indent_width = ctx->style.tree.indent;
   }

   layout->myPosX += indent_width;
   layout->myCursorX += indent_width;
   layout->mySizeX -= indent_width;
}

void monstro_unindent(MonstroContext *ctx, float indent_width)
{
   monstro::Layout *layout = ctx ? ctx->getCurrentLayout() : nullptr;
   if (!layout)
      return;

   if (indent_width <= 0.0f)
   {
      indent_width = ctx->style.tree.indent;
   }

   layout->myPosX -= indent_width;
   layout->myCursorX -= indent_width;
   layout->mySizeX += indent_width;
}

void monstro_spacer(MonstroContext *ctx, float size)
{
   monstro::Layout *layout = ctx ? ctx->getCurrentLayout() : nullptr;
   if (!layout)
      return;

   if (layout->myDirection == MONSTRO_LAYOUT_VERTICAL)
   {
      layout->myCursorY += size;
   }
   else
   {
      layout->myCursorX += size;
   }
}

void monstro_dummy(MonstroContext *ctx, float width, float height)
{
   if (!ctx || !ctx->current_window)
      return;
   monstro_add_item(ctx, width, height);
}


float monstro_percent(MonstroContext *ctx, float percent, MonstroLayoutDirection direction)
{
   monstro::Layout *layout = ctx ? ctx->getCurrentLayout() : nullptr;
   if (!layout)
      return 0.0f;

   if (direction == MONSTRO_LAYOUT_HORIZONTAL)
   {
      return layout->myAvailableWidth * (percent / 100.0f);
   }
   else
   {
      return layout->myAvailableHeight * (percent / 100.0f);
   }
}

/* ------------------------------------------------------------------------- */
/* Columns                                                                   */
/* ------------------------------------------------------------------------- */

void monstro_begin_columns_ex(MonstroContext *ctx, const char *str_id, int count, const float *widths)
{
   if (!ctx || !ctx->current_window || count < 1)
      return;
   if (ctx->columns_stack_top >= MONSTRO_MAX_COLUMN_STACK)
      return;
   if (count > MONSTRO_MAX_COLUMNS)
      count = MONSTRO_MAX_COLUMNS;

   monstro::Layout *layout = ctx->getCurrentLayout();
   if (!layout)
      return;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   float avail_w = layout->myAvailableWidth;
   if (avail_w <= 0.0f)
      avail_w = ctx->current_window->size.x - ctx->style.window.padding[0] * 2.0f;

   MonstroColumnsState *cs = &ctx->columns_stack[ctx->columns_stack_top++];
   cs->id = ctx->current_window->getChildId(str_id);
   cs->column_count = count;
   cs->current_column = 0;
   cs->columns_x = pos.x;
   cs->start_y = pos.y;
   cs->row_y = pos.y;
   cs->columns_w = avail_w;
   cs->row_max_h = 0.0f;
   cs->prev_visible_max_screen_x = layout->myVisibleMaxScreenX;

   float fixed_total = 0.0f;
   int flex_count = 0;
   for (int i = 0; i < count; i++)
   {
      float w = widths ? widths[i] : 0.0f;
      if (w > 0.0f)
      {
         cs->widths[i] = w;
         fixed_total += w;
      }
      else
      {
         cs->widths[i] = 0.0f;
         flex_count++;
      }
   }

   float flex_each = flex_count > 0 ? (avail_w - fixed_total) / (float)flex_count : 0.0f;
   if (flex_each < 0.0f)
      flex_each = 0.0f;

   float offset = 0.0f;
   for (int i = 0; i < count; i++)
   {
      cs->offsets[i] = offset;
      if (cs->widths[i] <= 0.0f)
      {
         cs->widths[i] = flex_each;
      }
      offset += cs->widths[i];
   }

   monstro_set_cursor_screen_pos(ctx, cs->columns_x + cs->offsets[0], cs->row_y);
   layout->myAvailableWidth = cs->widths[0];
   layout->myVisibleMaxScreenX = cs->columns_x + cs->widths[0];
}

void monstro_begin_columns(MonstroContext *ctx, const char *str_id, int count)
{
   monstro_begin_columns_ex(ctx, str_id, count, nullptr);
}

void monstro_next_column(MonstroContext *ctx)
{
   if (!ctx || ctx->columns_stack_top <= 0)
      return;

   MonstroColumnsState *cs = &ctx->columns_stack[ctx->columns_stack_top - 1];
   monstro::Layout *layout = ctx->getCurrentLayout();
   if (!layout)
      return;

   MonstroVec2 cur = monstro_get_cursor_screen_pos(ctx);
   float col_h = cur.y - cs->row_y;
   if (col_h > cs->row_max_h)
      cs->row_max_h = col_h;

   cs->current_column++;

   if (cs->current_column >= cs->column_count)
   {
      cs->current_column = 0;
      cs->row_y += cs->row_max_h;
      cs->row_max_h = 0.0f;
   }

   monstro_set_cursor_screen_pos(ctx, cs->columns_x + cs->offsets[cs->current_column], cs->row_y);
   layout->myAvailableWidth = cs->widths[cs->current_column];
   layout->myVisibleMaxScreenX = cs->columns_x + cs->offsets[cs->current_column] + cs->widths[cs->current_column];
}

void monstro_end_columns(MonstroContext *ctx)
{
   if (!ctx || ctx->columns_stack_top <= 0)
      return;

   MonstroColumnsState *cs = &ctx->columns_stack[ctx->columns_stack_top - 1];
   monstro::Layout *layout = ctx->getCurrentLayout();

   MonstroVec2 cur = monstro_get_cursor_screen_pos(ctx);
   float col_h = cur.y - cs->row_y;
   if (col_h > cs->row_max_h)
      cs->row_max_h = col_h;

   float final_y = cs->row_y + cs->row_max_h;
   float total_h = final_y - cs->start_y;

   ctx->columns_stack_top--;

   if (layout)
   {
      layout->myAvailableWidth = cs->columns_w;
      layout->myVisibleMaxScreenX = cs->prev_visible_max_screen_x;
      monstro_set_cursor_screen_pos(ctx, cs->columns_x, final_y);
      monstro_add_item(ctx, cs->columns_w, total_h);
   }
}

void monstro_draw_disabled_overlay(MonstroContext *ctx, MonstroRect rect)
{
   if (!ctx || !ctx->disabled || !ctx->current_window)
      return;
   MonstroColor bg = ctx->style.window.background;
   bg.a = 0.5f;
   ctx->current_window->canvas.addRectFilled(rect, monstro_color_to_u32(bg), 0.0f, MONSTRO_CORNER_NONE);
}
