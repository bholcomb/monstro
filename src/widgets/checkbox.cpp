#include "../context.h"
#include <cmath>
#include <string.h>

static bool checkbox_behavior(MonstroContext *ctx, MonstroRect r, uint64_t id, bool *out_hovered, bool *out_held)
{
   bool pressed = false;
   bool hovered = ctx->hoverItem(id, r);
   if (hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
   {
      ctx->setActiveId(id);
   }

   bool held = false;
   if (ctx->active_id == id)
   {
      if (!ctx->disabled && monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         held = true;
      }
      else
      {
         if (hovered)
         {
            pressed = true;
         }
         ctx->setActiveId(0);
      }
   }

   *out_hovered = hovered;
   *out_held = held;
   return pressed;
}

bool monstro_checkbox(MonstroContext *ctx, const char *label, bool *value, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label), MONSTRO_ARG(value)});

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style();
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;

   monstro_add_item(ctx, style->checkbox.padding[0], style->checkbox.padding[1]);

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);

   const char *display = monstro_display_text(label);
   float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
   float box_size = text_h;
   float text_w = monstro_font_measure_text(font, display, 0.0f).x;
   if (text_w == 0.0f)
      text_w = (float)strlen(display) * 8.0f;
   float spacing = style->checkbox.spacing;

   float total_w = box_size + spacing + text_w;
   float total_h = fmaxf(box_size, text_h);

   MonstroRect r = {pos.x, pos.y, total_w, total_h};
   uint64_t id = win->getItemId(label);

   bool hovered = false;
   bool held = false;
   bool pressed = checkbox_behavior(ctx, r, id, &hovered, &held);

   if (pressed)
   {
      *value = !*value;
   }

   MonstroStyleItem *bg_style = &style->checkbox.normal;
   if (hovered)
      bg_style = &style->checkbox.hover;
   if (held)
      bg_style = &style->checkbox.active;

   MonstroRect box_rect = {pos.x, pos.y + (total_h - box_size) * 0.5f, box_size, box_size};

   // Draw checkbox box background
   canvas->addStyleItem(*bg_style, box_rect, style->checkbox.rounding, MONSTRO_CORNER_ALL);

   // Draw checkbox border
   canvas->addRect(box_rect, monstro_color_to_u32(style->checkbox.border_color), style->checkbox.rounding,
                   MONSTRO_CORNER_ALL, style->checkbox.border);

   // Draw checkmark if checked
   if (*value)
   {
      MonstroStyleItem *cursor_style = hovered ? &style->checkbox.cursor_hover : &style->checkbox.cursor_normal;
      float pad = 3.0f;
      MonstroRect check_rect = {box_rect.x + pad, box_rect.y + pad, box_rect.w - pad * 2.0f, box_rect.h - pad * 2.0f};
      canvas->addStyleItem(*cursor_style, check_rect, 0.0f, MONSTRO_CORNER_NONE);
   }

   MonstroVec2 text_pos = {pos.x + box_size + spacing, pos.y + (total_h - text_h) * 0.5f};
   canvas->addText(font, text_pos, monstro_color_to_u32(style->text.color), display);

   monstro_add_item(ctx, total_w, total_h);
   monstro_draw_disabled_overlay(ctx, r);
   monstro_set_last_item(ctx, id, r, hovered, held, pressed, false);

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return pressed;
}
