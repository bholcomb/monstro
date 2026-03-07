#include "context.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <stdexcept>

static void assert_space(MonstroCanvas* canvas, uint32_t vertex_count, uint32_t index_count) {
    uint32_t required_verts = canvas->vertex_count + vertex_count;
    uint32_t required_idx = canvas->index_count + index_count;
    
    if (required_verts > canvas->vertex_capacity) {
        throw std::runtime_error("Canvas vertex buffer overflow");
    }
    if (required_idx > canvas->index_capacity) {
        throw std::runtime_error("Canvas index buffer overflow");
    }
}

MonstroCanvas* monstro_canvas_create(void) {
    MonstroCanvas* canvas = (MonstroCanvas*)calloc(1, sizeof(MonstroCanvas));
    if (!canvas) return nullptr;
    
    canvas->vertex_capacity = 4096;
    canvas->index_capacity = 4096 * 3;
    canvas->vertices = (MonstroVertex*)malloc(canvas->vertex_capacity * sizeof(MonstroVertex));
    canvas->indices = (uint32_t*)malloc(canvas->index_capacity * sizeof(uint32_t));
    
    if (!canvas->vertices || !canvas->indices) {
        free(canvas->vertices);
        free(canvas->indices);
        free(canvas);
        return nullptr;
    }
    
    canvas->clip_rect_stack[0] = {0, 0, 100000, 100000};
    canvas->clip_rect_stack_top = 0;
    canvas->texture_stack[0] = 0;
    canvas->texture_stack_top = 0;
    canvas->layer_stack[0] = 0;
    canvas->layer_stack_top = 0;
    
    return canvas;
}

void monstro_canvas_destroy(MonstroCanvas* canvas) {
    if (!canvas) return;
    
    if (!canvas->user_provided_vertex_buffer) {
        free(canvas->vertices);
    }
    if (!canvas->user_provided_index_buffer) {
        free(canvas->indices);
    }
    
    free(canvas);
}

void monstro_canvas_clear(MonstroCanvas* canvas) {
    if (!canvas) return;
    
    canvas->vertex_count = 0;
    canvas->index_count = 0;
    canvas->draw_command_count = 0;
    canvas->path_count = 0;
    canvas->dirty = false;
    
    canvas->clip_rect_stack[0] = {0, 0, 100000, 100000};
    canvas->clip_rect_stack_top = 0;
    canvas->texture_stack[0] = canvas->default_texture_id;
    canvas->texture_stack_top = 0;
    canvas->layer_stack[0] = 0;
    canvas->layer_stack_top = 0;
}

/* ------------------------------------------------------------------------- */
/* Clip Rect Stack                                                           */
/* ------------------------------------------------------------------------- */

void monstro_canvas_push_clip_rect(MonstroCanvas* canvas, MonstroRect rect) {
    if (!canvas || canvas->clip_rect_stack_top >= MONSTRO_MAX_CLIP_RECT_STACK - 1) return;
    
    canvas->dirty = true;
    canvas->clip_rect_stack_top++;
    canvas->clip_rect_stack[canvas->clip_rect_stack_top] = rect;
}

void monstro_canvas_push_clip_rect_fullscreen(MonstroCanvas* canvas) {
    if (!canvas) return;
    MonstroRect rect = {0, 0, 100000, 100000};
    monstro_canvas_push_clip_rect(canvas, rect);
}

void monstro_canvas_pop_clip_rect(MonstroCanvas* canvas) {
    if (!canvas || canvas->clip_rect_stack_top <= 0) return;
    canvas->clip_rect_stack_top--;
}

MonstroRect monstro_canvas_get_clip_rect(MonstroCanvas* canvas) {
    if (!canvas) return {0, 0, 0, 0};
    return canvas->clip_rect_stack[canvas->clip_rect_stack_top];
}

/* ------------------------------------------------------------------------- */
/* Layer Stack                                                               */
/* ------------------------------------------------------------------------- */

void monstro_canvas_push_layer(MonstroCanvas* canvas, int layer) {
    if (!canvas || canvas->layer_stack_top >= MONSTRO_MAX_LAYER_STACK - 1) return;
    canvas->dirty = true;
    canvas->layer_stack_top++;
    canvas->layer_stack[canvas->layer_stack_top] = layer;
}

void monstro_canvas_pop_layer(MonstroCanvas* canvas) {
    if (!canvas || canvas->layer_stack_top <= 0) return;
    canvas->layer_stack_top--;
}

/* ------------------------------------------------------------------------- */
/* Texture Stack                                                             */
/* ------------------------------------------------------------------------- */

void monstro_canvas_push_texture(MonstroCanvas* canvas, uint64_t texture_id) {
    if (!canvas || canvas->texture_stack_top >= MONSTRO_MAX_TEXTURE_STACK - 1) return;
    canvas->dirty = true;
    canvas->texture_stack_top++;
    canvas->texture_stack[canvas->texture_stack_top] = texture_id;
}

void monstro_canvas_pop_texture(MonstroCanvas* canvas) {
    if (!canvas || canvas->texture_stack_top <= 0) return;
    canvas->texture_stack_top--;
}

/* ------------------------------------------------------------------------- */
/* Draw Command Creation                                                     */
/* ------------------------------------------------------------------------- */

