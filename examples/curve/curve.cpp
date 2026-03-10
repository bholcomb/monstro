#include "curve.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

void curve_editor_init(CurveEditor *ce, float width, float height)
{
   memset(ce, 0, sizeof(*ce));
   ce->width = width > 0 ? width : 400.0f;
   ce->height = height > 0 ? height : 300.0f;
   ce->selected_point = -1;
   ce->dragging_handle = -1;
   ce->curve_color = 0xFF44AAFF;
   ce->point_color = 0xFFFFFFFF;
   ce->handle_color = 0xFFFF8844;
   ce->grid_color = 0x20FFFFFF;
}

void curve_editor_add_point(CurveEditor *ce, float x, float y)
{
   if (ce->point_count >= CURVE_MAX_POINTS)
      return;
   int i = ce->point_count++;
   ce->points[i].x = x;
   ce->points[i].y = y;
   float handle_dist = 0.1f;
   ce->points[i].ctrl_left_x = x - handle_dist;
   ce->points[i].ctrl_left_y = y;
   ce->points[i].ctrl_right_x = x + handle_dist;
   ce->points[i].ctrl_right_y = y;
}

static float to_screen_x(float val, float ox, float w) { return ox + val * w; }
static float to_screen_y(float val, float oy, float h) { return oy + (1.0f - val) * h; }
static float from_screen_x(float px, float ox, float w) { return (px - ox) / w; }
static float from_screen_y(float py, float oy, float h) { return 1.0f - (py - oy) / h; }

static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }

static float dist2(float ax, float ay, float bx, float by)
{
   float dx = ax - bx, dy = ay - by;
   return dx * dx + dy * dy;
}

