#ifndef MONSTRO_INTERNAL_LAYOUT_H
#define MONSTRO_INTERNAL_LAYOUT_H

#include "monstro.h"

namespace monstro
{

struct Layout
{
   MonstroLayoutDirection myDirection;
   float myPosX, myPosY;
   float mySizeX, mySizeY;
   float myCursorX, myCursorY;
   float myMaxX, myMaxY;
   float myScrollOffsetX = 0.0f, myScrollOffsetY = 0.0f;
   float myAvailableWidth = 0.0f, myAvailableHeight = 0.0f;
   float myVisibleMaxScreenX = 0.0f, myVisibleMaxScreenY = 0.0f;
   float myLastItemX = 0.0f, myLastItemY = 0.0f;
   float myLastItemW = 0.0f, myLastItemH = 0.0f;

   void init(MonstroLayoutDirection direction, float posX, float posY);
   void addItem(float width, float height);
};

} // namespace monstro

#endif