static MonstroDrawCommand* canvas_get_draw_command(MonstroCanvas* canvas, uint64_t texture_id) {
    if (canvas->draw_command_count > 0) {
        MonstroDrawCommand* prev = &canvas->draw_commands[canvas->draw_command_count - 1];
        
        MonstroRect clip = canvas->clip_rect_stack[canvas->clip_rect_stack_top];
        int layer = canvas->layer_stack[canvas->layer_stack_top];
        
        if (prev->texture_id == texture_id &&
            prev->layer == layer &&
            prev->clip_rect[0] == clip.x &&
            prev->clip_rect[1] == clip.y &&
            prev->clip_rect[2] == clip.w &&
            prev->clip_rect[3] == clip.h &&
            prev->custom_draw == nullptr) {
            return prev;
        }
    }
    
    if (canvas->draw_command_count >= MONSTRO_MAX_DRAW_COMMANDS) return nullptr;
    
    canvas->dirty = true;
    MonstroDrawCommand* cmd = &canvas->draw_commands[canvas->draw_command_count];
    memset(cmd, 0, sizeof(MonstroDrawCommand));
    
    MonstroRect clip = canvas->clip_rect_stack[canvas->clip_rect_stack_top];
    cmd->clip_rect[0] = clip.x;
    cmd->clip_rect[1] = clip.y;
    cmd->clip_rect[2] = clip.w;
    cmd->clip_rect[3] = clip.h;
    cmd->layer = canvas->layer_stack[canvas->layer_stack_top];
    cmd->order = canvas->draw_command_count;  // Preserve original draw order
    cmd->texture_id = texture_id;
    cmd->index_offset = canvas->index_count;
    cmd->index_count = 0;
    cmd->custom_draw = nullptr;
    cmd->user_data = nullptr;
    
    canvas->draw_command_count++;
    
    return cmd;
}

/* ------------------------------------------------------------------------- */
/* Precomputed Circle Vertices (12-step)                                     */
/* ------------------------------------------------------------------------- */

static MonstroVec2 g_circle_vtx[12];
static bool g_circle_built = false;

static void ensure_circle_vtx() {
    if (g_circle_built) return;
    for (int i = 0; i < 12; i++) {
        float a = ((float)i / 12.0f) * 2.0f * 3.14159265f;
        g_circle_vtx[i] = {cosf(a), sinf(a)};
    }
    g_circle_built = true;
}

static void path_arc_to_fast(MonstroCanvas* canvas, MonstroVec2 center, float radius, int a_min_of_12, int a_max_of_12) {
    ensure_circle_vtx();
    
    if (a_min_of_12 > a_max_of_12) return;
    
    if (radius == 0.0f) {
        if (canvas->path_count < MONSTRO_MAX_PATH_POINTS) {
            canvas->path[canvas->path_count++] = center;
        }
    } else {
        for (int a = a_min_of_12; a <= a_max_of_12; a++) {
            MonstroVec2 c = g_circle_vtx[a % 12];
            if (canvas->path_count < MONSTRO_MAX_PATH_POINTS) {
                canvas->path[canvas->path_count++] = {center.x + c.x * radius, center.y + c.y * radius};
            }
        }
    }
}

static void path_rect(MonstroCanvas* canvas, MonstroVec2 a, MonstroVec2 b, float rounding, MonstroCornerFlags rounding_corners) {
    float r = rounding;
    
    r = fminf(r, fabsf(b.x - a.x) * (((rounding_corners & MONSTRO_CORNER_BOTTOM) == MONSTRO_CORNER_BOTTOM) || 
                                      ((rounding_corners & MONSTRO_CORNER_TOP) == MONSTRO_CORNER_TOP) ? 0.5f : 1.0f) - 1.0f);
    r = fminf(r, fabsf(b.y - a.y) * (((rounding_corners & MONSTRO_CORNER_LEFT) == MONSTRO_CORNER_LEFT) || 
                                      ((rounding_corners & MONSTRO_CORNER_RIGHT) == MONSTRO_CORNER_RIGHT) ? 0.5f : 1.0f) - 1.0f);
    
    if (r <= 0.0f || rounding_corners == 0) {
        if (canvas->path_count < MONSTRO_MAX_PATH_POINTS) canvas->path[canvas->path_count++] = a;
        if (canvas->path_count < MONSTRO_MAX_PATH_POINTS) canvas->path[canvas->path_count++] = {b.x, a.y};
        if (canvas->path_count < MONSTRO_MAX_PATH_POINTS) canvas->path[canvas->path_count++] = b;
        if (canvas->path_count < MONSTRO_MAX_PATH_POINTS) canvas->path[canvas->path_count++] = {a.x, b.y};
    } else {
        float r0 = (rounding_corners & MONSTRO_CORNER_UL) ? r : 0.0f;
        float r1 = (rounding_corners & MONSTRO_CORNER_UR) ? r : 0.0f;
        float r2 = (rounding_corners & MONSTRO_CORNER_LR) ? r : 0.0f;
        float r3 = (rounding_corners & MONSTRO_CORNER_LL) ? r : 0.0f;
        path_arc_to_fast(canvas, {a.x + r0, a.y + r0}, r0, 6, 9);
        path_arc_to_fast(canvas, {b.x - r1, a.y + r1}, r1, 9, 12);
        path_arc_to_fast(canvas, {b.x - r2, b.y - r2}, r2, 0, 3);
        path_arc_to_fast(canvas, {a.x + r3, b.y - r3}, r3, 3, 6);
    }
}

