#include "canvas.h"
#include "drawbuffer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <stdexcept>

namespace monstro {

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

/* ------------------------------------------------------------------------- */
/* Lifecycle                                                                 */
/* ------------------------------------------------------------------------- */

void Canvas::init(DrawBuffer* draw_buffer) {
    buffer = draw_buffer;
    clear();
}

void Canvas::clear() {
    draw_command_count = 0;
    path_count = 0;
    myLastIndexCount = 0;

    clip_rect_stack[0] = {0, 0, 100000, 100000};
    clip_rect_stack_top = 0;
    texture_stack[0] = default_texture_id;
    texture_stack_top = 0;
    layer_stack[0] = 0;
    layer_stack_top = 0;
}

/* ------------------------------------------------------------------------- */
/* Private Helpers                                                           */
/* ------------------------------------------------------------------------- */

Reservation Canvas::reserveGeometry(uint32_t vert_count, uint32_t idx_count) {
    Reservation r = buffer->reserve(vert_count, idx_count);
    myLastIndexCount = buffer->myIndexCount;
    return r;
}

MonstroDrawCommand* Canvas::getOrCreateDrawCommand(uint64_t texture_id) {
    bool buffer_gap = (buffer->myIndexCount != myLastIndexCount);

    if (draw_command_count > 0 && !buffer_gap) {
        MonstroDrawCommand* prev = &draw_commands[draw_command_count - 1];

        MonstroRect clip = clip_rect_stack[clip_rect_stack_top];
        int layer = layer_stack[layer_stack_top];

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

    if (draw_command_count >= MONSTRO_MAX_DRAW_COMMANDS) return nullptr;

    MonstroDrawCommand* cmd = &draw_commands[draw_command_count];
    memset(cmd, 0, sizeof(MonstroDrawCommand));

    MonstroRect clip = clip_rect_stack[clip_rect_stack_top];
    cmd->clip_rect[0] = clip.x;
    cmd->clip_rect[1] = clip.y;
    cmd->clip_rect[2] = clip.w;
    cmd->clip_rect[3] = clip.h;
    cmd->layer = layer_stack[layer_stack_top];
    cmd->order = draw_command_count;
    cmd->texture_id = texture_id;
    cmd->index_offset = buffer->myIndexCount;
    cmd->index_count = 0;
    cmd->custom_draw = nullptr;
    cmd->user_data = nullptr;

    draw_command_count++;

    return cmd;
}

void Canvas::pathArcToFast(MonstroVec2 center, float radius, int a_min_of_12, int a_max_of_12) {
    ensure_circle_vtx();

    if (a_min_of_12 > a_max_of_12) return;

    if (radius == 0.0f) {
        if (path_count < MONSTRO_MAX_PATH_POINTS) {
            path[path_count++] = center;
        }
    } else {
        for (int a = a_min_of_12; a <= a_max_of_12; a++) {
            MonstroVec2 c = g_circle_vtx[a % 12];
            if (path_count < MONSTRO_MAX_PATH_POINTS) {
                path[path_count++] = {center.x + c.x * radius, center.y + c.y * radius};
            }
        }
    }
}

void Canvas::pathRect(MonstroVec2 a, MonstroVec2 b, float rounding, MonstroCornerFlags rounding_corners) {
    float r = rounding;

    r = fminf(r, fabsf(b.x - a.x) * (((rounding_corners & MONSTRO_CORNER_BOTTOM) == MONSTRO_CORNER_BOTTOM) ||
                                      ((rounding_corners & MONSTRO_CORNER_TOP) == MONSTRO_CORNER_TOP) ? 0.5f : 1.0f) - 1.0f);
    r = fminf(r, fabsf(b.y - a.y) * (((rounding_corners & MONSTRO_CORNER_LEFT) == MONSTRO_CORNER_LEFT) ||
                                      ((rounding_corners & MONSTRO_CORNER_RIGHT) == MONSTRO_CORNER_RIGHT) ? 0.5f : 1.0f) - 1.0f);

    if (r <= 0.0f || rounding_corners == 0) {
        if (path_count < MONSTRO_MAX_PATH_POINTS) path[path_count++] = a;
        if (path_count < MONSTRO_MAX_PATH_POINTS) path[path_count++] = {b.x, a.y};
        if (path_count < MONSTRO_MAX_PATH_POINTS) path[path_count++] = b;
        if (path_count < MONSTRO_MAX_PATH_POINTS) path[path_count++] = {a.x, b.y};
    } else {
        float r0 = (rounding_corners & MONSTRO_CORNER_UL) ? r : 0.0f;
        float r1 = (rounding_corners & MONSTRO_CORNER_UR) ? r : 0.0f;
        float r2 = (rounding_corners & MONSTRO_CORNER_LR) ? r : 0.0f;
        float r3 = (rounding_corners & MONSTRO_CORNER_LL) ? r : 0.0f;
        pathArcToFast({a.x + r0, a.y + r0}, r0, 6, 9);
        pathArcToFast({b.x - r1, a.y + r1}, r1, 9, 12);
        pathArcToFast({b.x - r2, b.y - r2}, r2, 0, 3);
        pathArcToFast({a.x + r3, b.y - r3}, r3, 3, 6);
    }
}

/* ------------------------------------------------------------------------- */
/* Clip Rect Stack                                                           */
/* ------------------------------------------------------------------------- */

void Canvas::pushClipRect(MonstroRect rect) {
    if (clip_rect_stack_top >= MONSTRO_MAX_CLIP_RECT_STACK - 1) return;
    clip_rect_stack_top++;
    clip_rect_stack[clip_rect_stack_top] = rect;
}

void Canvas::pushClipRectFullscreen() {
    MonstroRect rect = {0, 0, 100000, 100000};
    pushClipRect(rect);
}

void Canvas::popClipRect() {
    if (clip_rect_stack_top <= 0) return;
    clip_rect_stack_top--;
}

MonstroRect Canvas::getClipRect() const {
    return clip_rect_stack[clip_rect_stack_top];
}

/* ------------------------------------------------------------------------- */
/* Layer Stack                                                               */
/* ------------------------------------------------------------------------- */

void Canvas::pushLayer(int layer) {
    if (layer_stack_top >= MONSTRO_MAX_LAYER_STACK - 1) return;
    layer_stack_top++;
    layer_stack[layer_stack_top] = layer;
}

void Canvas::popLayer() {
    if (layer_stack_top <= 0) return;
    layer_stack_top--;
}

/* ------------------------------------------------------------------------- */
/* Texture Stack                                                             */
/* ------------------------------------------------------------------------- */

void Canvas::pushTexture(uint64_t texture_id) {
    if (texture_stack_top >= MONSTRO_MAX_TEXTURE_STACK - 1) return;
    texture_stack_top++;
    texture_stack[texture_stack_top] = texture_id;
}

void Canvas::popTexture() {
    if (texture_stack_top <= 0) return;
    texture_stack_top--;
}

/* ------------------------------------------------------------------------- */
/* Primitive Drawing                                                         */
/* ------------------------------------------------------------------------- */

void Canvas::addLine(MonstroVec2 p1, MonstroVec2 p2, uint32_t color, float thickness) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    dx /= len;
    dy /= len;

    float nx = -dy * thickness * 0.5f;
    float ny = dx * thickness * 0.5f;

    uint64_t tex = texture_stack[texture_stack_top];
    MonstroDrawCommand* cmd = getOrCreateDrawCommand(tex);
    if (!cmd) return;

    Reservation r = reserveGeometry(4, 6);

    r.vertices[0] = {{p1.x + nx, p1.y + ny}, {0.0f, 0.0f}, color};
    r.vertices[1] = {{p1.x - nx, p1.y - ny}, {0.0f, 0.0f}, color};
    r.vertices[2] = {{p2.x - nx, p2.y - ny}, {0.0f, 0.0f}, color};
    r.vertices[3] = {{p2.x + nx, p2.y + ny}, {0.0f, 0.0f}, color};

    r.indices[0] = r.baseVertex; r.indices[1] = r.baseVertex + 1; r.indices[2] = r.baseVertex + 2;
    r.indices[3] = r.baseVertex; r.indices[4] = r.baseVertex + 2; r.indices[5] = r.baseVertex + 3;
    cmd->index_count += 6;
}

void Canvas::addRect(MonstroRect rect, uint32_t color, float rounding, MonstroCornerFlags corners, float thickness) {
    MonstroVec2 a = {rect.x + 0.5f, rect.y + 0.5f};
    MonstroVec2 b = {rect.x + rect.w + 0.5f, rect.y + rect.h + 0.5f};

    path_count = 0;
    pathRect(a, b, rounding, corners);
    pathStroke(color, true, thickness);
}

void Canvas::addRectFilled(MonstroRect rect, uint32_t color, float rounding, MonstroCornerFlags corners) {
    MonstroVec2 a = {rect.x, rect.y};
    MonstroVec2 b = {rect.x + rect.w, rect.y + rect.h};

    if (rounding > 0.0f && corners != MONSTRO_CORNER_NONE) {
        path_count = 0;
        pathRect(a, b, rounding, corners);
        pathFill(color);
    } else {
        uint64_t tex = texture_stack[texture_stack_top];
        MonstroDrawCommand* cmd = getOrCreateDrawCommand(tex);
        if (!cmd) return;

        Reservation r = reserveGeometry(4, 6);

        r.vertices[0] = {{rect.x, rect.y}, {0.0f, 0.0f}, color};
        r.vertices[1] = {{rect.x + rect.w, rect.y}, {1.0f, 0.0f}, color};
        r.vertices[2] = {{rect.x + rect.w, rect.y + rect.h}, {1.0f, 1.0f}, color};
        r.vertices[3] = {{rect.x, rect.y + rect.h}, {0.0f, 1.0f}, color};

        r.indices[0] = r.baseVertex; r.indices[1] = r.baseVertex + 1; r.indices[2] = r.baseVertex + 2;
        r.indices[3] = r.baseVertex; r.indices[4] = r.baseVertex + 2; r.indices[5] = r.baseVertex + 3;
        cmd->index_count += 6;
    }
}

void Canvas::addRectFilledMultiColor(MonstroRect rect, uint32_t col_ul, uint32_t col_ur, uint32_t col_lr, uint32_t col_ll) {
    uint64_t tex = texture_stack[texture_stack_top];
    MonstroDrawCommand* cmd = getOrCreateDrawCommand(tex);
    if (!cmd) return;

    Reservation r = reserveGeometry(4, 6);

    r.vertices[0] = {{rect.x, rect.y}, {0.0f, 0.0f}, col_ul};
    r.vertices[1] = {{rect.x + rect.w, rect.y}, {1.0f, 0.0f}, col_ur};
    r.vertices[2] = {{rect.x + rect.w, rect.y + rect.h}, {1.0f, 1.0f}, col_lr};
    r.vertices[3] = {{rect.x, rect.y + rect.h}, {0.0f, 1.0f}, col_ll};

    r.indices[0] = r.baseVertex; r.indices[1] = r.baseVertex + 1; r.indices[2] = r.baseVertex + 2;
    r.indices[3] = r.baseVertex; r.indices[4] = r.baseVertex + 2; r.indices[5] = r.baseVertex + 3;
    cmd->index_count += 6;
}

void Canvas::addTriangleFilled(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color) {
    uint64_t tex = texture_stack[texture_stack_top];
    MonstroDrawCommand* cmd = getOrCreateDrawCommand(tex);
    if (!cmd) return;

    Reservation r = reserveGeometry(3, 3);

    r.vertices[0] = {{p1.x, p1.y}, {0.0f, 0.0f}, color};
    r.vertices[1] = {{p2.x, p2.y}, {0.0f, 0.0f}, color};
    r.vertices[2] = {{p3.x, p3.y}, {0.0f, 0.0f}, color};

    r.indices[0] = r.baseVertex; r.indices[1] = r.baseVertex + 1; r.indices[2] = r.baseVertex + 2;
    cmd->index_count += 3;
}

void Canvas::addTriangle(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness) {
    addLine(p1, p2, color, thickness);
    addLine(p2, p3, color, thickness);
    addLine(p3, p1, color, thickness);
}

void Canvas::addCircleFilled(MonstroVec2 center, float radius, uint32_t color, int num_segments) {
    if (num_segments < 3) return;

    uint64_t tex = texture_stack[texture_stack_top];
    MonstroDrawCommand* cmd = getOrCreateDrawCommand(tex);
    if (!cmd) return;

    Reservation r = reserveGeometry((uint32_t)(num_segments + 1), (uint32_t)(num_segments * 3));

    r.vertices[0] = {{center.x, center.y}, {0.5f, 0.5f}, color};

    for (int i = 0; i < num_segments; i++) {
        float angle = (float)i / (float)num_segments * 6.2831853f;
        r.vertices[i + 1] = {{center.x + cosf(angle) * radius, center.y + sinf(angle) * radius}, {0.0f, 0.0f}, color};
    }

    for (int i = 0; i < num_segments; i++) {
        r.indices[i * 3] = r.baseVertex;
        r.indices[i * 3 + 1] = r.baseVertex + 1 + i;
        r.indices[i * 3 + 2] = r.baseVertex + 1 + (i + 1) % num_segments;
    }
    cmd->index_count += num_segments * 3;
}

void Canvas::addCircle(MonstroVec2 center, float radius, uint32_t color, int num_segments, float thickness) {
    if (num_segments < 3) return;

    for (int i = 0; i < num_segments; i++) {
        float a1 = (float)i / (float)num_segments * 6.2831853f;
        float a2 = (float)(i + 1) / (float)num_segments * 6.2831853f;
        MonstroVec2 p1 = {center.x + cosf(a1) * radius, center.y + sinf(a1) * radius};
        MonstroVec2 p2 = {center.x + cosf(a2) * radius, center.y + sinf(a2) * radius};
        addLine(p1, p2, color, thickness);
    }
}

void Canvas::addQuad(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness) {
    addLine(p1, p2, color, thickness);
    addLine(p2, p3, color, thickness);
    addLine(p3, p4, color, thickness);
    addLine(p4, p1, color, thickness);
}

void Canvas::addQuadFilled(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color) {
    uint64_t tex = texture_stack[texture_stack_top];
    MonstroDrawCommand* cmd = getOrCreateDrawCommand(tex);
    if (!cmd) return;

    Reservation r = reserveGeometry(4, 6);

    r.vertices[0] = {{p1.x, p1.y}, {0.0f, 0.0f}, color};
    r.vertices[1] = {{p2.x, p2.y}, {1.0f, 0.0f}, color};
    r.vertices[2] = {{p3.x, p3.y}, {1.0f, 1.0f}, color};
    r.vertices[3] = {{p4.x, p4.y}, {0.0f, 1.0f}, color};

    r.indices[0] = r.baseVertex; r.indices[1] = r.baseVertex + 1; r.indices[2] = r.baseVertex + 2;
    r.indices[3] = r.baseVertex; r.indices[4] = r.baseVertex + 2; r.indices[5] = r.baseVertex + 3;
    cmd->index_count += 6;
}

/* ------------------------------------------------------------------------- */
/* Complex Shapes                                                            */
/* ------------------------------------------------------------------------- */

void Canvas::addPolyline(const MonstroVec2* points, int num_points, uint32_t color, bool closed, float thickness) {
    if (num_points < 2) return;

    int segments = closed ? num_points : num_points - 1;
    for (int i = 0; i < segments; i++) {
        int j = (i + 1) % num_points;
        addLine(points[i], points[j], color, thickness);
    }
}

void Canvas::addConvexPolyFilled(const MonstroVec2* points, int num_points, uint32_t color) {
    if (num_points < 3) return;

    uint64_t tex = texture_stack[texture_stack_top];
    MonstroDrawCommand* cmd = getOrCreateDrawCommand(tex);
    if (!cmd) return;

    uint32_t idx_count = (uint32_t)((num_points - 2) * 3);
    Reservation r = reserveGeometry((uint32_t)num_points, idx_count);

    for (int i = 0; i < num_points; i++) {
        r.vertices[i] = {{points[i].x, points[i].y}, {0.0f, 0.0f}, color};
    }

    uint32_t ii = 0;
    for (int i = 1; i < num_points - 1; i++) {
        r.indices[ii++] = r.baseVertex;
        r.indices[ii++] = r.baseVertex + i;
        r.indices[ii++] = r.baseVertex + i + 1;
    }
    cmd->index_count += idx_count;
}

void Canvas::addBezierCubic(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness, int num_segments) {
    if (num_segments < 1) return;

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

        addLine(prev, pt, color, thickness);
        prev = pt;
    }
}

