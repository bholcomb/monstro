#include "context.h"
#include "env.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Both libraries stay private to this file: their few process-wide settings
// cannot be reached through the library's exports.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_STATIC
#define STBI_NO_FAILURE_STRINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

using monstro::Env;
using monstro::requireEnv;

/* ------------------------------------------------------------------------- */
/* Loading into the atlas                                                    */
/* ------------------------------------------------------------------------- */

static void require_font_args(const char *function, const char *name, const void *data, float size)
{
   MONSTRO_REQUIRE(name, "%s: the font name must not be null", function);
   MONSTRO_REQUIRE(data, "%s: the font data or path must not be null", function);
   MONSTRO_REQUIRE(size > 0.0f, "%s: the font size must be positive (got %g)", function, (double)size);
}

static uint8_t *read_file(const char *path, size_t *out_size)
{
   FILE *f = fopen(path, "rb");
   if (!f)
      return nullptr;
   uint8_t *data = nullptr;
   long size = (fseek(f, 0, SEEK_END) == 0) ? ftell(f) : -1;
   if (size > 0 && fseek(f, 0, SEEK_SET) == 0)
   {
      data = (uint8_t *)malloc((size_t)size);
      if (data && fread(data, 1, (size_t)size, f) != (size_t)size)
      {
         free(data);
         data = nullptr;
      }
   }
   fclose(f);
   *out_size = data ? (size_t)size : 0;
   return data;
}

MonstroFont *monstro_font_load_ttf(MonstroEnv *env, const char *name, const void *ttf_data, size_t ttf_data_size,
                                   float pixel_size, uint32_t first_code_point, uint32_t code_point_count)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   require_font_args(__func__, name, ttf_data, pixel_size);
   MONSTRO_REQUIRE(code_point_count > 0, "%s: code_point_count must be positive", __func__);

   const unsigned char *data = (const unsigned char *)ttf_data;
   stbtt_fontinfo info;
   if (ttf_data_size < 12 || stbtt_GetFontOffsetForIndex(data, 0) < 0 ||
       !stbtt_InitFont(&info, data, stbtt_GetFontOffsetForIndex(data, 0)))
      return nullptr;

   MonstroGlyph *glyphs = (MonstroGlyph *)calloc(code_point_count, sizeof(MonstroGlyph));
   if (!glyphs)
      return nullptr;

   float scale = stbtt_ScaleForPixelHeight(&info, pixel_size);
   int ascent_i, descent_i, line_gap_i;
   stbtt_GetFontVMetrics(&info, &ascent_i, &descent_i, &line_gap_i);

   // Each glyph is rasterized into scratch coverage, then stored as white with that coverage as alpha.
   monstro::Atlas *atlas = &env->atlas;
   uint8_t *coverage = nullptr;
   size_t coverage_size = 0;
   for (uint32_t i = 0; i < code_point_count; i++)
   {
      int glyph_index = stbtt_FindGlyphIndex(&info, (int)(first_code_point + i));
      int advance, left_bearing, x0, y0, x1, y1;
      stbtt_GetGlyphHMetrics(&info, glyph_index, &advance, &left_bearing);
      stbtt_GetGlyphBitmapBox(&info, glyph_index, scale, scale, &x0, &y0, &x1, &y1);

      MonstroGlyph *g = &glyphs[i];
      int w = x1 - x0;
      int h = y1 - y0;
      g->size[0] = (float)(w > 0 ? w : 0);
      g->size[1] = (float)(h > 0 ? h : 0);
      g->offset[0] = (float)x0;
      g->offset[1] = (float)y0;
      g->advance[0] = (float)advance * scale;
      if (w <= 0 || h <= 0)
         continue;

      if ((size_t)(w * h) > coverage_size)
      {
         free(coverage);
         coverage_size = (size_t)(w * h);
         coverage = (uint8_t *)malloc(coverage_size);
         if (!coverage)
         {
            free(glyphs);
            return nullptr;
         }
      }
      stbtt_MakeGlyphBitmap(&info, coverage, w, h, w, scale, scale, glyph_index);

      uint32_t x, y;
      atlas->allocate((uint32_t)w, (uint32_t)h, name, &x, &y);
      for (int row = 0; row < h; row++)
         for (int col = 0; col < w; col++)
            atlas->pixel(x + (uint32_t)col, y + (uint32_t)row)[3] = coverage[row * w + col];

      g->min_tex_coord[0] = (float)x / (float)atlas->width;
      g->min_tex_coord[1] = (float)y / (float)atlas->height;
      g->max_tex_coord[0] = (float)(x + (uint32_t)w) / (float)atlas->width;
      g->max_tex_coord[1] = (float)(y + (uint32_t)h) / (float)atlas->height;
   }
   free(coverage);

   MonstroFont *font = env->addFont(name, __func__);
   font->size = pixel_size;
   font->ascent = (float)ascent_i * scale;
   font->descent = (float)descent_i * scale;
   font->line_height = (float)(ascent_i - descent_i + line_gap_i) * scale;
   font->glyphs = glyphs;
   font->glyph_count = code_point_count;
   font->first_code_point = first_code_point;
   atlas->version++;
   return font;
}

