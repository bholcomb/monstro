#include "drawbuffer.h"
#include <stdexcept>

namespace monstro {

DrawBuffer::DrawBuffer() {
    myVertexCapacity = MONSTRO_MAX_VERTICES;
    myIndexCapacity = MONSTRO_MAX_INDICES;
    myVertices = new (std::nothrow) MonstroVertex[myVertexCapacity];
    myIndices = new (std::nothrow) uint32_t[myIndexCapacity];

    if (!myVertices || !myIndices) {
        delete[] myVertices;
        delete[] myIndices;
        myVertices = nullptr;
        myIndices = nullptr;
        myVertexCapacity = 0;
        myIndexCapacity = 0;
    }
}

DrawBuffer::~DrawBuffer() {
    if (!myUserProvidedVertexBuffer) {
        delete[] myVertices;
    }
    if (!myUserProvidedIndexBuffer) {
        delete[] myIndices;
    }
}

void DrawBuffer::beginFrame() {
    myVertexCount = 0;
    myIndexCount = 0;
}

Reservation DrawBuffer::reserve(uint32_t vert_count, uint32_t idx_count) {
    if (myVertexCount + vert_count > myVertexCapacity) {
        throw std::runtime_error("DrawBuffer vertex buffer overflow");
    }
    if (myIndexCount + idx_count > myIndexCapacity) {
        throw std::runtime_error("DrawBuffer index buffer overflow");
    }

    Reservation r;
    r.vertices = {myVertices + myVertexCount, vert_count};
    r.indices = {myIndices + myIndexCount, idx_count};
    r.baseVertex = myVertexCount;
    myVertexCount += vert_count;
    myIndexCount += idx_count;
    return r;
}

void DrawBuffer::setVertexBuffer(MonstroVertex* buffer, size_t capacity_in_vertices) {
    if (!myUserProvidedVertexBuffer && myVertices) {
        delete[] myVertices;
    }
    myVertices = buffer;
    myVertexCapacity = capacity_in_vertices;
    myUserProvidedVertexBuffer = (buffer != nullptr);
}

void DrawBuffer::setIndexBuffer(uint32_t* buffer, size_t capacity_in_indices) {
    if (!myUserProvidedIndexBuffer && myIndices) {
        delete[] myIndices;
    }
    myIndices = buffer;
    myIndexCapacity = capacity_in_indices;
    myUserProvidedIndexBuffer = (buffer != nullptr);
}

void DrawBuffer::getVertexBuffer(const MonstroVertex** out_data, size_t* out_byte_size) const {
    if (out_data) *out_data = myVertices;
    if (out_byte_size) *out_byte_size = myVertexCount * sizeof(MonstroVertex);
}

void DrawBuffer::getIndexBuffer(const uint32_t** out_data, uint32_t* out_count) const {
    if (out_data) *out_data = myIndices;
    if (out_count) *out_count = myIndexCount;
}

} // namespace monstro
