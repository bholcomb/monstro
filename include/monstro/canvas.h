#ifndef MONSTRO_CANVAS_H
#define MONSTRO_CANVAS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "style.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Types
 */

typedef struct MonstroCanvas MonstroCanvas;
typedef struct MonstroFont MonstroFont;

typedef struct MonstroVec2 {
    float x, y;
} MonstroVec2;

typedef struct MonstroRect {
    float x, y, w, h;
} MonstroRect;

typedef enum MonstroCornerFlags {
    MONSTRO_CORNER_NONE   = 0,
    MONSTRO_CORNER_LL     = 1 << 0,
    MONSTRO_CORNER_LR     = 1 << 1,
    MONSTRO_CORNER_UR     = 1 << 2,
    MONSTRO_CORNER_UL     = 1 << 3,
    MONSTRO_CORNER_TOP    = MONSTRO_CORNER_UL | MONSTRO_CORNER_UR,
    MONSTRO_CORNER_BOTTOM = MONSTRO_CORNER_LL | MONSTRO_CORNER_LR,
    MONSTRO_CORNER_LEFT   = MONSTRO_CORNER_LL | MONSTRO_CORNER_UL,
    MONSTRO_CORNER_RIGHT  = MONSTRO_CORNER_LR | MONSTRO_CORNER_UR,
    MONSTRO_CORNER_ALL    = MONSTRO_CORNER_LL | MONSTRO_CORNER_LR | MONSTRO_CORNER_UL | MONSTRO_CORNER_UR
} MonstroCornerFlags;

typedef void (*MonstroCustomDrawCallback)(void* user_data, const float clip_rect[4], int layer);

typedef struct MonstroVertex {
    float position[2];
    float texcoord[2];
    uint32_t color;
} MonstroVertex;

typedef struct MonstroDrawCommand {
    float clip_rect[4];
    int layer;
    uint32_t order;  // Original draw order for stable sorting within same layer
    uint64_t texture_id;
    uint32_t index_offset;
    uint32_t index_count;
    MonstroCustomDrawCallback custom_draw;
    void* user_data;
} MonstroDrawCommand;

typedef struct MonstroNinePatch {
    uint64_t texture_id;
    MonstroVec2 uv_min;
    MonstroVec2 uv_max;
    float border_left;
    float border_right;
    float border_top;
    float border_bottom;
    uint32_t texture_width;
    uint32_t texture_height;
} MonstroNinePatch;

/*
 * Canvas Lifecycle
 */

MonstroCanvas* monstro_canvas_create(void);
void monstro_canvas_destroy(MonstroCanvas* canvas);
void monstro_canvas_clear(MonstroCanvas* canvas);

/*
 * Primitive Drawing
 */

void monstro_canvas_add_line(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, uint32_t color, float thickness);
void monstro_canvas_add_rect(MonstroCanvas* canvas, MonstroRect rect, uint32_t color, float rounding, MonstroCornerFlags corners, float thickness);
void monstro_canvas_add_rect_filled(MonstroCanvas* canvas, MonstroRect rect, uint32_t color, float rounding, MonstroCornerFlags corners);
void monstro_canvas_add_rect_filled_multi_color(MonstroCanvas* canvas, MonstroRect rect, uint32_t col_ul, uint32_t col_ur, uint32_t col_lr, uint32_t col_ll);
void monstro_canvas_add_triangle(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness);
void monstro_canvas_add_triangle_filled(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color);
void monstro_canvas_add_circle(MonstroCanvas* canvas, MonstroVec2 center, float radius, uint32_t color, int num_segments, float thickness);
void monstro_canvas_add_circle_filled(MonstroCanvas* canvas, MonstroVec2 center, float radius, uint32_t color, int num_segments);
void monstro_canvas_add_quad(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness);
void monstro_canvas_add_quad_filled(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color);

/*
 * Complex Shape Drawing
 */

