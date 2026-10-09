#ifndef MONSTRO_INTERNAL_DRAWBUFFER_H
#define MONSTRO_INTERNAL_DRAWBUFFER_H

#include "constants.h"
#include "fixed.h"
#include "monstro.h"

namespace monstro
{

struct Reservation
{
   MonstroVertex *vertices;
   uint32_t *indices;
   uint32_t baseVertex;
};

// Per-frame vertex and index storage. The context's own buffers are sized by
// MonstroContextConfig; buffers set by the caller keep the caller's capacity.
struct DrawBuffer
{
   CapacityBuffer<MonstroVertex> myVertices{"vertex buffer"};
   CapacityBuffer<uint32_t> myIndices{"index buffer"};
   bool myUserProvidedVertexBuffer = false;
   bool myUserProvidedIndexBuffer = false;
   int myFrame = 0;

   MonstroVertex *myOwnVertices = nullptr;
   uint32_t *myOwnIndices = nullptr;
   uint32_t myOwnVertexCapacity = 0;
   uint32_t myOwnIndexCapacity = 0;

   DrawBuffer() = default;
   ~DrawBuffer();

   DrawBuffer(const DrawBuffer &) = delete;
   DrawBuffer &operator=(const DrawBuffer &) = delete;

   bool init(uint32_t max_vertices, uint32_t max_indices);

   void beginFrame(int frame);

   // The only place draw geometry is allocated, and so the only place it can overflow.
   Reservation reserve(uint32_t vert_count, uint32_t idx_count, const char *owner);

   void setVertexBuffer(MonstroVertex *buffer, size_t capacity_in_vertices);
   void setIndexBuffer(uint32_t *buffer, size_t capacity_in_indices);

   void getVertexBuffer(const MonstroVertex **out_data, size_t *out_byte_size) const;
   void getIndexBuffer(const uint32_t **out_data, uint32_t *out_count) const;
};

} // namespace monstro

typedef monstro::DrawBuffer MonstroDrawBuffer;

#endif
