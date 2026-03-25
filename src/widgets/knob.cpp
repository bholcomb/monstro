#include "../context.h"
#include <cmath>
#include <cstdio>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static const float KNOB_MIN_ANGLE = (float)(M_PI * 0.75);
static const float KNOB_MAX_ANGLE = (float)(M_PI * 2.25);
static const int KNOB_ARC_SEGMENTS = 32;

static bool knob_behavior(MonstroContext *ctx, MonstroRect r, uint64_t id, bool *out_hovered, bool *out_held)
{
   monstro::Window *win = ctx->current_window;

   *out_hovered = false;
   *out_held = false;

   if (ctx->disabled || win != ctx->hovered_window)
      return false;

   float mx = ctx->mouse.pos_x;
   float my = ctx->mouse.pos_y;

   float cx = r.x + r.w * 0.5f;
   float cy = r.y + r.h * 0.5f;
   float radius = r.w * 0.5f;
   float dx = mx - cx;
   float dy = my - cy;
   bool hovered = (dx * dx + dy * dy) <= (radius * radius);

   if (hovered)
   {
      ctx->hovered_id = id;
      if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
      {
         ctx->setActiveId(id);
         ctx->focusWindow(win);
      }
   }

   bool held = false;
   if (ctx->active_id == id)
   {
      if (monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
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

static void draw_style_item_circle(monstro::Canvas *canvas, MonstroStyleItem *item, MonstroVec2 center, float radius)
{
   if (item->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addCircleFilled(center, radius, monstro_color_to_u32(item->data.color), KNOB_ARC_SEGMENTS);
   }
   else if (item->type == MONSTRO_STYLE_ITEM_IMAGE)
   {
      MonstroRect rect = {center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f};
      MonstroVec2 uv_min = {0.0f, 0.0f};
      MonstroVec2 uv_max = {1.0f, 1.0f};
      canvas->addImageRounded(item->data.texture_id, rect, uv_min, uv_max, 0xFFFFFFFF, radius, MONSTRO_CORNER_ALL);
   }
}

bool monstro_knob_sized(MonstroContext *ctx, const char *label, float *value, float min_val, float max_val,
                        const char *format, float diameter, const char *tooltip)
{
   if (!ctx || !ctx->current_window || !label || !value)
      return false;

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style;
   monstro::Canvas *canvas = &win->canvas;
   MonstroFont *font = style->font;
   MonstroStyleKnob *ks = &style->knob;

   uint64_t id = win->getChildId(label);
   const char *display = monstro_display_text(label);

   monstro_add_item(ctx, ks->padding[0], ks->padding[1]);

   float knob_diameter = diameter > 0.0f ? diameter : ks->diameter;
   float knob_radius = knob_diameter * 0.5f;

   float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
   float label_w = font ? monstro_font_measure_text(font, display, 0.0f).x : (float)strlen(display) * 8.0f;

   float total_w = label_w + ks->padding[0] + knob_diameter;
   float total_h = knob_diameter > text_h ? knob_diameter : text_h;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);

   float knob_x = pos.x + label_w + ks->padding[0];
   MonstroVec2 center = {knob_x + knob_radius, pos.y + knob_radius};
   MonstroRect knob_rect = {knob_x, pos.y, knob_diameter, knob_diameter};

   bool hovered = false;
   bool held = false;
   knob_behavior(ctx, knob_rect, id, &hovered, &held);

   float range = max_val - min_val;
   if (range == 0.0f)
      range = 1.0f;

   float old_value = *value;

   if (held && !ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
   {
      float sensitivity = range / (knob_diameter * 2.0f);
      *value += -ctx->mouse.delta_y * sensitivity;
   }

   *value = clampf(*value, min_val, max_val);
   float t = (*value - min_val) / range;

   float angle = KNOB_MIN_ANGLE + t * (KNOB_MAX_ANGLE - KNOB_MIN_ANGLE);

   // Track background arc
   if (ks->track_width > 0.0f)
   {
      float track_radius = knob_radius - ks->track_width * 0.5f;
      if (track_radius > 0.0f)
      {
         canvas->pathClear();
         canvas->pathArcTo(center, track_radius, KNOB_MIN_ANGLE, KNOB_MAX_ANGLE, KNOB_ARC_SEGMENTS);
         canvas->pathStroke(monstro_color_to_u32(ks->track_normal), false, ks->track_width);
      }

      if (t > 0.0f)
      {
         canvas->pathClear();
         canvas->pathArcTo(center, track_radius, KNOB_MIN_ANGLE, angle, KNOB_ARC_SEGMENTS);
         canvas->pathStroke(monstro_color_to_u32(ks->track_filled), false, ks->track_width);
      }
   }

   // Knob body
   MonstroStyleItem *body_style = &ks->normal;
   if (held)
      body_style = &ks->active;
   else if (hovered)
      body_style = &ks->hover;

   float body_radius = knob_radius - ks->track_width;
   if (body_radius < 1.0f)
      body_radius = 1.0f;
   draw_style_item_circle(canvas, body_style, center, body_radius);

   if (ks->border > 0.0f)
   {
      canvas->addCircle(center, body_radius, monstro_color_to_u32(ks->border_color), KNOB_ARC_SEGMENTS, ks->border);
   }

   // Indicator line
   MonstroStyleItem *ind_style = &ks->indicator_normal;
   if (held)
      ind_style = &ks->indicator_active;
   else if (hovered)
      ind_style = &ks->indicator_hover;

   if (ind_style->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      float cos_a = cosf(angle);
      float sin_a = sinf(angle);
      float inner_r = body_radius - ks->indicator_length;
      if (inner_r < 0.0f)
         inner_r = 0.0f;

      MonstroVec2 p1 = {center.x + cos_a * inner_r, center.y + sin_a * inner_r};
      MonstroVec2 p2 = {center.x + cos_a * body_radius, center.y + sin_a * body_radius};
      canvas->addLine(p1, p2, monstro_color_to_u32(ind_style->data.color), ks->indicator_width);
   }
   else if (ind_style->type == MONSTRO_STYLE_ITEM_IMAGE)
   {
      float img_size = body_radius * 2.0f;
      MonstroRect img_rect = {center.x - body_radius, center.y - body_radius, img_size, img_size};
      MonstroVec2 uv_min = {0.0f, 0.0f};
      MonstroVec2 uv_max = {1.0f, 1.0f};
      canvas->addImageRounded(ind_style->data.texture_id, img_rect, uv_min, uv_max, 0xFFFFFFFF, body_radius,
                              MONSTRO_CORNER_ALL);
   }

   // Value text centered on the knob
   if (format && font)
   {
      char buf[64];
      snprintf(buf, sizeof(buf), format, *value);
      MonstroRect text_rect = {knob_rect.x, knob_rect.y, knob_diameter, knob_diameter};
      canvas->addTextEx(font, text_rect, monstro_color_to_u32(style->text.color), buf, MONSTRO_ALIGN_CENTER);
   }

   // Label to the left of the knob
   if (font && label[0] != '#')
   {
      MonstroRect label_rect = {pos.x, pos.y, label_w, knob_diameter};
      canvas->addTextEx(font, label_rect, monstro_color_to_u32(style->text.color), display, MONSTRO_ALIGN_DEFAULT);
   }

   monstro_add_item(ctx, total_w, total_h);

   MonstroRect widget_rect = {pos.x, pos.y, total_w, total_h};
   monstro_draw_disabled_overlay(ctx, widget_rect);
   monstro_set_last_item(ctx, id, widget_rect, hovered, held, held, *value != old_value);

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return *value != old_value;
}

bool monstro_knob(MonstroContext *ctx, const char *label, float *value, float min_val, float max_val,
                  const char *format, const char *tooltip)
{
   return monstro_knob_sized(ctx, label, value, min_val, max_val, format, 0.0f, tooltip);
}
