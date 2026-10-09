#include "../context.h"
#include <cstdio>

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

void monstro_progress_bar(MonstroContext *ctx, float fraction, float width, float height, const char *overlay)
{
   monstro::requireWindow(ctx, __func__);

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return;

   MonstroStyle *style = &ctx->style;
   monstro::Canvas *canvas = &win->canvas;
   MonstroFont *font = style->font;
   MonstroStyleProgress *ps = &style->progress;

   monstro_add_item(ctx, ps->padding[0], ps->padding[1]);

   float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
   float bar_w = width > 0.0f ? width : 120.0f;
   float bar_h = height > 0.0f ? height : text_h + ps->padding[1] * 2.0f;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect bar_rect = {pos.x, pos.y, bar_w, bar_h};

   float t = clampf(fraction, 0.0f, 1.0f);

   // Background
   MonstroStyleItem *bg = &ps->normal;
   if (bg->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addRectFilled(bar_rect, monstro_color_to_u32(bg->data.color), ps->rounding, MONSTRO_CORNER_ALL);
   }

   if (ps->border > 0.0f)
   {
      canvas->addRect(bar_rect, monstro_color_to_u32(ps->border_color), ps->rounding, MONSTRO_CORNER_ALL, ps->border);
   }

   // Filled portion
   if (t > 0.0f)
   {
      MonstroRect fill_rect = {bar_rect.x, bar_rect.y, bar_rect.w * t, bar_rect.h};

      MonstroCornerFlags corners = MONSTRO_CORNER_LEFT;
      if (t >= 1.0f)
         corners = MONSTRO_CORNER_ALL;

      MonstroStyleItem *cursor = &ps->cursor_normal;
      if (cursor->type == MONSTRO_STYLE_ITEM_COLOR)
      {
         canvas->addRectFilled(fill_rect, monstro_color_to_u32(cursor->data.color), ps->cursor_rounding, corners);
      }

      if (ps->cursor_border > 0.0f)
      {
         canvas->addRect(fill_rect, monstro_color_to_u32(ps->cursor_border_color), ps->cursor_rounding, corners,
                         ps->cursor_border);
      }
   }

   // Overlay text
   // The overlay is plain text, never a format string; without one the bar shows its percentage.
   if (font)
   {
      char percent[16];
      const char *text = overlay;
      if (!text)
      {
         snprintf(percent, sizeof(percent), "%.0f%%", t * 100.0f);
         text = percent;
      }
      canvas->addTextEx(font, bar_rect, monstro_color_to_u32(style->text.color), text, MONSTRO_ALIGN_CENTER);
   }

   monstro_draw_disabled_overlay(ctx, bar_rect);
   monstro_add_item(ctx, bar_w, bar_h);
   monstro_set_last_item(ctx, 0, bar_rect, false, false, false, false);
}
