#ifndef MONSTRO_INTERNAL_CANVAS_H
#define MONSTRO_INTERNAL_CANVAS_H

#include "monstro.h"
#include "constants.h"
#include "drawbuffer.h"

namespace monstro {

struct Canvas {
    DrawBuffer* buffer = nullptr;

    MonstroDrawCommand draw_commands[MONSTRO_MAX_DRAW_COMMANDS];
    uint32_t draw_command_count = 0;

    MonstroRect clip_rect_stack[MONSTRO_MAX_CLIP_RECT_STACK];
    int clip_rect_stack_top = -1;

    uint64_t texture_stack[MONSTRO_MAX_TEXTURE_STACK];
    int texture_stack_top = -1;

    int layer_stack[MONSTRO_MAX_LAYER_STACK];
    int layer_stack_top = -1;

    MonstroVec2 path[MONSTRO_MAX_PATH_POINTS];
    int path_count = 0;

    uint64_t default_texture_id = 0;
    uint32_t myLastIndexCount = 0;

    void init(DrawBuffer* draw_buffer);
    void clear();

    void pushClipRect(MonstroRect rect);
    void pushClipRectFullscreen();
    void popClipRect();
    MonstroRect getClipRect() const;

    void pushLayer(int layer);
    void popLayer();

    void pushTexture(uint64_t texture_id);
    void popTexture();

    void addLine(MonstroVec2 p1, MonstroVec2 p2, uint32_t color, float thickness);
    void addRect(MonstroRect rect, uint32_t color, float rounding, MonstroCornerFlags corners, float thickness);
    void addRectFilled(MonstroRect rect, uint32_t color, float rounding, MonstroCornerFlags corners);
    void addRectFilledMultiColor(MonstroRect rect, uint32_t col_ul, uint32_t col_ur, uint32_t col_lr, uint32_t col_ll);
    void addTriangle(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness);
    void addTriangleFilled(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color);
    void addCircle(MonstroVec2 center, float radius, uint32_t color, int num_segments, float thickness);
    void addCircleFilled(MonstroVec2 center, float radius, uint32_t color, int num_segments);
    void addQuad(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness);
    void addQuadFilled(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color);

    void addPolyline(const MonstroVec2* points, int num_points, uint32_t color, bool closed, float thickness);
    void addConvexPolyFilled(const MonstroVec2* points, int num_points, uint32_t color);
    void addBezierCubic(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness, int num_segments);
    void addBezierQuadratic(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness, int num_segments);

    void addText(MonstroFont* font, MonstroVec2 pos, uint32_t color, const char* text);
    void addTextEx(MonstroFont* font, MonstroRect rect, uint32_t color, const char* text, MonstroAlign align);
    void addTextWrapped(MonstroFont* font, MonstroVec2 pos, uint32_t color, const char* text, float wrap_width);

    void addImage(uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color);
    void addImageRounded(uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color, float rounding, MonstroCornerFlags corners);
    void addNinePatch(const MonstroNinePatch* patch, MonstroRect rect, uint32_t tint_color);
    void addIcon(uint32_t icon_id, MonstroRect rect, uint32_t tint_color);

    void addCustomDraw(MonstroCustomDrawCallback callback, void* user_data);

    void pathClear();
    void pathMoveTo(MonstroVec2 pos);
    void pathLineTo(MonstroVec2 pos);
    void pathArcTo(MonstroVec2 center, float radius, float a_min, float a_max, int num_segments);
    void pathBezierCubicTo(MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, int num_segments);
    void pathBezierQuadraticTo(MonstroVec2 p2, MonstroVec2 p3, int num_segments);
    void pathFill(uint32_t color);
    void pathStroke(uint32_t color, bool closed, float thickness);

private:
    MonstroDrawCommand* getOrCreateDrawCommand(uint64_t texture_id);
    Reservation reserveGeometry(uint32_t vert_count, uint32_t idx_count);
    void pathArcToFast(MonstroVec2 center, float radius, int a_min_of_12, int a_max_of_12);
    void pathRect(MonstroVec2 a, MonstroVec2 b, float rounding, MonstroCornerFlags rounding_corners);
};

} // namespace monstro

#endif
