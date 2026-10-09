#include "canvas.h"
#include "context.h"
#include "env.h"
#include "helpers.h"
#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace monstro
{

/* ------------------------------------------------------------------------- */
/* Precomputed Circle Vertices (12-step)                                     */
/* ------------------------------------------------------------------------- */

// cos and sin of i * 30 degrees. A constant table: the library keeps no mutable global state.
static const MonstroVec2 CIRCLE_VERTICES[12] = {
    {1.0f, 0.0f},  {0.8660254f, 0.5f},   {0.5f, 0.8660254f},   {0.0f, 1.0f},  {-0.5f, 0.8660254f}, {-0.8660254f, 0.5f},
    {-1.0f, 0.0f}, {-0.8660254f, -0.5f}, {-0.5f, -0.8660254f}, {0.0f, -1.0f}, {0.5f, -0.8660254f}, {0.8660254f, -0.5f},
};

static const MonstroRect FULLSCREEN_CLIP = {0, 0, 100000, 100000};

/* ------------------------------------------------------------------------- */
/* Content hash                                                              */
/* ------------------------------------------------------------------------- */

static constexpr uint64_t HASH_SEED = 14695981039346656037ULL;
static constexpr uint64_t HASH_PRIME = 1099511628211ULL;

static inline void hash_word(uint64_t *hash, uint32_t word) { *hash = (*hash ^ word) * HASH_PRIME; }

static inline void hash_float(uint64_t *hash, float value)
{
   uint32_t word;
   memcpy(&word, &value, sizeof(word));
   hash_word(hash, word);
}

static inline void hash_u64(uint64_t *hash, uint64_t value)
{
   hash_word(hash, (uint32_t)value);
   hash_word(hash, (uint32_t)(value >> 32));
}

/* ------------------------------------------------------------------------- */
/* Reservation                                                               */
/* ------------------------------------------------------------------------- */

void Reservation::vertex(uint32_t i, float x, float y, float u, float v, uint32_t rgba)
{
   size_t n = first_vertex + i;
   float xy[2] = {x, y};
   float uv[2] = {u, v};
   memcpy(position.base + n * position.stride, xy, sizeof(xy));
   memcpy(texcoord.base + n * texcoord.stride, uv, sizeof(uv));
   memcpy(color.base + n * color.stride, &rgba, sizeof(rgba));
   hash_float(hash, x);
   hash_float(hash, y);
   hash_float(hash, u);
   hash_float(hash, v);
   hash_word(hash, rgba);
}

void Reservation::index(uint32_t i, uint32_t vertex)
{
   uint32_t value = base_index + vertex;
   indices[i] = value;
   hash_word(hash, value);
}

/* ------------------------------------------------------------------------- */
/* Lifecycle                                                                 */
/* ------------------------------------------------------------------------- */

Canvas::~Canvas()
{
   free(own_vertices);
   free(own_indices);
   free(recorded);
   free(sorted);
   free(path);
}

bool Canvas::init(Env *environment, int canvas_index, const MonstroEnvConfig &config)
{
   env = environment;
   index = canvas_index;
   own_vertices = (MonstroVertex *)malloc(sizeof(MonstroVertex) * config.max_vertices);
   own_indices = (uint32_t *)malloc(sizeof(uint32_t) * config.max_indices);
   recorded = (RecordedCommand *)malloc(sizeof(RecordedCommand) * config.max_draw_commands);
   sorted = (MonstroDrawCommand *)malloc(sizeof(MonstroDrawCommand) * config.max_draw_commands);
   path = (MonstroVec2 *)malloc(sizeof(MonstroVec2) * config.max_path_points);
   own_vertex_capacity = config.max_vertices;
   own_index_capacity = config.max_indices;
   command_capacity = config.max_draw_commands;
   path_capacity = config.max_path_points;
   return own_vertices && own_indices && recorded && sorted && path;
}

size_t Canvas::residentBytes() const
{
   return sizeof(Canvas) + sizeof(MonstroVertex) * own_vertex_capacity + sizeof(uint32_t) * own_index_capacity +
          (sizeof(RecordedCommand) + sizeof(MonstroDrawCommand)) * command_capacity +
          sizeof(MonstroVec2) * path_capacity;
}

static AttributeWriter attribute_writer(void *const *buffers, const MonstroVertexLayout &layout,
                                        MonstroVertexAttribute attribute)
{
   AttributeWriter writer;
   writer.base = (uint8_t *)buffers[attribute.buffer] + attribute.offset;
   writer.stride = layout.stride[attribute.buffer];
   return writer;
}

void Canvas::lend(Context *ctx, int frame_number, const MonstroFrameBuffers *buffers)
{
   borrower = ctx;
   frame = frame_number;
   owner = nullptr;
   owner_name = nullptr;

   host_vertices = buffers && buffers->vertex_buffers[0];
   if (host_vertices)
   {
      const MonstroVertexLayout &layout = env->config.vertex_layout;
      for (uint32_t b = 0; b < layout.buffer_count; b++)
         MONSTRO_REQUIRE(buffers->vertex_buffers[b],
                         "monstro_begin_frame_into: vertex_buffers[%u] is null but the environment's vertex layout "
                         "has %u buffers",
                         b, layout.buffer_count);
      MONSTRO_REQUIRE(buffers->vertex_capacity > 0, "monstro_begin_frame_into: vertex_capacity is 0");
      position_writer = attribute_writer(buffers->vertex_buffers, layout, layout.position);
      texcoord_writer = attribute_writer(buffers->vertex_buffers, layout, layout.texcoord);
      color_writer = attribute_writer(buffers->vertex_buffers, layout, layout.color);
      vertex_capacity = buffers->vertex_capacity;
   }
   else
   {
      uint8_t *base = (uint8_t *)own_vertices;
      position_writer = {base + offsetof(MonstroVertex, position), (uint32_t)sizeof(MonstroVertex)};
      texcoord_writer = {base + offsetof(MonstroVertex, texcoord), (uint32_t)sizeof(MonstroVertex)};
      color_writer = {base + offsetof(MonstroVertex, color), (uint32_t)sizeof(MonstroVertex)};
      vertex_capacity = own_vertex_capacity;
   }

   host_indices = buffers && buffers->indices;
   if (host_indices)
      MONSTRO_REQUIRE(buffers->index_capacity > 0, "monstro_begin_frame_into: index_capacity is 0");
   indices = host_indices ? buffers->indices : own_indices;
   index_capacity = host_indices ? buffers->index_capacity : own_index_capacity;

   vertex_count = 0;
   index_count = 0;
   command_count = 0;
   path_count = 0;
   geometry_hash = HASH_SEED;
   draw_data = {};

   clip_rects.clear();
   layers.clear();
   style_stack.clear();
   ids.clear();
   disabled_stack.clear();
   columns.clear();
   windows.clear();
   layouts.clear();
   groups.clear();
   tab_bars.clear();
   table.state = nullptr;
   has_tooltip = false;
}

const MonstroDrawData *Canvas::finish(uint64_t previous_hash)
{
   // Window order first, then layer within a window, then the order things were drawn in.
   std::sort(recorded, recorded + command_count,
             [](const RecordedCommand &a, const RecordedCommand &b)
             {
                if (a.owner->draw_rank != b.owner->draw_rank)
                   return a.owner->draw_rank < b.owner->draw_rank;
                if (a.command.layer != b.command.layer)
                   return a.command.layer < b.command.layer;
                return a.sequence < b.sequence;
             });

   uint64_t hash = geometry_hash;
   for (uint32_t i = 0; i < command_count; i++)
   {
      const MonstroDrawCommand &cmd = recorded[i].command;
      sorted[i] = cmd;
      for (float f : cmd.clip_rect)
         hash_float(&hash, f);
      hash_u64(&hash, cmd.texture_id);
      hash_word(&hash, cmd.vertex_offset);
      hash_word(&hash, cmd.vertex_count);
      hash_word(&hash, cmd.index_offset);
      hash_word(&hash, cmd.index_count);
      hash_word(&hash, (uint32_t)cmd.layer);
      static_assert(sizeof(cmd.custom_draw) <= sizeof(uint64_t), "a callback pointer fits the hash word");
      uint64_t callback = 0;
      memcpy(&callback, &cmd.custom_draw, sizeof(cmd.custom_draw));
      hash_u64(&hash, callback);
      hash_u64(&hash, (uint64_t)(uintptr_t)cmd.user_data);
   }

   draw_data.vertices = host_vertices ? nullptr : own_vertices;
   draw_data.vertex_count = vertex_count;
   draw_data.indices = indices;
   draw_data.index_count = index_count;
   draw_data.commands = sorted;
   draw_data.command_count = command_count;
   draw_data.content_hash = hash;
   draw_data.changed = hash != previous_hash;
   draw_data.atlas_version = env->atlas.version;
   draw_data.frame_id = frame_id;
   return &draw_data;
}

void Canvas::release()
{
   borrower = nullptr;
   owner = nullptr;
   owner_name = nullptr;
}

void Canvas::setOwner(const Window *window, const char *name)
{
   owner = window;
   owner_name = name;
}

/* ------------------------------------------------------------------------- */
/* Private Helpers                                                           */
/* ------------------------------------------------------------------------- */

Reservation Canvas::reserve(uint64_t texture_id, uint32_t vert_count, uint32_t idx_count)
{
   MonstroDrawCommand *cmd = getOrCreateDrawCommand(texture_id);

   uint32_t needed_vertices = vertex_count + vert_count;
   uint32_t needed_indices = index_count + idx_count;
   MONSTRO_REQUIRE(needed_vertices <= vertex_capacity,
                   "%s overflow: capacity %u, needed %u (window '%s', frame %d); simplify that UI or %s",
                   host_vertices ? "host vertex buffer" : "vertex buffer", vertex_capacity, needed_vertices,
                   owner_name ? owner_name : "?", frame,
                   host_vertices ? "pass larger buffers in MonstroFrameBuffers.vertex_capacity"
                                 : "raise MonstroEnvConfig.max_vertices");
   MONSTRO_REQUIRE(needed_indices <= index_capacity,
                   "%s overflow: capacity %u, needed %u (window '%s', frame %d); simplify that UI or %s",
                   host_indices ? "host index buffer" : "index buffer", index_capacity, needed_indices,
                   owner_name ? owner_name : "?", frame,
                   host_indices ? "pass a larger MonstroFrameBuffers.index_capacity"
                                : "raise MonstroEnvConfig.max_indices");

   Reservation r;
   r.position = position_writer;
   r.texcoord = texcoord_writer;
   r.color = color_writer;
   r.first_vertex = vertex_count;
   r.base_index = vertex_count - cmd->vertex_offset;
   r.indices = indices + index_count;
   r.hash = &geometry_hash;

   vertex_count = needed_vertices;
   index_count = needed_indices;
   cmd->vertex_count += vert_count;
   cmd->index_count += idx_count;
   return r;
}

Reservation Canvas::reserveSolid(uint32_t vert_count, uint32_t idx_count)
{
   return reserve(env->atlas.texture_id, vert_count, idx_count);
}

MonstroDrawCommand *Canvas::getOrCreateDrawCommand(uint64_t texture_id)
{
   // Geometry joins the last command when it continues it: same window, texture, clip and layer.
   if (command_count > 0)
   {
      RecordedCommand &last = recorded[command_count - 1];
      MonstroDrawCommand *prev = &last.command;
      MonstroRect clip = clip_rects.top();
      if (last.owner == owner && prev->custom_draw == nullptr && prev->texture_id == texture_id &&
          prev->layer == layers.top() && prev->clip_rect[0] == clip.x && prev->clip_rect[1] == clip.y &&
          prev->clip_rect[2] == clip.w && prev->clip_rect[3] == clip.h)
      {
         return prev;
      }
   }

   MonstroDrawCommand *cmd = newDrawCommand();
   cmd->texture_id = texture_id;
   return cmd;
}

MonstroDrawCommand *Canvas::newDrawCommand()
{
   MONSTRO_REQUIRE(owner, "drawing outside a window (frame %d)", frame);
   MONSTRO_REQUIRE(command_count < command_capacity,
                   "draw command list overflow: capacity %u, needed %u (window '%s', frame %d); simplify that UI or "
                   "raise MonstroEnvConfig.max_draw_commands",
                   command_capacity, command_count + 1, owner_name ? owner_name : "?", frame);
   RecordedCommand *rec = &recorded[command_count];
   memset(rec, 0, sizeof(*rec));
   rec->owner = owner;
   rec->sequence = command_count++;

   MonstroDrawCommand *cmd = &rec->command;
   MonstroRect clip = clip_rects.top();
   cmd->clip_rect[0] = clip.x;
   cmd->clip_rect[1] = clip.y;
   cmd->clip_rect[2] = clip.w;
   cmd->clip_rect[3] = clip.h;
   cmd->layer = layers.top();
   cmd->vertex_offset = vertex_count;
   cmd->index_offset = index_count;
   return cmd;
}

bool Canvas::clippedAway(float min_x, float min_y, float max_x, float max_y) const
{
   // Outside a window there is no clip rect; drawing there stops when the command is made.
   if (clip_rects.empty())
      return false;
   MonstroRect clip = clip_rects.top();
   return clip.w <= 0.0f || clip.h <= 0.0f || max_x <= clip.x || max_y <= clip.y || min_x >= clip.x + clip.w ||
          min_y >= clip.y + clip.h;
}

bool Canvas::clippedAway(const MonstroVec2 *points, int count, float margin) const
{
   float min_x = points[0].x, min_y = points[0].y, max_x = points[0].x, max_y = points[0].y;
   for (int i = 1; i < count; i++)
   {
      min_x = fminf(min_x, points[i].x);
      min_y = fminf(min_y, points[i].y);
      max_x = fmaxf(max_x, points[i].x);
      max_y = fmaxf(max_y, points[i].y);
   }
   return clippedAway(min_x - margin, min_y - margin, max_x + margin, max_y + margin);
}

void Canvas::pathPush(MonstroVec2 point)
{
   MONSTRO_REQUIRE(path_count < path_capacity,
                   "path overflow: capacity %u, needed %u (window '%s', frame %d); draw the path in pieces or raise "
                   "MonstroEnvConfig.max_path_points",
                   path_capacity, path_count + 1, owner_name ? owner_name : "?", frame);
   path[path_count++] = point;
}

void Canvas::pathArcToFast(MonstroVec2 center, float radius, int a_min_of_12, int a_max_of_12)
{
   if (a_min_of_12 > a_max_of_12)
      return;

   if (radius == 0.0f)
   {
      pathPush(center);
   }
   else
   {
      for (int a = a_min_of_12; a <= a_max_of_12; a++)
      {
         MonstroVec2 c = CIRCLE_VERTICES[a % 12];
         pathPush({center.x + c.x * radius, center.y + c.y * radius});
      }
   }
}

void Canvas::pathRect(MonstroVec2 a, MonstroVec2 b, float rounding, MonstroCornerFlags rounding_corners)
{
   float r = rounding;

   r = fminf(r, fabsf(b.x - a.x) * (((rounding_corners & MONSTRO_CORNER_BOTTOM) == MONSTRO_CORNER_BOTTOM) ||
                                            ((rounding_corners & MONSTRO_CORNER_TOP) == MONSTRO_CORNER_TOP)
                                        ? 0.5f
                                        : 1.0f) -
                    1.0f);
   r = fminf(r, fabsf(b.y - a.y) * (((rounding_corners & MONSTRO_CORNER_LEFT) == MONSTRO_CORNER_LEFT) ||
                                            ((rounding_corners & MONSTRO_CORNER_RIGHT) == MONSTRO_CORNER_RIGHT)
                                        ? 0.5f
                                        : 1.0f) -
                    1.0f);

   if (r <= 0.0f || rounding_corners == 0)
   {
      pathPush(a);
      pathPush({b.x, a.y});
      pathPush(b);
      pathPush({a.x, b.y});
   }
   else
   {
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

// The two triangles of quad 0-1-2-3, from index i.
static void quad_indices(Reservation &r, uint32_t i, uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3)
{
   r.index(i + 0, v0);
   r.index(i + 1, v1);
   r.index(i + 2, v2);
   r.index(i + 3, v0);
   r.index(i + 4, v2);
   r.index(i + 5, v3);
}

/* ------------------------------------------------------------------------- */
/* Clip Rect Stack                                                           */
/* ------------------------------------------------------------------------- */

void Canvas::pushClipRect(MonstroRect rect)
{
   MonstroRect parent = clip_rects.top();
   float x1 = (rect.x > parent.x) ? rect.x : parent.x;
   float y1 = (rect.y > parent.y) ? rect.y : parent.y;
   float x2_rect = rect.x + rect.w;
   float y2_rect = rect.y + rect.h;
   float x2_parent = parent.x + parent.w;
   float y2_parent = parent.y + parent.h;
   float x2 = (x2_rect < x2_parent) ? x2_rect : x2_parent;
   float y2 = (y2_rect < y2_parent) ? y2_rect : y2_parent;

   MonstroRect clipped;
   clipped.x = x1;
   clipped.y = y1;
   clipped.w = (x2 > x1) ? (x2 - x1) : 0.0f;
   clipped.h = (y2 > y1) ? (y2 - y1) : 0.0f;

   clip_rects.push(clipped);
}

void Canvas::pushBaseClipRect(MonstroRect rect) { clip_rects.push(rect); }

void Canvas::pushClipRectFullscreen() { pushClipRect(FULLSCREEN_CLIP); }

void Canvas::popClipRect() { clip_rects.pop(); }

MonstroRect Canvas::getClipRect() const { return clip_rects.empty() ? FULLSCREEN_CLIP : clip_rects.top(); }

/* ------------------------------------------------------------------------- */
/* Layer Stack                                                               */
/* ------------------------------------------------------------------------- */

void Canvas::pushLayer(int layer) { layers.push(layer); }

void Canvas::popLayer() { layers.pop(); }

/* ------------------------------------------------------------------------- */
/* Primitive Drawing                                                         */
/* ------------------------------------------------------------------------- */

void Canvas::addLine(MonstroVec2 p1, MonstroVec2 p2, uint32_t color, float thickness)
{
   float dx = p2.x - p1.x;
   float dy = p2.y - p1.y;
   float len = sqrtf(dx * dx + dy * dy);
   if (len < 0.0001f)
      return;
   MonstroVec2 ends[2] = {p1, p2};
   if (clippedAway(ends, 2, thickness * 0.5f))
      return;

   dx /= len;
   dy /= len;

   float nx = -dy * thickness * 0.5f;
   float ny = dx * thickness * 0.5f;

   MonstroVec2 w = env->atlas.white_uv;
   Reservation r = reserveSolid(4, 6);
   r.vertex(0, p1.x + nx, p1.y + ny, w.x, w.y, color);
   r.vertex(1, p1.x - nx, p1.y - ny, w.x, w.y, color);
   r.vertex(2, p2.x - nx, p2.y - ny, w.x, w.y, color);
   r.vertex(3, p2.x + nx, p2.y + ny, w.x, w.y, color);
   quad_indices(r, 0, 0, 1, 2, 3);
}

void Canvas::addRect(MonstroRect rect, uint32_t color, float rounding, MonstroCornerFlags corners, float thickness)
{
   MonstroVec2 a = {rect.x + 0.5f, rect.y + 0.5f};
   MonstroVec2 b = {rect.x + rect.w + 0.5f, rect.y + rect.h + 0.5f};

   path_count = 0;
   pathRect(a, b, rounding, corners);
   pathStroke(color, true, thickness);
}

void Canvas::addRectFilled(MonstroRect rect, uint32_t color, float rounding, MonstroCornerFlags corners)
{
   if (rounding > 0.0f && corners != MONSTRO_CORNER_NONE)
   {
      MonstroVec2 a = {rect.x, rect.y};
      MonstroVec2 b = {rect.x + rect.w, rect.y + rect.h};
      path_count = 0;
      pathRect(a, b, rounding, corners);
      pathFill(color);
      return;
   }
   addRectFilledMultiColor(rect, color, color, color, color);
}

void Canvas::addRectFilledMultiColor(MonstroRect rect, uint32_t col_ul, uint32_t col_ur, uint32_t col_lr,
                                     uint32_t col_ll)
{
   if (clippedAway(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h))
      return;
   MonstroVec2 w = env->atlas.white_uv;
   Reservation r = reserveSolid(4, 6);
   r.vertex(0, rect.x, rect.y, w.x, w.y, col_ul);
   r.vertex(1, rect.x + rect.w, rect.y, w.x, w.y, col_ur);
   r.vertex(2, rect.x + rect.w, rect.y + rect.h, w.x, w.y, col_lr);
   r.vertex(3, rect.x, rect.y + rect.h, w.x, w.y, col_ll);
   quad_indices(r, 0, 0, 1, 2, 3);
}

void Canvas::addTriangleFilled(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color)
{
   MonstroVec2 corners[3] = {p1, p2, p3};
   if (clippedAway(corners, 3, 0.0f))
      return;
   MonstroVec2 w = env->atlas.white_uv;
   Reservation r = reserveSolid(3, 3);
   r.vertex(0, p1.x, p1.y, w.x, w.y, color);
   r.vertex(1, p2.x, p2.y, w.x, w.y, color);
   r.vertex(2, p3.x, p3.y, w.x, w.y, color);
   r.index(0, 0);
   r.index(1, 1);
   r.index(2, 2);
}

void Canvas::addTriangle(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness)
{
   addLine(p1, p2, color, thickness);
   addLine(p2, p3, color, thickness);
   addLine(p3, p1, color, thickness);
}

void Canvas::addCircleFilled(MonstroVec2 center, float radius, uint32_t color, int num_segments)
{
   if (num_segments < 3)
      return;
   if (clippedAway(center.x - radius, center.y - radius, center.x + radius, center.y + radius))
      return;

   MonstroVec2 w = env->atlas.white_uv;
   Reservation r = reserveSolid((uint32_t)(num_segments + 1), (uint32_t)(num_segments * 3));
   r.vertex(0, center.x, center.y, w.x, w.y, color);
   for (int i = 0; i < num_segments; i++)
   {
      float angle = (float)i / (float)num_segments * 6.2831853f;
      r.vertex(i + 1, center.x + cosf(angle) * radius, center.y + sinf(angle) * radius, w.x, w.y, color);
   }
   for (int i = 0; i < num_segments; i++)
   {
      r.index(i * 3, 0);
      r.index(i * 3 + 1, 1 + i);
      r.index(i * 3 + 2, 1 + (i + 1) % num_segments);
   }
}

void Canvas::addCircle(MonstroVec2 center, float radius, uint32_t color, int num_segments, float thickness)
{
   if (num_segments < 3)
      return;
   float reach = radius + thickness * 0.5f;
   if (clippedAway(center.x - reach, center.y - reach, center.x + reach, center.y + reach))
      return;

   for (int i = 0; i < num_segments; i++)
   {
      float a1 = (float)i / (float)num_segments * 6.2831853f;
      float a2 = (float)(i + 1) / (float)num_segments * 6.2831853f;
      MonstroVec2 p1 = {center.x + cosf(a1) * radius, center.y + sinf(a1) * radius};
      MonstroVec2 p2 = {center.x + cosf(a2) * radius, center.y + sinf(a2) * radius};
      addLine(p1, p2, color, thickness);
   }
}

void Canvas::addQuad(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color, float thickness)
{
   addLine(p1, p2, color, thickness);
   addLine(p2, p3, color, thickness);
   addLine(p3, p4, color, thickness);
   addLine(p4, p1, color, thickness);
}

void Canvas::addQuadFilled(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color)
{
   MonstroVec2 corners[4] = {p1, p2, p3, p4};
   if (clippedAway(corners, 4, 0.0f))
      return;
   MonstroVec2 w = env->atlas.white_uv;
   Reservation r = reserveSolid(4, 6);
   r.vertex(0, p1.x, p1.y, w.x, w.y, color);
   r.vertex(1, p2.x, p2.y, w.x, w.y, color);
   r.vertex(2, p3.x, p3.y, w.x, w.y, color);
   r.vertex(3, p4.x, p4.y, w.x, w.y, color);
   quad_indices(r, 0, 0, 1, 2, 3);
}

void Canvas::pathEllipse(MonstroVec2 center, MonstroVec2 radii, float rotation, int num_segments)
{
   float c = cosf(rotation), s = sinf(rotation);
   for (int i = 0; i < num_segments; i++)
   {
      float angle = (float)i / (float)num_segments * 6.2831853f;
      float x = cosf(angle) * radii.x, y = sinf(angle) * radii.y;
      pathPush({center.x + x * c - y * s, center.y + x * s + y * c});
   }
}

void Canvas::addEllipse(MonstroVec2 center, MonstroVec2 radii, float rotation, uint32_t color, int num_segments,
                        float thickness)
{
   float reach = fmaxf(radii.x, radii.y) + thickness * 0.5f;
   if (clippedAway(center.x - reach, center.y - reach, center.x + reach, center.y + reach))
      return;
   path_count = 0;
   pathEllipse(center, radii, rotation, num_segments);
   pathStroke(color, true, thickness);
}

void Canvas::addEllipseFilled(MonstroVec2 center, MonstroVec2 radii, float rotation, uint32_t color, int num_segments)
{
   float reach = fmaxf(radii.x, radii.y);
   if (clippedAway(center.x - reach, center.y - reach, center.x + reach, center.y + reach))
      return;
   path_count = 0;
   pathEllipse(center, radii, rotation, num_segments);
   pathFill(color);
}

/* ------------------------------------------------------------------------- */
/* Complex Shapes                                                            */
/* ------------------------------------------------------------------------- */

void Canvas::addPolyline(const MonstroVec2 *points, int num_points, uint32_t color, bool closed, float thickness)
{
   if (num_points < 2)
      return;

   int segments = closed ? num_points : num_points - 1;
   for (int i = 0; i < segments; i++)
   {
      int j = (i + 1) % num_points;
      addLine(points[i], points[j], color, thickness);
   }
}

void Canvas::addConvexPolyFilled(const MonstroVec2 *points, int num_points, uint32_t color)
{
   if (num_points < 3)
      return;
   if (clippedAway(points, num_points, 0.0f))
      return;

   MonstroVec2 w = env->atlas.white_uv;
   uint32_t idx_count = (uint32_t)((num_points - 2) * 3);
   Reservation r = reserveSolid((uint32_t)num_points, idx_count);
   for (int i = 0; i < num_points; i++)
      r.vertex(i, points[i].x, points[i].y, w.x, w.y, color);

   uint32_t ii = 0;
   for (int i = 1; i < num_points - 1; i++)
   {
      r.index(ii++, 0);
      r.index(ii++, i);
      r.index(ii++, i + 1);
   }
}

void Canvas::addBezierCubic(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, uint32_t color,
                            float thickness, int num_segments)
{
   if (num_segments < 1)
      return;

   MonstroVec2 prev = p1;
   for (int i = 1; i <= num_segments; i++)
   {
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

void Canvas::addBezierQuadratic(MonstroVec2 p1, MonstroVec2 p2, MonstroVec2 p3, uint32_t color, float thickness,
                                int num_segments)
{
   if (num_segments < 1)
      return;

   MonstroVec2 prev = p1;
   for (int i = 1; i <= num_segments; i++)
   {
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

// Text with embedded {icon:ID} tags; it wraps at wrap_width when that is positive.
void Canvas::addTextRun(MonstroFont *font, MonstroVec2 pos, uint32_t color, const char *text, float wrap_width)
{
   uint64_t atlas_texture = env->atlas.texture_id;
   float pen_x = pos.x;
   float line_y = 0;
   float line_height = monstro_font_get_line_height(font);
   float ascent = monstro_font_get_ascent(font);
   bool wrap = wrap_width > 0.0f;

   const char *s = text;
   while (*s)
   {
      uint32_t icon_id;
      if (parse_icon_tag(&s, &icon_id))
      {
         const IconEntry *icon = icon_id ? env->icons.find(icon_id) : nullptr;
         if (!icon)
            continue;
         float icon_sz = line_height;
         if (wrap && pen_x - pos.x + icon_sz > wrap_width && pen_x > pos.x)
         {
            pen_x = pos.x;
            line_y += line_height;
         }
         MonstroRect icon_rect = {pen_x, pos.y + line_y, icon_sz, icon_sz};
         addImage(icon->in_atlas ? atlas_texture : icon->texture_id, icon_rect, icon->uv_min, icon->uv_max,
                  icon->is_monochrome ? color : 0xFFFFFFFF);
         pen_x += icon_sz;
         continue;
      }

      uint32_t codepoint = decode_utf8(&s);
      if (codepoint == '\n')
      {
         pen_x = pos.x;
         line_y += line_height;
         continue;
      }
      if (codepoint == '\r')
         continue;

      MonstroGlyph glyph;
      if (!monstro_font_get_glyph(font, codepoint, &glyph))
         continue;

      if (wrap && pen_x - pos.x + glyph.advance[0] > wrap_width && pen_x > pos.x)
      {
         pen_x = pos.x;
         line_y += line_height;
      }

      if (glyph.size[0] > 0 && glyph.size[1] > 0)
      {
         MonstroRect rect = {pen_x + glyph.offset[0], pos.y + line_y + glyph.offset[1] + ascent, glyph.size[0],
                             glyph.size[1]};
         addImage(atlas_texture, rect, {glyph.min_tex_coord[0], glyph.min_tex_coord[1]},
                  {glyph.max_tex_coord[0], glyph.max_tex_coord[1]}, color);
      }
      pen_x += glyph.advance[0];
   }
}

void Canvas::addText(MonstroFont *font, MonstroVec2 pos, uint32_t color, const char *text)
{
   if (!font || !text)
      return;
   addTextRun(font, pos, color, text, 0.0f);
}

void Canvas::addTextWrapped(MonstroFont *font, MonstroVec2 pos, uint32_t color, const char *text, float wrap_width)
{
   if (!font || !text || wrap_width <= 0.0f)
      return;
   addTextRun(font, pos, color, text, wrap_width);
}

void Canvas::addTextEx(MonstroFont *font, MonstroRect rect, uint32_t color, const char *text, MonstroAlign align)
{
   if (!font || !text)
      return;

   MonstroVec2 text_size = monstro_font_measure_text(font, text, 0.0f);

   float x = rect.x;
   float y = rect.y;

   // Centring sets both of an axis's flags; one flag alone keeps text at that edge.
   uint32_t horizontal = align & MONSTRO_ALIGN_HCENTER;
   if (horizontal == MONSTRO_ALIGN_HCENTER)
      x = rect.x + (rect.w - text_size.x) * 0.5f;
   else if (horizontal == MONSTRO_ALIGN_RIGHT)
      x = rect.x + rect.w - text_size.x;

   uint32_t vertical = align & MONSTRO_ALIGN_VCENTER;
   if (vertical == MONSTRO_ALIGN_VCENTER)
      y = rect.y + (rect.h - text_size.y) * 0.5f;
   else if (vertical == MONSTRO_ALIGN_BOTTOM)
      y = rect.y + rect.h - text_size.y;

   addText(font, {x, y}, color, text);
}

/* ------------------------------------------------------------------------- */
/* Image Drawing                                                             */
/* ------------------------------------------------------------------------- */

void Canvas::addImage(uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max,
                      uint32_t tint_color)
{
   if (clippedAway(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h))
      return;
   Reservation r = reserve(texture_id, 4, 6);
   r.vertex(0, rect.x, rect.y, uv_min.x, uv_min.y, tint_color);
   r.vertex(1, rect.x + rect.w, rect.y, uv_max.x, uv_min.y, tint_color);
   r.vertex(2, rect.x + rect.w, rect.y + rect.h, uv_max.x, uv_max.y, tint_color);
   r.vertex(3, rect.x, rect.y + rect.h, uv_min.x, uv_max.y, tint_color);
   quad_indices(r, 0, 0, 1, 2, 3);
}

void Canvas::addImageRounded(uint64_t texture_id, MonstroRect rect, MonstroVec2 uv_min, MonstroVec2 uv_max,
                             uint32_t tint_color, float rounding, MonstroCornerFlags corners)
{
   if (rounding <= 0.0f || corners == MONSTRO_CORNER_NONE)
   {
      addImage(texture_id, rect, uv_min, uv_max, tint_color);
      return;
   }

   if (clippedAway(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h))
      return;
   MonstroVec2 a = {rect.x, rect.y};
   MonstroVec2 b = {rect.x + rect.w, rect.y + rect.h};

   path_count = 0;
   pathRect(a, b, rounding, corners);
   if (path_count < 3)
      return;

   int num_points = (int)path_count;
   uint32_t idx_count = (uint32_t)((num_points - 2) * 3);
   Reservation r = reserve(texture_id, (uint32_t)num_points, idx_count);

   float inv_w = (rect.w > 0.0f) ? 1.0f / rect.w : 0.0f;
   float inv_h = (rect.h > 0.0f) ? 1.0f / rect.h : 0.0f;
   for (int i = 0; i < num_points; i++)
   {
      float t_x = (path[i].x - rect.x) * inv_w;
      float t_y = (path[i].y - rect.y) * inv_h;
      float u = uv_min.x + (uv_max.x - uv_min.x) * t_x;
      float v = uv_min.y + (uv_max.y - uv_min.y) * t_y;
      r.vertex(i, path[i].x, path[i].y, u, v, tint_color);
   }

   uint32_t ii = 0;
   for (int i = 1; i < num_points - 1; i++)
   {
      r.index(ii++, 0);
      r.index(ii++, i);
      r.index(ii++, i + 1);
   }
   path_count = 0;
}

void Canvas::addNinePatch(const MonstroNinePatch *patch, MonstroRect rect, uint32_t tint_color)
{
   if (!patch)
      return;
   if (clippedAway(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h))
      return;

   Reservation r = reserve(patch->texture_id, 16, 54);

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

   float uv_xp[4] = {patch->uv_min.x, patch->uv_min.x + patch->border_left * scale_x,
                     patch->uv_max.x - patch->border_right * scale_x, patch->uv_max.x};
   float uv_yp[4] = {patch->uv_min.y, patch->uv_min.y + patch->border_top * scale_y,
                     patch->uv_max.y - patch->border_bottom * scale_y, patch->uv_max.y};

   uint32_t vi = 0;
   for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
         r.vertex(vi++, xp[j], yp[i], uv_xp[j], uv_yp[i], tint_color);

   // Nine quads over the 4 x 4 grid of vertices, row by row.
   uint32_t ii = 0;
   for (uint32_t row = 0; row < 3; row++)
   {
      for (uint32_t col = 0; col < 3; col++)
      {
         uint32_t v0 = row * 4 + col;
         quad_indices(r, ii, v0, v0 + 4, v0 + 5, v0 + 1);
         ii += 6;
      }
   }
}

/* ------------------------------------------------------------------------- */
/* Custom Draw                                                               */
/* ------------------------------------------------------------------------- */

void Canvas::addCustomDraw(MonstroCustomDrawCallback callback, void *user_data)
{
   if (!callback)
      return;

   MonstroDrawCommand *cmd = newDrawCommand();
   cmd->custom_draw = callback;
   cmd->user_data = user_data;
}

/* ------------------------------------------------------------------------- */
/* Path API                                                                  */
/* ------------------------------------------------------------------------- */

void Canvas::pathClear() { path_count = 0; }

void Canvas::pathMoveTo(MonstroVec2 pos)
{
   path_count = 0;
   pathPush(pos);
}

void Canvas::pathLineTo(MonstroVec2 pos) { pathPush(pos); }

void Canvas::pathArcTo(MonstroVec2 center, float radius, float a_min, float a_max, int num_segments)
{
   if (num_segments < 1)
      return;

   for (int i = 0; i <= num_segments; i++)
   {
      float t = (float)i / (float)num_segments;
      float angle = a_min + (a_max - a_min) * t;
      pathPush({center.x + cosf(angle) * radius, center.y + sinf(angle) * radius});
   }
}

void Canvas::pathBezierCubicTo(MonstroVec2 p2, MonstroVec2 p3, MonstroVec2 p4, int num_segments)
{
   if (path_count < 1 || num_segments < 1)
      return;

   MonstroVec2 p1 = path[path_count - 1];
   for (int i = 1; i <= num_segments; i++)
   {
      float t = (float)i / (float)num_segments;
      float u = 1.0f - t;
      MonstroVec2 pt;
      pt.x = u * u * u * p1.x + 3 * u * u * t * p2.x + 3 * u * t * t * p3.x + t * t * t * p4.x;
      pt.y = u * u * u * p1.y + 3 * u * u * t * p2.y + 3 * u * t * t * p3.y + t * t * t * p4.y;
      pathPush(pt);
   }
}

void Canvas::pathBezierQuadraticTo(MonstroVec2 p2, MonstroVec2 p3, int num_segments)
{
   if (path_count < 1 || num_segments < 1)
      return;

   MonstroVec2 p1 = path[path_count - 1];
   for (int i = 1; i <= num_segments; i++)
   {
      float t = (float)i / (float)num_segments;
      float u = 1.0f - t;
      MonstroVec2 pt;
      pt.x = u * u * p1.x + 2 * u * t * p2.x + t * t * p3.x;
      pt.y = u * u * p1.y + 2 * u * t * p2.y + t * t * p3.y;
      pathPush(pt);
   }
}

void Canvas::pathFill(uint32_t color)
{
   if (path_count < 3)
      return;
   addConvexPolyFilled(path, (int)path_count, color);
   path_count = 0;
}

void Canvas::pathStroke(uint32_t color, bool closed, float thickness)
{
   if (path_count < 2)
      return;
   addPolyline(path, (int)path_count, color, closed, thickness);
   path_count = 0;
}

} // namespace monstro
