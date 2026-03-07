#include "context.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* ------------------------------------------------------------------------- */
/* Font Manager                                                              */
/* ------------------------------------------------------------------------- */

MonstroFontManager* monstro_font_manager_create(void) {
    MonstroFontManager* manager = (MonstroFontManager*)calloc(1, sizeof(MonstroFontManager));
    return manager;
}

void monstro_font_manager_destroy(MonstroFontManager* manager) {
    if (!manager) return;
    
    for (int i = 0; i < manager->font_count; i++) {
        if (manager->fonts[i]) {
            monstro_font_destroy(manager->fonts[i]);
        }
    }
    free(manager);
}

MonstroFontManager* monstro_context_get_font_manager(MonstroContext* ctx) {
    if (!ctx) return nullptr;
    return ctx->font_manager;
}

/* ------------------------------------------------------------------------- */
/* TTF Font Loading                                                          */
/* ------------------------------------------------------------------------- */

MonstroFont* monstro_font_load_ttf(
    MonstroFontManager* manager,
    const void* ttf_data,
    size_t ttf_data_size,
    float pixel_size,
    uint32_t first_code_point,
    uint32_t code_point_count,
    MonstroTextureData* out_atlas_data)
{
    if (!manager || !ttf_data || ttf_data_size == 0 || code_point_count == 0) return nullptr;
    
    uint32_t atlas_width = MONSTRO_FONT_ATLAS_DEFAULT_SIZE;
    uint32_t atlas_height = MONSTRO_FONT_ATLAS_DEFAULT_SIZE;
    
    uint8_t* atlas_bitmap = (uint8_t*)calloc(atlas_width * atlas_height, 1);
    if (!atlas_bitmap) return nullptr;
    
    stbtt_packedchar* packed_chars = (stbtt_packedchar*)calloc(code_point_count, sizeof(stbtt_packedchar));
    if (!packed_chars) {
        free(atlas_bitmap);
        return nullptr;
    }
    
    stbtt_pack_context pack_ctx;
    if (!stbtt_PackBegin(&pack_ctx, atlas_bitmap, atlas_width, atlas_height, 0, 1, nullptr)) {
        free(atlas_bitmap);
        free(packed_chars);
        return nullptr;
    }
    
    stbtt_PackSetOversampling(&pack_ctx, 1, 1);
    
    if (!stbtt_PackFontRange(&pack_ctx, (const unsigned char*)ttf_data, 0, pixel_size,
                              first_code_point, code_point_count, packed_chars)) {
        stbtt_PackEnd(&pack_ctx);
        free(atlas_bitmap);
        free(packed_chars);
        return nullptr;
    }
    
    stbtt_PackEnd(&pack_ctx);
    
    stbtt_fontinfo font_info;
    if (!stbtt_InitFont(&font_info, (const unsigned char*)ttf_data, 0)) {
        free(atlas_bitmap);
        free(packed_chars);
        return nullptr;
    }
    
    float scale = stbtt_ScaleForPixelHeight(&font_info, pixel_size);
    
    int ascent_i, descent_i, line_gap_i;
    stbtt_GetFontVMetrics(&font_info, &ascent_i, &descent_i, &line_gap_i);
    
    float ascent = (float)ascent_i * scale;
    float descent = (float)descent_i * scale;
    float line_height = (float)(ascent_i - descent_i + line_gap_i) * scale;
    
    MonstroGlyph* glyphs = (MonstroGlyph*)calloc(code_point_count, sizeof(MonstroGlyph));
    if (!glyphs) {
        free(atlas_bitmap);
        free(packed_chars);
        return nullptr;
    }
    
    for (uint32_t i = 0; i < code_point_count; i++) {
        stbtt_packedchar* pc = &packed_chars[i];
        
        glyphs[i].min_tex_coord[0] = (float)pc->x0 / (float)atlas_width;
        glyphs[i].min_tex_coord[1] = (float)pc->y0 / (float)atlas_height;
        glyphs[i].max_tex_coord[0] = (float)pc->x1 / (float)atlas_width;
        glyphs[i].max_tex_coord[1] = (float)pc->y1 / (float)atlas_height;
        
        glyphs[i].size[0] = (float)(pc->x1 - pc->x0);
        glyphs[i].size[1] = (float)(pc->y1 - pc->y0);
        glyphs[i].offset[0] = pc->xoff;
        glyphs[i].offset[1] = pc->yoff;
        glyphs[i].advance[0] = pc->xadvance;
        glyphs[i].advance[1] = 0.0f;
    }
    
    free(packed_chars);
    
    MonstroFont* font = (MonstroFont*)calloc(1, sizeof(MonstroFont));
    if (!font) {
        free(atlas_bitmap);
        free(glyphs);
        return nullptr;
    }
    
    font->size = pixel_size;
    font->line_height = line_height;
    font->ascent = ascent;
    font->descent = descent;
    font->glyphs = glyphs;
    font->glyph_count = code_point_count;
    font->first_code_point = first_code_point;
    font->atlas_width = atlas_width;
    font->atlas_height = atlas_height;
    font->atlas_data = atlas_bitmap;
    font->atlas_data_size = atlas_width * atlas_height;
    font->atlas_format = MONSTRO_TEXTURE_FORMAT_R8;
    font->atlas_texture_id = 0;
    
    if (out_atlas_data) {
        out_atlas_data->width = atlas_width;
        out_atlas_data->height = atlas_height;
        out_atlas_data->format = MONSTRO_TEXTURE_FORMAT_R8;
        out_atlas_data->data_size = font->atlas_data_size;
        out_atlas_data->data = font->atlas_data;
    }
    
    return font;
}

