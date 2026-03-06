#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

// Monstro UI Fragment Shader
// Samples texture and multiplies by vertex color

// Bindless texture/sampler arrays (VulkEase standard)
layout(set = 0, binding = 0) uniform texture2D ve_textures[];
layout(set = 1, binding = 0) uniform sampler ve_samplers[];

// Input from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

// Output color
layout(location = 0) out vec4 outColor;

// Texture type constants
const uint UI_TEXTURE_RGBA = 0;
const uint UI_TEXTURE_FONT_ATLAS = 1;

// Push constants (must match vertex shader layout)
layout(push_constant) uniform PushConstants {
    uint64_t vertexBufferAddress;
    uint64_t indexBufferAddress;
    vec2 screenSize;
    uint textureIndex;
    uint samplerIndex;
    uint indexOffset;
    uint textureType;
} pc;

void main() {
    if (pc.textureType == UI_TEXTURE_FONT_ATLAS) {
        // Font atlas (R8): vertex color provides RGB, red channel is alpha/coverage
        vec4 texColor = texture(
            sampler2D(ve_textures[nonuniformEXT(pc.textureIndex)], 
                      ve_samplers[nonuniformEXT(pc.samplerIndex)]), 
            fragTexCoord);
        outColor = vec4(fragColor.rgb, fragColor.a * texColor.r);
    } else {
        // RGBA texture: for solid color UI elements (UV=0,0), just use vertex color
        // This avoids issues with the white texture not being set up correctly
        if (fragTexCoord.x == 0.0 && fragTexCoord.y == 0.0) {
            outColor = fragColor;
        } else {
            vec4 texColor = texture(
                sampler2D(ve_textures[nonuniformEXT(pc.textureIndex)], 
                          ve_samplers[nonuniformEXT(pc.samplerIndex)]), 
                fragTexCoord);
            outColor = texColor * fragColor;
        }
    }
}
