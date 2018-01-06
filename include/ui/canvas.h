/*-----------------------------------------------------------------------------
Copyright 2017
Bob Holcomb
-----------------------------------------------------------------------------*/

#pragma once

#include "ui/export.h"

#include "ui/util.h"
#include "ui/style.h"

#include <string>
#include <vector>

namespace UI
{
   const int MAX_VERTS = 65536;
   const int MAX_ELEMENTS = 65536 * 3;

   class Font;

   enum class Corners {
      NONE = 0,
      LL = 1 << 0,
      LR = 1 << 1, 
      UR = 1 << 2,
      UL = 1 << 3,
      TOP = UL | UR,
      BOTTOM = LL | LR,
      LEFT = LL | UL,
      RIGHT = LR | UR,
      ALL = LL | LR | UL | UR
   };

   struct UI_DLL DrawCommand
   {
      int layer;
      int id;
      UInt32 elementOffset;
      UInt32 elementCount;
      Rect clipRect;
      UInt32 texture;

      DrawCommand()
      {
         layer = 0; 
         elementOffset = 0;
         elementCount = 0;
         clipRect = Rect(0, 0, 8192, 8192);
         texture = -1;
      }
   };

   class UI_DLL Canvas
   {
   public:
      Canvas(int width, int height);
      virtual ~Canvas();

      virtual void reset();

      virtual void pushDrawLayer(int layer);
      virtual void popDrawLayer();

      virtual void pushClipRect(Rect r);
      virtual void popClipRect();
      virtual void pushFullscreenClipRect();

      virtual void pushTexture(UInt32 tex);
      virtual void popTexture();

      virtual void addLine(Vec2 a, Vec2 b, Color col, float thickness = 1.0f);
      virtual void addPolyLine(const std::vector<Vec2>& points, Color color, bool closed = false, float thickness = 1.0f);
      virtual void addBezierCurve(Vec2 pos0, Vec2 cp0, Vec2 cp1, Vec2 pos1, Color col, float thickness = 1.0f, int numSegments = 0);
      virtual void addRect(const Rect& r, Color col, bool filled = false, float rounding = 0.0f, Corners roundedCorners = Corners::NONE);
      virtual void addRectMultiColor(const Rect& r, const std::vector<Color>& col, float rounding = 0.0f, Corners roundedCorners = Corners::NONE);
      virtual void addTriangle(Vec2 a, Vec2 b, Vec2 c, Color col);
      virtual void addCircle(Vec2 center, float radius, Color col, bool filled = false, int segments = 12);
      virtual void addConvexPolyFilled(const std::vector<Vec2>& points, Color color);
      virtual void addText(const Rect& r, Color col, const std::string& text, Font& f, bool clip = true, Alignment align = Alignment::DEFAULT);
      virtual void addIcon(const Rect& r, UInt32 iconIndex);
      virtual void addImage(const Rect& r, UInt32 tex, Vec2 uvMin = Vec2::zero(), Vec2 uvMax = Vec2::one(), Color col = Color::white());


      virtual void pathFill(Color col);
      virtual void pathStroke(Color col, bool closed, float thickness = 1.0f);
      virtual void pathLineTo(Vec2 pos);
      virtual void pathArcTo(Vec2 center, float radius, float aMin, float aMax, int numSegments = 12);
      virtual void pathBezierCurveTo(Vec2 p2, Vec2 p3, Vec2 p4, int num_segments = 0);
      virtual void pathRect(const Rect& r, float rounding = 0.0f, Corners roundedCorners = Corners::NONE);

   protected:      
      virtual void primativeRect(Vec2 a, Vec2 c, Color col);
      virtual void primativeRectUv(Vec2 a, Vec2 c, Vec2 uva, Vec2 uvc, Color col);
      virtual void primativeText(float size, Vec2 pos, Color col, const std::string& text);
      virtual void primativeVert(Vec2 pos, Vec2 uv, Color col);

      virtual void writeVertex(Vec2 a, Vec2 uv, Color col);
      virtual void writeIndex(UInt16 idx);
      virtual void indexTwoTriangles();

      virtual DrawCommand& addDrawCommand();
      virtual DrawCommand& currentCommand();

      virtual Rect& currentClipRect();
      virtual UInt32 currentTexture();
      virtual int currentLayer();

      virtual void updateClipRect();
      virtual void updateTexture();
      virtual void updateLayer();


   protected:
      int myWidth;
      int myHeight;
      
      Vert* myVerts;
      UInt16* myIndexes;
      UInt32 myVertCount;
      UInt32 myIndexCount;

      std::vector<DrawCommand> myCmdBuffer;
      std::vector<Rect> myClipStack;
      std::vector<UInt32> myTextureStatck;
      std::vector<int> myLayerStack;
      std::vector<Vec2> myPath;
   };
}