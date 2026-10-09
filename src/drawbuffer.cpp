#include "drawbuffer.h"
#include <cstdlib>

namespace monstro
{

DrawBuffer::~DrawBuffer()
{
   std::free(myOwnVertices);
   std::free(myOwnIndices);
}

bool DrawBuffer::init(uint32_t max_vertices, uint32_t max_indices)
{
   myOwnVertices = (MonstroVertex *)std::malloc(sizeof(MonstroVertex) * max_vertices);
   myOwnIndices = (uint32_t *)std::malloc(sizeof(uint32_t) * max_indices);
   if (!myOwnVertices || !myOwnIndices)
      return false;

   myOwnVertexCapacity = max_vertices;
   myOwnIndexCapacity = max_indices;
   setVertexBuffer(nullptr, 0);
   setIndexBuffer(nullptr, 0);
   return true;
}

void DrawBuffer::beginFrame(int frame)
{
   myFrame = frame;
   myVertices.myCount = 0;
   myIndices.myCount = 0;
}

Reservation DrawBuffer::reserve(uint32_t vert_count, uint32_t idx_count, const char *owner)
{
   Reservation r;
   r.baseVertex = myVertices.myCount;
   r.vertices =
       myVertices.reserve(vert_count, owner, myFrame,
                          myUserProvidedVertexBuffer ? "pass a larger buffer to monstro_draw_buffer_set_vertex_buffer"
                                                     : "raise MonstroContextConfig.max_vertices");
   r.indices =
       myIndices.reserve(idx_count, owner, myFrame,
                         myUserProvidedIndexBuffer ? "pass a larger buffer to monstro_draw_buffer_set_index_buffer"
                                                   : "raise MonstroContextConfig.max_indices");
   return r;
}

void DrawBuffer::setVertexBuffer(MonstroVertex *buffer, size_t capacity_in_vertices)
{
   // A null buffer hands storage back to the context's own buffer.
   myUserProvidedVertexBuffer = (buffer != nullptr);
   myVertices.myData = buffer ? buffer : myOwnVertices;
   myVertices.myCapacity = buffer ? capacity_in_vertices : myOwnVertexCapacity;
}

void DrawBuffer::setIndexBuffer(uint32_t *buffer, size_t capacity_in_indices)
{
   myUserProvidedIndexBuffer = (buffer != nullptr);
   myIndices.myData = buffer ? buffer : myOwnIndices;
   myIndices.myCapacity = buffer ? capacity_in_indices : myOwnIndexCapacity;
}

void DrawBuffer::getVertexBuffer(const MonstroVertex **out_data, size_t *out_byte_size) const
{
   if (out_data)
      *out_data = myVertices.myData;
   if (out_byte_size)
      *out_byte_size = myVertices.myCount * sizeof(MonstroVertex);
}

void DrawBuffer::getIndexBuffer(const uint32_t **out_data, uint32_t *out_count) const
{
   if (out_data)
      *out_data = myIndices.myData;
   if (out_count)
      *out_count = myIndices.myCount;
}

} // namespace monstro
