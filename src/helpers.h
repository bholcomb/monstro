#ifndef MONSTRO_INTERNAL_HELPERS_H
#define MONSTRO_INTERNAL_HELPERS_H

#include <stdint.h>
#include <stddef.h>
#include "monstro/types.h"

uint64_t monstro_hash_data(const void* data, size_t size, uint64_t seed);

void monstro_add_item(MonstroContext* ctx, float width, float height);

void monstro_set_last_item(MonstroContext* ctx, uint64_t id, MonstroRect rect,
                           bool hovered, bool active, bool clicked, bool edited);

void monstro_draw_disabled_overlay(MonstroContext* ctx, MonstroRect rect);

const char* monstro_display_text(const char* label);

#endif
