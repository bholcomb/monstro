#include "../context.h"
#include <string.h>

void monstro_label(MonstroContext *ctx, const char *text)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(text)});

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return;

   MonstroStyle *style = &ctx->style();
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;

   // Space before the label, as other widgets leave before themselves, so its text is level with theirs.
   monstro_add_item(ctx, style->text.padding[0], style->text.padding[1]);

   MonstroVec2 text_size = monstro_font_measure_text(font, text, 0.0f);
   if (text_size.x == 0.0f && text_size.y == 0.0f)
   {
      text_size.x = (float)strlen(text) * 8.0f;
      text_size.y = 16.0f;
   }

   float padding_x = style->text.padding[0];
   float padding_y = style->text.padding[1];
   float outer_w = text_size.x + padding_x * 2.0f;
   float outer_h = text_size.y + padding_y * 2.0f;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect text_rect = {pos.x + padding_x, pos.y + padding_y, text_size.x, text_size.y};

   uint32_t color = monstro_color_to_u32(style->text.color);
   canvas->addTextEx(font, text_rect, color, text, style->text.alignment);

   MonstroRect item_rect = {pos.x, pos.y, outer_w, outer_h};
   monstro_draw_disabled_overlay(ctx, item_rect);
   monstro_add_item(ctx, outer_w, outer_h);
   monstro_set_last_item(ctx, 0, item_rect, false, false, false, false);
}

void monstro_label_sized(MonstroContext *ctx, const char *text, float width, float height)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(text)});

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return;

   MonstroStyle *style = &ctx->style();
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;

   // Space before the label, as other widgets leave before themselves, so its text is level with theirs.
   monstro_add_item(ctx, style->text.padding[0], style->text.padding[1]);

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect text_rect = {pos.x, pos.y, width, height};

   uint32_t color = monstro_color_to_u32(style->text.color);
   canvas->addTextEx(font, text_rect, color, text, style->text.alignment);

   monstro_draw_disabled_overlay(ctx, text_rect);
   monstro_add_item(ctx, width, height);
   monstro_set_last_item(ctx, 0, text_rect, false, false, false, false);
}

void monstro_label_wrapped(MonstroContext *ctx, const char *text, float wrap_width)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(text)});

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return;

   MonstroStyle *style = &ctx->style();
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;

   // Space before the label, as other widgets leave before themselves, so its text is level with theirs.
   monstro_add_item(ctx, style->text.padding[0], style->text.padding[1]);

   float padding_x = style->text.padding[0];
   float padding_y = style->text.padding[1];

   // By default the label fills the layout's width as other full-width widgets do, padding included, so its text
   // stops short of the window's edge or scrollbar.
   if (wrap_width <= 0.0f)
   {
      monstro::Layout *layout = win->getCurrentLayout();
      if (layout)
      {
         wrap_width = layout->fillWidth() - padding_x * 2.0f;
      }
      if (wrap_width <= 0.0f)
         wrap_width = 200.0f;
   }

   MonstroVec2 text_size = monstro_font_measure_text_wrapped(font, text, wrap_width);
   if (text_size.x == 0.0f && text_size.y == 0.0f)
   {
      text_size.x = wrap_width;
      text_size.y = 16.0f;
   }

   float outer_w = wrap_width + padding_x * 2.0f;
   float outer_h = text_size.y + padding_y * 2.0f;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroVec2 text_pos = {pos.x + padding_x, pos.y + padding_y};

   uint32_t color = monstro_color_to_u32(style->text.color);
   canvas->addTextWrapped(font, text_pos, color, text, wrap_width);

   MonstroRect wrap_item_rect = {pos.x, pos.y, outer_w, outer_h};
   monstro_draw_disabled_overlay(ctx, wrap_item_rect);
   monstro_add_item(ctx, outer_w, outer_h);
   monstro_set_last_item(ctx, 0, wrap_item_rect, false, false, false, false);
}

void monstro_label_colored(MonstroContext *ctx, const char *text, MonstroColor color)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(text)});

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return;

   MonstroStyle *style = &ctx->style();
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;

   // Space before the label, as other widgets leave before themselves, so its text is level with theirs.
   monstro_add_item(ctx, style->text.padding[0], style->text.padding[1]);

   MonstroVec2 text_size = monstro_font_measure_text(font, text, 0.0f);
   if (text_size.x == 0.0f && text_size.y == 0.0f)
   {
      text_size.x = (float)strlen(text) * 8.0f;
      text_size.y = 16.0f;
   }

   float padding_x = style->text.padding[0];
   float padding_y = style->text.padding[1];
   float outer_w = text_size.x + padding_x * 2.0f;
   float outer_h = text_size.y + padding_y * 2.0f;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect text_rect = {pos.x + padding_x, pos.y + padding_y, text_size.x, text_size.y};

   uint32_t col = monstro_color_to_u32(color);
   canvas->addTextEx(font, text_rect, col, text, style->text.alignment);

   MonstroRect colored_item_rect = {pos.x, pos.y, outer_w, outer_h};
   monstro_draw_disabled_overlay(ctx, colored_item_rect);
   monstro_add_item(ctx, outer_w, outer_h);
   monstro_set_last_item(ctx, 0, colored_item_rect, false, false, false, false);
}
