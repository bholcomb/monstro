#include "toast.h"
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

void toast_init(ToastManager *tm) { memset(tm, 0, sizeof(*tm)); }

void toast_show(ToastManager *tm, ToastType type, float duration, const char *fmt, ...)
{
   for (int i = 0; i < TOAST_MAX_NOTIFICATIONS; i++)
   {
      if (!tm->toasts[i].active)
      {
         ToastNotification *t = &tm->toasts[i];
         t->type = type;
         t->duration = duration;
         t->elapsed = 0.0f;
         t->active = true;

         va_list args;
         va_start(args, fmt);
         vsnprintf(t->message, TOAST_MAX_MSG_LEN, fmt, args);
         va_end(args);
         return;
      }
   }
}

static MonstroColor toast_bg_color(ToastType type)
{
   switch (type)
   {
   case TOAST_INFO:
      return {0.16f, 0.16f, 0.20f, 0.90f};
   case TOAST_SUCCESS:
      return {0.12f, 0.24f, 0.12f, 0.90f};
   case TOAST_WARNING:
      return {0.24f, 0.20f, 0.08f, 0.90f};
   case TOAST_ERROR:
      return {0.24f, 0.10f, 0.10f, 0.90f};
   default:
      return {0.16f, 0.16f, 0.20f, 0.90f};
   }
}

static MonstroColor toast_text_color(ToastType type)
{
   switch (type)
   {
   case TOAST_INFO:
      return {0.78f, 0.78f, 0.86f, 1.0f};
   case TOAST_SUCCESS:
      return {0.39f, 0.94f, 0.39f, 1.0f};
   case TOAST_WARNING:
      return {1.0f, 0.82f, 0.31f, 1.0f};
   case TOAST_ERROR:
      return {1.0f, 0.39f, 0.39f, 1.0f};
   default:
      return {0.78f, 0.78f, 0.86f, 1.0f};
   }
}

static const char *toast_prefix(ToastType type)
{
   switch (type)
   {
   case TOAST_INFO:
      return "Info";
   case TOAST_SUCCESS:
      return "OK";
   case TOAST_WARNING:
      return "Warning";
   case TOAST_ERROR:
      return "Error";
   default:
      return "";
   }
}

void toast_draw(ToastManager *tm, MonstroContext *ctx, float dt)
{
   float display_w = 0, display_h = 0;
   monstro_get_display_size(ctx, &display_w, &display_h);

   float toast_w = 280.0f;
   float margin = 16.0f;
   float y_offset = margin;

   for (int i = 0; i < TOAST_MAX_NOTIFICATIONS; i++)
   {
      ToastNotification *t = &tm->toasts[i];
      if (!t->active)
         continue;

      t->elapsed += dt;

      float alpha = 1.0f;
      float fade_start = t->duration - 0.5f;
      if (t->elapsed > fade_start && t->duration > 0.5f)
      {
         alpha = 1.0f - (t->elapsed - fade_start) / 0.5f;
         if (alpha < 0.0f)
            alpha = 0.0f;
      }
      if (t->elapsed < 0.2f)
      {
         alpha = t->elapsed / 0.2f;
      }

      if (t->elapsed >= t->duration)
      {
         t->active = false;
         continue;
      }

      float x_pos = display_w - toast_w - margin;
      float y_pos = y_offset;

      char win_id[32];
      snprintf(win_id, sizeof(win_id), "##toast_%d", i);

      monstro_set_next_window_pos(ctx, x_pos, y_pos, MONSTRO_COND_ALWAYS);
      monstro_set_next_window_size(ctx, toast_w, 0, MONSTRO_COND_FIRST_USE_EVER);

      MonstroWindowFlags flags = (MonstroWindowFlags)(MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_BACKGROUND |
                                                      MONSTRO_WINDOW_INPUTS | MONSTRO_WINDOW_AUTO_RESIZE);

      MonstroColor bg = toast_bg_color(t->type);
      bg.a *= alpha;
      const MonstroStyleWindow *base_win_style = monstro_get_window_style(ctx);
      MonstroStyleWindow toast_style = *base_win_style;
      toast_style.fixed_background.type = MONSTRO_STYLE_ITEM_COLOR;
      toast_style.fixed_background.data.color = bg;
      monstro_push_window_style(ctx, &toast_style);

      if (monstro_begin_window(ctx, win_id, nullptr, flags))
      {
         char header[32];
         snprintf(header, sizeof(header), "[%s]", toast_prefix(t->type));
         monstro_label_colored(ctx, header, toast_text_color(t->type));
         monstro_label_wrapped(ctx, t->message, toast_w - 20.0f);

         float frac = t->elapsed / t->duration;
         monstro_progress_bar(ctx, 1.0f - frac, toast_w - 20.0f, 3.0f, nullptr);
         monstro_end_window(ctx);
      }

      monstro_pop_style(ctx, 1);

      y_offset += 80.0f;
   }
}
