#include "monstro/draw.h"
#include "canvas.h"

// The canvas a draw call works on: it must be lent to a context for a frame.
static monstro::Canvas *lent(MonstroCanvas *canvas, const char *function)
{
   MONSTRO_REQUIRE(canvas, "%s: the canvas must not be null", function);
   MONSTRO_REQUIRE(canvas->borrower,
                   "%s: the canvas is not lent to a frame; draw between monstro_begin_frame and monstro_end_frame",
                   function);
   return canvas;
}

extern "C"
{

/* Clip Rect */

void monstro_push_clip_rect(MonstroCanvas *canvas, MonstroRect rect) { lent(canvas, __func__)->pushClipRect(rect); }

void monstro_pop_clip_rect(MonstroCanvas *canvas) { lent(canvas, __func__)->popClipRect(); }

/* Shape Primitives */

void monstro_draw_line(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, uint32_t color, float thickness)
{
   lent(canvas, __func__)->addLine(p1, p2, color, thickness);
}

void monstro_draw_rect(MonstroCanvas *canvas, MonstroRect rect, uint32_t color, float rounding,
                       MonstroCornerFlags corners, float thickness)
{
   lent(canvas, __func__)->addRect(rect, color, rounding, corners, thickness);
}

void monstro_draw_rect_filled(MonstroCanvas *canvas, MonstroRect rect, uint32_t color, float rounding,
                              MonstroCornerFlags corners)
{
   lent(canvas, __func__)->addRectFilled(rect, color, rounding, corners);
}

void monstro_draw_rect_filled_multi_color(MonstroCanvas *canvas, MonstroRect rect, uint32_t col_ul, uint32_t col_ur,
                                          uint32_t col_lr, uint32_t col_ll)
{
   lent(canvas, __func__)->addRectFilledMultiColor(rect, col_ul, col_ur, col_lr, col_ll);
}

void monstro_draw_triangle(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color,
                           float thickness)
{
   lent(canvas, __func__)->addTriangle(p1, p2, p3, color, thickness);
}

void monstro_draw_triangle_filled(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color)
{
   lent(canvas, __func__)->addTriangleFilled(p1, p2, p3, color);
}

void monstro_draw_circle(MonstroCanvas *canvas, MonstroVec2 center, float radius, uint32_t color, int num_segments,
                         float thickness)
{
   lent(canvas, __func__)->addCircle(center, radius, color, num_segments, thickness);
}

void monstro_draw_circle_filled(MonstroCanvas *canvas, MonstroVec2 center, float radius, uint32_t color,
                                int num_segments)
{
   lent(canvas, __func__)->addCircleFilled(center, radius, color, num_segments);
}

void monstro_draw_quad(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4,
                       uint32_t color, float thickness)
{
   lent(canvas, __func__)->addQuad(p1, p2, p3, p4, color, thickness);
}

void monstro_draw_quad_filled(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4,
                              uint32_t color)
{
   lent(canvas, __func__)->addQuadFilled(p1, p2, p3, p4, color);
}

void monstro_draw_ellipse(MonstroCanvas *canvas, MonstroVec2 center, MonstroVec2 radii, float rotation, uint32_t color,
                          int num_segments, float thickness)
{
   MONSTRO_REQUIRE(num_segments >= 3, "%s: num_segments is %d; an ellipse needs at least 3", __func__, num_segments);
   MONSTRO_REQUIRE(radii.x >= 0.0f && radii.y >= 0.0f, "%s: radii (%g, %g) must not be negative", __func__,
                   (double)radii.x, (double)radii.y);
   lent(canvas, __func__)->addEllipse(center, radii, rotation, color, num_segments, thickness);
}

void monstro_draw_ellipse_filled(MonstroCanvas *canvas, MonstroVec2 center, MonstroVec2 radii, float rotation,
                                 uint32_t color, int num_segments)
{
   MONSTRO_REQUIRE(num_segments >= 3, "%s: num_segments is %d; an ellipse needs at least 3", __func__, num_segments);
   MONSTRO_REQUIRE(radii.x >= 0.0f && radii.y >= 0.0f, "%s: radii (%g, %g) must not be negative", __func__,
                   (double)radii.x, (double)radii.y);
   lent(canvas, __func__)->addEllipseFilled(center, radii, rotation, color, num_segments);
}

/* Complex Shapes */

void monstro_draw_polyline(MonstroCanvas *canvas, const MonstroVec2 *points, int count, uint32_t color, bool closed,
                           float thickness)
{
   MONSTRO_REQUIRE(points, "%s: points must not be null", __func__);
   lent(canvas, __func__)->addPolyline(points, count, color, closed, thickness);
}

void monstro_draw_convex_poly_filled(MonstroCanvas *canvas, const MonstroVec2 *points, int count, uint32_t color)
{
   MONSTRO_REQUIRE(points, "%s: points must not be null", __func__);
   lent(canvas, __func__)->addConvexPolyFilled(points, count, color);
}

void monstro_draw_bezier_cubic(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4,
                               uint32_t color, float thickness, int num_segments)
{
   lent(canvas, __func__)->addBezierCubic(p1, p2, p3, p4, color, thickness, num_segments);
}

void monstro_draw_bezier_quadratic(MonstroCanvas *canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3,
                                   uint32_t color, float thickness, int num_segments)
{
   lent(canvas, __func__)->addBezierQuadratic(p1, p2, p3, color, thickness, num_segments);
}

/* Text */

void monstro_draw_text(MonstroCanvas *canvas, MonstroFont *font, MonstroVec2 pos, uint32_t color, const char *text)
{
   MONSTRO_REQUIRE(font && text, "%s: the font and text must not be null", __func__);
   lent(canvas, __func__)->addText(font, pos, color, text);
}

void monstro_draw_text_aligned(MonstroCanvas *canvas, MonstroFont *font, MonstroRect rect, uint32_t color,
                               const char *text, MonstroAlign align)
{
   MONSTRO_REQUIRE(font && text, "%s: the font and text must not be null", __func__);
   lent(canvas, __func__)->addTextEx(font, rect, color, text, align);
}

void monstro_draw_text_wrapped(MonstroCanvas *canvas, MonstroFont *font, MonstroVec2 pos, uint32_t color,
                               const char *text, float wrap_width)
{
   MONSTRO_REQUIRE(font && text, "%s: the font and text must not be null", __func__);
   lent(canvas, __func__)->addTextWrapped(font, pos, color, text, wrap_width);
}

/* Images */

void monstro_draw_image(MonstroCanvas *canvas, uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min,
                        MonstroVec2 uv_max, uint32_t tint_color)
{
   lent(canvas, __func__)->addImage(texture_id, rect, uv_min, uv_max, tint_color);
}

void monstro_draw_image_rounded(MonstroCanvas *canvas, uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min,
                                MonstroVec2 uv_max, uint32_t tint_color, float rounding, MonstroCornerFlags corners)
{
   lent(canvas, __func__)->addImageRounded(texture_id, rect, uv_min, uv_max, tint_color, rounding, corners);
}

void monstro_draw_nine_patch(MonstroCanvas *canvas, const MonstroNinePatch *patch, MonstroRect rect,
                             uint32_t tint_color)
{
   MONSTRO_REQUIRE(patch, "%s: the patch must not be null", __func__);
   lent(canvas, __func__)->addNinePatch(patch, rect, tint_color);
}

/* Custom Draw Callback */

void monstro_draw_custom(MonstroCanvas *canvas, MonstroCustomDrawCallback callback, void *user_data)
{
   MONSTRO_REQUIRE(callback, "%s: the callback must not be null", __func__);
   lent(canvas, __func__)->addCustomDraw(callback, user_data);
}

/* Path API */

void monstro_path_clear(MonstroCanvas *canvas) { lent(canvas, __func__)->pathClear(); }

void monstro_path_move_to(MonstroCanvas *canvas, MonstroVec2 pos) { lent(canvas, __func__)->pathMoveTo(pos); }

void monstro_path_line_to(MonstroCanvas *canvas, MonstroVec2 pos) { lent(canvas, __func__)->pathLineTo(pos); }

void monstro_path_arc_to(MonstroCanvas *canvas, MonstroVec2 center, float radius, float a_min, float a_max,
                         int num_segments)
{
   lent(canvas, __func__)->pathArcTo(center, radius, a_min, a_max, num_segments);
}

void monstro_path_bezier_cubic_to(MonstroCanvas *canvas, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4,
                                  int num_segments)
{
   lent(canvas, __func__)->pathBezierCubicTo(p2, p3, p4, num_segments);
}

void monstro_path_bezier_quadratic_to(MonstroCanvas *canvas, MonstroVec2 p2, MonstroVec2 p3, int num_segments)
{
   lent(canvas, __func__)->pathBezierQuadraticTo(p2, p3, num_segments);
}

void monstro_path_fill(MonstroCanvas *canvas, uint32_t color) { lent(canvas, __func__)->pathFill(color); }

void monstro_path_stroke(MonstroCanvas *canvas, uint32_t color, bool closed, float thickness)
{
   lent(canvas, __func__)->pathStroke(color, closed, thickness);
}

} // extern "C"
