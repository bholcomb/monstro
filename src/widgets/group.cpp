#include "../context.h"

void monstro_begin_group(MonstroContext *ctx, MonstroLayoutDirection layout, MonstroGroupFlags flags)
{
   monstro::requireWindow(ctx, __func__);

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return;

   MonstroStyle *style = &ctx->style();
   float pad_x = style->window.group_padding[0];
   float pad_y = style->window.group_padding[1];

   monstro_add_item(ctx, pad_x, pad_y);

   ctx->canvas->groups.push(flags);

   ctx->canvas->pushLayer(ctx->canvas->layers.top() + 1);

   win->beginLayout(layout);

   // The group's padding is inside its edges on both sides: what fills its width stops a padding short.
   monstro::Layout *l = win->getCurrentLayout();
   if (l)
   {
      l->myPosX += pad_x;
      l->myPosY += pad_y;
      l->myContentRight -= pad_x;
   }
}

void monstro_end_group(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return;

   MONSTRO_REQUIRE(win->groupDepth() > 0,
                   "group stack underflow: monstro_end_group without monstro_begin_group in window '%s'", win->name);
   MonstroGroupFlags flags = ctx->canvas->groups.pop();

   MonstroStyle *style = &ctx->style();
   float pad_x = style->window.group_padding[0];
   float pad_y = style->window.group_padding[1];

   monstro::Layout *layout = win->getCurrentLayout();
   if (!layout)
      return;

   float group_w = layout->mySizeX + pad_x * 2.0f;
   float group_h = layout->mySizeY + pad_y * 2.0f;
   float group_x = win->pos.x + layout->myPosX - pad_x + layout->myScrollOffsetX;
   float group_y = win->pos.y + layout->myPosY - pad_y + layout->myScrollOffsetY;
   MonstroRect group_rect = {group_x, group_y, group_w, group_h};

   layout->mySizeX += pad_x * 2.0f;
   layout->mySizeY += pad_y * 2.0f;

   ctx->canvas->popLayer();

   if (flags & MONSTRO_GROUP_BACKGROUND)
   {
      ctx->canvas->addStyleItem(style->window.group_background, group_rect, style->window.rounding, MONSTRO_CORNER_ALL);
   }

   if (flags & MONSTRO_GROUP_BORDER)
   {
      float border = style->window.group_border;
      if (border > 0.0f)
      {
         ctx->canvas->addRect(group_rect, monstro_color_to_u32(style->window.group_border_color),
                              style->window.rounding, MONSTRO_CORNER_ALL, border);
      }
   }

   win->endLayout();
}
