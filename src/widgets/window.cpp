#include "../context.h"
#include <cmath>
#include <cstdio>
#include <cstring>

/* ------------------------------------------------------------------------- */
/* Next Window Configuration                                                 */
/* ------------------------------------------------------------------------- */

void monstro_set_next_window_pos(MonstroContext *ctx, float x, float y, MonstroCondition cond)
{
   monstro::requireContext(ctx, __func__);
   ctx->next_window_pos_x = x;
   ctx->next_window_pos_y = y;
   ctx->next_window_pos_cond = cond;
}

void monstro_set_next_window_size(MonstroContext *ctx, float width, float height, MonstroCondition cond)
{
   monstro::requireContext(ctx, __func__);
   ctx->next_window_size_x = width;
   ctx->next_window_size_y = height;
   ctx->next_window_size_cond = cond;
}

void monstro_set_next_window_focus(MonstroContext *ctx)
{
   monstro::requireContext(ctx, __func__);
   ctx->next_window_focus = true;
}

/* ------------------------------------------------------------------------- */
/* Window Lifecycle                                                          */
/* ------------------------------------------------------------------------- */

bool monstro_begin_window(MonstroContext *ctx, const char *name, bool *p_open, MonstroWindowFlags flags)
{
   monstro::requireContext(ctx, __func__, {MONSTRO_ARG(name)});
   MONSTRO_REQUIRE(ctx->in_frame, "monstro_begin_window called outside a frame: window and widget calls belong "
                                  "between monstro_begin_frame and monstro_end_frame");

   monstro::Window *win = ctx->findWindow(name);
   if (!win)
      win = ctx->createWindow(name, flags);
   else
   {
      if (!win->title_overridden)
      {
         const char *title = monstro_display_text(name);
         if (!title)
            title = "";
         monstro_utf8_copy(win->title, sizeof(win->title), title);
      }
   }

   win->flags = flags;
   win->child_region = false;

   return win->begin(p_open);
}

void monstro_end_window(MonstroContext *ctx)
{
   monstro::Window *win = monstro::requireWindow(ctx, __func__);
   MONSTRO_REQUIRE(win != ctx->root_window, "monstro_end_window without a matching monstro_begin_window (frame %d)",
                   ctx->frame);
   MONSTRO_REQUIRE(!win->child_region, "monstro_end_window on child region '%s': end it with monstro_end_child",
                   win->name);
   win->end();
   ctx->popWindow();
}

/* ------------------------------------------------------------------------- */
/* Current Window Configuration                                              */
/* ------------------------------------------------------------------------- */

void monstro_set_window_pos(MonstroContext *ctx, float x, float y, MonstroCondition cond)
{
   monstro::requireWindow(ctx, __func__);
   ctx->current_window->setPosition(x, y, cond);
}

void monstro_set_window_size(MonstroContext *ctx, float width, float height, MonstroCondition cond)
{
   monstro::requireWindow(ctx, __func__);
   ctx->current_window->setSize(width, height, cond);
}

void monstro_set_window_focus(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   ctx->focusWindow(ctx->current_window);
}

void monstro_set_window_layout(MonstroContext *ctx, MonstroLayoutDirection direction)
{
   monstro::requireWindow(ctx, __func__);
   ctx->current_window->setLayoutDirection(direction);
}

void monstro_set_window_title(MonstroContext *ctx, const char *title)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(title)});
   monstro_utf8_copy(ctx->current_window->title, sizeof(ctx->current_window->title), title);
   ctx->current_window->title_overridden = true;
}

/* ------------------------------------------------------------------------- */
/* Window Configuration by Name                                              */
/* ------------------------------------------------------------------------- */

void monstro_set_window_pos_named(MonstroContext *ctx, const char *name, float x, float y, MonstroCondition cond)
{
   monstro::requireContext(ctx, __func__, {MONSTRO_ARG(name)});
   monstro::Window *win = ctx->findWindow(name);
   if (win)
      win->setPosition(x, y, cond);
}

void monstro_set_window_size_named(MonstroContext *ctx, const char *name, float width, float height,
                                   MonstroCondition cond)
{
   monstro::requireContext(ctx, __func__, {MONSTRO_ARG(name)});
   monstro::Window *win = ctx->findWindow(name);
   if (win)
      win->setSize(width, height, cond);
}

void monstro_set_window_focus_named(MonstroContext *ctx, const char *name)
{
   monstro::requireContext(ctx, __func__, {MONSTRO_ARG(name)});
   monstro::Window *win = ctx->findWindow(name);
   if (win)
      ctx->focusWindow(win);
}

void monstro_set_window_layout_named(MonstroContext *ctx, const char *name, MonstroLayoutDirection direction)
{
   monstro::requireContext(ctx, __func__, {MONSTRO_ARG(name)});
   monstro::Window *win = ctx->findWindow(name);
   if (win)
      win->setLayoutDirection(direction);
}