void Canvas::addBezierQuadratic(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness, int num_segments) {
    if (num_segments < 1) return;

    MonstroVec2 prev = p1;
    for (int i = 1; i <= num_segments; i++) {
        float t = (float)i / (float)num_segments;
        float u = 1.0f - t;

        MonstroVec2 pt;
        pt.x = u * u * p1.x + 2 * u * t * p2.x + t * t * p3.x;
        pt.y = u * u * p1.y + 2 * u * t * p2.y + t * t * p3.y;

        addLine(prev, pt, color, thickness);
        prev = pt;
    }
}

/* ------------------------------------------------------------------------- */
/* Text Drawing                                                              */
/* ------------------------------------------------------------------------- */

static uint32_t decode_utf8(const char** str) {
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

void Canvas::addText(MonstroFont* font, MonstroVec2 pos, uint32_t color, const char* text) {
    if (!font || !text) return;

    uint64_t texture_id = monstro_font_get_atlas_texture(font);
    if (texture_id == 0) return;

    float pen_x = pos.x;
    float pen_y = pos.y;
    float line_y = 0;
    float line_height = monstro_font_get_line_height(font);

    const char* s = text;
    while (*s) {
        uint32_t codepoint = decode_utf8(&s);

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
            addImage(texture_id, rect, uv_min, uv_max, color);
        }

        pen_x += glyph.advance[0];
    }
}

