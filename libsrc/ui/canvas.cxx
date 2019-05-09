/*-----------------------------------------------------------------------------
Copyright 2017
Bob Holcomb
-----------------------------------------------------------------------------*/

#include "ui/canvas.h"

const double PI = 3.14159265359;

using namespace UI;

Canvas::Canvas(int width, int height)
   :myWidth(width)
   ,myHeight(height)
{
   reset();
   myVerts = new Vert[MAX_VERTS];
   myIndexes = new UInt16[MAX_ELEMENTS];
}

Canvas::~Canvas()
{
}

void Canvas::reset()
{
   myVertCount = 0;
   myIndexCount = 0;
   myPath.clear();
   myClipStack.clear();
   myTextureStatck.clear();
   //myTextureStatck.push_back(theDefaultTexture);
   myLayerStack.clear();
   myCmdBuffer.clear();

   addDrawCommand();
}

void Canvas::pushDrawLayer(int layer)
{
   myLayerStack.push_back(layer);
   updateLayer();
}

void Canvas::popDrawLayer()
{
   if (myLayerStack.size() > 0)
   {
      myLayerStack.pop_back();
      updateLayer();
   }
}

void Canvas::pushClipRect(Rect r)
{
   myClipStack.push_back(r);
   updateClipRect();
}

void Canvas::popClipRect()
{
   if (myClipStack.size() > 0)
   {
      myClipStack.pop_back();
      updateClipRect();
   }
}

void Canvas::pushTexture(UInt32 tex)
{
   myTextureStatck.push_back(tex);
   updateTexture();
}

void Canvas::popTexture()
{
   if (myTextureStatck.size() > 0)
   {
      myTextureStatck.pop_back();
      updateTexture();
   }
}

void Canvas::addLine(Vec2 a, Vec2 b, Color col, float thickness /*= 1.0f*/)
{
   if (col.a == 0.0)
      return;

   pathLineTo(a + Vec2(0.5f, 0.5f));
   pathLineTo(b + Vec2(0.5f, 0.5f));
   pathStroke(col, false, thickness);
}

void Canvas::addPolyLine(const std::vector<Vec2>& points, Color color, bool closed /*= false*/, float thickness /*= 1.0f*/)
{
   if (points.size() < 2)
      return;

   Vec2 uv = Vec2::zero();

   int count = points.size();
   if (!closed)
      count = count - 1;

   bool thickLine = thickness > 1.0f;

   for (int i1 = 0; i1 < count; i1++)
   {
      int i2 = (i1 + 1) == points.size() ? 0 : i1 + 1;
      Vec2 p1 = points[i1];
      Vec2 p2 = points[i2];
      Vec2 diff = p2 - p1;
      diff.normalize();

      float dx = diff.x * (thickness * 0.5f);
      float dy = diff.y * (thickness * 0.5f);

      indexTwoTriangles();

      writeVertex(Vec2(p1.x + dy, p1.y - dx), uv, color);
      writeVertex(Vec2(p2.x + dy, p2.y - dx), uv, color);
      writeVertex(Vec2(p2.x - dy, p2.y + dx), uv, color);
      writeVertex(Vec2(p1.x - dy, p1.y + dx), uv, color);
   }
}

void Canvas::addBezierCurve(Vec2 pos0, Vec2 cp0, Vec2 cp1, Vec2 pos1, Color col, float thickness /*= 1.0f*/, int numSegments /*= 0*/)
{

}

void Canvas::addRect(const Rect& r, Color col, bool filled /*= false*/, float rounding /*= 0.0f*/, Corners roundedCorners /*= Corners::NONE*/)
{
   if (col.a == 0.0f)
      return;

   pathRect(r, rounding, roundedCorners);
   if (filled == false)
      pathStroke(col, true);
   else
      pathFill(col);
}

void Canvas::addRectMultiColor(const Rect& r, const std::vector<Color>& col, float rounding /*= 0.0f*/, Corners roundedCorners /*= Corners::NONE*/)
{

}

void Canvas::addTriangle(Vec2 a, Vec2 b, Vec2 c, Color col)
{
   if (col.a == 0.0)
      return;

   pathLineTo(a);
   pathLineTo(b);
   pathLineTo(c);
   pathFill(col);
}

void Canvas::addCircle(Vec2 center, float radius, Color col, bool filled /*= false*/, int segments /*= 12*/)
{
   if (col.a == 0.0)
      return;

   float a_max = PI * 2.0f * ((float)segments - 1.0f) / (float)segments;
   pathArcTo(center, radius, 0.0f, a_max, segments);
   if (filled)
      pathFill(col);
   else
      pathStroke(col, true);
}

