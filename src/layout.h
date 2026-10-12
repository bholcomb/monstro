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
   // The content region's right edge, in the window's unscrolled coordinates (those of myPosX + myCursorX).
   float myContentRight = 0.0f;
   float myAvailableHeight = 0.0f;
   float myVisibleMaxScreenY = 0.0f;
   float myLastItemX = 0.0f, myLastItemY = 0.0f;
   float myLastItemW = 0.0f, myLastItemH = 0.0f;

   void init(MonstroLayoutDirection direction, float posX, float posY);
   void addItem(float width, float height);
   // The room from the cursor to the content region's right edge: the width a widget that fills the width takes,
   // wherever it starts (a padding in, under an indent, after other items in a row). 0 when there is none.
   float fillWidth() const;
};

} // namespace monstro

#endif
