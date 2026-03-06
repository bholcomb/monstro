#ifndef MONSTRO_LAYOUT_H
#define MONSTRO_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MonstroContext MonstroContext;

typedef enum MonstroLayoutDirection {
    MONSTRO_LAYOUT_HORIZONTAL = 0,
    MONSTRO_LAYOUT_VERTICAL = 1
} MonstroLayoutDirection;

void monstro_begin_layout(MonstroContext* ctx, MonstroLayoutDirection direction);
void monstro_begin_layout_at(MonstroContext* ctx, float x, float y, MonstroLayoutDirection direction);
void monstro_end_layout(MonstroContext* ctx);

void monstro_add_item(MonstroContext* ctx, float width, float height);

MonstroVec2 monstro_get_cursor_pos(MonstroContext* ctx);
void monstro_set_cursor_pos(MonstroContext* ctx, float x, float y);

MonstroVec2 monstro_get_cursor_screen_pos(MonstroContext* ctx);
void monstro_set_cursor_screen_pos(MonstroContext* ctx, float x, float y);

void monstro_indent(MonstroContext* ctx, float indent_width);
void monstro_unindent(MonstroContext* ctx, float indent_width);

void monstro_spacer(MonstroContext* ctx, float size);
float monstro_percent(MonstroContext* ctx, float percent, MonstroLayoutDirection direction);

MonstroRect monstro_get_item_rect(MonstroContext* ctx);

#ifdef __cplusplus
}
#endif

#endif