MonstroFont* monstro_font_load_ttf_from_file(
    MonstroFontManager* manager,
    const char* path,
    float pixel_size,
    uint32_t first_code_point,
    uint32_t code_point_count,
    MonstroTextureData* out_atlas_data)
{
    if (!manager || !path) return nullptr;
    
    FILE* f = fopen(path, "rb");
    if (!f) return nullptr;
    
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(f);
        return nullptr;
    }
    
    uint8_t* ttf_data = (uint8_t*)malloc(file_size);
    if (!ttf_data) {
        fclose(f);
        return nullptr;
    }
    
    size_t bytes_read = fread(ttf_data, 1, file_size, f);
    fclose(f);
    
    if (bytes_read != (size_t)file_size) {
        free(ttf_data);
        return nullptr;
    }
    
    MonstroFont* font = monstro_font_load_ttf(manager, ttf_data, file_size, pixel_size, 
                                               first_code_point, code_point_count, out_atlas_data);
    
    free(ttf_data);
    return font;
}

/* ------------------------------------------------------------------------- */
/* Bitmap Font Loading                                                       */
/* ------------------------------------------------------------------------- */

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
    MonstroTextureData* out_atlas_data)
{
    if (!manager || !image_data || grid_cols == 0 || grid_rows == 0) return nullptr;
    
    uint32_t glyph_count = grid_cols * grid_rows;
    
    MonstroGlyph* glyphs = (MonstroGlyph*)calloc(glyph_count, sizeof(MonstroGlyph));
    if (!glyphs) return nullptr;
    
    float step_u = 1.0f / (float)grid_cols;
    float step_v = 1.0f / (float)grid_rows;
    
    for (uint32_t i = 0; i < glyph_count; i++) {
        uint32_t col = i % grid_cols;
        uint32_t row = i / grid_cols;
        
        float cx = (float)col / (float)grid_cols;
        float cy = (float)row / (float)grid_rows;
        
        glyphs[i].min_tex_coord[0] = cx;
        glyphs[i].min_tex_coord[1] = cy;
        glyphs[i].max_tex_coord[0] = cx + step_u;
        glyphs[i].max_tex_coord[1] = cy + step_v;
        
        glyphs[i].size[0] = font_size_px;
        glyphs[i].size[1] = font_size_px;
        glyphs[i].offset[0] = 0.0f;
        glyphs[i].offset[1] = 0.0f;
        glyphs[i].advance[0] = font_size_px * 0.6f;
        glyphs[i].advance[1] = 0.0f;
    }
    
    size_t data_size = image_width * image_height * 4;
    uint8_t* atlas_copy = (uint8_t*)malloc(data_size);
    if (!atlas_copy) {
        free(glyphs);
        return nullptr;
    }
    memcpy(atlas_copy, image_data, data_size);
    
    MonstroFont* font = (MonstroFont*)calloc(1, sizeof(MonstroFont));
    if (!font) {
        free(atlas_copy);
        free(glyphs);
        return nullptr;
    }
    
    font->size = font_size_px;
    font->line_height = font_size_px;
    font->ascent = font_size_px;
    font->descent = 0.0f;
    font->glyphs = glyphs;
    font->glyph_count = glyph_count;
    font->first_code_point = first_code_point;
    font->atlas_width = image_width;
    font->atlas_height = image_height;
    font->atlas_data = atlas_copy;
    font->atlas_data_size = data_size;
    font->atlas_format = MONSTRO_TEXTURE_FORMAT_RGBA8;
    font->atlas_texture_id = 0;
    
    if (out_atlas_data) {
        out_atlas_data->width = image_width;
        out_atlas_data->height = image_height;
        out_atlas_data->format = MONSTRO_TEXTURE_FORMAT_RGBA8;
        out_atlas_data->data_size = data_size;
        out_atlas_data->data = font->atlas_data;
    }
    
    return font;
}

