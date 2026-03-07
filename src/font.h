#ifndef MONSTRO_INTERNAL_FONT_H
#define MONSTRO_INTERNAL_FONT_H

#include "monstro.h"
#include "constants.h"

struct MonstroFont {
    char name[64] = {};
    float size = 0.0f;
    float line_height = 0.0f;
    float ascent = 0.0f;
    float descent = 0.0f;

    MonstroGlyph* glyphs = nullptr;
    uint32_t glyph_count = 0;
    uint32_t first_code_point = 0;

    uint64_t atlas_texture_id = 0;
    uint32_t atlas_width = 0;
    uint32_t atlas_height = 0;

    uint8_t* atlas_data = nullptr;
    size_t atlas_data_size = 0;
    MonstroTextureFormat atlas_format = MONSTRO_TEXTURE_FORMAT_R8;
};

struct MonstroFontManager {
    MonstroFont* fonts[MONSTRO_MAX_FONTS] = {};
    int font_count = 0;
};

#endif
