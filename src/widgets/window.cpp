#include "../context.h"
#include <cmath>
#include <cstring>

/* ------------------------------------------------------------------------- */
/* Next Window Configuration                                                 */
/* ------------------------------------------------------------------------- */

void monstro_set_next_window_pos(MonstroContext *ctx, float x, float y, MonstroCondition cond)
{
   if (!ctx)
      return;
   ctx->next_window_pos_x = x;
   ctx->next_window_pos_y = y;
   ctx->next_window_pos_cond = cond;
}

void monstro_set_next_window_size(MonstroContext *ctx, float width, float height, MonstroCondition cond)
{
   if (!ctx)
      return;
   ctx->next_window_size_x = width;
   ctx->next_window_size_y = height;
   ctx->next_window_size_cond = cond;
}

void monstro_set_next_window_focus(MonstroContext *ctx)
{
   if (!ctx)
      return;
   ctx->next_window_focus = true;
}

/* ------------------------------------------------------------------------- */
/* Window Lifecycle                                                          */
/* ------------------------------------------------------------------------- */

bool monstro_begin_window(MonstroContext *ctx, const char *name, bool *p_open, MonstroWindowFlags flags)
{
   if (!ctx || !name)
      return false;

   monstro::Window *win = ctx->findWindow(name);

   if (!win)
   {
      win = ctx->createWindow(name, flags);
      if (!win)
         return false;
   }

   win->flags = flags;

   return win->begin(p_open);
}

void monstro_end_window(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return;

   monstro::Window *win = ctx->current_window;
   win->end();
   ctx->popWindow();
}

/* ------------------------------------------------------------------------- */
/* Current Window Configuration                                              */
/* ------------------------------------------------------------------------- */

void monstro_set_window_pos(MonstroContext *ctx, float x, float y, MonstroCondition cond)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->setPosition(x, y, cond);
}

void monstro_set_window_size(MonstroContext *ctx, float width, float height, MonstroCondition cond)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->setSize(width, height, cond);
}

void monstro_set_window_focus(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->focusWindow(ctx->current_window);
}

void monstro_set_window_layout(MonstroContext *ctx, MonstroLayoutDirection direction)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->setLayoutDirection(direction);
}

/* ------------------------------------------------------------------------- */
/* Window Configuration by Name                                              */
/* ------------------------------------------------------------------------- */

void monstro_set_window_pos_named(MonstroContext *ctx, const char *name, float x, float y, MonstroCondition cond)
{
   if (!ctx)
      return;
   monstro::Window *win = ctx->findWindow(name);
   if (win)
      win->setPosition(x, y, cond);
}

void monstro_set_window_size_named(MonstroContext *ctx, const char *name, float width, float height,
                                   MonstroCondition cond)
{
   if (!ctx)
      return;
   monstro::Window *win = ctx->findWindow(name);
   if (win)
      win->setSize(width, height, cond);
}

void monstro_set_window_focus_named(MonstroContext *ctx, const char *name)
{
   if (!ctx)
      return;
   monstro::Window *win = ctx->findWindow(name);
   if (win)
      ctx->focusWindow(win);
}

void monstro_set_window_layout_named(MonstroContext *ctx, const char *name, MonstroLayoutDirection direction)
{
   if (!ctx)
      return;
   monstro::Window *win = ctx->findWindow(name);
   if (win)
      win->setLayoutDirection(direction);
}

/* ------------------------------------------------------------------------- */
/* Window Properties                                                         */
/* ------------------------------------------------------------------------- */

MonstroRect monstro_get_window_rect(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return {0, 0, 0, 0};
   return ctx->current_window->getRect();
}

MonstroVec2 monstro_get_window_pos(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return {0.0f, 0.0f};
   return ctx->current_window->pos;
}

MonstroVec2 monstro_get_window_size(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return {0.0f, 0.0f};
   return ctx->current_window->size;
}

/* ------------------------------------------------------------------------- */
/* Scrolling                                                                 */
/* ------------------------------------------------------------------------- */

float monstro_get_scroll_x(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return 0.0f;
   return ctx->current_window->scroll_x;
}

float monstro_get_scroll_y(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return 0.0f;
   return ctx->current_window->scroll_y;
}

void monstro_set_scroll_x(MonstroContext *ctx, float scroll_x)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->scroll_x = scroll_x;
}

void monstro_set_scroll_y(MonstroContext *ctx, float scroll_y)
{
   if (!ctx || !ctx->current_window)
      return;
   ctx->current_window->scroll_y = scroll_y;
}

/* ------------------------------------------------------------------------- */
/* Window State Queries                                                      */
/* ------------------------------------------------------------------------- */

bool monstro_is_window_collapsed(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return false;
   return ctx->current_window->collapsed;
}

bool monstro_is_window_focused(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return false;
   return ctx->focused_window == ctx->current_window;
}

bool monstro_is_window_hovered(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return false;
   return monstro_rect_contains(ctx->current_window->getRect(), ctx->mouse.pos_x, ctx->mouse.pos_y);
}

/* ------------------------------------------------------------------------- */
/* Canvas Access                                                             */
/* ------------------------------------------------------------------------- */

MonstroCanvas *monstro_get_window_canvas(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return nullptr;
   return reinterpret_cast<MonstroCanvas *>(&ctx->current_window->canvas);
}

/* ------------------------------------------------------------------------- */
/* Child Windows / Scroll Regions                                            */
/* ------------------------------------------------------------------------- */

bool monstro_begin_child(MonstroContext *ctx, const char *str_id, float width, float height, MonstroWindowFlags flags)
{
   if (!ctx || !ctx->current_window || !str_id)
      return false;

   monstro::Window *parent = ctx->current_window;
   if (parent->skip_items)
      return false;

   MonstroStyle *style = &ctx->style;

   MonstroVec2 cursor = parent->getCursorScreenPos();

   monstro::Layout *parent_layout = parent->getCurrentLayout();
   if (width <= 0.0f && parent_layout)
   {
      width = parent_layout->myAvailableWidth;
   }
   if (height <= 0.0f && parent_layout)
   {
      height = parent_layout->myAvailableHeight;
   }

   MonstroWindowFlags child_flags =
       (MonstroWindowFlags)(flags | MONSTRO_WINDOW_CHILD | MONSTRO_WINDOW_INPUTS |
                            MONSTRO_WINDOW_SCROLL_WITH_MOUSE | MONSTRO_WINDOW_VERTICAL_SCROLLBAR);

   if (!(flags & MONSTRO_WINDOW_BORDERS))
   {
      child_flags = (MonstroWindowFlags)(child_flags & ~MONSTRO_WINDOW_BORDERS);
   }

   char child_name[320];
   snprintf(child_name, sizeof(child_name), "child_%s_%s", parent->name, str_id);

   monstro_set_next_window_pos(ctx, cursor.x, cursor.y, MONSTRO_COND_ALWAYS);
   monstro_set_next_window_size(ctx, width, height, MONSTRO_COND_ALWAYS);

   bool opened = monstro_begin_window(ctx, child_name, nullptr, child_flags);

   if (opened)
   {
      parent->addItem(width, height);
   }

   return opened;
}

void monstro_end_child(MonstroContext *ctx)
{
   if (!ctx || !ctx->current_window)
      return;
   if (!(ctx->current_window->flags & MONSTRO_WINDOW_CHILD))
      return;

   monstro_end_window(ctx);
}
