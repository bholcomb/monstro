#ifndef MONSTRO_INTERNAL_CANVAS_H
#define MONSTRO_INTERNAL_CANVAS_H

#include "constants.h"
#include "fixed.h"
#include "idStack.h"
#include "layout.h"
#include "monstro.h"
#include "uistate.h"

namespace monstro
{

struct Context;
struct Env;
struct Window;

// Where one vertex attribute is written: a base pointer (buffer plus offset) and the buffer's stride.
struct AttributeWriter
{
   uint8_t *base = nullptr;
   uint32_t stride = 0;
};

// Space for one primitive's geometry. Vertices and indices are only ever
// written, never read back (they may be write-combined GPU memory); the
// frame's content hash is taken from the values as they are written.
struct Reservation
{
   AttributeWriter position, texcoord, color;
   uint32_t first_vertex;
   uint32_t base_index; // the first vertex, counted from its command's vertex_offset
   uint32_t *indices;
   uint64_t *hash;

   void vertex(uint32_t i, float x, float y, float u, float v, uint32_t rgba);
   // Index i of the primitive refers to its vertex `vertex`.
   void index(uint32_t i, uint32_t vertex);
};

struct RecordedCommand
{
   MonstroDrawCommand command;
   const Window *owner;
   uint32_t sequence;
};

/*
 * The frame scratch an environment lends a context for one frame: the draw
 * buffers and command list, the path, the clip and layer stacks, and the
 * per-frame UI state (the working style, ID, layout and other balanced
 * stacks). Nothing in it survives from one frame to the next.
 */
struct Canvas
{
   Env *env = nullptr;
   int index = 0;
   Context *borrower = nullptr;
   int frame = 0;         // the borrower's frame number, for messages
   uint64_t frame_id = 0; // the environment's count of frames lent, for the draw data

   // The window being drawn: commands are tagged with it and sorted by window order at the end of the frame.
   const Window *owner = nullptr;
   const char *owner_name = nullptr;

   /* Geometry ------------------------------------------------------------ */
   MonstroVertex *own_vertices = nullptr;
   uint32_t own_vertex_capacity = 0;
   uint32_t *own_indices = nullptr;
   uint32_t own_index_capacity = 0;

   AttributeWriter position_writer, texcoord_writer, color_writer;
   bool host_vertices = false;
   uint32_t vertex_count = 0;
   uint32_t vertex_capacity = 0;

   uint32_t *indices = nullptr;
   bool host_indices = false;
   uint32_t index_count = 0;
   uint32_t index_capacity = 0;

   uint64_t geometry_hash = 0;

   /* Commands and path --------------------------------------------------- */
   RecordedCommand *recorded = nullptr;
   MonstroDrawCommand *sorted = nullptr;
   uint32_t command_count = 0;
   uint32_t command_capacity = 0;

   MonstroVec2 *path = nullptr;
   uint32_t path_count = 0;
   uint32_t path_capacity = 0;

   FixedStack<MonstroRect, MONSTRO_MAX_CLIP_RECT_STACK> clip_rects{"clip rect stack"};
   FixedStack<int, MONSTRO_MAX_LAYER_STACK> layers{"layer stack"};

   MonstroDrawData draw_data = {};

   /* Per-frame UI state of the borrowing context ------------------------- */
   MonstroStyle style = {};
   FixedStack<MonstroStyleBackup, MONSTRO_MAX_STYLE_VAR_STACK> style_stack{"style stack"};
   IdStack ids;
   FixedStack<bool, MONSTRO_MAX_DISABLED_STACK> disabled_stack{"disabled stack"};
   FixedStack<MonstroColumnsState, MONSTRO_MAX_COLUMN_STACK> columns{"columns stack"};
   FixedStack<Window *, MONSTRO_MAX_WINDOW_DEPTH> windows{"window stack"};
   FixedStack<Layout, MONSTRO_MAX_LAYOUT_STACK_DEPTH> layouts{"layout stack"};
   FixedStack<MonstroGroupFlags, MONSTRO_MAX_GROUP_STACK_DEPTH> groups{"group stack"};
   FixedStack<MonstroTabBarState *, MONSTRO_MAX_TAB_BAR_DEPTH> tab_bars{"tab bar stack"};
   MonstroTableFrame table;
   char tooltip_text[MONSTRO_MAX_TOOLTIP_TEXT] = {};
   bool has_tooltip = false;

   Canvas() = default;
   ~Canvas();
   Canvas(const Canvas &) = delete;
   Canvas &operator=(const Canvas &) = delete;

   bool init(Env *environment, int canvas_index, const MonstroEnvConfig &config);
   size_t residentBytes() const;

   // Lend the canvas to ctx for one frame, writing into buffers when given.
   void lend(Context *ctx, int frame_number, const MonstroFrameBuffers *buffers);
   // Sort the frame's commands into window order and fill draw_data.
   const MonstroDrawData *finish(uint64_t previous_hash);
   void release();

   void setOwner(const Window *window, const char *name);

   void pushClipRect(MonstroRect rect);
   // A window's first clip rect: taken as is, not intersected with the enclosing window's.
   void pushBaseClipRect(MonstroRect rect);
   void pushClipRectFullscreen();
   void popClipRect();
   MonstroRect getClipRect() const;

   void pushLayer(int layer);
   void popLayer();

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
   // The command a primitive drawn now with texture_id joins, and room for its geometry.
   Reservation reserve(uint64_t texture_id, uint32_t vert_count, uint32_t idx_count);
   // Solid shapes sample the atlas's white texel, so they batch with text.
   Reservation reserveSolid(uint32_t vert_count, uint32_t idx_count);
   MonstroDrawCommand *getOrCreateDrawCommand(uint64_t texture_id);
   MonstroDrawCommand *newDrawCommand();
   void pathPush(MonstroVec2 point);
   void pathArcToFast(MonstroVec2 center, float radius, int a_min_of_12, int a_max_of_12);
   void pathRect(MonstroVec2 a, MonstroVec2 b, float rounding, MonstroCornerFlags rounding_corners);
   void addTextRun(MonstroFont *font, MonstroVec2 pos, uint32_t color, const char *text, float wrap_width);
};

} // namespace monstro

#endif
