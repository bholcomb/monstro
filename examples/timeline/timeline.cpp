#include "timeline.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

void timeline_init(Timeline *tl, float total_duration)
{
   memset(tl, 0, sizeof(*tl));
   tl->total_duration = total_duration;
   tl->view_start = 0;
   tl->view_end = total_duration;
   tl->track_height = 30.0f;
   tl->header_width = 100.0f;
   tl->selected_clip = -1;
   tl->dragging_clip = -1;
}

int timeline_add_track(Timeline *tl, const char *name)
{
   if (tl->track_count >= TIMELINE_MAX_TRACKS)
      return -1;
   int i = tl->track_count++;
   strncpy(tl->tracks[i].name, name, TIMELINE_MAX_NAME - 1);
   return i;
}

int timeline_add_clip(Timeline *tl, int track, const char *name, float start, float duration, uint32_t color)
{
   if (tl->clip_count >= TIMELINE_MAX_CLIPS)
      return -1;
   int i = tl->clip_count++;
   strncpy(tl->clips[i].name, name, TIMELINE_MAX_NAME - 1);
   tl->clips[i].track = track;
   tl->clips[i].start = start;
   tl->clips[i].duration = duration;
   tl->clips[i].color = color;
   return i;
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }

void timeline_draw(Timeline *tl, MonstroContext *ctx, float dt)
{
   (void)dt;
   monstro_set_next_window_pos(ctx, 50, 50, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 800, 350, MONSTRO_COND_FIRST_USE_EVER);

   if (monstro_begin_window(ctx, "Timeline", nullptr, MONSTRO_WINDOW_DEFAULT))
   {

      monstro_slider_float(ctx, "Playhead", &tl->playhead, 0, tl->total_duration, "%.2f", nullptr);
      monstro_separator(ctx);

      monstro_add_item(ctx, 0, 2);
      MonstroVec2 origin = monstro_get_cursor_screen_pos(ctx);

      float tl_w = 650.0f;
      float tl_h = tl->track_height * (float)tl->track_count;
      float content_w = tl_w - tl->header_width;
      float time_range = tl->view_end - tl->view_start;
      if (time_range <= 0)
         time_range = 1.0f;

      MonstroCanvas *canvas = monstro_get_window_canvas(ctx);
      MonstroFont *font = monstro_get_style(ctx)->font;
      MonstroRect tl_rect = {origin.x, origin.y, tl_w, tl_h + 20.0f};

      monstro_draw_rect_filled(canvas, tl_rect, 0xFF1A1A2E, 0, MONSTRO_CORNER_NONE);

      float ruler_y = origin.y;
      float ruler_h = 18.0f;
      MonstroRect ruler_rect = {origin.x + tl->header_width, ruler_y, content_w, ruler_h};
      monstro_draw_rect_filled(canvas, ruler_rect, 0xFF252538, 0, MONSTRO_CORNER_NONE);

      float step = 1.0f;
      if (time_range > 20)
         step = 5.0f;
      if (time_range > 50)
         step = 10.0f;
      for (float t = ceilf(tl->view_start / step) * step; t <= tl->view_end; t += step)
      {
         float x = origin.x + tl->header_width + (t - tl->view_start) / time_range * content_w;
         monstro_draw_line(canvas, {x, ruler_y}, {x, ruler_y + ruler_h}, 0x60FFFFFF, 1.0f);
         char buf[16];
         snprintf(buf, sizeof(buf), "%.0f", t);
         monstro_draw_text(canvas, font, {x + 2, ruler_y + 2}, 0xFFCCCCCC, buf);
      }

      float tracks_y = origin.y + ruler_h + 2;
      for (int tr = 0; tr < tl->track_count; tr++)
      {
         float ty = tracks_y + (float)tr * tl->track_height;
         MonstroRect header = {origin.x, ty, tl->header_width, tl->track_height};
         monstro_draw_rect_filled(canvas, header, 0xFF222236, 0, MONSTRO_CORNER_NONE);
         monstro_draw_rect(canvas, header, 0x40FFFFFF, 0, MONSTRO_CORNER_NONE, 1.0f);
         monstro_draw_text(canvas, font, {origin.x + 4, ty + 8}, 0xFFCCCCCC, tl->tracks[tr].name);

         MonstroRect track_bg = {origin.x + tl->header_width, ty, content_w, tl->track_height};
         uint32_t bg = (tr % 2 == 0) ? 0xFF1E1E30 : 0xFF222238;
         monstro_draw_rect_filled(canvas, track_bg, bg, 0, MONSTRO_CORNER_NONE);
      }

      for (int c = 0; c < tl->clip_count; c++)
      {
         TimelineClip *clip = &tl->clips[c];
         if (clip->track < 0 || clip->track >= tl->track_count)
            continue;

         float x0 = origin.x + tl->header_width + (clip->start - tl->view_start) / time_range * content_w;
         float cw = clip->duration / time_range * content_w;
         float cy = tracks_y + (float)clip->track * tl->track_height + 2;
         float ch = tl->track_height - 4;

         MonstroRect cr = {x0, cy, cw, ch};
         uint32_t col = clip->color;
         if (c == tl->selected_clip)
            col = 0xFFFFFFFF;
         monstro_draw_rect_filled(canvas, cr, clip->color, 4.0f, MONSTRO_CORNER_ALL);
         monstro_draw_rect(canvas, cr, col, 4.0f, MONSTRO_CORNER_ALL, c == tl->selected_clip ? 2.0f : 1.0f);

         monstro_push_clip_rect(canvas, cr);
         monstro_draw_text(canvas, font, {x0 + 4, cy + 6}, 0xFFFFFFFF, clip->name);
         monstro_pop_clip_rect(canvas);
      }

      float ph_x = origin.x + tl->header_width + (tl->playhead - tl->view_start) / time_range * content_w;
      monstro_draw_line(canvas, {ph_x, origin.y}, {ph_x, tracks_y + tl_h}, 0xFFFF4444, 2.0f);

      MonstroVec2 tri[3] = {{ph_x - 5, origin.y}, {ph_x + 5, origin.y}, {ph_x, origin.y + 8}};
      monstro_draw_triangle_filled(canvas, tri[0], tri[1], tri[2], 0xFFFF4444);

      float total_h = ruler_h + 2 + tl_h;
      if (monstro_invisible_button(ctx, "##timeline_area", tl_w, total_h))
      {
      }

      MonstroVec2 mp = monstro_get_mouse_pos(ctx);
      float mx = mp.x, my = mp.y;

      if (monstro_is_item_hovered(ctx) && monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
      {
         tl->selected_clip = -1;
         tl->dragging_clip = -1;

         for (int c = 0; c < tl->clip_count; c++)
         {
            TimelineClip *clip = &tl->clips[c];
            float x0 = origin.x + tl->header_width + (clip->start - tl->view_start) / time_range * content_w;
            float cw = clip->duration / time_range * content_w;
            float cy = tracks_y + (float)clip->track * tl->track_height + 2;
            float ch = tl->track_height - 4;
            if (mx >= x0 && mx <= x0 + cw && my >= cy && my <= cy + ch)
            {
               tl->selected_clip = c;
               tl->dragging_clip = c;
               tl->drag_offset = mx - x0;
               break;
            }
         }
      }

      if (tl->dragging_clip >= 0 && monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         float new_x = mx - tl->drag_offset;
         float new_start = tl->view_start + (new_x - origin.x - tl->header_width) / content_w * time_range;
         tl->clips[tl->dragging_clip].start =
             clampf(new_start, 0, tl->total_duration - tl->clips[tl->dragging_clip].duration);
      }

      if (monstro_is_mouse_released(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         tl->dragging_clip = -1;
      }

      if (monstro_is_item_hovered(ctx))
      {
         float wy = monstro_get_mouse_wheel(ctx);
         if (wy != 0)
         {
            float zoom_factor = 1.0f + wy * -0.1f;
            float center = tl->view_start + time_range * 0.5f;
            float new_range = time_range * zoom_factor;
            if (new_range > 0.5f && new_range < tl->total_duration * 2.0f)
            {
               tl->view_start = center - new_range * 0.5f;
               tl->view_end = center + new_range * 0.5f;
            }
         }
      }
      monstro_end_window(ctx);
   }
}
