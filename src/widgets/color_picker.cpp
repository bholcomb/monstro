#include "../context.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void rgb_to_hsv(float r, float g, float b, float *h, float *s, float *v)
{
   float mx = std::max({r, g, b});
   float mn = std::min({r, g, b});
   float d = mx - mn;

   *v = mx;
   *s = (mx > 0.0f) ? (d / mx) : 0.0f;

   if (d < 1e-6f)
   {
      *h = 0.0f;
   }
   else if (mx == r)
   {
      *h = std::fmod((g - b) / d + 6.0f, 6.0f) / 6.0f;
   }
   else if (mx == g)
   {
      *h = ((b - r) / d + 2.0f) / 6.0f;
   }
   else
   {
      *h = ((r - g) / d + 4.0f) / 6.0f;
   }
}

static void hsv_to_rgb(float h, float s, float v, float *r, float *g, float *b)
{
   if (s <= 0.0f)
   {
      *r = *g = *b = v;
      return;
   }
   float hh = std::fmod(h * 6.0f, 6.0f);
   int i = (int)hh;
   float f = hh - (float)i;
   float p = v * (1.0f - s);
   float q = v * (1.0f - s * f);
   float t = v * (1.0f - s * (1.0f - f));
   switch (i)
   {
   case 0:
      *r = v;
      *g = t;
      *b = p;
      break;
   case 1:
      *r = q;
      *g = v;
      *b = p;
      break;
   case 2:
      *r = p;
      *g = v;
      *b = t;
      break;
   case 3:
      *r = p;
      *g = q;
      *b = v;
      break;
   case 4:
      *r = t;
      *g = p;
      *b = v;
      break;
   default:
      *r = v;
      *g = p;
      *b = q;
      break;
   }
}

static uint32_t pack_rgba(float r, float g, float b, float a)
{
   uint8_t ri = (uint8_t)(clampf(r, 0, 1) * 255.0f + 0.5f);
   uint8_t gi = (uint8_t)(clampf(g, 0, 1) * 255.0f + 0.5f);
   uint8_t bi = (uint8_t)(clampf(b, 0, 1) * 255.0f + 0.5f);
   uint8_t ai = (uint8_t)(clampf(a, 0, 1) * 255.0f + 0.5f);
   return ((uint32_t)ai << 24) | ((uint32_t)bi << 16) | ((uint32_t)gi << 8) | (uint32_t)ri;
}

static uint32_t hue_color(float h)
{
   float r, g, b;
   hsv_to_rgb(h, 1.0f, 1.0f, &r, &g, &b);
   return pack_rgba(r, g, b, 1.0f);
}

