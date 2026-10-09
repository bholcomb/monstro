#ifndef MONSTRO_ICONS_H
#define MONSTRO_ICONS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t MonstroIconId;

/* Icons belong to the environment and are shared by every context. Register
   them between frames; registering an ID again replaces it, and running out of
   icon slots (MonstroEnvConfig.max_icons) stops the process. */

/* Register a grid-based sprite sheet atlas.
   Icons are assigned IDs starting at first_id, numbered sequentially:
   first_id, first_id+1, ..., first_id + (cols * rows - 1).
   Icons are laid out left-to-right, top-to-bottom. */
void monstro_register_icon_atlas(MonstroEnv* env, MonstroIconId first_id,
                                 uint64_t texture_id,
                                 int icon_width, int icon_height,
                                 int atlas_width, int atlas_height,
                                 int cols, int rows);

/* Register a single icon with an explicit ID, texture, and UVs. */
void monstro_register_icon(MonstroEnv* env, MonstroIconId id,
                           uint64_t texture_id,
                           MonstroVec2 uv_min, MonstroVec2 uv_max);

/* Register a range of icon font glyphs as monochrome icons.
   Each glyph's icon ID is its codepoint (code points the font lacks are skipped):
   first_codepoint, first_codepoint+1, ..., first_codepoint + count - 1.
   They live in the font atlas, and are tinted with the current text color. */
void monstro_register_icon_font(MonstroEnv* env, MonstroFont* font,
                                uint32_t first_codepoint, int count);

/* Look up an icon's texture and UVs by ID. Returns false if no icon has the ID. */
bool monstro_get_icon(MonstroEnv* env, MonstroIconId id,
                      uint64_t* out_texture_id,
                      MonstroVec2* out_uv_min, MonstroVec2* out_uv_max);

/* Standalone icon widget. Draws the icon at the given size. */
void monstro_icon(MonstroContext* ctx, MonstroIconId icon_id, float size);

/* Write "{icon:ID}" into buf. Returns buf for convenience.
   Use with snprintf/strcat to embed icons in label strings. */
const char* monstro_icon_str(MonstroIconId id, char* buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif
