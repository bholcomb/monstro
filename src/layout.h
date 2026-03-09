#ifndef MONSTRO_INTERNAL_LAYOUT_H
#define MONSTRO_INTERNAL_LAYOUT_H

#include "monstro.h"

namespace monstro {

struct Layout {
    MonstroLayoutDirection myDirection;
    float myPosX, myPosY;
    float mySizeX, mySizeY;
    float myCursorX, myCursorY;
    float myMaxX, myMaxY;
    float myScrollOffsetX = 0.0f, myScrollOffsetY = 0.0f;

    void init(MonstroLayoutDirection direction, float posX, float posY);
    void addItem(float width, float height);
    void nextLine();
};

} // namespace monstro

#endif
