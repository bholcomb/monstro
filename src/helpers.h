#ifndef MONSTRO_INTERNAL_HELPERS_H
#define MONSTRO_INTERNAL_HELPERS_H

#include "monstro/types.h"
#include <stddef.h>
#include <stdint.h>

void monstro_add_item(MonstroContext *ctx, float width, float height);

void monstro_set_last_item(MonstroContext *ctx, uint64_t id, MonstroRect rect, bool hovered, bool active, bool clicked,
                           bool edited);

void monstro_draw_disabled_overlay(MonstroContext *ctx, MonstroRect rect);

/*
 * A label's text before ## is its ID, the text after it is shown: "speed##Speed: 12.3" shows "Speed: 12.3" and
 * keeps the ID "speed" as the number changes. Without ##, the whole label is both.
 */
const char *monstro_display_text(const char *label);
size_t monstro_id_text_length(const char *label);
void monstro_copy_id_text(const char *label, char *out, size_t out_size);

#define MONSTRO_REPLACEMENT_CHARACTER 0xFFFDu

/*
 * UTF-8. Text is decoded one code point at a time; invalid or truncated
 * sequences decode as U+FFFD and decoding never reads past the terminator.
 * Byte offsets used for editing and truncation always sit on code point
 * boundaries.
 */
uint32_t decode_utf8(const char **str);
// Bytes in the longest prefix of whole code points that fits in max_bytes.
size_t monstro_utf8_fit(const char *text, size_t max_bytes);
// Copies whole code points into out (out_size >= 1) and terminates it.
void monstro_utf8_copy(char *out, size_t out_size, const char *text);
// Drops a code point cut off by the end of text, as truncating snprintf leaves it.
void monstro_utf8_trim_partial(char *text);
int monstro_utf8_next(const char *text, int pos);
int monstro_utf8_prev(const char *text, int pos);
// Moves pos back to the start of the code point it falls in.
int monstro_utf8_snap(const char *text, int pos);
// Code points in the first `bytes` bytes.
int monstro_utf8_count(const char *text, int bytes);
bool parse_icon_tag(const char **str, uint32_t *out_icon_id);

#endif