void Canvas::addTextEx(MonstroFont* font, MonstroRect rect, uint32_t color, const char* text, MonstroAlign align) {
    if (!font || !text) return;

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
    addText(font, pos, color, text);
}

/* ------------------------------------------------------------------------- */
/* Image Drawing                                                             */
/* ------------------------------------------------------------------------- */

void Canvas::addImage(uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color) {
    MonstroDrawCommand* cmd = getOrCreateDrawCommand(texture_id);
    if (!cmd) return;

    Reservation r = reserveGeometry(4, 6);

    r.vertices[0] = {{rect.x, rect.y}, {uv_min.x, uv_min.y}, tint_color};
    r.vertices[1] = {{rect.x + rect.w, rect.y}, {uv_max.x, uv_min.y}, tint_color};
    r.vertices[2] = {{rect.x + rect.w, rect.y + rect.h}, {uv_max.x, uv_max.y}, tint_color};
    r.vertices[3] = {{rect.x, rect.y + rect.h}, {uv_min.x, uv_max.y}, tint_color};

    r.indices[0] = r.baseVertex; r.indices[1] = r.baseVertex + 1; r.indices[2] = r.baseVertex + 2;
    r.indices[3] = r.baseVertex; r.indices[4] = r.baseVertex + 2; r.indices[5] = r.baseVertex + 3;
    cmd->index_count += 6;
}

