#ifndef EXAMPLE_CURVE_H
#define EXAMPLE_CURVE_H

#include <monstro.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CURVE_MAX_POINTS 64

typedef struct CurvePoint
{
   float x, y;
   float ctrl_left_x, ctrl_left_y;
   float ctrl_right_x, ctrl_right_y;
} CurvePoint;

typedef struct CurveEditor
{
   CurvePoint points[CURVE_MAX_POINTS];
   int point_count;

   int selected_point;
   int dragging_handle;

   float width, height;
   uint32_t curve_color;
   uint32_t point_color;
   uint32_t handle_color;
   uint32_t grid_color;
} CurveEditor;

void curve_editor_init(CurveEditor *ce, float width, float height);
void curve_editor_add_point(CurveEditor *ce, float x, float y);
void curve_editor_draw(CurveEditor *ce, MonstroContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
