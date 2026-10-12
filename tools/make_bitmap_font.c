/* Renders a TrueType font into a bitmap font for monstro_font_load_bitmap: a grid of square cells,
 * one code point per cell from the first, white with the glyph's coverage in alpha. The font's
 * ascent to descent fills the cell's height, the baseline at the ascent; each glyph is centred in
 * the advance Monstro gives a bitmap glyph, 0.6 of the cell.
 *
 * From the repository root, build it and regenerate the examples' atlas:
 *   mkdir -p build && cc -O2 -Iexternals tools/make_bitmap_font.c -lm -o build/make_bitmap_font
 *   build/make_bitmap_font examples/fonts/DejaVuSansMono.ttf examples/fonts/DejaVuSansMono-bitmap.png 32 16 6 32
 */
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>

static void fail(const char *message, const char *detail)
{
   fprintf(stderr, "make_bitmap_font: %s %s\n", message, detail);
   exit(1);
}

static int clamp(int value, int low, int high) { return value < low ? low : value > high ? high : value; }

int main(int argc, char **argv)
{
   if (argc != 7)
      fail("usage: make_bitmap_font font.ttf out.png cell_px columns rows first_code_point", "");
   int cell = atoi(argv[3]), cols = atoi(argv[4]), rows = atoi(argv[5]), first = atoi(argv[6]);
   if (cell <= 0 || cols <= 0 || rows <= 0 || first < 0)
      fail("cell_px, columns and rows must be positive and first_code_point not negative", "");

   FILE *file = fopen(argv[1], "rb");
   if (!file)
      fail("cannot open", argv[1]);
   fseek(file, 0, SEEK_END);
   long size = ftell(file);
   fseek(file, 0, SEEK_SET);
   unsigned char *ttf = malloc((size_t)size);
   if (!ttf || fread(ttf, 1, (size_t)size, file) != (size_t)size)
      fail("cannot read", argv[1]);
   fclose(file);

   stbtt_fontinfo font;
   if (!stbtt_InitFont(&font, ttf, stbtt_GetFontOffsetForIndex(ttf, 0)))
      fail("not a TrueType font:", argv[1]);
   float scale = stbtt_ScaleForPixelHeight(&font, (float)cell);
   int ascent, descent, line_gap;
   stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
   int baseline = (int)(ascent * scale + 0.5f);

   int width = cols * cell, height = rows * cell;
   unsigned char *coverage = calloc((size_t)width * height, 1);
   unsigned char *rgba = malloc((size_t)width * height * 4);
   if (!coverage || !rgba)
      fail("out of memory", "");
   for (int i = 0; i < cols * rows; i++)
   {
      int code_point = first + i;
      if (!stbtt_FindGlyphIndex(&font, code_point))
         continue;
      int advance, left_bearing, x0, y0, x1, y1;
      stbtt_GetCodepointHMetrics(&font, code_point, &advance, &left_bearing);
      stbtt_GetCodepointBitmapBox(&font, code_point, scale, scale, &x0, &y0, &x1, &y1);
      int cell_x = (i % cols) * cell, cell_y = (i / cols) * cell;
      if (x1 - x0 > cell || y1 - y0 > cell)
      {
         fprintf(stderr, "make_bitmap_font: U+%04X is larger than its %d px cell\n", code_point, cell);
         return 1;
      }
      /* A glyph past the descent (DejaVu's underscore) moves up into its cell. */
      int x = clamp(cell_x + (int)((cell * 0.6f - advance * scale) / 2) + x0, cell_x, cell_x + cell - (x1 - x0));
      int y = clamp(cell_y + baseline + y0, cell_y, cell_y + cell - (y1 - y0));
      stbtt_MakeCodepointBitmap(&font, coverage + (size_t)y * width + x, x1 - x0, y1 - y0, width, scale, scale,
                                code_point);
   }

   for (int i = 0; i < width * height; i++)
   {
      rgba[i * 4 + 0] = rgba[i * 4 + 1] = rgba[i * 4 + 2] = 255;
      rgba[i * 4 + 3] = coverage[i];
   }
   if (!stbi_write_png(argv[2], width, height, 4, rgba, width * 4))
      fail("cannot write", argv[2]);
   return 0;
}