MonstroFont* monstro_font_load_bitmap_from_file(
    MonstroFontManager* manager,
    const char* path,
    uint32_t grid_cols,
    uint32_t grid_rows,
    uint32_t first_code_point,
    float font_size_px,
    MonstroTextureData* out_atlas_data)
{
    if (!manager || !path) return nullptr;
    
    int width, height, channels;
    uint8_t* image_data = stbi_load(path, &width, &height, &channels, 4);
    if (!image_data) return nullptr;
    
    uint32_t cell_width = width / grid_cols;
    uint32_t cell_height = height / grid_rows;
    
    MonstroFont* font = monstro_font_load_bitmap(manager, image_data, width, height,
                                                  grid_cols, grid_rows, cell_width, cell_height,
                                                  first_code_point, font_size_px, out_atlas_data);
    
    stbi_image_free(image_data);
    
    if (font && font->atlas_data != image_data) {
        return font;
    }
    
    return font;
}

/* ------------------------------------------------------------------------- */
/* Font Destruction                                                          */
/* ------------------------------------------------------------------------- */

void monstro_font_destroy(MonstroFont* font) {
    if (!font) return;
    
    if (font->glyphs) {
        free(font->glyphs);
    }
    if (font->atlas_data) {
        free(font->atlas_data);
    }
    free(font);
}

/* ------------------------------------------------------------------------- */
/* Font Atlas Texture                                                        */
/* ------------------------------------------------------------------------- */

void monstro_font_set_atlas_texture(MonstroFont* font, uint64_t texture_id) {
    if (!font) return;
    font->atlas_texture_id = texture_id;
}

uint64_t monstro_font_get_atlas_texture(MonstroFont* font) {
    if (!font) return 0;
    return font->atlas_texture_id;
}

/* ------------------------------------------------------------------------- */
/* Font Manager Registry                                                     */
/* ------------------------------------------------------------------------- */

void monstro_font_add(MonstroFontManager* manager, MonstroFont* font, const char* name, float size) {
    if (!manager || !font || manager->font_count >= MONSTRO_MAX_FONTS) return;
    
    strncpy(font->name, name, sizeof(font->name) - 1);
    font->name[sizeof(font->name) - 1] = '\0';
    font->size = size;
    
    manager->fonts[manager->font_count++] = font;
}

MonstroFont* monstro_font_find(MonstroFontManager* manager, const char* name, float size) {
    if (!manager || !name) return nullptr;
    
    for (int i = 0; i < manager->font_count; i++) {
        MonstroFont* font = manager->fonts[i];
        if (font && strcmp(font->name, name) == 0) {
            if (size <= 0 || fabsf(font->size - size) < 0.5f) {
                return font;
            }
        }
    }
    
    return nullptr;
}

/* ------------------------------------------------------------------------- */
/* Font Measurement                                                          */
/* ------------------------------------------------------------------------- */

static uint32_t decode_utf8(const char** str) {
    const uint8_t* s = (const uint8_t*)*str;
    uint32_t c = *s++;
    
    if (c < 0x80) {
        *str = (const char*)s;
        return c;
    }
    
    if ((c & 0xE0) == 0xC0) {
        c = ((c & 0x1F) << 6) | (*s++ & 0x3F);
    } else if ((c & 0xF0) == 0xE0) {
        c = ((c & 0x0F) << 12) | ((*s++ & 0x3F) << 6);
        c |= (*s++ & 0x3F);
    } else if ((c & 0xF8) == 0xF0) {
        c = ((c & 0x07) << 18) | ((*s++ & 0x3F) << 12);
        c |= ((*s++ & 0x3F) << 6);
        c |= (*s++ & 0x3F);
    }
    
    *str = (const char*)s;
    return c;
}

