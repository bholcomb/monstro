#ifndef MONSTRO_FONT_H
#define MONSTRO_FONT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MonstroContext MonstroContext;
typedef struct MonstroFontManager MonstroFontManager;
typedef struct MonstroFont MonstroFont;

typedef enum MonstroTextureFormat {
    MONSTRO_TEXTURE_FORMAT_R8 = 0,
    MONSTRO_TEXTURE_FORMAT_RGBA8 = 1
} MonstroTextureFormat;

typedef struct MonstroGlyph {
    float min_tex_coord[2];
    float max_tex_coord[2];
    float size[2];
    float offset[2];
    float advance[2];
} MonstroGlyph;

typedef struct MonstroTextureData {
    uint32_t width;
    uint32_t height;
    MonstroTextureFormat format;
    size_t data_size;
    const void* data;
} MonstroTextureData;

MonstroFontManager* monstro_font_manager_create(void);
void monstro_font_manager_destroy(MonstroFontManager* manager);

MonstroFontManager* monstro_context_get_font_manager(MonstroContext* ctx);

MonstroFont* monstro_font_load_ttf(
    MonstroFontManager* manager,
    const void* ttf_data,
    size_t ttf_data_size,
    float pixel_size,
    uint32_t first_code_point,
    uint32_t code_point_count,
    MonstroTextureData* out_atlas_data
);

MonstroFont* monstro_font_load_ttf_from_file(
    MonstroFontManager* manager,
    const char* path,
    float pixel_size,
    uint32_t first_code_point,
    uint32_t code_point_count,
    MonstroTextureData* out_atlas_data
);

MonstroFont* monstro_font_load_bitmap(
    MonstroFontManager* manager,
    const void* image_data,
    uint32_t image_width,
    uint32_t image_height,
    uint32_t grid_cols,
    uint32_t grid_rows,
    uint32_t cell_width,
    uint32_t cell_height,
    uint32_t first_code_point,
    float font_size_px,
    MonstroTextureData* out_atlas_data
);

MonstroFont* monstro_font_load_bitmap_from_file(
    MonstroFontManager* manager,
    const char* path,
    uint32_t grid_cols,
    uint32_t grid_rows,
    uint32_t first_code_point,
    float font_size_px,
    MonstroTextureData* out_atlas_data
);

void monstro_font_destroy(MonstroFont* font);

void monstro_font_set_atlas_texture(MonstroFont* font, uint64_t texture_id);
uint64_t monstro_font_get_atlas_texture(MonstroFont* font);

void monstro_font_add(MonstroFontManager* manager, MonstroFont* font, const char* name, float size);
MonstroFont* monstro_font_find(MonstroFontManager* manager, const char* name, float size);

MonstroVec2 monstro_font_measure_text(MonstroFont* font, const char* text_utf8, float wrap_width);
MonstroVec2 monstro_font_measure_text_wrapped(MonstroFont* font, const char* text_utf8, float wrap_width);

bool monstro_font_get_glyph(MonstroFont* font, uint32_t code_point, MonstroGlyph* out_glyph);
uint32_t monstro_font_get_glyph_count(MonstroFont* font);

float monstro_font_get_size(MonstroFont* font);
float monstro_font_get_line_height(MonstroFont* font);
float monstro_font_get_ascent(MonstroFont* font);
float monstro_font_get_descent(MonstroFont* font);

void monstro_font_get_atlas_size(MonstroFont* font, uint32_t* out_width, uint32_t* out_height);

#ifdef __cplusplus
}
#endif

#endif
