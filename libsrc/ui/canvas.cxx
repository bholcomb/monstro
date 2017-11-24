/*-----------------------------------------------------------------------------
Copyright 2017
Bob Holcomb
-----------------------------------------------------------------------------*/

#include "ui/canvas.h"

using namespace UI;

const int MAX_VERTS = 65536;
const int MAX_ELEMENTS = 65536 * 4;

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