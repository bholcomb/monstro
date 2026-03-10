#ifndef MONSTRO_INTERNAL_DRAWBUFFER_H
#define MONSTRO_INTERNAL_DRAWBUFFER_H

#include "constants.h"
#include "monstro.h"
#include <stdexcept>

namespace monstro
{

template <typename T> struct WriteSpan
{
   T *myData;
   uint32_t mySize;

   T &operator[](uint32_t i)
   {
      if (i >= mySize)
      {
         throw std::out_of_range("WriteSpan index out of bounds");
      }
      return myData[i];
   }

   const T &operator[](uint32_t i) const
   {
      if (i >= mySize)
      {
         throw std::out_of_range("WriteSpan index out of bounds");
      }
      return myData[i];
   }
};

struct Reservation
{
   WriteSpan<MonstroVertex> vertices;
   WriteSpan<uint32_t> indices;
   uint32_t baseVertex;
};

struct DrawBuffer
{
   MonstroVertex *myVertices = nullptr;
   uint32_t *myIndices = nullptr;
   uint32_t myVertexCount = 0;
   uint32_t myIndexCount = 0;
   size_t myVertexCapacity = 0;
   size_t myIndexCapacity = 0;
   bool myUserProvidedVertexBuffer = false;
   bool myUserProvidedIndexBuffer = false;

   DrawBuffer();
   ~DrawBuffer();

   DrawBuffer(const DrawBuffer &) = delete;
   DrawBuffer &operator=(const DrawBuffer &) = delete;

   void beginFrame();

   Reservation reserve(uint32_t vert_count, uint32_t idx_count);

   void setVertexBuffer(MonstroVertex *buffer, size_t capacity_in_vertices);
   void setIndexBuffer(uint32_t *buffer, size_t capacity_in_indices);

   void getVertexBuffer(const MonstroVertex **out_data, size_t *out_byte_size) const;
   void getIndexBuffer(const uint32_t **out_data, uint32_t *out_count) const;
};

} // namespace monstro

typedef monstro::DrawBuffer MonstroDrawBuffer;

#endif