void curve_editor_draw(CurveEditor *ce, MonstroContext *ctx)
{
   monstro_add_item(ctx, 0, 4);
   monstro_label(ctx, "Curve Editor");
   monstro_add_item(ctx, 0, 2);

   MonstroVec2 origin = monstro_get_cursor_screen_pos(ctx);
   float w = ce->width, h = ce->height;
   MonstroRect area = {origin.x, origin.y, w, h};
   MonstroCanvas *canvas = monstro_get_window_canvas(ctx);

   monstro_draw_rect_filled(canvas, area, 0xFF1A1A2E, 0, MONSTRO_CORNER_NONE);

   for (int i = 1; i <= 4; i++)
   {
      float t = (float)i / 5.0f;
      monstro_draw_line(canvas, {origin.x + t * w, origin.y}, {origin.x + t * w, origin.y + h}, ce->grid_color, 1.0f);
      monstro_draw_line(canvas, {origin.x, origin.y + t * h}, {origin.x + w, origin.y + t * h}, ce->grid_color, 1.0f);
   }

   monstro_draw_rect(canvas, area, 0x80FFFFFF, 0, MONSTRO_CORNER_NONE, 1.0f);

   monstro_push_clip_rect(canvas, area);

   for (int i = 0; i < ce->point_count - 1; i++)
   {
      CurvePoint *a = &ce->points[i];
      CurvePoint *b = &ce->points[i + 1];
      MonstroVec2 p1 = {to_screen_x(a->x, origin.x, w), to_screen_y(a->y, origin.y, h)};
      MonstroVec2 p2 = {to_screen_x(a->ctrl_right_x, origin.x, w), to_screen_y(a->ctrl_right_y, origin.y, h)};
      MonstroVec2 p3 = {to_screen_x(b->ctrl_left_x, origin.x, w), to_screen_y(b->ctrl_left_y, origin.y, h)};
      MonstroVec2 p4 = {to_screen_x(b->x, origin.x, w), to_screen_y(b->y, origin.y, h)};
      monstro_draw_bezier_cubic(canvas, p1, p2, p3, p4, ce->curve_color, 2.0f, 32);
   }

   if (ce->selected_point >= 0 && ce->selected_point < ce->point_count)
   {
      CurvePoint *p = &ce->points[ce->selected_point];
      MonstroVec2 ps = {to_screen_x(p->x, origin.x, w), to_screen_y(p->y, origin.y, h)};
      MonstroVec2 cl = {to_screen_x(p->ctrl_left_x, origin.x, w), to_screen_y(p->ctrl_left_y, origin.y, h)};
      MonstroVec2 cr = {to_screen_x(p->ctrl_right_x, origin.x, w), to_screen_y(p->ctrl_right_y, origin.y, h)};
      monstro_draw_line(canvas, cl, ps, ce->handle_color, 1.0f);
      monstro_draw_line(canvas, ps, cr, ce->handle_color, 1.0f);
      monstro_draw_circle_filled(canvas, cl, 4.0f, ce->handle_color, 8);
      monstro_draw_circle_filled(canvas, cr, 4.0f, ce->handle_color, 8);
   }

   for (int i = 0; i < ce->point_count; i++)
   {
      MonstroVec2 ps = {to_screen_x(ce->points[i].x, origin.x, w), to_screen_y(ce->points[i].y, origin.y, h)};
      uint32_t col = (i == ce->selected_point) ? 0xFF00FF00 : ce->point_color;
      monstro_draw_circle_filled(canvas, ps, 5.0f, col, 12);
      monstro_draw_circle(canvas, ps, 5.0f, 0xFF000000, 12, 1.0f);
   }

   monstro_pop_clip_rect(canvas);

   if (monstro_invisible_button(ctx, "##curve_area", w, h))
   {
   }

   MonstroVec2 mp = monstro_get_mouse_pos(ctx);
   float mx = mp.x, my = mp.y;

   if (monstro_is_item_hovered(ctx) && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
   {
      float best_d = 100.0f;
      int best_i = -1;
      int best_handle = -1;

      for (int i = 0; i < ce->point_count; i++)
      {
         CurvePoint *p = &ce->points[i];
         float px = to_screen_x(p->x, origin.x, w);
         float py = to_screen_y(p->y, origin.y, h);
         float d = dist2(mx, my, px, py);
         if (d < best_d)
         {
            best_d = d;
            best_i = i;
            best_handle = 0;
         }

         if (i == ce->selected_point)
         {
            float clx = to_screen_x(p->ctrl_left_x, origin.x, w);
            float cly = to_screen_y(p->ctrl_left_y, origin.y, h);
            d = dist2(mx, my, clx, cly);
            if (d < best_d)
            {
               best_d = d;
               best_i = i;
               best_handle = 1;
            }

            float crx = to_screen_x(p->ctrl_right_x, origin.x, w);
            float cry = to_screen_y(p->ctrl_right_y, origin.y, h);
            d = dist2(mx, my, crx, cry);
            if (d < best_d)
            {
               best_d = d;
               best_i = i;
               best_handle = 2;
            }
         }
      }

      ce->selected_point = best_i;
      ce->dragging_handle = best_handle;
   }

   if (monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT) && ce->selected_point >= 0 && ce->dragging_handle >= 0)
   {
      float dx = from_screen_x(mx, origin.x, w);
      float dy = from_screen_y(my, origin.y, h);
      dx = clampf(dx, 0, 1);
      dy = clampf(dy, 0, 1);

      CurvePoint *p = &ce->points[ce->selected_point];
      if (ce->dragging_handle == 0)
      {
         float ox = dx - p->x, oy = dy - p->y;
         p->x = dx;
         p->y = dy;
         p->ctrl_left_x += ox;
         p->ctrl_left_y += oy;
         p->ctrl_right_x += ox;
         p->ctrl_right_y += oy;
      }
      else if (ce->dragging_handle == 1)
      {
         p->ctrl_left_x = dx;
         p->ctrl_left_y = dy;
      }
      else
      {
         p->ctrl_right_x = dx;
         p->ctrl_right_y = dy;
      }
   }

   if (monstro_is_mouse_released(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
   {
      ce->dragging_handle = -1;
   }
}