static bool drag_behavior(MonstroContext *ctx, MonstroRect r, uint64_t id, bool *out_hovered, bool *out_held)
{
   bool hovered = ctx->hoverItem(id, r);
   if (hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
   {
      ctx->setActiveId(id);
      ctx->focusWindow(ctx->current_window);
   }

   bool held = false;
   if (ctx->active_id == id)
   {
      // Held for the frame of a tap too, which presses and releases in it, and let go at its end.
      held = !ctx->disabled && ctx->mouse.isHeld(MONSTRO_MOUSE_BUTTON_LEFT);
      if (ctx->disabled || !monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
         ctx->setActiveId(0);
   }

   *out_hovered = hovered || held;
   *out_held = held;
   return held;
}

static void draw_checkerboard(monstro::Canvas *canvas, MonstroRect r, float cell_size)
{
   uint32_t light = pack_rgba(0.7f, 0.7f, 0.7f, 1.0f);
   uint32_t dark = pack_rgba(0.5f, 0.5f, 0.5f, 1.0f);
   int cols = (int)(r.w / cell_size) + 1;
   int rows = (int)(r.h / cell_size) + 1;
   for (int row = 0; row < rows; row++)
   {
      for (int col = 0; col < cols; col++)
      {
         float cx = r.x + (float)col * cell_size;
         float cy = r.y + (float)row * cell_size;
         float cw = std::min(cell_size, r.x + r.w - cx);
         float ch = std::min(cell_size, r.y + r.h - cy);
         if (cw <= 0 || ch <= 0)
            continue;
         uint32_t c = ((row + col) % 2 == 0) ? light : dark;
         MonstroRect cr = {cx, cy, cw, ch};
         canvas->addRectFilled(cr, c, 0, MONSTRO_CORNER_NONE);
      }
   }
}

bool monstro_color_picker(MonstroContext *ctx, const char *label, MonstroColor *color, MonstroColorPickerFlags flags)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(color)});

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   monstro::Canvas *canvas = ctx->canvas;
   bool changed = false;
   float h, s, v;
   rgb_to_hsv(color->r, color->g, color->b, &h, &s, &v);

   const float sv_size = 150.0f;
   const float bar_w = 20.0f;
   const float bar_gap = 6.0f;

   bool show_alpha = !(flags & MONSTRO_COLOR_PICKER_NO_ALPHA);
   bool show_sliders = !(flags & MONSTRO_COLOR_PICKER_NO_SLIDERS);
   bool show_hex = !(flags & MONSTRO_COLOR_PICKER_NO_HEX);
   bool show_preview = !(flags & MONSTRO_COLOR_PICKER_NO_PREVIEW);

   float total_w = sv_size + bar_gap + bar_w;
   if (show_alpha)
      total_w += bar_gap + bar_w;

   monstro_add_item(ctx, 0, 2);
   MonstroVec2 origin = monstro_get_cursor_screen_pos(ctx);

   /* --- SV square --- */
   {
      uint64_t id = win->getChildId("##cp_sv");
      MonstroRect sv_rect = {origin.x, origin.y, sv_size, sv_size};
      bool hov, held;
      drag_behavior(ctx, sv_rect, id, &hov, &held);

      if (held)
      {
         s = clampf((ctx->mouse.pos_x - sv_rect.x) / sv_rect.w, 0, 1);
         v = 1.0f - clampf((ctx->mouse.pos_y - sv_rect.y) / sv_rect.h, 0, 1);
         changed = true;
      }

      uint32_t hue_col = hue_color(h);
      canvas->addRectFilled(sv_rect, 0xFFFFFFFF, 0, MONSTRO_CORNER_NONE);
      canvas->addRectFilledMultiColor(sv_rect, 0xFFFFFFFF, hue_col, hue_col, 0xFFFFFFFF);
      canvas->addRectFilledMultiColor(sv_rect, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000);
      canvas->addRect(sv_rect, pack_rgba(0.3f, 0.3f, 0.3f, 1.0f), 0, MONSTRO_CORNER_NONE, 1.0f);

      float cx = sv_rect.x + s * sv_rect.w;
      float cy = sv_rect.y + (1.0f - v) * sv_rect.h;
      MonstroVec2 cursor_center = {cx, cy};
      canvas->addCircle(cursor_center, 5.0f, 0xFFFFFFFF, 12, 2.0f);
      canvas->addCircle(cursor_center, 4.0f, 0xFF000000, 12, 1.0f);
   }

   /* --- Hue bar --- */
   {
      uint64_t id = win->getChildId("##cp_hue");
      float hue_x = origin.x + sv_size + bar_gap;
      MonstroRect hue_rect = {hue_x, origin.y, bar_w, sv_size};
      bool hov, held;
      drag_behavior(ctx, hue_rect, id, &hov, &held);

      if (held)
      {
         h = clampf((ctx->mouse.pos_y - hue_rect.y) / hue_rect.h, 0.0f, 1.0f);
         changed = true;
      }

      int segments = 6;
      float seg_h = sv_size / (float)segments;
      float hue_colors[] = {0, 1.0f / 6, 2.0f / 6, 3.0f / 6, 4.0f / 6, 5.0f / 6, 1.0f};
      for (int i = 0; i < segments; i++)
      {
         MonstroRect seg = {hue_x, origin.y + (float)i * seg_h, bar_w, seg_h};
         uint32_t top = hue_color(hue_colors[i]);
         uint32_t bot = hue_color(hue_colors[i + 1]);
         canvas->addRectFilledMultiColor(seg, top, top, bot, bot);
      }
      canvas->addRect(hue_rect, pack_rgba(0.3f, 0.3f, 0.3f, 1.0f), 0, MONSTRO_CORNER_NONE, 1.0f);

      float marker_y = hue_rect.y + h * hue_rect.h;
      MonstroRect marker = {hue_rect.x - 1, marker_y - 2, hue_rect.w + 2, 4};
      canvas->addRect(marker, 0xFFFFFFFF, 0, MONSTRO_CORNER_NONE, 2.0f);
   }

   /* --- Alpha bar --- */
   if (show_alpha)
   {
      uint64_t id = win->getChildId("##cp_alpha");
      float alpha_x = origin.x + sv_size + bar_gap + bar_w + bar_gap;
      MonstroRect alpha_rect = {alpha_x, origin.y, bar_w, sv_size};
      bool hov, held;
      drag_behavior(ctx, alpha_rect, id, &hov, &held);

      if (held)
      {
         color->a = 1.0f - clampf((ctx->mouse.pos_y - alpha_rect.y) / alpha_rect.h, 0, 1);
         changed = true;
      }

      draw_checkerboard(canvas, alpha_rect, 6.0f);

      float cr, cg, cb;
      hsv_to_rgb(h, s, v, &cr, &cg, &cb);
      uint32_t top_col = pack_rgba(cr, cg, cb, 1.0f);
      uint32_t bot_col = pack_rgba(cr, cg, cb, 0.0f);
      canvas->addRectFilledMultiColor(alpha_rect, top_col, top_col, bot_col, bot_col);
      canvas->addRect(alpha_rect, pack_rgba(0.3f, 0.3f, 0.3f, 1.0f), 0, MONSTRO_CORNER_NONE, 1.0f);

      float marker_y = alpha_rect.y + (1.0f - color->a) * alpha_rect.h;
      MonstroRect marker = {alpha_rect.x - 1, marker_y - 2, alpha_rect.w + 2, 4};
      canvas->addRect(marker, 0xFFFFFFFF, 0, MONSTRO_CORNER_NONE, 2.0f);
   }

   monstro_add_item(ctx, total_w, sv_size);

   /* --- Preview swatch --- */
   if (show_preview)
   {
      monstro_add_item(ctx, 0, 4);
      MonstroVec2 prev_pos = monstro_get_cursor_screen_pos(ctx);
      float prev_w = total_w;
      float prev_h = 24.0f;
      MonstroRect prev_rect = {prev_pos.x, prev_pos.y, prev_w, prev_h};

      draw_checkerboard(canvas, prev_rect, 6.0f);

      float cr, cg, cb;
      hsv_to_rgb(h, s, v, &cr, &cg, &cb);
      canvas->addRectFilled(prev_rect, pack_rgba(cr, cg, cb, color->a), 0, MONSTRO_CORNER_NONE);
      canvas->addRect(prev_rect, pack_rgba(0.3f, 0.3f, 0.3f, 1.0f), 0, MONSTRO_CORNER_NONE, 1.0f);

      monstro_add_item(ctx, prev_w, prev_h);
   }

   /* --- RGB sliders --- */
   if (show_sliders)
   {
      float cr, cg, cb;
      hsv_to_rgb(h, s, v, &cr, &cg, &cb);
      float old_r = cr, old_g = cg, old_b = cb;

      monstro_push_id(ctx, label);

      if (monstro_slider_float(ctx, "R", &cr, 0, 1, "%.3f", nullptr))
         changed = true;
      if (monstro_slider_float(ctx, "G", &cg, 0, 1, "%.3f", nullptr))
         changed = true;
      if (monstro_slider_float(ctx, "B", &cb, 0, 1, "%.3f", nullptr))
         changed = true;
      if (show_alpha)
      {
         if (monstro_slider_float(ctx, "A", &color->a, 0, 1, "%.3f", nullptr))
            changed = true;
      }

      if (cr != old_r || cg != old_g || cb != old_b)
      {
         rgb_to_hsv(cr, cg, cb, &h, &s, &v);
      }

      monstro_pop_id(ctx);
   }

   /* --- Hex input --- */
   if (show_hex)
   {
      float cr, cg, cb;
      hsv_to_rgb(h, s, v, &cr, &cg, &cb);

      char hex_buf[16];
      if (show_alpha)
      {
         snprintf(hex_buf, sizeof(hex_buf), "%02X%02X%02X%02X", (int)(clampf(cr, 0, 1) * 255 + 0.5f),
                  (int)(clampf(cg, 0, 1) * 255 + 0.5f), (int)(clampf(cb, 0, 1) * 255 + 0.5f),
                  (int)(clampf(color->a, 0, 1) * 255 + 0.5f));
      }
      else
      {
         snprintf(hex_buf, sizeof(hex_buf), "%02X%02X%02X", (int)(clampf(cr, 0, 1) * 255 + 0.5f),
                  (int)(clampf(cg, 0, 1) * 255 + 0.5f), (int)(clampf(cb, 0, 1) * 255 + 0.5f));
      }

      monstro_push_id(ctx, label);
      if (monstro_input_text(ctx, "cp_hex##Hex", hex_buf, sizeof(hex_buf), MONSTRO_INPUT_TEXT_CHARS_HEXADECIMAL,
                             nullptr))
      {
         unsigned int rv = 0, gv = 0, bv = 0, av = 255;
         int len = (int)strlen(hex_buf);
         if (len == 6)
         {
            sscanf(hex_buf, "%02x%02x%02x", &rv, &gv, &bv);
            cr = (float)rv / 255.0f;
            cg = (float)gv / 255.0f;
            cb = (float)bv / 255.0f;
            rgb_to_hsv(cr, cg, cb, &h, &s, &v);
            changed = true;
         }
         else if (len == 8)
         {
            sscanf(hex_buf, "%02x%02x%02x%02x", &rv, &gv, &bv, &av);
            cr = (float)rv / 255.0f;
            cg = (float)gv / 255.0f;
            cb = (float)bv / 255.0f;
            color->a = (float)av / 255.0f;
            rgb_to_hsv(cr, cg, cb, &h, &s, &v);
            changed = true;
         }
      }
      monstro_pop_id(ctx);
   }

   if (changed)
   {
      hsv_to_rgb(h, s, v, &color->r, &color->g, &color->b);
   }

   return changed;
}

