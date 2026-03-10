#include "plot.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

void plot_init(PlotWidget *pw, const char *title, float width, float height)
{
   memset(pw, 0, sizeof(*pw));
   strncpy(pw->title, title ? title : "Plot", sizeof(pw->title) - 1);
   pw->width = width > 0 ? width : 400.0f;
   pw->height = height > 0 ? height : 250.0f;
   pw->auto_fit = true;
   pw->show_grid = true;
   pw->show_crosshair = true;
}

int plot_add_series(PlotWidget *pw, const char *label, PlotType type, uint32_t color)
{
   if (pw->series_count >= PLOT_MAX_SERIES)
      return -1;
   int i = pw->series_count++;
   strncpy(pw->series[i].label, label ? label : "", sizeof(pw->series[i].label) - 1);
   pw->series[i].type = type;
   pw->series[i].color = color;
   pw->series[i].bar_width = 0.8f;
   return i;
}

void plot_set_data(PlotWidget *pw, int si, const float *x, const float *y, int count)
{
   if (si < 0 || si >= pw->series_count)
      return;
   int n = count < PLOT_MAX_POINTS ? count : PLOT_MAX_POINTS;
   memcpy(pw->series[si].x, x, (size_t)n * sizeof(float));
   memcpy(pw->series[si].y, y, (size_t)n * sizeof(float));
   pw->series[si].count = n;
}

void plot_set_range(PlotWidget *pw, float x_min, float x_max, float y_min, float y_max)
{
   pw->x_min = x_min;
   pw->x_max = x_max;
   pw->y_min = y_min;
   pw->y_max = y_max;
   pw->auto_fit = false;
}

static void compute_range(PlotWidget *pw)
{
   if (!pw->auto_fit)
      return;
   float xmin = 1e30f, xmax = -1e30f, ymin = 1e30f, ymax = -1e30f;
   for (int s = 0; s < pw->series_count; s++)
   {
      PlotSeries *sr = &pw->series[s];
      for (int i = 0; i < sr->count; i++)
      {
         if (sr->x[i] < xmin)
            xmin = sr->x[i];
         if (sr->x[i] > xmax)
            xmax = sr->x[i];
         if (sr->y[i] < ymin)
            ymin = sr->y[i];
         if (sr->y[i] > ymax)
            ymax = sr->y[i];
      }
   }
   if (xmin >= xmax)
   {
      xmin = 0;
      xmax = 1;
   }
   if (ymin >= ymax)
   {
      ymin = 0;
      ymax = 1;
   }
   float xpad = (xmax - xmin) * 0.05f;
   float ypad = (ymax - ymin) * 0.1f;
   pw->x_min = xmin - xpad;
   pw->x_max = xmax + xpad;
   pw->y_min = ymin - ypad;
   pw->y_max = ymax + ypad;
}

static float map_x(float val, float vmin, float vmax, float px_min, float px_max)
{
   if (vmax <= vmin)
      return px_min;
   return px_min + (val - vmin) / (vmax - vmin) * (px_max - px_min);
}

static float map_y(float val, float vmin, float vmax, float py_min, float py_max)
{
   if (vmax <= vmin)
      return py_max;
   return py_max - (val - vmin) / (vmax - vmin) * (py_max - py_min);
}

