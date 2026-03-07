#ifndef MONSTRO_INTERNAL_CANVAS_H
#define MONSTRO_INTERNAL_CANVAS_H

#include "monstro.h"
#include "constants.h"

struct MonstroCanvas {
    MonstroVertex* vertices = nullptr;
    uint32_t* indices = nullptr;
    uint32_t vertex_count = 0;
    uint32_t index_count = 0;
    size_t vertex_capacity = 0;
    size_t index_capacity = 0;
    bool user_provided_vertex_buffer = false;
    bool user_provided_index_buffer = false;

    MonstroDrawCommand draw_commands[MONSTRO_MAX_DRAW_COMMANDS];
    uint32_t draw_command_count = 0;

    MonstroRect clip_rect_stack[MONSTRO_MAX_CLIP_RECT_STACK];
    int clip_rect_stack_top = -1;

    uint64_t texture_stack[MONSTRO_MAX_TEXTURE_STACK];
    int texture_stack_top = -1;

    int layer_stack[MONSTRO_MAX_LAYER_STACK];
    int layer_stack_top = -1;

    MonstroVec2 path[MONSTRO_MAX_PATH_POINTS];
    int path_count = 0;

    bool dirty = false;
    uint64_t default_texture_id = 0;
};

#endif
