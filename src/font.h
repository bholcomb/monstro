#ifndef MONSTRO_INTERNAL_FONT_H
#define MONSTRO_INTERNAL_FONT_H

#include "constants.h"
#include "monstro.h"

// A font owned by an environment; its glyphs point into the environment's atlas.
struct MonstroFont
{
   char name[MONSTRO_FONT_NAME_SIZE] = {};
   float size = 0.0f;
   float line_height = 0.0f;
   float ascent = 0.0f;
   float descent = 0.0f;

   MonstroGlyph *glyphs = nullptr;
   uint32_t glyph_count = 0;
   uint32_t first_code_point = 0;
};

// Width of the first `bytes` bytes of text, which end on a code point boundary.
float monstro_font_measure_prefix(MonstroFont *font, const char *text_utf8, size_t bytes);

// One line of text that starts at a given point: the line is the text up to end, it is width wide, and the next
// line starts at next. A line ends at a newline (hard_break) or where it would grow wider than wrap_width (no limit
// when that is not positive): after its last word, or within a word too long for a whole line. The spaces where
// it breaks belong to neither line. Measuring and drawing break lines here, so they always agree.
struct MonstroTextLine
{
   const char *end;
   const char *next;
   float width;
   bool hard_break;
};
MonstroTextLine monstro_font_wrap_line(MonstroFont *font, const char *text_utf8, float wrap_width);

#endif
