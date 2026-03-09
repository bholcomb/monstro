#ifndef MONSTRO_INTERNAL_HELPERS_H
#define MONSTRO_INTERNAL_HELPERS_H

#include <stdint.h>
#include <stddef.h>
#include "monstro/types.h"

uint64_t monstro_hash_data(const void* data, size_t size, uint64_t seed);

void monstro_add_item(MonstroContext* ctx, float width, float height);

// Returns the display portion of a label string.
// Format: "id##Display Text" returns "Display Text".
// If no "##" is found, returns the original string unchanged.
const char* monstro_display_text(const char* label);

#endif
