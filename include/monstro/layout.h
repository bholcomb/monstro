#ifndef MONSTRO_LAYOUT_H
#define MONSTRO_LAYOUT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void monstro_begin_layout(MonstroContext* ctx, MonstroLayoutDirection direction);
void monstro_begin_layout_at(MonstroContext* ctx, float x, float y, MonstroLayoutDirection direction);
void monstro_end_layout(MonstroContext* ctx);

void monstro_add_item(MonstroContext* ctx, float width, float height);

void monstro_set_last_item(MonstroContext* ctx, uint64_t id, MonstroRect rect,
                           bool hovered, bool active, bool clicked, bool edited);

MonstroVec2 monstro_get_cursor_pos(MonstroContext* ctx);
void monstro_set_cursor_pos(MonstroContext* ctx, float x, float y);

MonstroVec2 monstro_get_cursor_screen_pos(MonstroContext* ctx);
void monstro_set_cursor_screen_pos(MonstroContext* ctx, float x, float y);

void monstro_indent(MonstroContext* ctx, float indent_width);
void monstro_unindent(MonstroContext* ctx, float indent_width);

void monstro_spacer(MonstroContext* ctx, float size);
void monstro_dummy(MonstroContext* ctx, float width, float height);
void monstro_same_line(MonstroContext* ctx, float offset_x, float spacing);
float monstro_percent(MonstroContext* ctx, float percent, MonstroLayoutDirection direction);

void monstro_begin_columns(MonstroContext* ctx, const char* str_id, int count);
void monstro_begin_columns_ex(MonstroContext* ctx, const char* str_id, int count, const float* widths);
void monstro_next_column(MonstroContext* ctx);
void monstro_end_columns(MonstroContext* ctx);

#ifdef __cplusplus
}
#endif

#endif