void Canvas::addConvexPolyFilled(const std::vector<Vec2>& points, Color color)
{

}

void Canvas::addText(const Rect& r, Color col, const std::string& text, Font& f, bool clip /*= true*/, Alignment align /*= Alignment::DEFAULT*/)
{

}

void Canvas::addIcon(const Rect& r, UInt32 iconIndex)
{

}

void Canvas::addImage(const Rect& r, UInt32 tex, Vec2 uvMin /*= Vec2::zero()*/, Vec2 uvMax /*= Vec2::one()*/, Color col /*= Color::white()*/)
{

}

void Canvas::pathFill(Color col)
{

}

void Canvas::pathStroke(Color col, bool closed, float thickness /*= 1.0f*/)
{

}

void Canvas::pathLineTo(Vec2 pos)
{

}

void Canvas::pathArcTo(Vec2 center, float radius, float aMin, float aMax, int numSegments /*= 12*/)
{

}

void Canvas::pathBezierCurveTo(Vec2 p2, Vec2 p3, Vec2 p4, int num_segments /*= 0*/)
{

}

void Canvas::pathRect(const Rect& r, float rounding /*= 0.0f*/, Corners roundedCorners /*= Corners::NONE*/)
{

}

void Canvas::primitiveRect(Vec2 a, Vec2 c, Color col)
{

}

void Canvas::primitiveRectUv(Vec2 a, Vec2 c, Vec2 uva, Vec2 uvc, Color col)
{

}

void Canvas::primitiveText(float size, Vec2 pos, Color col, const std::string& text)
{

}

void Canvas::primitiveVert(Vec2 pos, Vec2 uv, Color col)
{
   writeIndex((UInt16)myVertCount);
   myVertCount++;
   writeVertex(pos, uv, col);
}

void Canvas::writeVertex(Vec2 a, Vec2 uv, Color col)
{
   myVerts[myVertCount].pos = a;
   myVerts[myVertCount].uv = uv;
   myVerts[myVertCount].color = col.toUInt32();
   myVertCount++;
}

void Canvas::writeIndex(UInt16 idx)
{
   myIndexes[myIndexCount] = idx;
   myIndexCount++;

   currentCommand().elementCount++;
}

void Canvas::indexTwoTriangles()
{
   UInt16 startVertex = (UInt16)myVertCount;
   writeIndex(startVertex); writeIndex(startVertex + 1); writeIndex(startVertex + 2);
   writeIndex(startVertex); writeIndex(startVertex + 2); writeIndex(startVertex + 3);

}

DrawCommand& Canvas::addDrawCommand()
{
   DrawCommand cmd;
   cmd.layer = currentLayer();
   cmd.clipRect = currentClipRect();
   cmd.texture = currentTexture();
   cmd.elementOffset = myIndexCount;
   cmd.elementCount = 0;
   myCmdBuffer.push_back(cmd);

   return myCmdBuffer.back();
}

DrawCommand& Canvas::currentCommand()
{
   return myCmdBuffer.back();
}

Rect& Canvas::currentClipRect()
{
   return myClipStack.back();
}

UInt32 Canvas::currentTexture()
{
   return myTextureStatck.back();
}

int Canvas::currentLayer()
{
   return 0;
}

void Canvas::updateClipRect()
{
   DrawCommand& cmd = currentCommand();
   if (cmd.elementCount != 0)
   {
      addDrawCommand();
   }
   else
   {
      Rect& clipRect = currentClipRect();
      //if (myCmdBuffer.size() >= 2 && (myCmdBuffer[myCmdBuffer.size() - 2].clipRect - clipRect).LengthSquared < 0.0001f)
      //   myCmdBuffer.RemoveAt(myCmdBuffer.Count - 1);
      //else
         cmd.clipRect = clipRect;
   }
}

void Canvas::updateTexture()
{
   DrawCommand& cmd = currentCommand();
   UInt32 t = currentTexture();
   if ((cmd.elementCount != 0) && (cmd.texture != t))
   {
      addDrawCommand();
   }
}

void Canvas::updateLayer()
{
   DrawCommand& cmd = currentCommand();
   int layer = currentLayer();
   if ((cmd.elementCount != 0) && (cmd.layer != layer))
   {
      addDrawCommand();
   }
}

void Canvas::pushFullscreenClipRect()
{
   pushClipRect(Rect(0, 0, myWidth, myHeight));
}
