#include "../context.h"
#include <cstdio>
#include <cstring>

static bool is_popup_open_internal(MonstroContext *ctx, uint64_t popup_id)
{
   for (int i = 0; i < ctx->popup_stack.size(); i++)
   {
      if (ctx->popup_stack[i].popup_id == popup_id)
      {
         return true;
      }
   }
   return false;
}

static void build_window_name(char *out, size_t out_size, const char *prefix, const char *label)
{
   if (!out || out_size == 0)
      return;

   char id[256];
   monstro_copy_id_text(label, id, sizeof(id));
   const char *title = monstro_display_text(label);
   if (!title)
      title = "";
   if (id[0] == '\0')
   {
      monstro_utf8_copy(id, sizeof(id), title);
   }

   snprintf(out, out_size, "%s%s##%s", prefix ? prefix : "", id, title);
   monstro_utf8_trim_partial(out);
}

void monstro_open_popup(MonstroContext *ctx, const char *label) { monstro_open_popup_ex(ctx, label, false); }

void monstro_open_popup_ex(MonstroContext *ctx, const char *label, bool modal)
{
   monstro::requireWindow(ctx, __func__);
   if (!label)
      return;

   monstro::Window *win = ctx->current_window;
   uint64_t popup_id = win->getChildId(label);

   if (is_popup_open_internal(ctx, popup_id))
      return;

   if (!modal)
   {
      while (!ctx->popup_stack.empty() && ctx->popup_stack.top().parent_window == win)
      {
         ctx->popup_stack.pop();
      }
   }

   MonstroPopupInternal *popup = &ctx->popup_stack.push({});
   popup->popup_id = popup_id;
   popup->parent_window = win;
   popup->parent_id = win->getChildId("popups");
   popup->open_pos_x = ctx->mouse.pos_x;
   popup->open_pos_y = ctx->mouse.pos_y;
   popup->open_frame = ctx->frame;
   popup->is_modal = modal;
}

void monstro_close_current_popup(MonstroContext *ctx)
{
   if (!ctx)
      return;
   ctx->popup_stack.pop();
}

bool monstro_begin_popup(MonstroContext *ctx, const char *label, MonstroWindowFlags extra_flags)
{
   monstro::requireWindow(ctx, __func__);
   if (!label)
      return false;

   monstro::Window *parent_win = ctx->current_window;
   uint64_t popup_id = parent_win->getChildId(label);

   if (!is_popup_open_internal(ctx, popup_id))
   {
      return false;
   }

   MonstroWindowFlags flags =
       (MonstroWindowFlags)(extra_flags | MONSTRO_WINDOW_POPUP | MONSTRO_WINDOW_AUTO_RESIZE |
                            MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_INPUTS);

   char popup_name[320];
   if (flags & MONSTRO_WINDOW_CHILD_MENU)
   {
      build_window_name(popup_name, sizeof(popup_name), "menu_", label);
   }
   else if (flags & MONSTRO_WINDOW_COMBO)
   {
      build_window_name(popup_name, sizeof(popup_name), "combo_", label);
   }
   else
   {
      build_window_name(popup_name, sizeof(popup_name), "popup_", label);
   }

   // Without a p_open the window always begins; monstro_end_popup ends it.
   monstro_begin_window(ctx, popup_name, nullptr, flags);
   monstro::Window *popup = ctx->current_window;
   popup->makeLastSibling();

   if (!(parent_win->flags & MONSTRO_WINDOW_BORDERS))
   {
      popup->flags = (MonstroWindowFlags)(popup->flags & ~MONSTRO_WINDOW_BORDERS);
   }

   return true;
}

