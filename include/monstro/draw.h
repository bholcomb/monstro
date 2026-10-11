#ifndef MONSTRO_DRAW_H
#define MONSTRO_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Clip Rect                                                                 */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_push_clip_rect(MonstroCanvas* canvas, MonstroRect rect);
MONSTRO_API void monstro_pop_clip_rect(MonstroCanvas* canvas);

/* ------------------------------------------------------------------------- */
/* Shape Primitives                                                          */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_draw_line(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2,
                       uint32_t color, float thickness);

MONSTRO_API void monstro_draw_rect(MonstroCanvas* canvas, MonstroRect rect,
                       uint32_t color, float rounding, MonstroCornerFlags corners,
                       float thickness);

MONSTRO_API void monstro_draw_rect_filled(MonstroCanvas* canvas, MonstroRect rect,
                              uint32_t color, float rounding, MonstroCornerFlags corners);

MONSTRO_API void monstro_draw_rect_filled_multi_color(MonstroCanvas* canvas, MonstroRect rect,
                                          uint32_t col_ul, uint32_t col_ur,
                                          uint32_t col_lr, uint32_t col_ll);

MONSTRO_API void monstro_draw_triangle(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2,
                           MonstroVec2 p3, uint32_t color, float thickness);

MONSTRO_API void monstro_draw_triangle_filled(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2,
                                  MonstroVec2 p3, uint32_t color);

MONSTRO_API void monstro_draw_circle(MonstroCanvas* canvas, MonstroVec2 center, float radius,
                         uint32_t color, int num_segments, float thickness);

MONSTRO_API void monstro_draw_circle_filled(MonstroCanvas* canvas, MonstroVec2 center, float radius,
                                uint32_t color, int num_segments);

MONSTRO_API void monstro_draw_quad(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2,
                       MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness);

MONSTRO_API void monstro_draw_quad_filled(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2,
                              MonstroVec2 p3, MonstroVec2 p4, uint32_t color);

/*
 * An ellipse with radii.x along its own x axis and radii.y along its y axis,
 * turned by rotation radians (clockwise on screen, as +y is down), outlined
 * with num_segments points (at least 3).
 */
MONSTRO_API void monstro_draw_ellipse(MonstroCanvas* canvas, MonstroVec2 center, MonstroVec2 radii, float rotation,
                          uint32_t color, int num_segments, float thickness);

MONSTRO_API void monstro_draw_ellipse_filled(MonstroCanvas* canvas, MonstroVec2 center, MonstroVec2 radii, float rotation,
                                 uint32_t color, int num_segments);

/* ------------------------------------------------------------------------- */
/* Complex Shapes                                                            */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_draw_polyline(MonstroCanvas* canvas, const MonstroVec2* points, int count,
                           uint32_t color, bool closed, float thickness);

MONSTRO_API void monstro_draw_convex_poly_filled(MonstroCanvas* canvas, const MonstroVec2* points,
                                     int count, uint32_t color);

MONSTRO_API void monstro_draw_bezier_cubic(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2,
                               MonstroVec2 p3, MonstroVec2 p4,
                               uint32_t color, float thickness, int num_segments);

MONSTRO_API void monstro_draw_bezier_quadratic(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2,
                                   MonstroVec2 p3,
                                   uint32_t color, float thickness, int num_segments);

/* ------------------------------------------------------------------------- */
/* Text                                                                      */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_draw_text(MonstroCanvas* canvas, MonstroFont* font,
                       MonstroVec2 pos, uint32_t color, const char* text);

MONSTRO_API void monstro_draw_text_aligned(MonstroCanvas* canvas, MonstroFont* font,
                               MonstroRect rect, uint32_t color,
                               const char* text, MonstroAlign align);

MONSTRO_API void monstro_draw_text_wrapped(MonstroCanvas* canvas, MonstroFont* font,
                               MonstroVec2 pos, uint32_t color,
                               const char* text, float wrap_width);

/*
 * Text aligned in rect as monstro_draw_text_aligned places it, then turned
 * with rect by angle radians (clockwise on screen) about pivot: a pitch
 * ladder's numbers rolling with the horizon, a label along a dial's needle.
 */
MONSTRO_API void monstro_draw_text_rotated(MonstroCanvas* canvas, MonstroFont* font,
                               MonstroRect rect, uint32_t color,
                               const char* text, MonstroAlign align,
                               MonstroVec2 pivot, float angle);

/* ------------------------------------------------------------------------- */
/* Images                                                                    */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_draw_image(MonstroCanvas* canvas, uint64_t texture_id, MonstroRect rect,
                        MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color);

MONSTRO_API void monstro_draw_image_rounded(MonstroCanvas* canvas, uint64_t texture_id, MonstroRect rect,
                                MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color,
                                float rounding, MonstroCornerFlags corners);

MONSTRO_API void monstro_draw_nine_patch(MonstroCanvas* canvas, const MonstroNinePatch* patch,
                             MonstroRect rect, uint32_t tint_color);

/* ------------------------------------------------------------------------- */
/* Custom Draw Callback                                                      */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_draw_custom(MonstroCanvas* canvas, MonstroCustomDrawCallback callback,
                         void* user_data);

/* ------------------------------------------------------------------------- */
/* Path API                                                                  */
/* ------------------------------------------------------------------------- */

MONSTRO_API void monstro_path_clear(MonstroCanvas* canvas);
MONSTRO_API void monstro_path_move_to(MonstroCanvas* canvas, MonstroVec2 pos);
MONSTRO_API void monstro_path_line_to(MonstroCanvas* canvas, MonstroVec2 pos);
MONSTRO_API void monstro_path_arc_to(MonstroCanvas* canvas, MonstroVec2 center, float radius,
                         float a_min, float a_max, int num_segments);
MONSTRO_API void monstro_path_bezier_cubic_to(MonstroCanvas* canvas, MonstroVec2 p2, MonstroVec2 p3,
                                  MonstroVec2 p4, int num_segments);
MONSTRO_API void monstro_path_bezier_quadratic_to(MonstroCanvas* canvas, MonstroVec2 p2,
                                      MonstroVec2 p3, int num_segments);
MONSTRO_API void monstro_path_fill(MonstroCanvas* canvas, uint32_t color);
MONSTRO_API void monstro_path_stroke(MonstroCanvas* canvas, uint32_t color, bool closed, float thickness);

#ifdef __cplusplus
}
#endif

#endif