/* ------------------------------------------------------------------------- */
/* Primitive Drawing                                                         */
/* ------------------------------------------------------------------------- */

void monstro_canvas_add_line(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, uint32_t color, float thickness) {
    if (!canvas) return;
    
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;
    
    dx /= len;
    dy /= len;
    
    float nx = -dy * thickness * 0.5f;
    float ny = dx * thickness * 0.5f;
    
    assert_space(canvas, 4, 6);
    
    uint64_t tex = canvas->texture_stack[canvas->texture_stack_top];
    MonstroDrawCommand* cmd = canvas_get_draw_command(canvas, tex);
    if (!cmd) return;
    
    uint32_t base = canvas->vertex_count;
    MonstroVertex* v = canvas->vertices + canvas->vertex_count;
    
    v[0] = {{p1.x + nx, p1.y + ny}, {0.0f, 0.0f}, color};
    v[1] = {{p1.x - nx, p1.y - ny}, {0.0f, 0.0f}, color};
    v[2] = {{p2.x - nx, p2.y - ny}, {0.0f, 0.0f}, color};
    v[3] = {{p2.x + nx, p2.y + ny}, {0.0f, 0.0f}, color};
    canvas->vertex_count += 4;
    
    uint32_t* idx = canvas->indices + canvas->index_count;
    idx[0] = base; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base; idx[4] = base + 2; idx[5] = base + 3;
    canvas->index_count += 6;
    cmd->index_count += 6;
}

void monstro_canvas_add_rect(MonstroCanvas* canvas, MonstroRect rect, uint32_t color, float rounding, MonstroCornerFlags corners, float thickness) {
    if (!canvas) return;
    
    MonstroVec2 a = {rect.x + 0.5f, rect.y + 0.5f};
    MonstroVec2 b = {rect.x + rect.w + 0.5f, rect.y + rect.h + 0.5f};
    
    canvas->path_count = 0;
    path_rect(canvas, a, b, rounding, corners);
    monstro_canvas_path_stroke(canvas, color, true, thickness);
}

void monstro_canvas_add_rect_filled(MonstroCanvas* canvas, MonstroRect rect, uint32_t color, float rounding, MonstroCornerFlags corners) {
    if (!canvas) return;
    
    MonstroVec2 a = {rect.x, rect.y};
    MonstroVec2 b = {rect.x + rect.w, rect.y + rect.h};
    
    if (rounding > 0.0f && corners != MONSTRO_CORNER_NONE) {
        canvas->path_count = 0;
        path_rect(canvas, a, b, rounding, corners);
        monstro_canvas_path_fill(canvas, color);
    } else {
        assert_space(canvas, 4, 6);
        
        uint64_t tex = canvas->texture_stack[canvas->texture_stack_top];
        MonstroDrawCommand* cmd = canvas_get_draw_command(canvas, tex);
        if (!cmd) return;
        
        uint32_t base = canvas->vertex_count;
        MonstroVertex* v = canvas->vertices + canvas->vertex_count;
        
        v[0] = {{rect.x, rect.y}, {0.0f, 0.0f}, color};
        v[1] = {{rect.x + rect.w, rect.y}, {1.0f, 0.0f}, color};
        v[2] = {{rect.x + rect.w, rect.y + rect.h}, {1.0f, 1.0f}, color};
        v[3] = {{rect.x, rect.y + rect.h}, {0.0f, 1.0f}, color};
        canvas->vertex_count += 4;
        
        uint32_t* idx = canvas->indices + canvas->index_count;
        idx[0] = base; idx[1] = base + 1; idx[2] = base + 2;
        idx[3] = base; idx[4] = base + 2; idx[5] = base + 3;
        canvas->index_count += 6;
        cmd->index_count += 6;
    }
}

void monstro_canvas_add_rect_filled_multi_color(MonstroCanvas* canvas, MonstroRect rect, uint32_t col_ul, uint32_t col_ur, uint32_t col_lr, uint32_t col_ll) {
    if (!canvas) return;
    
    assert_space(canvas, 4, 6);
    
    uint64_t tex = canvas->texture_stack[canvas->texture_stack_top];
    MonstroDrawCommand* cmd = canvas_get_draw_command(canvas, tex);
    if (!cmd) return;
    
    uint32_t base = canvas->vertex_count;
    MonstroVertex* v = canvas->vertices + canvas->vertex_count;
    
    v[0] = {{rect.x, rect.y}, {0.0f, 0.0f}, col_ul};
    v[1] = {{rect.x + rect.w, rect.y}, {1.0f, 0.0f}, col_ur};
    v[2] = {{rect.x + rect.w, rect.y + rect.h}, {1.0f, 1.0f}, col_lr};
    v[3] = {{rect.x, rect.y + rect.h}, {0.0f, 1.0f}, col_ll};
    canvas->vertex_count += 4;
    
    uint32_t* idx = canvas->indices + canvas->index_count;
    idx[0] = base; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base; idx[4] = base + 2; idx[5] = base + 3;
    canvas->index_count += 6;
    cmd->index_count += 6;
}

