#ifndef MONSTRO_LAYOUT_H
#define MONSTRO_LAYOUT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

MONSTRO_API void monstro_begin_layout(MonstroContext* ctx, MonstroLayoutDirection direction);
MONSTRO_API void monstro_begin_layout_at(MonstroContext* ctx, float x, float y, MonstroLayoutDirection direction);
MONSTRO_API void monstro_end_layout(MonstroContext* ctx);

MONSTRO_API void monstro_add_item(MonstroContext* ctx, float width, float height);

MONSTRO_API void monstro_set_last_item(MonstroContext* ctx, uint64_t id, MonstroRect rect,
                           bool hovered, bool active, bool clicked, bool edited);

MONSTRO_API MonstroVec2 monstro_get_cursor_pos(MonstroContext* ctx);
MONSTRO_API void monstro_set_cursor_pos(MonstroContext* ctx, float x, float y);

MONSTRO_API MonstroVec2 monstro_get_cursor_screen_pos(MonstroContext* ctx);
MONSTRO_API void monstro_set_cursor_screen_pos(MonstroContext* ctx, float x, float y);
MONSTRO_API MonstroVec2 monstro_get_content_region_avail(MonstroContext* ctx);

MONSTRO_API void monstro_indent(MonstroContext* ctx, float indent_width);
MONSTRO_API void monstro_unindent(MonstroContext* ctx, float indent_width);

MONSTRO_API void monstro_spacer(MonstroContext* ctx, float size);
MONSTRO_API void monstro_dummy(MonstroContext* ctx, float width, float height);
MONSTRO_API float monstro_percent(MonstroContext* ctx, float percent, MonstroLayoutDirection direction);

MONSTRO_API void monstro_begin_columns(MonstroContext* ctx, const char* str_id, int count);
MONSTRO_API void monstro_begin_columns_ex(MonstroContext* ctx, const char* str_id, int count, const float* widths);
MONSTRO_API void monstro_next_column(MonstroContext* ctx);
MONSTRO_API void monstro_end_columns(MonstroContext* ctx);

#ifdef __cplusplus
}
#endif

#endif
