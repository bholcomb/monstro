#ifndef MONSTRO_INTERNAL_CANVAS_H
#define MONSTRO_INTERNAL_CANVAS_H

#include "constants.h"
#include "drawbuffer.h"
#include "monstro.h"

namespace monstro
{

typedef bool (*MonstroIconLookupFn)(void *user_data, uint32_t icon_id, uint64_t *out_texture_id,
                                    MonstroVec2 *out_uv_min, MonstroVec2 *out_uv_max, bool *out_monochrome);

struct Canvas
{
   DrawBuffer *buffer = nullptr;
   const char *owner_name = nullptr;

   CapacityBuffer<MonstroDrawCommand> commands{"draw command list"};

   FixedStack<MonstroRect, MONSTRO_MAX_CLIP_RECT_STACK> clip_rects{"clip rect stack"};
   FixedStack<uint64_t, MONSTRO_MAX_TEXTURE_STACK> textures{"texture stack"};
   FixedStack<int, MONSTRO_MAX_LAYER_STACK> layers{"layer stack"};

   CapacityBuffer<MonstroVec2> path{"path"};

   uint64_t default_texture_id = 0;
   uint32_t myLastIndexCount = 0;

   MonstroIconLookupFn icon_lookup = nullptr;
   void *icon_lookup_user_data = nullptr;

   Canvas() = default;
   ~Canvas();
   Canvas(const Canvas &) = delete;
   Canvas &operator=(const Canvas &) = delete;

   void allocate(const char *owner, uint32_t max_draw_commands, uint32_t max_path_points);
   void init(DrawBuffer *draw_buffer);
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

   void addPolyline(const MonstroVec2 *points, int num_points, uint32_t color, bool closed, float thickness);
   void addConvexPolyFilled(const MonstroVec2 *points, int num_points, uint32_t color);
   void addBezierCubic(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness,
                       int num_segments);
   void addBezierQuadratic(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness,
                           int num_segments);

   void addText(MonstroFont *font, MonstroVec2 pos, uint32_t color, const char *text);
   void addTextEx(MonstroFont *font, MonstroRect rect, uint32_t color, const char *text, MonstroAlign align);
   void addTextWrapped(MonstroFont *font, MonstroVec2 pos, uint32_t color, const char *text, float wrap_width);

   void addImage(uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max, uint32_t tint_color);
   void addImageRounded(uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max,
                        uint32_t tint_color, float rounding, MonstroCornerFlags corners);
   void addNinePatch(const MonstroNinePatch *patch, MonstroRect rect, uint32_t tint_color);

   void addCustomDraw(MonstroCustomDrawCallback callback, void *user_data);

   void pathClear();
   void pathMoveTo(MonstroVec2 pos);
   void pathLineTo(MonstroVec2 pos);
   void pathArcTo(MonstroVec2 center, float radius, float a_min, float a_max, int num_segments);
   void pathBezierCubicTo(MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, int num_segments);
   void pathBezierQuadraticTo(MonstroVec2 p2, MonstroVec2 p3, int num_segments);
   void pathFill(uint32_t color);
   void pathStroke(uint32_t color, bool closed, float thickness);

 private:
   MonstroDrawCommand *getOrCreateDrawCommand(uint64_t texture_id);
   MonstroDrawCommand *newDrawCommand();
   void pathPush(MonstroVec2 point);
   Reservation reserveGeometry(uint32_t vert_count, uint32_t idx_count);
   void pathArcToFast(MonstroVec2 center, float radius, int a_min_of_12, int a_max_of_12);
   void pathRect(MonstroVec2 a, MonstroVec2 b, float rounding, MonstroCornerFlags rounding_corners);
};

} // namespace monstro

#endif