void monstro_canvas_add_triangle_filled(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color) {
    if (!canvas) return;
    
    assert_space(canvas, 3, 3);
    
    uint64_t tex = canvas->texture_stack[canvas->texture_stack_top];
    MonstroDrawCommand* cmd = canvas_get_draw_command(canvas, tex);
    if (!cmd) return;
    
    uint32_t base = canvas->vertex_count;
    MonstroVertex* v = canvas->vertices + canvas->vertex_count;
    
    v[0] = {{p1.x, p1.y}, {0.0f, 0.0f}, color};
    v[1] = {{p2.x, p2.y}, {0.0f, 0.0f}, color};
    v[2] = {{p3.x, p3.y}, {0.0f, 0.0f}, color};
    canvas->vertex_count += 3;
    
    uint32_t* idx = canvas->indices + canvas->index_count;
    idx[0] = base; idx[1] = base + 1; idx[2] = base + 2;
    canvas->index_count += 3;
    cmd->index_count += 3;
}

void monstro_canvas_add_triangle(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness) {
    monstro_canvas_add_line(canvas, p1, p2, color, thickness);
    monstro_canvas_add_line(canvas, p2, p3, color, thickness);
    monstro_canvas_add_line(canvas, p3, p1, color, thickness);
}

void monstro_canvas_add_circle_filled(MonstroCanvas* canvas, MonstroVec2 center, float radius, uint32_t color, int num_segments) {
    if (!canvas || num_segments < 3) return;
    
    assert_space(canvas, (uint32_t)(num_segments + 1), (uint32_t)(num_segments * 3));
    
    uint64_t tex = canvas->texture_stack[canvas->texture_stack_top];
    MonstroDrawCommand* cmd = canvas_get_draw_command(canvas, tex);
    if (!cmd) return;
    
    uint32_t base = canvas->vertex_count;
    MonstroVertex* v = canvas->vertices + canvas->vertex_count;
    
    v[0] = {{center.x, center.y}, {0.5f, 0.5f}, color};
    
    for (int i = 0; i < num_segments; i++) {
        float angle = (float)i / (float)num_segments * 6.2831853f;
        v[i + 1] = {{center.x + cosf(angle) * radius, center.y + sinf(angle) * radius}, {0.0f, 0.0f}, color};
    }
    canvas->vertex_count += num_segments + 1;
    
    uint32_t* idx = canvas->indices + canvas->index_count;
    for (int i = 0; i < num_segments; i++) {
        idx[i * 3] = base;
        idx[i * 3 + 1] = base + 1 + i;
        idx[i * 3 + 2] = base + 1 + (i + 1) % num_segments;
    }
    canvas->index_count += num_segments * 3;
    cmd->index_count += num_segments * 3;
}

void monstro_canvas_add_circle(MonstroCanvas* canvas, MonstroVec2 center, float radius, uint32_t color, int num_segments, float thickness) {
    if (!canvas || num_segments < 3) return;
    
    for (int i = 0; i < num_segments; i++) {
        float a1 = (float)i / (float)num_segments * 6.2831853f;
        float a2 = (float)(i + 1) / (float)num_segments * 6.2831853f;
        MonstroVec2 p1 = {center.x + cosf(a1) * radius, center.y + sinf(a1) * radius};
        MonstroVec2 p2 = {center.x + cosf(a2) * radius, center.y + sinf(a2) * radius};
        monstro_canvas_add_line(canvas, p1, p2, color, thickness);
    }
}

void monstro_canvas_add_quad(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness) {
    monstro_canvas_add_line(canvas, p1, p2, color, thickness);
    monstro_canvas_add_line(canvas, p2, p3, color, thickness);
    monstro_canvas_add_line(canvas, p3, p4, color, thickness);
    monstro_canvas_add_line(canvas, p4, p1, color, thickness);
}

void monstro_canvas_add_quad_filled(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color) {
    if (!canvas) return;
    
    assert_space(canvas, 4, 6);
    
    uint64_t tex = canvas->texture_stack[canvas->texture_stack_top];
    MonstroDrawCommand* cmd = canvas_get_draw_command(canvas, tex);
    if (!cmd) return;
    
    uint32_t base = canvas->vertex_count;
    MonstroVertex* v = canvas->vertices + canvas->vertex_count;
    
    v[0] = {{p1.x, p1.y}, {0.0f, 0.0f}, color};
    v[1] = {{p2.x, p2.y}, {1.0f, 0.0f}, color};
    v[2] = {{p3.x, p3.y}, {1.0f, 1.0f}, color};
    v[3] = {{p4.x, p4.y}, {0.0f, 1.0f}, color};
    canvas->vertex_count += 4;
    
    uint32_t* idx = canvas->indices + canvas->index_count;
    idx[0] = base; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base; idx[4] = base + 2; idx[5] = base + 3;
    canvas->index_count += 6;
    cmd->index_count += 6;
}

/* ------------------------------------------------------------------------- */
/* Complex Shapes                                                            */
/* ------------------------------------------------------------------------- */

void monstro_canvas_add_polyline(MonstroCanvas* canvas, const MonstroVec2* points, int num_points, uint32_t color, bool closed, float thickness) {
    if (!canvas || num_points < 2) return;
    
    int segments = closed ? num_points : num_points - 1;
    for (int i = 0; i < segments; i++) {
        int j = (i + 1) % num_points;
        monstro_canvas_add_line(canvas, points[i], points[j], color, thickness);
    }
}