void Canvas::addImageRounded(uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color, float rounding, MonstroCornerFlags corners) {
    if (rounding <= 0.0f || corners == MONSTRO_CORNER_NONE) {
        addImage(texture_id, rect, uv_min, uv_max, tint_color);
        return;
    }

    MonstroVec2 a = {rect.x, rect.y};
    MonstroVec2 b = {rect.x + rect.w, rect.y + rect.h};

    path_count = 0;
    pathRect(a, b, rounding, corners);

    if (path_count < 3) return;

    int num_points = path_count;

    MonstroDrawCommand* cmd = getOrCreateDrawCommand(texture_id);
    if (!cmd) return;

    uint32_t idx_count = (uint32_t)((num_points - 2) * 3);
    Reservation r = reserveGeometry((uint32_t)num_points, idx_count);

    float inv_w = (rect.w > 0.0f) ? 1.0f / rect.w : 0.0f;
    float inv_h = (rect.h > 0.0f) ? 1.0f / rect.h : 0.0f;

    for (int i = 0; i < num_points; i++) {
        float t_x = (path[i].x - rect.x) * inv_w;
        float t_y = (path[i].y - rect.y) * inv_h;
        float u = uv_min.x + (uv_max.x - uv_min.x) * t_x;
        float uv = uv_min.y + (uv_max.y - uv_min.y) * t_y;
        r.vertices[i] = {{path[i].x, path[i].y}, {u, uv}, tint_color};
    }

    uint32_t ii = 0;
    for (int i = 1; i < num_points - 1; i++) {
        r.indices[ii++] = r.baseVertex;
        r.indices[ii++] = r.baseVertex + i;
        r.indices[ii++] = r.baseVertex + i + 1;
    }
    cmd->index_count += idx_count;

    path_count = 0;
}

