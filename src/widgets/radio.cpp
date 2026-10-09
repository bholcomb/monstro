#include "../context.h"
#include <cmath>
#include <string.h>

bool monstro_radio_button(MonstroContext *ctx, const char *label, int *value, int button_value, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label), MONSTRO_ARG(value)});

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style();
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;
   MonstroStyleToggle *os = &style->option;

   uint64_t id = win->getChildId(label);
   const char *display = monstro_display_text(label);

   monstro_add_item(ctx, os->padding[0], os->padding[1]);

   float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
   float circle_d = text_h;
   float radius = circle_d * 0.5f;
   float text_w = monstro_font_measure_text(font, display, 0.0f).x;
   if (text_w == 0.0f)
      text_w = (float)strlen(display) * 8.0f;

   float total_w = circle_d + os->spacing + text_w;
   float total_h = fmaxf(circle_d, text_h);

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect r = {pos.x, pos.y, total_w, total_h};

   bool hovered = false;
   bool pressed = false;

   if (!ctx->disabled && win == ctx->hovered_window)
   {
      float mx = ctx->mouse.pos_x;
      float my = ctx->mouse.pos_y;
      hovered = mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;

      if (hovered)
      {
         ctx->hovered_id = id;
         if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
         {
            pressed = true;
         }
      }
   }

   bool is_selected = (*value == button_value);

   if (pressed)
   {
      *value = button_value;
      is_selected = true;
   }

   MonstroStyleItem *bg = &os->normal;
   if (is_selected)
      bg = &os->active;
   if (hovered && !is_selected)
      bg = &os->hover;

   float cx = pos.x + radius;
   float cy = pos.y + total_h * 0.5f;
   MonstroVec2 center = {cx, cy};

   if (bg->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addCircleFilled(center, radius, monstro_color_to_u32(bg->data.color), 24);
   }

   canvas->addCircle(center, radius, monstro_color_to_u32(os->border_color), 24, os->border > 0.0f ? os->border : 1.0f);

   if (is_selected)
   {
      MonstroStyleItem *cursor = hovered ? &os->cursor_hover : &os->cursor_normal;
      uint32_t dot_color;
      if (cursor->type == MONSTRO_STYLE_ITEM_COLOR)
      {
         dot_color = monstro_color_to_u32(cursor->data.color);
      }
      else
      {
         dot_color = 0xFFFFFFFF;
      }
      canvas->addCircleFilled(center, radius * 0.5f, dot_color, 24);
   }

   if (font)
   {
      MonstroVec2 text_pos = {pos.x + circle_d + os->spacing, pos.y + (total_h - text_h) * 0.5f};
      canvas->addText(font, text_pos, monstro_color_to_u32(hovered ? os->text_hover : os->text_normal), display);
   }

   monstro_add_item(ctx, total_w, total_h);
   monstro_draw_disabled_overlay(ctx, r);
   monstro_set_last_item(ctx, id, r, hovered, false, pressed, false);

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return pressed;
}