void monstro_canvas_add_polyline(MonstroCanvas* canvas, const MonstroVec2* points, int num_points, uint32_t color, bool closed, float thickness);
void monstro_canvas_add_convex_poly_filled(MonstroCanvas* canvas, const MonstroVec2* points, int num_points, uint32_t color);
void monstro_canvas_add_bezier_cubic(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness, int num_segments);
void monstro_canvas_add_bezier_quadratic(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness, int num_segments);

/*
 * Text Drawing
 */

void monstro_canvas_add_text(MonstroCanvas* canvas, MonstroFont* font, MonstroVec2 pos, uint32_t color, const char* text);
void monstro_canvas_add_text_ex(MonstroCanvas* canvas, MonstroFont* font, MonstroRect rect, uint32_t color, const char* text, MonstroAlign align);

/*
 * Image Drawing
 */

void monstro_canvas_add_image(MonstroCanvas* canvas, uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color);
void monstro_canvas_add_image_rounded(MonstroCanvas* canvas, uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color, float rounding, MonstroCornerFlags corners);
void monstro_canvas_add_nine_patch(MonstroCanvas* canvas, const MonstroNinePatch* patch, MonstroRect rect, uint32_t tint_color);
void monstro_canvas_add_icon(MonstroCanvas* canvas, uint32_t icon_id, MonstroRect rect, uint32_t tint_color);

/*
 * Custom Drawing
 */

void monstro_canvas_add_custom_draw(MonstroCanvas* canvas, MonstroCustomDrawCallback callback, void* user_data);

/*
 * Stateful Path API
 */

void monstro_canvas_path_clear(MonstroCanvas* canvas);
void monstro_canvas_path_move_to(MonstroCanvas* canvas, MonstroVec2 pos);
void monstro_canvas_path_line_to(MonstroCanvas* canvas, MonstroVec2 pos);
void monstro_canvas_path_arc_to(MonstroCanvas* canvas, MonstroVec2 center, float radius, float a_min, float a_max, int num_segments);
void monstro_canvas_path_bezier_cubic_to(MonstroCanvas* canvas, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, int num_segments);
void monstro_canvas_path_bezier_quadratic_to(MonstroCanvas* canvas, MonstroVec2 p2, MonstroVec2 p3, int num_segments);
void monstro_canvas_path_fill(MonstroCanvas* canvas, uint32_t color);
void monstro_canvas_path_stroke(MonstroCanvas* canvas, uint32_t color, bool closed, float thickness);

/*
 * Clip Rect Stack
 */

void monstro_canvas_push_clip_rect(MonstroCanvas* canvas, MonstroRect rect);
void monstro_canvas_push_clip_rect_fullscreen(MonstroCanvas* canvas);
void monstro_canvas_pop_clip_rect(MonstroCanvas* canvas);
MonstroRect monstro_canvas_get_clip_rect(MonstroCanvas* canvas);

/*
 * Texture Stack
 */

void monstro_canvas_push_texture(MonstroCanvas* canvas, uint64_t texture_id);
void monstro_canvas_pop_texture(MonstroCanvas* canvas);

/*
 * Layer Stack
 */

void monstro_canvas_push_layer(MonstroCanvas* canvas, int layer);
void monstro_canvas_pop_layer(MonstroCanvas* canvas);

/*
 * Draw Command and Buffer Access
 */

void monstro_canvas_get_draw_commands(MonstroCanvas* canvas, const MonstroDrawCommand** out_commands, uint32_t* out_count);
void monstro_canvas_get_vertex_buffer(MonstroCanvas* canvas, const MonstroVertex** out_data, size_t* out_byte_size);
void monstro_canvas_get_index_buffer(MonstroCanvas* canvas, const uint32_t** out_data, uint32_t* out_count);

void monstro_canvas_set_vertex_buffer(MonstroCanvas* canvas, MonstroVertex* buffer, size_t capacity_in_vertices);
void monstro_canvas_set_index_buffer(MonstroCanvas* canvas, uint32_t* buffer, size_t capacity_in_indices);

/*
 * Icon Registry
 */

uint64_t monstro_icon_register(uint64_t texture_id, MonstroVec2 uv_min, MonstroVec2 uv_max);
void monstro_icon_unregister(uint64_t icon_id);

#ifdef __cplusplus
}
#endif

#endif
