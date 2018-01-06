/*-----------------------------------------------------------------------------
Copyright 2017
Bob Holcomb
-----------------------------------------------------------------------------*/

#include "ui/util.h"

#include <cmath>

using namespace UI;

bool Rect::contains(Vec2 point)
{
   return(point.x > left && point.x < right &&
      point.y > bottom && point.y < top);
}


Color::Color(UInt32 val)
{
   float s = 1.0f / 255.0f;
   r = (val & 0xff) * s;
   g = ((val >> 8) & 0xff) * s;
   b = ((val >> 16) & 0xff) * s;
   a = ((val >> 24) & 0xff) * s;
}

UInt32 Color::toUInt32()
{
   UInt32 i = 0;

   i  = (UInt32)(r * 255.0f + 0.5f);
   i |= (UInt32)(g * 255.0f + 0.5f) << 8;
   i |= (UInt32)(b * 255.0f + 0.5f) << 16;
   i |= (UInt32)(a * 255.0f + 0.5f) << 24;

   return i;
}

void Vec2::normalize()
{
   float mag = sqrt(x * x + y * y);
   x /= mag;
   y /= mag;
}
