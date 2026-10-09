#include "../context.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string.h>

static double clampd(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }

static bool slider_behavior(MonstroContext *ctx, MonstroRect r, uint64_t id, bool *out_hovered, bool *out_held)
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
      if (!ctx->disabled && monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         held = true;
      }
      else
      {
         ctx->setActiveId(0);
      }
   }

   *out_hovered = hovered || held;
   *out_held = held;
   return held;
}

// The slider body for both value types. Values travel as double so that an
// int slider keeps every int exactly; an int slider snaps to whole numbers
// and formats its value text as an int.
static bool slider_impl(MonstroContext *ctx, const char *label, double *value, double min_val, double max_val,
                        bool integer, const char *format, float width, float height, const char *tooltip)
{

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style();
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;
   MonstroStyleSlider *ss = &style->slider;

   uint64_t id = win->getChildId(label);
   const char *display = monstro_display_text(label);

   monstro_add_item(ctx, ss->padding[0], ss->padding[1]);

   float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
   float label_w = monstro_font_measure_text(font, display, 0.0f).x;
   if (label_w == 0.0f)
      label_w = (float)strlen(display) * 8.0f;

   float slider_h = height > 0.0f ? height : std::max(text_h, ss->cursor_size[1]) + ss->padding[1] * 2.0f;
   float slider_w = width > 0.0f ? width : 120.0f;
   if (width <= 0.0f)
   {
      monstro::Layout *layout = win->getCurrentLayout();
      if (layout && layout->myAvailableWidth > 0.0f)
      {
         float avail = layout->myAvailableWidth - label_w - ss->spacing[0];
         if (avail > 0.0f)
            slider_w = avail;
      }
   }
   float total_w = label_w + ss->spacing[0] + slider_w;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);

   MonstroRect label_rect = {pos.x, pos.y, label_w, slider_h};
   MonstroRect slider_rect = {pos.x + label_w + ss->spacing[0], pos.y, slider_w, slider_h};

   bool hovered = false;
   bool held = false;
   bool dragging = slider_behavior(ctx, slider_rect, id, &hovered, &held);

   double range = max_val - min_val;
   if (range == 0.0)
      range = 1.0;

   double old_value = *value;

   if (held)
   {
      double ratio = clampd((ctx->mouse.pos_x - slider_rect.x) / slider_rect.w, 0.0, 1.0);
      *value = integer ? min_val + std::round(ratio * range) : min_val + ratio * range;
   }

   *value = clampd(*value, min_val, max_val);
   float t = (float)((*value - min_val) / range);

   // Background bar
   float bar_y = slider_rect.y + (slider_rect.h - ss->bar_height) * 0.5f;
   MonstroRect bar_rect = {slider_rect.x, bar_y, slider_rect.w, ss->bar_height};

   MonstroColor bar_col = hovered ? ss->bar_hover : ss->bar_normal;
   canvas->addRectFilled(bar_rect, monstro_color_to_u32(bar_col), ss->rounding, MONSTRO_CORNER_ALL);

   if (ss->border > 0.0f)
   {
      canvas->addRect(bar_rect, monstro_color_to_u32(ss->border_color), ss->rounding, MONSTRO_CORNER_ALL, ss->border);
   }

   // Filled portion
   if (t > 0.0f)
   {
      MonstroRect filled_rect = {bar_rect.x, bar_rect.y, bar_rect.w * t, bar_rect.h};
      canvas->addRectFilled(filled_rect, monstro_color_to_u32(ss->bar_filled), ss->rounding, MONSTRO_CORNER_LEFT);
   }

   // Cursor
   float cursor_x = slider_rect.x + t * slider_rect.w - ss->cursor_size[0] * 0.5f;
   float cursor_y = slider_rect.y + (slider_rect.h - ss->cursor_size[1]) * 0.5f;
   MonstroRect cursor_rect = {cursor_x, cursor_y, ss->cursor_size[0], ss->cursor_size[1]};

   MonstroStyleItem *cursor_style = &ss->cursor_normal;
   if (held)
      cursor_style = &ss->cursor_active;
   else if (hovered)
      cursor_style = &ss->cursor_hover;

   if (cursor_style->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addRectFilled(cursor_rect, monstro_color_to_u32(cursor_style->data.color), ss->rounding,
                            MONSTRO_CORNER_ALL);
   }

   // Value text overlay on the slider bar
   if (format && font)
   {
      char buf[64];
      if (integer)
         snprintf(buf, sizeof(buf), format, (int)*value);
      else
         snprintf(buf, sizeof(buf), format, *value);
      monstro_utf8_trim_partial(buf);
      canvas->addTextEx(font, slider_rect, monstro_color_to_u32(style->text.color), buf, MONSTRO_ALIGN_CENTER);
   }

   // Label
   if (font)
   {
      canvas->addTextEx(font, label_rect, monstro_color_to_u32(style->text.color), display, MONSTRO_ALIGN_LEFT);
   }

   monstro_add_item(ctx, total_w, slider_h);

   MonstroRect full_r = {pos.x, pos.y, total_w, slider_h};
   monstro_draw_disabled_overlay(ctx, full_r);
   monstro_set_last_item(ctx, id, full_r, hovered, held, dragging, *value != old_value);

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return *value != old_value;
}

bool monstro_slider_float_ex(MonstroContext *ctx, const char *label, float *value, float min_val, float max_val,
                             const char *format, float width, float height, MonstroSliderFlags flags,
                             const char *tooltip)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label), MONSTRO_ARG(value)});
   (void)flags;

   double v = *value;
   bool changed = slider_impl(ctx, label, &v, min_val, max_val, false, format, width, height, tooltip);
   *value = (float)v;
   return changed;
}

bool monstro_slider_float(MonstroContext *ctx, const char *label, float *value, float min_val, float max_val,
                          const char *format, const char *tooltip)
{
   return monstro_slider_float_ex(ctx, label, value, min_val, max_val, format, 0.0f, 0.0f, MONSTRO_SLIDER_NONE,
                                  tooltip);
}

bool monstro_slider_int_ex(MonstroContext *ctx, const char *label, int *value, int min_val, int max_val,
                           const char *format, float width, float height, MonstroSliderFlags flags, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label), MONSTRO_ARG(value)});
   (void)flags;

   double v = *value;
   bool changed = slider_impl(ctx, label, &v, min_val, max_val, true, format ? format : "%d", width, height, tooltip);
   *value = (int)v;
   return changed;
}

bool monstro_slider_int(MonstroContext *ctx, const char *label, int *value, int min_val, int max_val,
                        const char *format, const char *tooltip)
{
   return monstro_slider_int_ex(ctx, label, value, min_val, max_val, format, 0.0f, 0.0f, MONSTRO_SLIDER_NONE, tooltip);
}