void plot_draw(PlotWidget *pw, MonstroContext *ctx)
{
   compute_range(pw);

   monstro_add_item(ctx, 0, 4);
   if (pw->title[0])
   {
      monstro_label(ctx, pw->title);
   }

   monstro_add_item(ctx, 0, 2);
   MonstroVec2 origin = monstro_get_cursor_screen_pos(ctx);

   float plot_w = pw->width;
   float plot_h = pw->height;
   MonstroRect plot_rect = {origin.x, origin.y, plot_w, plot_h};

   MonstroCanvas *canvas = monstro_get_window_canvas(ctx);

   uint32_t bg_color = 0xFF1E1E2E;
   uint32_t grid_color = 0x30FFFFFF;
   uint32_t axis_color = 0x80FFFFFF;

   monstro_draw_rect_filled(canvas, plot_rect, bg_color, 0, MONSTRO_CORNER_NONE);
   monstro_draw_rect(canvas, plot_rect, axis_color, 0, MONSTRO_CORNER_NONE, 1.0f);

   if (pw->show_grid)
   {
      int grid_lines = 5;
      for (int i = 1; i < grid_lines; i++)
      {
         float t = (float)i / (float)grid_lines;
         float gx = origin.x + t * plot_w;
         float gy = origin.y + t * plot_h;
         monstro_draw_line(canvas, {gx, origin.y}, {gx, origin.y + plot_h}, grid_color, 1.0f);
         monstro_draw_line(canvas, {origin.x, gy}, {origin.x + plot_w, gy}, grid_color, 1.0f);
      }
   }

   monstro_push_clip_rect(canvas, plot_rect);

   for (int s = 0; s < pw->series_count; s++)
   {
      PlotSeries *sr = &pw->series[s];
      if (sr->count < 1)
         continue;

      switch (sr->type)
      {
      case PLOT_LINE:
      {
         for (int i = 1; i < sr->count; i++)
         {
            MonstroVec2 p1 = {map_x(sr->x[i - 1], pw->x_min, pw->x_max, origin.x, origin.x + plot_w),
                              map_y(sr->y[i - 1], pw->y_min, pw->y_max, origin.y, origin.y + plot_h)};
            MonstroVec2 p2 = {map_x(sr->x[i], pw->x_min, pw->x_max, origin.x, origin.x + plot_w),
                              map_y(sr->y[i], pw->y_min, pw->y_max, origin.y, origin.y + plot_h)};
            monstro_draw_line(canvas, p1, p2, sr->color, 2.0f);
         }
         break;
      }
      case PLOT_SCATTER:
      {
         for (int i = 0; i < sr->count; i++)
         {
            MonstroVec2 c = {map_x(sr->x[i], pw->x_min, pw->x_max, origin.x, origin.x + plot_w),
                             map_y(sr->y[i], pw->y_min, pw->y_max, origin.y, origin.y + plot_h)};
            monstro_draw_circle_filled(canvas, c, 3.0f, sr->color, 8);
         }
         break;
      }
      case PLOT_BAR:
      {
         float range = pw->x_max - pw->x_min;
         float bw_data = (range / (float)sr->count) * sr->bar_width;
         float zero_y = map_y(0, pw->y_min, pw->y_max, origin.y, origin.y + plot_h);
         for (int i = 0; i < sr->count; i++)
         {
            float cx = map_x(sr->x[i], pw->x_min, pw->x_max, origin.x, origin.x + plot_w);
            float bw_px = bw_data / range * plot_w;
            float top_y = map_y(sr->y[i], pw->y_min, pw->y_max, origin.y, origin.y + plot_h);
            float rx = cx - bw_px * 0.5f;
            float ry = std::min(top_y, zero_y);
            float rh = std::fabs(top_y - zero_y);
            MonstroRect br = {rx, ry, bw_px, rh};
            monstro_draw_rect_filled(canvas, br, sr->color, 0, MONSTRO_CORNER_NONE);
         }
         break;
      }
      }
   }

   if (monstro_invisible_button(ctx, "##plot_area", plot_w, plot_h))
   {
   }
   if (monstro_is_item_hovered(ctx) && pw->show_crosshair)
   {
      MonstroVec2 mp = monstro_get_mouse_pos(ctx);
      float mx = mp.x, my = mp.y;
      if (mx >= origin.x && mx <= origin.x + plot_w && my >= origin.y && my <= origin.y + plot_h)
      {
         monstro_draw_line(canvas, {mx, origin.y}, {mx, origin.y + plot_h}, 0x60FFFFFF, 1.0f);
         monstro_draw_line(canvas, {origin.x, my}, {origin.x + plot_w, my}, 0x60FFFFFF, 1.0f);

         float data_x = pw->x_min + (mx - origin.x) / plot_w * (pw->x_max - pw->x_min);
         float data_y = pw->y_max - (my - origin.y) / plot_h * (pw->y_max - pw->y_min);
         char tip[64];
         snprintf(tip, sizeof(tip), "x=%.2f y=%.2f", data_x, data_y);
         monstro_tooltip(ctx, tip);
      }
   }

   monstro_pop_clip_rect(canvas);

   if (pw->series_count > 0)
   {
      monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
      for (int s = 0; s < pw->series_count; s++)
      {
         MonstroColor c;
         c.r = (float)((pw->series[s].color >> 0) & 0xFF) / 255.0f;
         c.g = (float)((pw->series[s].color >> 8) & 0xFF) / 255.0f;
         c.b = (float)((pw->series[s].color >> 16) & 0xFF) / 255.0f;
         c.a = 1.0f;
         monstro_label_colored(ctx, pw->series[s].label, c);
      }
      monstro_end_layout(ctx);
   }
}