MonstroFont *monstro_font_load_ttf_from_file(MonstroEnv *env, const char *name, const char *path, float pixel_size,
                                             uint32_t first_code_point, uint32_t code_point_count)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   require_font_args(__func__, name, path, pixel_size);

   size_t size = 0;
   uint8_t *data = read_file(path, &size);
   if (!data)
      return nullptr;
   MonstroFont *font = monstro_font_load_ttf(env, name, data, size, pixel_size, first_code_point, code_point_count);
   free(data);
   return font;
}

MonstroFont *monstro_font_load_bitmap(MonstroEnv *env, const char *name, const void *image_data, uint32_t image_width,
                                      uint32_t image_height, uint32_t grid_cols, uint32_t grid_rows,
                                      uint32_t first_code_point, float font_size_px)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   require_font_args(__func__, name, image_data, font_size_px);
   MONSTRO_REQUIRE(grid_cols > 0 && grid_rows > 0 && image_width >= grid_cols && image_height >= grid_rows,
                   "%s: a %u x %u image cannot hold a %u x %u grid", __func__, image_width, image_height, grid_cols,
                   grid_rows);

   uint32_t glyph_count = grid_cols * grid_rows;
   MonstroGlyph *glyphs = (MonstroGlyph *)calloc(glyph_count, sizeof(MonstroGlyph));
   if (!glyphs)
      return nullptr;

   monstro::Atlas *atlas = &env->atlas;
   uint32_t x, y;
   atlas->allocate(image_width, image_height, name, &x, &y);
   const uint8_t *source = (const uint8_t *)image_data;
   for (uint32_t row = 0; row < image_height; row++)
      memcpy(atlas->pixel(x, y + row), source + (size_t)row * image_width * 4, (size_t)image_width * 4);

   float cell_w = (float)image_width / (float)grid_cols;
   float cell_h = (float)image_height / (float)grid_rows;
   for (uint32_t i = 0; i < glyph_count; i++)
   {
      float cx = (float)x + cell_w * (float)(i % grid_cols);
      float cy = (float)y + cell_h * (float)(i / grid_cols);
      glyphs[i].min_tex_coord[0] = cx / (float)atlas->width;
      glyphs[i].min_tex_coord[1] = cy / (float)atlas->height;
      glyphs[i].max_tex_coord[0] = (cx + cell_w) / (float)atlas->width;
      glyphs[i].max_tex_coord[1] = (cy + cell_h) / (float)atlas->height;
      glyphs[i].size[0] = font_size_px;
      glyphs[i].size[1] = font_size_px;
      glyphs[i].advance[0] = font_size_px * 0.6f;
   }

   MonstroFont *font = env->addFont(name, __func__);
   font->size = font_size_px;
   font->line_height = font_size_px;
   font->ascent = font_size_px;
   font->descent = 0.0f;
   font->glyphs = glyphs;
   font->glyph_count = glyph_count;
   font->first_code_point = first_code_point;
   atlas->version++;
   return font;
}

MonstroFont *monstro_font_load_bitmap_from_file(MonstroEnv *env, const char *name, const char *path, uint32_t grid_cols,
                                                uint32_t grid_rows, uint32_t first_code_point, float font_size_px)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   require_font_args(__func__, name, path, font_size_px);

   int width, height, channels;
   uint8_t *image_data = stbi_load(path, &width, &height, &channels, 4);
   if (!image_data)
      return nullptr;
   MonstroFont *font = monstro_font_load_bitmap(env, name, image_data, (uint32_t)width, (uint32_t)height, grid_cols,
                                                grid_rows, first_code_point, font_size_px);
   stbi_image_free(image_data);
   return font;
}

MonstroFont *monstro_font_find(MonstroEnv *env, const char *name, float size)
{
   requireEnv(env, __func__);
   MONSTRO_REQUIRE(name, "%s: the name must not be null", __func__);
   for (MonstroFont &font : env->fonts)
   {
      if (strcmp(font.name, name) == 0 && (size <= 0 || fabsf(font.size - size) < 0.5f))
         return &font;
   }
   return nullptr;
}

void monstro_env_set_default_font(MonstroEnv *env, MonstroFont *font)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   MONSTRO_REQUIRE(font, "%s: the font must not be null", __func__);
   env->default_font = font;
}

MonstroFont *monstro_env_get_default_font(MonstroEnv *env) { return requireEnv(env, __func__)->default_font; }

MonstroTextureData monstro_env_get_atlas(MonstroEnv *env)
{
   requireEnv(env, __func__);
   MonstroTextureData data = {};
   data.width = env->atlas.width;
   data.height = env->atlas.height;
   data.format = MONSTRO_TEXTURE_FORMAT_RGBA8;
   data.data_size = (size_t)env->atlas.width * env->atlas.height * 4;
   data.data = env->atlas.pixels;
   data.version = env->atlas.version;
   return data;
}

void monstro_env_set_atlas_texture(MonstroEnv *env, uint64_t texture_id)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   env->atlas.texture_id = texture_id;
}