MonstroVec2 monstro_font_measure_text(MonstroFont* font, const char* text_utf8, float wrap_width) {
    MonstroVec2 result = {0.0f, 0.0f};
    
    if (!font || !text_utf8) {
        return result;
    }
    
    if (wrap_width > 0.0f) {
        return monstro_font_measure_text_wrapped(font, text_utf8, wrap_width);
    }
    
    float width = 0;
    float max_width = 0;
    int line_count = 1;
    
    const char* s = text_utf8;
    while (*s) {
        uint32_t codepoint = decode_utf8(&s);
        
        if (codepoint == '\n') {
            if (width > max_width) max_width = width;
            width = 0;
            line_count++;
            continue;
        }
        
        if (codepoint == '\r') continue;
        
        if (codepoint >= font->first_code_point && 
            codepoint < font->first_code_point + font->glyph_count) {
            MonstroGlyph* g = &font->glyphs[codepoint - font->first_code_point];
            width += g->advance[0];
        }
    }
    
    if (width > max_width) max_width = width;
    
    result.x = max_width;
    result.y = font->line_height * line_count;
    return result;
}

MonstroVec2 monstro_font_measure_text_wrapped(MonstroFont* font, const char* text_utf8, float wrap_width) {
    MonstroVec2 result = {0.0f, 0.0f};
    
    if (!font || !text_utf8 || wrap_width <= 0) {
        return result;
    }
    
    float width = 0;
    float max_width = 0;
    int line_count = 1;
    
    const char* s = text_utf8;
    while (*s) {
        uint32_t codepoint = decode_utf8(&s);
        
        if (codepoint == '\n') {
            if (width > max_width) max_width = width;
            width = 0;
            line_count++;
            continue;
        }
        
        if (codepoint == '\r') continue;
        
        float advance = 0;
        if (codepoint >= font->first_code_point && 
            codepoint < font->first_code_point + font->glyph_count) {
            MonstroGlyph* g = &font->glyphs[codepoint - font->first_code_point];
            advance = g->advance[0];
        }
        
        if (width + advance > wrap_width && width > 0) {
            if (width > max_width) max_width = width;
            width = 0;
            line_count++;
        }
        
        width += advance;
    }
    
    if (width > max_width) max_width = width;
    
    result.x = max_width;
    result.y = font->line_height * line_count;
    return result;
}

/* ------------------------------------------------------------------------- */
/* Font Glyph Access                                                         */
/* ------------------------------------------------------------------------- */

bool monstro_font_get_glyph(MonstroFont* font, uint32_t code_point, MonstroGlyph* out_glyph) {
    if (!font || !out_glyph) return false;
    
    if (code_point < font->first_code_point || 
        code_point >= font->first_code_point + font->glyph_count) {
        return false;
    }
    
    *out_glyph = font->glyphs[code_point - font->first_code_point];
    return true;
}

uint32_t monstro_font_get_glyph_count(MonstroFont* font) {
    if (!font) return 0;
    return font->glyph_count;
}

/* ------------------------------------------------------------------------- */
/* Font Properties                                                           */
/* ------------------------------------------------------------------------- */

float monstro_font_get_size(MonstroFont* font) {
    if (!font) return 0;
    return font->size;
}

float monstro_font_get_line_height(MonstroFont* font) {
    if (!font) return 0;
    return font->line_height;
}

float monstro_font_get_ascent(MonstroFont* font) {
    if (!font) return 0;
    return font->ascent;
}

float monstro_font_get_descent(MonstroFont* font) {
    if (!font) return 0;
    return font->descent;
}

void monstro_font_get_atlas_size(MonstroFont* font, uint32_t* out_width, uint32_t* out_height) {
    if (!font) {
        if (out_width) *out_width = 0;
        if (out_height) *out_height = 0;
        return;
    }
    
    if (out_width) *out_width = font->atlas_width;
    if (out_height) *out_height = font->atlas_height;
}