bool monstro_color_button(MonstroContext *ctx, const char *label, MonstroColor color, float width, float height)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label)});

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   monstro::Canvas *canvas = ctx->canvas;
   MonstroStyle *style = &ctx->style();

   monstro_add_item(ctx, style->button.padding[0], style->button.padding[1]);

   uint64_t id = win->getItemId(label);
   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect r = {pos.x, pos.y, width, height};

   bool held = false, pressed = false;
   bool hovered = ctx->hoverItem(id, r);
   if (hovered && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
   {
      ctx->setActiveId(id);
      ctx->focusWindow(win);
   }
   if (ctx->active_id == id)
   {
      if (!ctx->disabled && monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         held = true;
      }
      else
      {
         if (hovered)
            pressed = true;
         ctx->setActiveId(0);
      }
   }

   draw_checkerboard(canvas, r, 4.0f);
   canvas->addRectFilled(r, monstro_color_to_u32(color), style->button.rounding, MONSTRO_CORNER_ALL);

   uint32_t border = hovered ? 0xFFFFFFFF : pack_rgba(0.4f, 0.4f, 0.4f, 1.0f);
   canvas->addRect(r, border, style->button.rounding, MONSTRO_CORNER_ALL, held ? 2.0f : 1.0f);
   monstro_draw_disabled_overlay(ctx, r);

   monstro_add_item(ctx, width, height);
   monstro_set_last_item(ctx, id, r, hovered, held, pressed, false);

   return pressed;
}
