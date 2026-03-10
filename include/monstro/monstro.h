#ifndef MONSTRO_MONSTRO_H
#define MONSTRO_MONSTRO_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void monstro_init(void);
void monstro_shutdown(void);

MonstroContext* monstro_context_create(void);
void monstro_context_destroy(MonstroContext* ctx);

MonstroDrawBuffer* monstro_context_get_draw_buffer(MonstroContext* ctx);

void monstro_context_get_draw_commands(MonstroContext* ctx, const MonstroDrawCommand** out_commands, uint32_t* out_count);
void monstro_draw_buffer_get_vertex_buffer(MonstroDrawBuffer* buf, const MonstroVertex** out_data, size_t* out_byte_size);
void monstro_draw_buffer_get_index_buffer(MonstroDrawBuffer* buf, const uint32_t** out_data, uint32_t* out_count);
void monstro_draw_buffer_set_vertex_buffer(MonstroDrawBuffer* buf, MonstroVertex* buffer, size_t capacity_in_vertices);
void monstro_draw_buffer_set_index_buffer(MonstroDrawBuffer* buf, uint32_t* buffer, size_t capacity_in_indices);

void monstro_context_set_default_font(MonstroContext* ctx, MonstroFont* font);
MonstroFont* monstro_context_get_default_font(MonstroContext* ctx);

void monstro_set_display_size(MonstroContext* ctx, float width, float height);
void monstro_get_display_size(MonstroContext* ctx, float* out_width, float* out_height);

void monstro_begin_frame(MonstroContext* ctx, float delta_time);
void monstro_end_frame(MonstroContext* ctx);

float monstro_get_delta_time(MonstroContext* ctx);
float monstro_get_time(MonstroContext* ctx);

void monstro_push_id(MonstroContext* ctx, const char* name);
void monstro_pop_id(MonstroContext* ctx);
uint64_t monstro_get_id(MonstroContext* ctx, const char* name);

#ifdef __cplusplus
}
#endif

#endif
