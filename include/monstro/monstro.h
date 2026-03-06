#ifndef MONSTRO_MONSTRO_H
#define MONSTRO_MONSTRO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MonstroContext MonstroContext;
typedef struct MonstroCanvas MonstroCanvas;

void monstro_init(void);
void monstro_shutdown(void);

MonstroContext* monstro_context_create(void);
void monstro_context_destroy(MonstroContext* ctx);

MonstroCanvas* monstro_context_get_canvas(MonstroContext* ctx);

typedef struct MonstroFont MonstroFont;
void monstro_context_set_default_font(MonstroContext* ctx, MonstroFont* font);
MonstroFont* monstro_context_get_default_font(MonstroContext* ctx);

void monstro_begin_frame(MonstroContext* ctx, float display_width, float display_height, float delta_time);
void monstro_end_frame(MonstroContext* ctx);

float monstro_get_delta_time(MonstroContext* ctx);
float monstro_get_time(MonstroContext* ctx);

void monstro_push_id(MonstroContext* ctx, uint64_t id);
void monstro_pop_id(MonstroContext* ctx);
uint64_t monstro_get_id(MonstroContext* ctx, uint64_t id);

uint64_t monstro_hash_string(const char* str);

#define MONSTRO_ID(x) _Generic((x), \
    char*: monstro_hash_string((const char*)(x)), \
    const char*: monstro_hash_string((const char*)(x)), \
    default: (uint64_t)(x) \
)

#ifdef __cplusplus
}
#endif

#endif