uint64_t monstro_env_get_atlas_texture(MonstroEnv *env) { return requireEnv(env, __func__)->atlas.texture_id; }

/* ------------------------------------------------------------------------- */
/* Font Measurement                                                          */
/* ------------------------------------------------------------------------- */

// Measures text up to its terminator or, when end is given, up to end.
static MonstroVec2 measure_unwrapped(MonstroFont *font, const char *text_utf8, const char *end)
{
   float width = 0;
   float max_width = 0;
   int line_count = 1;
   float icon_size = font->line_height;

   const char *s = text_utf8;
   while (*s && (!end || s < end))
   {
      uint32_t icon_id;
      if (parse_icon_tag(&s, &icon_id))
      {
         width += icon_size;
         continue;
      }

      uint32_t codepoint = decode_utf8(&s);

      if (codepoint == '\n')
      {
         if (width > max_width)
            max_width = width;
         width = 0;
         line_count++;
         continue;
      }

      if (codepoint == '\r')
         continue;

      if (codepoint >= font->first_code_point && codepoint < font->first_code_point + font->glyph_count)
      {
         MonstroGlyph *g = &font->glyphs[codepoint - font->first_code_point];
         width += g->advance[0];
      }
   }

   if (width > max_width)
      max_width = width;

   MonstroVec2 result;
   result.x = max_width;
   result.y = font->line_height * line_count;
   return result;
}

MonstroVec2 monstro_font_measure_text(MonstroFont *font, const char *text_utf8, float wrap_width)
{
   if (!font || !text_utf8)
   {
      return {0.0f, 0.0f};
   }

   if (wrap_width > 0.0f)
   {
      return monstro_font_measure_text_wrapped(font, text_utf8, wrap_width);
   }

   return measure_unwrapped(font, text_utf8, nullptr);
}

float monstro_font_measure_prefix(MonstroFont *font, const char *text_utf8, size_t bytes)
{
   if (!font || !text_utf8)
      return 0.0f;
   return measure_unwrapped(font, text_utf8, text_utf8 + bytes).x;
}

MonstroVec2 monstro_font_measure_text_wrapped(MonstroFont *font, const char *text_utf8, float wrap_width)
{
   MonstroVec2 result = {0.0f, 0.0f};

   if (!font || !text_utf8 || wrap_width <= 0)
   {
      return result;
   }

   float width = 0;
   float max_width = 0;
   int line_count = 1;
   float icon_size = font->line_height;

   const char *s = text_utf8;
   while (*s)
   {
      uint32_t icon_id;
      if (parse_icon_tag(&s, &icon_id))
      {
         if (width + icon_size > wrap_width && width > 0)
         {
            if (width > max_width)
               max_width = width;
            width = 0;
            line_count++;
         }
         width += icon_size;
         continue;
      }

      uint32_t codepoint = decode_utf8(&s);

      if (codepoint == '\n')
      {
         if (width > max_width)
            max_width = width;
         width = 0;
         line_count++;
         continue;
      }

      if (codepoint == '\r')
         continue;

      float advance = 0;
      if (codepoint >= font->first_code_point && codepoint < font->first_code_point + font->glyph_count)
      {
         MonstroGlyph *g = &font->glyphs[codepoint - font->first_code_point];
         advance = g->advance[0];
      }

      if (width + advance > wrap_width && width > 0)
      {
         if (width > max_width)
            max_width = width;
         width = 0;
         line_count++;
      }

      width += advance;
   }

   if (width > max_width)
      max_width = width;

   result.x = max_width;
   result.y = font->line_height * line_count;
   return result;
}

/* ------------------------------------------------------------------------- */
/* Font Glyph Access                                                         */
/* ------------------------------------------------------------------------- */

bool monstro_font_get_glyph(MonstroFont *font, uint32_t code_point, MonstroGlyph *out_glyph)
{
   if (!font || !out_glyph)
      return false;

   if (code_point < font->first_code_point || code_point >= font->first_code_point + font->glyph_count)
   {
      return false;
   }

   *out_glyph = font->glyphs[code_point - font->first_code_point];
   return true;
}

const char *monstro_font_get_name(MonstroFont *font)
{
   MONSTRO_REQUIRE(font, "%s: the font must not be null", __func__);
   return font->name;
}

uint32_t monstro_font_get_glyph_count(MonstroFont *font)
{
   if (!font)
      return 0;
   return font->glyph_count;
}

/* ------------------------------------------------------------------------- */
/* Font Properties                                                           */
/* ------------------------------------------------------------------------- */

float monstro_font_get_size(MonstroFont *font)
{
   if (!font)
      return 0;
   return font->size;
}

float monstro_font_get_line_height(MonstroFont *font)
{
   if (!font)
      return 0;
   return font->line_height;
}

float monstro_font_get_ascent(MonstroFont *font)
{
   if (!font)
      return 0;
   return font->ascent;
}

float monstro_font_get_descent(MonstroFont *font)
{
   if (!font)
      return 0;
   return font->descent;
}
