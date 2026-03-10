#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

// Monstro UI Vertex Shader
// Uses VulkEase bindless/buffer device address pattern

// Vertex structure matching MonstroVertex
struct Vertex {
    vec2 position;
    vec2 texcoord;
    uint color;  // RGBA packed as uint32
};

// Buffer reference for bindless vertex access
layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex vertices[];
};

// Buffer reference for index buffer access
layout(buffer_reference, scalar) readonly buffer IndexBuffer {
    uint indices[];
};

// Outputs to fragment shader
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

// Push constants
layout(push_constant) uniform PushConstants {
    uint64_t vertexBufferAddress;
    uint64_t indexBufferAddress;
    vec2 screenSize;        // For converting pixel coords to NDC
    uint textureIndex;
    uint samplerIndex;
    uint indexOffset;       // Offset into index buffer for this draw
    uint textureType;       // 0=RGBA, 1=Font atlas (R8), 2=none, use vertex color.
} pc;

// Unpack RGBA from uint32 (ABGR format - Vulkan standard)
vec4 unpackColor(uint packed) {
    float r = float((packed >> 0) & 0xFF) / 255.0;
    float g = float((packed >> 8) & 0xFF) / 255.0;
    float b = float((packed >> 16) & 0xFF) / 255.0;
    float a = float((packed >> 24) & 0xFF) / 255.0;
    return vec4(r, g, b, a);
}

void main() {
    // Get buffer references
    VertexBuffer vertexBuffer = VertexBuffer(pc.vertexBufferAddress);
    IndexBuffer indexBuffer = IndexBuffer(pc.indexBufferAddress);
    
    // Fetch index and then vertex
    uint index = indexBuffer.indices[pc.indexOffset + gl_VertexIndex];
    Vertex vertex = vertexBuffer.vertices[index];
    
    // Convert pixel coordinates to NDC [-1, 1]
    // UI uses top-left origin (Y increases downward)
    // Vulkan NDC: X [-1,1] left to right, Y [-1,1] top to bottom (with VK_KHR_maintenance1)
    vec2 pos = vertex.position;
    pos.x = (pos.x / pc.screenSize.x) * 2.0 - 1.0;  // [0, width] -> [-1, 1]
    pos.y = (pos.y / pc.screenSize.y) * 2.0 - 1.0;  // [0, height] -> [-1, 1] (top-left origin)
    
    gl_Position = vec4(pos, 0.0, 1.0);
    fragTexCoord = vertex.texcoord;
    fragColor = unpackColor(vertex.color);
}