void Canvas::addNinePatch(const MonstroNinePatch* patch, MonstroRect rect, uint32_t tint_color) {
    if (!patch) return;

    MonstroDrawCommand* cmd = getOrCreateDrawCommand(patch->texture_id);
    if (!cmd) return;

    Reservation r = reserveGeometry(16, 54);

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

    uint32_t vi = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            r.vertices[vi++] = {{xp[j], yp[i]}, {uv_xp[j], uv_yp[i]}, tint_color};
        }
    }

    uint32_t ii = 0;

    #define NINE_PATCH_QUAD(v0, v1, v2, v3) \
        r.indices[ii++] = r.baseVertex + v0; \
        r.indices[ii++] = r.baseVertex + v1; \
        r.indices[ii++] = r.baseVertex + v2; \
        r.indices[ii++] = r.baseVertex + v0; \
        r.indices[ii++] = r.baseVertex + v2; \
        r.indices[ii++] = r.baseVertex + v3;

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

    cmd->index_count += 54;
}

void Canvas::addIcon(uint32_t icon_id, MonstroRect rect, uint32_t tint_color) {
    (void)icon_id; (void)rect; (void)tint_color;
}

/* ------------------------------------------------------------------------- */
/* Custom Draw                                                               */
/* ------------------------------------------------------------------------- */

