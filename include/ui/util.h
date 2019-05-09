/*-----------------------------------------------------------------------------
Copyright 2017
Bob Holcomb
-----------------------------------------------------------------------------*/

#pragma once

namespace UI
{
   typedef unsigned char UInt8;
   typedef unsigned short UInt16;
   typedef unsigned int UInt32;
   typedef unsigned long long UInt64;
   typedef char Int8;
   typedef int Int32;
   typedef short Int16;
   typedef long long Int64;

   struct Vec2
   {
      Vec2(): x(0), y(0) {}
      Vec2(float X, float Y): x(X), y(Y) {}
      float x;
      float y;

      static Vec2 zero() { return Vec2(0, 0); }
      static Vec2 one() { return Vec2(1, 1); }

      Vec2 operator+(const Vec2& rhs) { Vec2 res; res.x = x + rhs.x; res.y = y + rhs.y; return res; }
      Vec2& operator+=(const Vec2& rhs) { x += rhs.x; y += rhs.y; return *this; }

      Vec2 operator-(const Vec2& rhs) { Vec2 res; res.x = x - rhs.x; res.y = y - rhs.y; return res; }
      Vec2& operator-=(const Vec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }

      void normalize();
   };

   struct Color
   {
      Color() : r(0), g(0), b(0), a(1) {}
      explicit Color(float R, float G, float B, float A): r(R), g(G), b(B), a(A) {}
      explicit Color(UInt8 R, UInt8 G, UInt8 B, UInt8 A) : r(R / 255.0f), g(G / 255.0f), b(B / 255.0f), a(A / 255.0f) {}
      explicit Color(UInt32 val);

      UInt32 toUInt32();

      static Color white() { return Color(1.0f, 1.0f, 1.0f, 1.0f); };
      static Color black() { return Color(0.0f, 0.0f, 0.0f, 0.0f); };

      float r;
      float g;
      float b;
      float a;
   };

   struct Vert
   {
      Vert(){}

      Vec2 pos;
      Vec2 uv;
      UInt32 color;
   };

   struct Rect
   {
      float left;
      float bottom;
      float right;
      float top;

      Rect(): left(0), bottom(0), right(0), top(0) {}
      Rect(float l, float b, float r, float t) : left(l), bottom(b), right(r), top(t){}

      bool contains(Vec2 point);
   };
}