void monstro_canvas_add_convex_poly_filled(MonstroCanvas* canvas, const MonstroVec2* points, int num_points, uint32_t color) {
    if (!canvas || num_points < 3) return;
    
    assert_space(canvas, num_points, (num_points - 2) * 3);
    
    uint64_t tex = canvas->texture_stack[canvas->texture_stack_top];
    MonstroDrawCommand* cmd = canvas_get_draw_command(canvas, tex);
    if (!cmd) return;
    
    uint32_t base = canvas->vertex_count;
    MonstroVertex* v = canvas->vertices + canvas->vertex_count;
    
    for (int i = 0; i < num_points; i++) {
        v[i] = {{points[i].x, points[i].y}, {0.0f, 0.0f}, color};
    }
    canvas->vertex_count += num_points;
    
    uint32_t* idx = canvas->indices + canvas->index_count;
    int icount = 0;
    for (int i = 1; i < num_points - 1; i++) {
        idx[icount++] = base;
        idx[icount++] = base + i;
        idx[icount++] = base + i + 1;
    }
    canvas->index_count += icount;
    cmd->index_count += icount;
}

void monstro_canvas_add_bezier_cubic(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness, int num_segments) {
    if (!canvas || num_segments < 1) return;
    
    MonstroVec2 prev = p1;
    for (int i = 1; i <= num_segments; i++) {
        float t = (float)i / (float)num_segments;
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;
        
        MonstroVec2 pt;
        pt.x = uuu * p1.x + 3 * uu * t * p2.x + 3 * u * tt * p3.x + ttt * p4.x;
        pt.y = uuu * p1.y + 3 * uu * t * p2.y + 3 * u * tt * p3.y + ttt * p4.y;
        
        monstro_canvas_add_line(canvas, prev, pt, color, thickness);
        prev = pt;
    }
}

void monstro_canvas_add_bezier_quadratic(MonstroCanvas* canvas, MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness, int num_segments) {
    if (!canvas || num_segments < 1) return;
    
    MonstroVec2 prev = p1;
    for (int i = 1; i <= num_segments; i++) {
        float t = (float)i / (float)num_segments;
        float u = 1.0f - t;
        
        MonstroVec2 pt;
        pt.x = u * u * p1.x + 2 * u * t * p2.x + t * t * p3.x;
        pt.y = u * u * p1.y + 2 * u * t * p2.y + t * t * p3.y;
        
        monstro_canvas_add_line(canvas, prev, pt, color, thickness);
        prev = pt;
    }
}

/* ------------------------------------------------------------------------- */
/* Text Drawing (stub)                                                       */
/* ------------------------------------------------------------------------- */

static uint32_t decode_utf8_canvas(const char** str) {
    const uint8_t* s = (const uint8_t*)*str;
    uint32_t c = *s++;
    
    if (c < 0x80) {
        *str = (const char*)s;
        return c;
    }
    
    if ((c & 0xE0) == 0xC0) {
        c = ((c & 0x1F) << 6) | (*s++ & 0x3F);
    } else if ((c & 0xF0) == 0xE0) {
        c = ((c & 0x0F) << 12) | ((*s++ & 0x3F) << 6);
        c |= (*s++ & 0x3F);
    } else if ((c & 0xF8) == 0xF0) {
        c = ((c & 0x07) << 18) | ((*s++ & 0x3F) << 12);
        c |= ((*s++ & 0x3F) << 6);
        c |= (*s++ & 0x3F);
    }
    
    *str = (const char*)s;
    return c;
}

void monstro_canvas_add_text(MonstroCanvas* canvas, MonstroFont* font, MonstroVec2 pos, uint32_t color, const char* text) {
    if (!canvas || !font || !text) return;
    
    uint64_t texture_id = monstro_font_get_atlas_texture(font);
    if (texture_id == 0) return;
    
    float pen_x = pos.x;
    float pen_y = pos.y;
    float line_y = 0;
    float line_height = monstro_font_get_line_height(font);
    
    const char* s = text;
    while (*s) {
        uint32_t codepoint = decode_utf8_canvas(&s);
        
        if (codepoint == '\n') {
            pen_x = pos.x;
            line_y += line_height;
            continue;
        }
        if (codepoint == '\r') continue;
        
        MonstroGlyph glyph;
        if (!monstro_font_get_glyph(font, codepoint, &glyph)) continue;
        
        float x = pen_x + glyph.offset[0];
        float y = pen_y + line_y + glyph.offset[1] + monstro_font_get_ascent(font);
        
        if (glyph.size[0] > 0 && glyph.size[1] > 0) {
            MonstroRect rect = {x, y, glyph.size[0], glyph.size[1]};
            MonstroVec2 uv_min = {glyph.min_tex_coord[0], glyph.min_tex_coord[1]};
            MonstroVec2 uv_max = {glyph.max_tex_coord[0], glyph.max_tex_coord[1]};
            monstro_canvas_add_image(canvas, texture_id, rect, uv_min, uv_max, color);
        }
        
        pen_x += glyph.advance[0];
    }
}