bool monstro_begin_popup_modal(MonstroContext *ctx, const char *label, bool *p_open, MonstroWindowFlags flags)
{
   monstro::requireWindow(ctx, __func__);
   if (!label)
      return false;

   if (p_open && !*p_open)
      return false;

   monstro::Window *parent_win = ctx->current_window;
   uint64_t popup_id = parent_win->getChildId(label);

   if (!is_popup_open_internal(ctx, popup_id))
   {
      return false;
   }

   MonstroStyle *style = &ctx->style;
   monstro::Canvas *root_canvas = &ctx->root_window->canvas;
   MonstroRect fullscreen = {0.0f, 0.0f, ctx->display_width, ctx->display_height};
   root_canvas->pushLayer(1000);
   root_canvas->addRectFilled(fullscreen, monstro_color_to_u32(style->window.modal_dim_color), 0.0f,
                              MONSTRO_CORNER_NONE);
   root_canvas->popLayer();

   MonstroWindowFlags modal_flags =
       (MonstroWindowFlags)((flags & ~MONSTRO_WINDOW_SCROLL_WITH_MOUSE) | MONSTRO_WINDOW_MODAL | MONSTRO_WINDOW_POPUP |
                            MONSTRO_WINDOW_TITLE_BAR | MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_BACKGROUND |
                            MONSTRO_WINDOW_INPUTS | MONSTRO_WINDOW_MOVABLE | MONSTRO_WINDOW_AUTO_RESIZE);

   char popup_name[320];
   build_window_name(popup_name, sizeof(popup_name), "modal_", label);

   monstro::Window *existing = ctx->findWindow(popup_name);
   if (existing && existing->size.x > 0.0f && existing->size.y > 0.0f)
   {
      float cx = (ctx->display_width - existing->size.x) * 0.5f;
      float cy = (ctx->display_height - existing->size.y) * 0.5f;
      monstro_set_next_window_pos(ctx, cx, cy, MONSTRO_COND_ALWAYS);
   }

   // p_open was checked above, so the window always begins here.
   monstro_begin_window(ctx, popup_name, p_open, modal_flags);
   ctx->current_window->makeLastSibling();

   if (p_open && ctx->keyboard.isPressed(MONSTRO_KEY_ESCAPE))
   {
      *p_open = false;
      monstro_close_current_popup(ctx);
      monstro_end_popup(ctx);
      return false;
   }

   if (p_open && !*p_open)
   {
      monstro_close_current_popup(ctx);
   }

   return true;
}

bool monstro_begin_popup_context_item(MonstroContext *ctx, const char *label, MonstroMouseButton mouse_button)
{
   if (!ctx || !label)
      return false;

   if (ctx->hovered_id != 0 && monstro_is_mouse_clicked(ctx, mouse_button, false))
   {
      monstro_open_popup(ctx, label);
   }

   return monstro_begin_popup(ctx, label, (MonstroWindowFlags)0);
}

bool monstro_begin_popup_context_window(MonstroContext *ctx, const char *label, MonstroMouseButton mouse_button,
                                        bool also_over_items)
{
   if (!ctx || !label)
      return false;

   if (ctx->hovered_window != nullptr && monstro_is_mouse_clicked(ctx, mouse_button, false))
   {
      if (also_over_items || ctx->hovered_id == 0)
      {
         monstro_open_popup(ctx, label);
      }
   }

   return monstro_begin_popup(ctx, label, (MonstroWindowFlags)0);
}

bool monstro_begin_popup_context_void(MonstroContext *ctx, const char *label, MonstroMouseButton mouse_button)
{
   if (!ctx || !label)
      return false;

   if (ctx->hovered_window == nullptr && monstro_is_mouse_clicked(ctx, mouse_button, false))
   {
      monstro_open_popup(ctx, label);
   }

   return monstro_begin_popup(ctx, label, (MonstroWindowFlags)0);
}

void monstro_end_popup(MonstroContext *ctx)
{
   monstro::requireWindow(ctx, __func__);
   monstro_end_window(ctx);
}

bool monstro_is_popup_open(MonstroContext *ctx, const char *str_id)
{
   monstro::Window *win = monstro::requireWindow(ctx, __func__);
   if (!str_id)
      return false;

   uint64_t popup_id = win->getChildId(str_id);

   return is_popup_open_internal(ctx, popup_id);
}
