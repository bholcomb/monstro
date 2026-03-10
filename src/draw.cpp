#include "monstro/draw.h"
#include "canvas.h"

static monstro::Canvas *c(MonstroCanvas *canvas) { return reinterpret_cast<monstro::Canvas *>(canvas); }

extern "C"
{

/* Clip Rect */

void monstro_push_clip_rect(MonstroCanvas *canvas, MonstroRect rect)
{
   if (!canvas)
      return;
   c(canvas)->pushClipRect(rect);
}

void monstro_pop_clip_rect(MonstroCanvas *canvas)
{
   if (!canvas)
      return;
   c(canvas)->popClipRect();
}

/* Shape Primitives */

void monstro_draw_line(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, uint32_t color, float thickness)
{
   if (!canvas)
      return;
   c(canvas)->addLine(p1, p2, color, thickness);
}

void monstro_draw_rect(MonstroCanvas *canvas, MonstroRect rect, uint32_t color, float rounding,
                       MonstroCornerFlags corners, float thickness)
{
   if (!canvas)
      return;
   c(canvas)->addRect(rect, color, rounding, corners, thickness);
}

void monstro_draw_rect_filled(MonstroCanvas *canvas, MonstroRect rect, uint32_t color, float rounding,
                              MonstroCornerFlags corners)
{
   if (!canvas)
      return;
   c(canvas)->addRectFilled(rect, color, rounding, corners);
}

void monstro_draw_rect_filled_multi_color(MonstroCanvas *canvas, MonstroRect rect, uint32_t col_ul, uint32_t col_ur,
                                          uint32_t col_lr, uint32_t col_ll)
{
   if (!canvas)
      return;
   c(canvas)->addRectFilledMultiColor(rect, col_ul, col_ur, col_lr, col_ll);
}

void monstro_draw_triangle(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color,
                           float thickness)
{
   if (!canvas)
      return;
   c(canvas)->addTriangle(p1, p2, p3, color, thickness);
}

void monstro_draw_triangle_filled(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color)
{
   if (!canvas)
      return;
   c(canvas)->addTriangleFilled(p1, p2, p3, color);
}

void monstro_draw_circle(MonstroCanvas *canvas, MonstroVec2 center, float radius, uint32_t color, int num_segments,
                         float thickness)
{
   if (!canvas)
      return;
   c(canvas)->addCircle(center, radius, color, num_segments, thickness);
}

void monstro_draw_circle_filled(MonstroCanvas *canvas, MonstroVec2 center, float radius, uint32_t color,
                                int num_segments)
{
   if (!canvas)
      return;
   c(canvas)->addCircleFilled(center, radius, color, num_segments);
}

void monstro_draw_quad(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4,
                       uint32_t color, float thickness)
{
   if (!canvas)
      return;
   c(canvas)->addQuad(p1, p2, p3, p4, color, thickness);
}

void monstro_draw_quad_filled(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4,
                              uint32_t color)
{
   if (!canvas)
      return;
   c(canvas)->addQuadFilled(p1, p2, p3, p4, color);
}

/* Complex Shapes */

void monstro_draw_polyline(MonstroCanvas *canvas, const MonstroVec2 *points, int count, uint32_t color, bool closed,
                           float thickness)
{
   if (!canvas || !points || count < 2)
      return;
   c(canvas)->addPolyline(points, count, color, closed, thickness);
}

void monstro_draw_convex_poly_filled(MonstroCanvas *canvas, const MonstroVec2 *points, int count, uint32_t color)
{
   if (!canvas || !points || count < 3)
      return;
   c(canvas)->addConvexPolyFilled(points, count, color);
}

void monstro_draw_bezier_cubic(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4,
                               uint32_t color, float thickness, int num_segments)
{
   if (!canvas)
      return;
   c(canvas)->addBezierCubic(p1, p2, p3, p4, color, thickness, num_segments);
}

void monstro_draw_bezier_quadratic(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3,
                                   uint32_t color, float thickness, int num_segments)
{
   if (!canvas)
      return;
   c(canvas)->addBezierQuadratic(p1, p2, p3, color, thickness, num_segments);
}

/* Text */

void monstro_draw_text(MonstroCanvas *canvas, MonstroFont *font, MonstroVec2 pos, uint32_t color, const char *text)
{
   if (!canvas || !font || !text)
      return;
   c(canvas)->addText(font, pos, color, text);
}

void monstro_draw_text_aligned(MonstroCanvas *canvas, MonstroFont *font, MonstroRect rect, uint32_t color,
                               const char *text, MonstroAlign align)
{
   if (!canvas || !font || !text)
      return;
   c(canvas)->addTextEx(font, rect, color, text, align);
}

void monstro_draw_text_wrapped(MonstroCanvas *canvas, MonstroFont *font, MonstroVec2 pos, uint32_t color,
                               const char *text, float wrap_width)
{
   if (!canvas || !font || !text)
      return;
   c(canvas)->addTextWrapped(font, pos, color, text, wrap_width);
}

/* Images */

void monstro_draw_image(MonstroCanvas *canvas, uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min,
                        MonstroVec2 uv_max, uint32_t tint_color)
{
   if (!canvas)
      return;
   c(canvas)->addImage(texture_id, rect, uv_min, uv_max, tint_color);
}

void monstro_draw_image_rounded(MonstroCanvas *canvas, uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min,
                                MonstroVec2 uv_max, uint32_t tint_color, float rounding, MonstroCornerFlags corners)
{
   if (!canvas)
      return;
   c(canvas)->addImageRounded(texture_id, rect, uv_min, uv_max, tint_color, rounding, corners);
}

void monstro_draw_nine_patch(MonstroCanvas *canvas, const MonstroNinePatch *patch, MonstroRect rect,
                             uint32_t tint_color)
{
   if (!canvas || !patch)
      return;
   c(canvas)->addNinePatch(patch, rect, tint_color);
}

/* Custom Draw Callback */

void monstro_draw_custom(MonstroCanvas *canvas, MonstroCustomDrawCallback callback, void *user_data)
{
   if (!canvas || !callback)
      return;
   c(canvas)->addCustomDraw(callback, user_data);
}

/* Path API */

void monstro_path_clear(MonstroCanvas *canvas)
{
   if (!canvas)
      return;
   c(canvas)->pathClear();
}

void monstro_path_move_to(MonstroCanvas *canvas, MonstroVec2 pos)
{
   if (!canvas)
      return;
   c(canvas)->pathMoveTo(pos);
}

void monstro_path_line_to(MonstroCanvas *canvas, MonstroVec2 pos)
{
   if (!canvas)
      return;
   c(canvas)->pathLineTo(pos);
}

void monstro_path_arc_to(MonstroCanvas *canvas, MonstroVec2 center, float radius, float a_min, float a_max,
                         int num_segments)
{
   if (!canvas)
      return;
   c(canvas)->pathArcTo(center, radius, a_min, a_max, num_segments);
}

void monstro_path_bezier_cubic_to(MonstroCanvas *canvas, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4,
                                  int num_segments)
{
   if (!canvas)
      return;
   c(canvas)->pathBezierCubicTo(p2, p3, p4, num_segments);
}

void monstro_path_bezier_quadratic_to(MonstroCanvas *canvas, MonstroVec2 p2, MonstroVec2 p3, int num_segments)
{
   if (!canvas)
      return;
   c(canvas)->pathBezierQuadraticTo(p2, p3, num_segments);
}

void monstro_path_fill(MonstroCanvas *canvas, uint32_t color)
{
   if (!canvas)
      return;
   c(canvas)->pathFill(color);
}

void monstro_path_stroke(MonstroCanvas *canvas, uint32_t color, bool closed, float thickness)
{
   if (!canvas)
      return;
   c(canvas)->pathStroke(color, closed, thickness);
}

} // extern "C"