void monstro_canvas_add_text_ex(MonstroCanvas* canvas, MonstroFont* font, MonstroRect rect, uint32_t color, const char* text, MonstroAlign align) {
    if (!canvas || !font || !text) return;
    
    MonstroVec2 text_size = monstro_font_measure_text(font, text, 0.0f);
    
    float x = rect.x;
    float y = rect.y;
    
    if (align & MONSTRO_ALIGN_HCENTER) {
        x = rect.x + (rect.w - text_size.x) * 0.5f;
    } else if (align & MONSTRO_ALIGN_RIGHT) {
        x = rect.x + rect.w - text_size.x;
    }
    
    if (align & MONSTRO_ALIGN_VCENTER) {
        y = rect.y + (rect.h - text_size.y) * 0.5f;
    } else if (align & MONSTRO_ALIGN_BOTTOM) {
        y = rect.y + rect.h - text_size.y;
    }
    
    MonstroVec2 pos = {x, y};
    monstro_canvas_add_text(canvas, font, pos, color, text);
}

/* ------------------------------------------------------------------------- */
/* Image Drawing                                                             */
/* ------------------------------------------------------------------------- */

void monstro_canvas_add_image(MonstroCanvas* canvas, uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color) {
    if (!canvas) return;
    
    assert_space(canvas, 4, 6);
    
    MonstroDrawCommand* cmd = canvas_get_draw_command(canvas, texture_id);
    if (!cmd) return;
    
    uint32_t base = canvas->vertex_count;
    MonstroVertex* v = canvas->vertices + canvas->vertex_count;
    
    v[0] = {{rect.x, rect.y}, {uv_min.x, uv_min.y}, tint_color};
    v[1] = {{rect.x + rect.w, rect.y}, {uv_max.x, uv_min.y}, tint_color};
    v[2] = {{rect.x + rect.w, rect.y + rect.h}, {uv_max.x, uv_max.y}, tint_color};
    v[3] = {{rect.x, rect.y + rect.h}, {uv_min.x, uv_max.y}, tint_color};
    canvas->vertex_count += 4;
    
    uint32_t* idx = canvas->indices + canvas->index_count;
    idx[0] = base; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base; idx[4] = base + 2; idx[5] = base + 3;
    canvas->index_count += 6;
    cmd->index_count += 6;
}

void monstro_canvas_add_image_rounded(MonstroCanvas* canvas, uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color, float rounding, MonstroCornerFlags corners) {
    if (rounding <= 0.0f || corners == MONSTRO_CORNER_NONE) {
        monstro_canvas_add_image(canvas, texture_id, rect, uv_min, uv_max, tint_color);
        return;
    }
    
    if (!canvas) return;
    
    MonstroVec2 a = {rect.x, rect.y};
    MonstroVec2 b = {rect.x + rect.w, rect.y + rect.h};
    
    canvas->path_count = 0;
    path_rect(canvas, a, b, rounding, corners);
    
    if (canvas->path_count < 3) return;
    
    int num_points = canvas->path_count;
    assert_space(canvas, num_points, (num_points - 2) * 3);
    
    MonstroDrawCommand* cmd = canvas_get_draw_command(canvas, texture_id);
    if (!cmd) return;
    
    uint32_t base = canvas->vertex_count;
    MonstroVertex* v = canvas->vertices + canvas->vertex_count;
    
    float inv_w = (rect.w > 0.0f) ? 1.0f / rect.w : 0.0f;
    float inv_h = (rect.h > 0.0f) ? 1.0f / rect.h : 0.0f;
    
    for (int i = 0; i < num_points; i++) {
        float t_x = (canvas->path[i].x - rect.x) * inv_w;
        float t_y = (canvas->path[i].y - rect.y) * inv_h;
        float u = uv_min.x + (uv_max.x - uv_min.x) * t_x;
        float uv = uv_min.y + (uv_max.y - uv_min.y) * t_y;
        v[i] = {{canvas->path[i].x, canvas->path[i].y}, {u, uv}, tint_color};
    }
    canvas->vertex_count += num_points;
    
    uint32_t* idx = canvas->indices + canvas->index_count;
    int icount = 0;
    for (int i = 1; i < num_points - 1; i++) {
        idx[icount++] = base;
        idx[icount++] = base + i;
        idx[icount++] = base + i + 1;
    }
    canvas->index_count += icount;
    cmd->index_count += icount;
    
    canvas->path_count = 0;
}

