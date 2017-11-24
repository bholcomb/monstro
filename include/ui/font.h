/*-----------------------------------------------------------------------------
Copyright 2017
Bob Holcomb
-----------------------------------------------------------------------------*/

#include "ui/util.h"

#include <vector>

namespace UI
{
   struct Glyph
   {
      Vec2 minTex;
      Vec2 maxTex;
      Vec2 size;
      Vec2 offset;
      Vec2 advance;
   };

   class Font
   {
   public:
      Font();
      virtual ~Font();

      Glyph* glyph(int glyph);

   protected:
      std::vector<Glyph> myGlyphs;
   };
}