void Canvas::addCustomDraw(MonstroCustomDrawCallback callback, void* user_data) {
    if (!callback || draw_command_count >= MONSTRO_MAX_DRAW_COMMANDS) return;

    MonstroDrawCommand* cmd = &draw_commands[draw_command_count];

    MonstroRect clip = clip_rect_stack[clip_rect_stack_top];
    cmd->clip_rect[0] = clip.x;
    cmd->clip_rect[1] = clip.y;
    cmd->clip_rect[2] = clip.w;
    cmd->clip_rect[3] = clip.h;
    cmd->layer = layer_stack[layer_stack_top];
    cmd->order = draw_command_count;
    cmd->texture_id = 0;
    cmd->index_offset = 0;
    cmd->index_count = 0;
    cmd->custom_draw = callback;
    cmd->user_data = user_data;

    draw_command_count++;
}

/* ------------------------------------------------------------------------- */
/* Path API                                                                  */
/* ------------------------------------------------------------------------- */

void Canvas::pathClear() {
    path_count = 0;
}

void Canvas::pathMoveTo(MonstroVec2 pos) {
    path_count = 0;
    if (path_count < MONSTRO_MAX_PATH_POINTS) {
        path[path_count++] = pos;
    }
}

void Canvas::pathLineTo(MonstroVec2 pos) {
    if (path_count >= MONSTRO_MAX_PATH_POINTS) return;
    path[path_count++] = pos;
}

void Canvas::pathArcTo(MonstroVec2 center, float radius, float a_min, float a_max, int num_segments) {
    if (num_segments < 1) return;

    for (int i = 0; i <= num_segments; i++) {
        if (path_count >= MONSTRO_MAX_PATH_POINTS) break;
        float t = (float)i / (float)num_segments;
        float angle = a_min + (a_max - a_min) * t;
        MonstroVec2 pt = {center.x + cosf(angle) * radius, center.y + sinf(angle) * radius};
        path[path_count++] = pt;
    }
}

void Canvas::pathBezierCubicTo(MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, int num_segments) {
    if (path_count < 1 || num_segments < 1) return;

    MonstroVec2 p1 = path[path_count - 1];

    for (int i = 1; i <= num_segments; i++) {
        if (path_count >= MONSTRO_MAX_PATH_POINTS) break;
        float t = (float)i / (float)num_segments;
        float u = 1.0f - t;
        MonstroVec2 pt;
        pt.x = u*u*u * p1.x + 3*u*u*t * p2.x + 3*u*t*t * p3.x + t*t*t * p4.x;
        pt.y = u*u*u * p1.y + 3*u*u*t * p2.y + 3*u*t*t * p3.y + t*t*t * p4.y;
        path[path_count++] = pt;
    }
}

void Canvas::pathBezierQuadraticTo(MonstroVec2 p2, MonstroVec2 p3, int num_segments) {
    if (path_count < 1 || num_segments < 1) return;

    MonstroVec2 p1 = path[path_count - 1];

    for (int i = 1; i <= num_segments; i++) {
        if (path_count >= MONSTRO_MAX_PATH_POINTS) break;
        float t = (float)i / (float)num_segments;
        float u = 1.0f - t;
        MonstroVec2 pt;
        pt.x = u*u * p1.x + 2*u*t * p2.x + t*t * p3.x;
        pt.y = u*u * p1.y + 2*u*t * p2.y + t*t * p3.y;
        path[path_count++] = pt;
    }
}

void Canvas::pathFill(uint32_t color) {
    if (path_count < 3) return;
    addConvexPolyFilled(path, path_count, color);
    path_count = 0;
}

void Canvas::pathStroke(uint32_t color, bool closed, float thickness) {
    if (path_count < 2) return;
    addPolyline(path, path_count, color, closed, thickness);
    path_count = 0;
}

} // namespace monstro
