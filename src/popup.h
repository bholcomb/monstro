#ifndef MONSTRO_INTERNAL_POPUP_H
#define MONSTRO_INTERNAL_POPUP_H

#include <stdint.h>

namespace monstro { struct Window; }

struct MonstroPopupInternal {
    uint64_t popup_id;
    monstro::Window* parent_window;
    uint64_t parent_id;
    float open_pos_x, open_pos_y;
    int open_frame;
};

#endif
