#ifndef MONSTRO_FONT_H
#define MONSTRO_FONT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fonts belong to the environment: each is loaded once, packed into the
 * environment's one RGBA8 atlas, and shared by every context. They live until
 * monstro_shutdown. Loading returns NULL when the data or file cannot be read
 * as a font; running out of atlas space or font slots stops the process.
 */
MonstroFont* monstro_font_load_ttf(
    MonstroEnv* env,
    const char* name,
    const void* ttf_data,
    size_t ttf_data_size,
    float pixel_size,
    uint32_t first_code_point,
    uint32_t code_point_count
);

MonstroFont* monstro_font_load_ttf_from_file(
    MonstroEnv* env,
    const char* name,
    const char* path,
    float pixel_size,
    uint32_t first_code_point,
    uint32_t code_point_count
);

/* A grid of glyph cells in an RGBA8 image (image_width * image_height * 4 bytes). */
MonstroFont* monstro_font_load_bitmap(
    MonstroEnv* env,
    const char* name,
    const void* image_data,
    uint32_t image_width,
    uint32_t image_height,
    uint32_t grid_cols,
    uint32_t grid_rows,
    uint32_t first_code_point,
    float font_size_px
);

MonstroFont* monstro_font_load_bitmap_from_file(
    MonstroEnv* env,
    const char* name,
    const char* path,
    uint32_t grid_cols,
    uint32_t grid_rows,
    uint32_t first_code_point,
    float font_size_px
);

/* The font loaded with this name and, when size > 0, this size; NULL if none. */
MonstroFont* monstro_font_find(MonstroEnv* env, const char* name, float size);

/* The font themes without a font of their own use: the first font loaded, until set. */
void monstro_env_set_default_font(MonstroEnv* env, MonstroFont* font);
MonstroFont* monstro_env_get_default_font(MonstroEnv* env);

/*
 * The atlas holds every font's glyphs and a white texel that solid shapes
 * sample, so text and fills share one texture. Draw commands for both carry
 * the atlas texture ID: 0 until the host sets its own. Upload the pixels again
 * whenever the version changes (each font load changes it).
 */
MonstroTextureData monstro_env_get_atlas(MonstroEnv* env);
void monstro_env_set_atlas_texture(MonstroEnv* env, uint64_t texture_id);
uint64_t monstro_env_get_atlas_texture(MonstroEnv* env);

MonstroVec2 monstro_font_measure_text(MonstroFont* font, const char* text_utf8, float wrap_width);
MonstroVec2 monstro_font_measure_text_wrapped(MonstroFont* font, const char* text_utf8, float wrap_width);

bool monstro_font_get_glyph(MonstroFont* font, uint32_t code_point, MonstroGlyph* out_glyph);
uint32_t monstro_font_get_glyph_count(MonstroFont* font);

const char* monstro_font_get_name(MonstroFont* font);
float monstro_font_get_size(MonstroFont* font);
float monstro_font_get_line_height(MonstroFont* font);
float monstro_font_get_ascent(MonstroFont* font);
float monstro_font_get_descent(MonstroFont* font);

#ifdef __cplusplus
}
#endif

#endif