void monstro_canvas_add_nine_patch(MonstroCanvas* canvas, const MonstroNinePatch* patch, MonstroRect rect, uint32_t tint_color) {
    if (!canvas || !patch) return;
    
    assert_space(canvas, 16, 54);
    
    MonstroDrawCommand* cmd = canvas_get_draw_command(canvas, patch->texture_id);
    if (!cmd) return;
    
    float rp_x = rect.x;
    float rp_y = rect.y;
    float re_x = rect.x + rect.w;
    float re_y = rect.y + rect.h;
    
    float xp[4] = {rp_x, rp_x + patch->border_left, re_x - patch->border_right, re_x};
    float yp[4] = {rp_y, rp_y + patch->border_top, re_y - patch->border_bottom, re_y};
    
    float tex_w = (float)patch->texture_width;
    float tex_h = (float)patch->texture_height;
    float scale_x = (tex_w > 0) ? 1.0f / tex_w : 1.0f;
    float scale_y = (tex_h > 0) ? 1.0f / tex_h : 1.0f;
    
    float uv_xp[4] = {
        patch->uv_min.x,
        patch->uv_min.x + patch->border_left * scale_x,
        patch->uv_max.x - patch->border_right * scale_x,
        patch->uv_max.x
    };
    float uv_yp[4] = {
        patch->uv_min.y,
        patch->uv_min.y + patch->border_top * scale_y,
        patch->uv_max.y - patch->border_bottom * scale_y,
        patch->uv_max.y
    };
    
    uint32_t start_vertex = canvas->vertex_count;
    MonstroVertex* v = canvas->vertices + canvas->vertex_count;
    
    int vert_count = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            v[vert_count] = {{xp[j], yp[i]}, {uv_xp[j], uv_yp[i]}, tint_color};
            vert_count++;
        }
    }
    canvas->vertex_count += 16;
    
    uint32_t* idx = canvas->indices + canvas->index_count;
    int idx_count = 0;
    
    #define NINE_PATCH_QUAD(v0, v1, v2, v3) \
        idx[idx_count++] = start_vertex + v0; \
        idx[idx_count++] = start_vertex + v1; \
        idx[idx_count++] = start_vertex + v2; \
        idx[idx_count++] = start_vertex + v0; \
        idx[idx_count++] = start_vertex + v2; \
        idx[idx_count++] = start_vertex + v3;
    
    NINE_PATCH_QUAD(0, 4, 5, 1);
    NINE_PATCH_QUAD(1, 5, 6, 2);
    NINE_PATCH_QUAD(2, 6, 7, 3);
    
    NINE_PATCH_QUAD(4, 8, 9, 5);
    NINE_PATCH_QUAD(5, 9, 10, 6);
    NINE_PATCH_QUAD(6, 10, 11, 7);
    
    NINE_PATCH_QUAD(8, 12, 13, 9);
    NINE_PATCH_QUAD(9, 13, 14, 10);
    NINE_PATCH_QUAD(10, 14, 15, 11);
    
    #undef NINE_PATCH_QUAD
    
    canvas->index_count += idx_count;
    cmd->index_count += idx_count;
}

void monstro_canvas_add_icon(MonstroCanvas* canvas, uint32_t icon_id, MonstroRect rect, uint32_t tint_color) {
    (void)canvas; (void)icon_id; (void)rect; (void)tint_color;
}

/* ------------------------------------------------------------------------- */
/* Custom Draw                                                               */
/* ------------------------------------------------------------------------- */

void monstro_canvas_add_custom_draw(MonstroCanvas* canvas, MonstroCustomDrawCallback callback, void* user_data) {
    if (!canvas || !callback || canvas->draw_command_count >= MONSTRO_MAX_DRAW_COMMANDS) return;
    
    canvas->dirty = true;
    MonstroDrawCommand* cmd = &canvas->draw_commands[canvas->draw_command_count];
    
    MonstroRect clip = canvas->clip_rect_stack[canvas->clip_rect_stack_top];
    cmd->clip_rect[0] = clip.x;
    cmd->clip_rect[1] = clip.y;
    cmd->clip_rect[2] = clip.w;
    cmd->clip_rect[3] = clip.h;
    cmd->layer = canvas->layer_stack[canvas->layer_stack_top];
    cmd->order = canvas->draw_command_count;  // Preserve original draw order
    cmd->texture_id = 0;
    cmd->index_offset = 0;
    cmd->index_count = 0;
    cmd->custom_draw = callback;
    cmd->user_data = user_data;
    
    canvas->draw_command_count++;
}

/* ------------------------------------------------------------------------- */
/* Path API                                                                  */
/* ------------------------------------------------------------------------- */

void monstro_canvas_path_clear(MonstroCanvas* canvas) {
    if (!canvas) return;
    canvas->path_count = 0;
}

void monstro_canvas_path_move_to(MonstroCanvas* canvas, MonstroVec2 pos) {
    if (!canvas) return;
    canvas->path_count = 0;
    if (canvas->path_count < MONSTRO_MAX_PATH_POINTS) {
        canvas->path[canvas->path_count++] = pos;
    }
}

void monstro_canvas_path_line_to(MonstroCanvas* canvas, MonstroVec2 pos) {
    if (!canvas || canvas->path_count >= MONSTRO_MAX_PATH_POINTS) return;
    canvas->path[canvas->path_count++] = pos;
}

void monstro_canvas_path_arc_to(MonstroCanvas* canvas, MonstroVec2 center, float radius, float a_min, float a_max, int num_segments) {
    if (!canvas || num_segments < 1) return;
    
    for (int i = 0; i <= num_segments; i++) {
        if (canvas->path_count >= MONSTRO_MAX_PATH_POINTS) break;
        float t = (float)i / (float)num_segments;
        float angle = a_min + (a_max - a_min) * t;
        MonstroVec2 pt = {center.x + cosf(angle) * radius, center.y + sinf(angle) * radius};
        canvas->path[canvas->path_count++] = pt;
    }
}

void monstro_canvas_path_bezier_cubic_to(MonstroCanvas* canvas, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, int num_segments) {
    if (!canvas || canvas->path_count < 1 || num_segments < 1) return;
    
    MonstroVec2 p1 = canvas->path[canvas->path_count - 1];
    
    for (int i = 1; i <= num_segments; i++) {
        if (canvas->path_count >= MONSTRO_MAX_PATH_POINTS) break;
        float t = (float)i / (float)num_segments;
        float u = 1.0f - t;
        MonstroVec2 pt;
        pt.x = u*u*u * p1.x + 3*u*u*t * p2.x + 3*u*t*t * p3.x + t*t*t * p4.x;
        pt.y = u*u*u * p1.y + 3*u*u*t * p2.y + 3*u*t*t * p3.y + t*t*t * p4.y;
        canvas->path[canvas->path_count++] = pt;
    }
}

