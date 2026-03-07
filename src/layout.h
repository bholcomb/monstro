#ifndef MONSTRO_INTERNAL_LAYOUT_H
#define MONSTRO_INTERNAL_LAYOUT_H

#include "monstro.h"

struct MonstroLayoutInternal {
    MonstroLayoutDirection direction;
    float pos_x, pos_y;
    float size_x, size_y;
    float cursor_x, cursor_y;
    float max_x, max_y;
};

#endif
