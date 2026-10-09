#include "../context.h"
#include <cmath>
#include <string.h>

static bool button_behavior(MonstroContext *ctx, MonstroRect r, uint64_t id, bool *out_hovered, bool *out_held,
                            MonstroButtonFlags flags)
{
   monstro::Window *win = ctx->current_window;

   if ((flags & MONSTRO_BUTTON_DISABLED) || ctx->disabled || win != ctx->hovered_window)
   {
      *out_hovered = false;
      *out_held = false;
      return false;
   }

   bool pressed = false;
   float mx = ctx->mouse.pos_x;
   float my = ctx->mouse.pos_y;

   bool hovered = mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
   if (hovered)
   {
      ctx->hovered_id = id;

      bool require_no_key_mods = (flags & MONSTRO_BUTTON_NO_KEY_MODIFIERS) != 0;
      bool mods_ok =
          !require_no_key_mods || !(ctx->keyboard.getMods() & (MONSTRO_MOD_CTRL | MONSTRO_MOD_SHIFT | MONSTRO_MOD_ALT));

      if (mods_ok)
      {
         if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
         {
            if (flags & MONSTRO_BUTTON_PRESSED_ON_CLICK)
            {
               pressed = true;
               ctx->setActiveId(0);
            }
            else
            {
               ctx->setActiveId(id);
            }
            ctx->focusWindow(win);
         }
      }
   }

   bool held = false;
   if (ctx->active_id == id)
   {
      if (monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         held = true;
         if ((flags & MONSTRO_BUTTON_REPEAT) && hovered &&
             monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, true))
         {
            pressed = true;
         }
      }
      else
      {
         if (hovered && !(flags & MONSTRO_BUTTON_PRESSED_ON_CLICK))
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

static void draw_button_background(monstro::Canvas *canvas, MonstroRect r, MonstroStyle *style, bool hovered, bool held)
{
   MonstroStyleItem *background = &style->button.normal;
   if (hovered)
      background = &style->button.hover;
   if (held)
      background = &style->button.active;

   switch (background->type)
   {
   case MONSTRO_STYLE_ITEM_COLOR:
      canvas->addRectFilled(r, monstro_color_to_u32(background->data.color), style->button.rounding,
                            MONSTRO_CORNER_ALL);
      canvas->addRect(r, monstro_color_to_u32(style->button.border_color), style->button.rounding, MONSTRO_CORNER_ALL,
                      style->button.border);
      break;
   case MONSTRO_STYLE_ITEM_IMAGE:
   {
      MonstroVec2 uv_min = {0.0f, 0.0f};
      MonstroVec2 uv_max = {1.0f, 1.0f};
      canvas->addImage(background->data.texture_id, r, uv_min, uv_max, 0xFFFFFFFF);
      break;
   }
   default:
      break;
   }
}

static void draw_button_text(monstro::Canvas *canvas, MonstroFont *font, MonstroRect r, MonstroStyle *style,
                             const char *text, bool hovered, bool held)
{
   if (!font)
      return;

   MonstroColor text_color = style->button.text_normal;
   if (hovered)
      text_color = style->button.text_hover;
   if (held)
      text_color = style->button.text_active;

   canvas->pushClipRect(r);
   canvas->addTextEx(font, r, monstro_color_to_u32(text_color), text, style->button.text_alignment);
   canvas->popClipRect();
}

static void button_size_from_label(MonstroStyle *style, MonstroFont *font, const char *display, float width,
                                   float height, float *out_width, float *out_height)
{
   float text_w = 0.0f;
   if (font)
   {
      text_w = monstro_font_measure_text(font, display, 0.0f).x;
   }
   else if (display)
   {
      text_w = (float)strlen(display) * 8.0f;
   }

   float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
   *out_width = width > 0.0f ? width : (text_w + style->button.padding[0] * 2.0f);
   *out_height = height > 0.0f ? height : (text_h + style->button.padding[1] * 2.0f);
}

static void maybe_close_button_popup(MonstroContext *ctx, MonstroButtonFlags flags, bool pressed)
{
   if (!pressed || !ctx || !ctx->current_window)
      return;
   if ((flags & MONSTRO_BUTTON_DONT_CLOSE_POPUPS) != 0)
      return;
   if (ctx->current_window->flags & MONSTRO_WINDOW_POPUP)
   {
      monstro_close_current_popup(ctx);
   }
}

bool monstro_button_ex(MonstroContext *ctx, const char *label, float width, float height, MonstroButtonFlags flags,
                       const char *tooltip)
{
   monstro::requireWindow(ctx, __func__);
   if (!label)
      return false;

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style;
   monstro::Canvas *canvas = &ctx->current_window->canvas;
   MonstroFont *font = style->font;

   monstro_add_item(ctx, style->button.padding[0], style->button.padding[1]);

   uint64_t id = win->getChildId(label);
   const char *display = monstro_display_text(label);
   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   float w = 0.0f;
   float h = 0.0f;
   button_size_from_label(style, font, display, width, height, &w, &h);

   MonstroRect r = {pos.x, pos.y, w, h};

   bool hovered = false;
   bool held = false;
   bool pressed = button_behavior(ctx, r, id, &hovered, &held, flags);

   draw_button_background(canvas, r, style, hovered, held);
   draw_button_text(canvas, font, r, style, display, hovered, held);
   monstro_draw_disabled_overlay(ctx, r);
   maybe_close_button_popup(ctx, flags, pressed);

   monstro_add_item(ctx, w, h);
   monstro_set_last_item(ctx, id, r, hovered, held, pressed, false);

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return pressed;
}

bool monstro_button(MonstroContext *ctx, const char *label, const char *tooltip)
{
   return monstro_button_ex(ctx, label, 0.0f, 0.0f, MONSTRO_BUTTON_NONE, tooltip);
}

bool monstro_button_sized(MonstroContext *ctx, const char *label, float width, float height, const char *tooltip)
{
   return monstro_button_ex(ctx, label, width, height, MONSTRO_BUTTON_NONE, tooltip);
}

bool monstro_image_button_ex(MonstroContext *ctx, const char *str_id, uint64_t texture_id, float width, float height,
                             MonstroButtonFlags flags, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__);
   if (!str_id)
      return false;

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style;
   monstro::Canvas *canvas = &ctx->current_window->canvas;

   // Add spacing before button (like C#: win.addItem(style.button.padding))
   monstro_add_item(ctx, style->button.padding[0], style->button.padding[1]);

   uint64_t id = win->getChildId(str_id);
   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect r = {pos.x, pos.y, width, height};

   bool hovered = false;
   bool held = false;
   bool pressed = button_behavior(ctx, r, id, &hovered, &held, flags);

   draw_button_background(canvas, r, style, hovered, held);

   MonstroRect ir = r;
   ir.x += style->button.image_padding[0];
   ir.y += style->button.image_padding[1];
   ir.w -= style->button.image_padding[0] * 2.0f;
   ir.h -= style->button.image_padding[1] * 2.0f;

   MonstroVec2 uv_min = {0.0f, 0.0f};
   MonstroVec2 uv_max = {1.0f, 1.0f};
   canvas->addImage(texture_id, ir, uv_min, uv_max, 0xFFFFFFFF);
   monstro_draw_disabled_overlay(ctx, r);
   maybe_close_button_popup(ctx, flags, pressed);

   monstro_add_item(ctx, width, height);
   monstro_set_last_item(ctx, id, r, hovered, held, pressed, false);

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return pressed;
}

bool monstro_image_button(MonstroContext *ctx, const char *str_id, uint64_t texture_id, float width, float height,
                          const char *tooltip)
{
   return monstro_image_button_ex(ctx, str_id, texture_id, width, height, MONSTRO_BUTTON_NONE, tooltip);
}

bool monstro_invisible_button_ex(MonstroContext *ctx, const char *str_id, float width, float height,
                                 MonstroButtonFlags flags)
{
   monstro::requireWindow(ctx, __func__);
   if (!str_id)
      return false;

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   uint64_t id = win->getChildId(str_id);
   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect r = {pos.x, pos.y, width, height};

   bool hovered = false;
   bool held = false;
   bool pressed = button_behavior(ctx, r, id, &hovered, &held, flags);
   maybe_close_button_popup(ctx, flags, pressed);

   monstro_add_item(ctx, width, height);
   monstro_set_last_item(ctx, id, r, hovered, held, pressed, false);

   return pressed;
}

bool monstro_invisible_button(MonstroContext *ctx, const char *str_id, float width, float height)
{
   return monstro_invisible_button_ex(ctx, str_id, width, height, MONSTRO_BUTTON_NONE);
}