void monstro_set_window_title_named(MonstroContext *ctx, const char *name, const char *title)
{
   monstro::requireContext(ctx, __func__, {MONSTRO_ARG(name), MONSTRO_ARG(title)});
   monstro::Window *win = ctx->findWindow(name);
   if (!win)
      return;
   monstro_utf8_copy(win->title, sizeof(win->title), title);
   win->title_overridden = true;
}

/* ------------------------------------------------------------------------- */
/* Window Properties                                                         */
/* ------------------------------------------------------------------------- */

MonstroRect monstro_get_window_rect(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   return ctx->current_window->getRect();
}

MonstroVec2 monstro_get_window_pos(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   return ctx->current_window->pos;
}

MonstroVec2 monstro_get_window_size(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   return ctx->current_window->size;
}

/* ------------------------------------------------------------------------- */
/* Scrolling                                                                 */
/* ------------------------------------------------------------------------- */

float monstro_get_scroll_x(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   return ctx->current_window->scroll_x;
}

float monstro_get_scroll_y(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   return ctx->current_window->scroll_y;
}

void monstro_set_scroll_x(MonstroContext *ctx, float scroll_x)
{
   monstro::requireWindow(ctx, __func__);
   ctx->current_window->scroll_x = scroll_x;
}

void monstro_set_scroll_y(MonstroContext *ctx, float scroll_y)
{
   monstro::requireWindow(ctx, __func__);
   ctx->current_window->scroll_y = scroll_y;
}

/* ------------------------------------------------------------------------- */
/* Window State Queries                                                      */
/* ------------------------------------------------------------------------- */

bool monstro_is_window_collapsed(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   return ctx->current_window->collapsed;
}

bool monstro_is_window_focused(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   return ctx->focused_window == ctx->current_window;
}

bool monstro_is_window_hovered(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   return monstro_rect_contains(ctx->current_window->getRect(), ctx->mouse.pos_x, ctx->mouse.pos_y);
}

/* ------------------------------------------------------------------------- */
/* Canvas Access                                                             */
/* ------------------------------------------------------------------------- */

MonstroCanvas *monstro_get_window_canvas(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   return ctx->canvas;
}

/* ------------------------------------------------------------------------- */
/* Child Windows / Scroll Regions                                            */
/* ------------------------------------------------------------------------- */

bool monstro_begin_child(MonstroContext *ctx, const char *str_id, float width, float height, MonstroWindowFlags flags)
{
   monstro::Window *parent = monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(str_id)});
   if (parent->skip_items)
      return false;

   MonstroVec2 cursor = parent->getCursorScreenPos();

   MonstroVec2 avail = monstro_get_content_region_avail(ctx);
   if (width <= 0.0f)
   {
      width = avail.x + width;
      if (width < 0.0f)
         width = 0.0f;
   }
   if (height <= 0.0f)
   {
      height = avail.y + height;
      if (height < 0.0f)
         height = 0.0f;
   }

   MonstroWindowFlags child_flags =
       (MonstroWindowFlags)(flags | MONSTRO_WINDOW_CHILD | MONSTRO_WINDOW_INPUTS | MONSTRO_WINDOW_SCROLL_WITH_MOUSE |
                            MONSTRO_WINDOW_VERTICAL_SCROLLBAR);

   if (!(flags & MONSTRO_WINDOW_BORDERS))
   {
      child_flags = (MonstroWindowFlags)(child_flags & ~MONSTRO_WINDOW_BORDERS);
   }

   char child_id[256];
   monstro_copy_id_text(str_id, child_id, sizeof(child_id));
   if (child_id[0] == '\0')
   {
      const char *fallback_id = monstro_display_text(str_id);
      if (!fallback_id)
         fallback_id = "";
      monstro_utf8_copy(child_id, sizeof(child_id), fallback_id);
   }

   const char *child_title = monstro_display_text(str_id);
   if (!child_title)
      child_title = "";

   char child_name[256 + 256 + 256 + 16];
   snprintf(child_name, sizeof(child_name), "child_%s_%s##%s", parent->name, child_id, child_title);
   monstro_utf8_trim_partial(child_name);

   monstro_set_next_window_pos(ctx, cursor.x, cursor.y, MONSTRO_COND_ALWAYS);
   monstro_set_next_window_size(ctx, width, height, MONSTRO_COND_ALWAYS);

   // Without a p_open the window always begins; monstro_end_child ends it.
   monstro_begin_window(ctx, child_name, nullptr, child_flags);
   ctx->current_window->child_region = true;
   parent->addItem(width, height);
   return true;
}

void monstro_end_child(MonstroContext *ctx)
{
   monstro::Window *win = monstro::requireWindow(ctx, __func__);
   MONSTRO_REQUIRE(win->child_region, "monstro_end_child: the current window '%s' was not begun by monstro_begin_child",
                   win->name);
   win->child_region = false;
   monstro_end_window(ctx);
}