void monstro_canvas_path_bezier_quadratic_to(MonstroCanvas* canvas, MonstroVec2 p2, MonstroVec2 p3, int num_segments) {
    if (!canvas || canvas->path_count < 1 || num_segments < 1) return;
    
    MonstroVec2 p1 = canvas->path[canvas->path_count - 1];
    
    for (int i = 1; i <= num_segments; i++) {
        if (canvas->path_count >= MONSTRO_MAX_PATH_POINTS) break;
        float t = (float)i / (float)num_segments;
        float u = 1.0f - t;
        MonstroVec2 pt;
        pt.x = u*u * p1.x + 2*u*t * p2.x + t*t * p3.x;
        pt.y = u*u * p1.y + 2*u*t * p2.y + t*t * p3.y;
        canvas->path[canvas->path_count++] = pt;
    }
}

void monstro_canvas_path_fill(MonstroCanvas* canvas, uint32_t color) {
    if (!canvas || canvas->path_count < 3) return;
    monstro_canvas_add_convex_poly_filled(canvas, canvas->path, canvas->path_count, color);
    canvas->path_count = 0;
}

void monstro_canvas_path_stroke(MonstroCanvas* canvas, uint32_t color, bool closed, float thickness) {
    if (!canvas || canvas->path_count < 2) return;
    monstro_canvas_add_polyline(canvas, canvas->path, canvas->path_count, color, closed, thickness);
    canvas->path_count = 0;
}

/* ------------------------------------------------------------------------- */
/* Draw Command Access                                                       */
/* ------------------------------------------------------------------------- */

static int compare_draw_commands(const void* a, const void* b) {
    const MonstroDrawCommand* cmd_a = (const MonstroDrawCommand*)a;
    const MonstroDrawCommand* cmd_b = (const MonstroDrawCommand*)b;
    // Sort by layer first, then by original draw order for stability
    if (cmd_a->layer != cmd_b->layer) {
        return cmd_a->layer - cmd_b->layer;
    }
    return (int)cmd_a->order - (int)cmd_b->order;
}

void monstro_canvas_get_draw_commands(MonstroCanvas* canvas, const MonstroDrawCommand** out_commands, uint32_t* out_count) {
    if (!canvas) {
        if (out_commands) *out_commands = nullptr;
        if (out_count) *out_count = 0;
        return;
    }
    
    if (canvas->dirty && canvas->draw_command_count > 1) {
        qsort(canvas->draw_commands, canvas->draw_command_count,
              sizeof(MonstroDrawCommand), compare_draw_commands);
        canvas->dirty = false;
    }
    
    if (out_commands) *out_commands = canvas->draw_commands;
    if (out_count) *out_count = canvas->draw_command_count;
}

void monstro_canvas_get_vertex_buffer(MonstroCanvas* canvas, const MonstroVertex** out_data, size_t* out_byte_size) {
    if (!canvas) {
        if (out_data) *out_data = nullptr;
        if (out_byte_size) *out_byte_size = 0;
        return;
    }
    if (out_data) *out_data = canvas->vertices;
    if (out_byte_size) *out_byte_size = canvas->vertex_count * sizeof(MonstroVertex);
}

void monstro_canvas_get_index_buffer(MonstroCanvas* canvas, const uint32_t** out_data, uint32_t* out_count) {
    if (!canvas) {
        if (out_data) *out_data = nullptr;
        if (out_count) *out_count = 0;
        return;
    }
    if (out_data) *out_data = canvas->indices;
    if (out_count) *out_count = canvas->index_count;
}

void monstro_canvas_set_vertex_buffer(MonstroCanvas* canvas, MonstroVertex* buffer, size_t capacity_in_vertices) {
    if (!canvas) return;
    
    if (!canvas->user_provided_vertex_buffer && canvas->vertices) {
        free(canvas->vertices);
    }
    
    canvas->vertices = buffer;
    canvas->vertex_capacity = capacity_in_vertices;
    canvas->user_provided_vertex_buffer = (buffer != nullptr);
}

void monstro_canvas_set_index_buffer(MonstroCanvas* canvas, uint32_t* buffer, size_t capacity_in_indices) {
    if (!canvas) return;
    
    if (!canvas->user_provided_index_buffer && canvas->indices) {
        free(canvas->indices);
    }
    
    canvas->indices = buffer;
    canvas->index_capacity = capacity_in_indices;
    canvas->user_provided_index_buffer = (buffer != nullptr);
}

/* ------------------------------------------------------------------------- */
/* Icon Registry (stub)                                                      */
/* ------------------------------------------------------------------------- */

uint64_t monstro_icon_register(uint64_t texture_id, MonstroVec2 uv_min, MonstroVec2 uv_max) {
    (void)texture_id; (void)uv_min; (void)uv_max;
    return 0;
}

void monstro_icon_unregister(uint64_t icon_id) {
    (void)icon_id;
}
