#ifndef MONSTRO_INTERNAL_HELPERS_H
#define MONSTRO_INTERNAL_HELPERS_H

#include <stdint.h>
#include <stddef.h>

struct MonstroContext;

uint64_t monstro_hash_combine(uint64_t seed, uint64_t value);
uint64_t monstro_hash_data(const void* data, size_t size, uint64_t seed);

void monstro_add_item(MonstroContext* ctx, float width, float height);

#endif
