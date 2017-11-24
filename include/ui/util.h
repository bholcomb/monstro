/*-----------------------------------------------------------------------------
Copyright 2017
Bob Holcomb
-----------------------------------------------------------------------------*/

namespace UI
{
   typedef unsigned short UInt16;
   typedef unsigned int UInt32;

   struct Vec2
   {
      Vec2(): x(0), y(0) {}
      Vec2(float X, float Y): x(X), y(Y) {}
      float x;
      float y;

      static Vec2 zero() { return Vec2(0, 0); }
      static Vec2 one() { return Vec2(1, 1); }
   };

   struct Color
   {
      Color() : r(0), g(0), b(0), a(1) {}
      explicit Color(float R, float G, float B, float A): r(R), g(G), b(B), a(A) {}
      explicit Color(int R, int G, int B, int A);
      Color(UInt32 val);

      UInt32 val();

      static Color white() { return Color(1.0, 1.0, 1.0, 1.0); };
      static Color black() { return Color(0.0, 0.0, 0.0, 0.0); };

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
   };
}