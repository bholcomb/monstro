#include "context.h"
#include "env.h"
#include <cstdio>
#include <cstring>

using monstro::Env;
using monstro::IconEntry;
using monstro::requireEnv;

extern "C"
{

void monstro_register_icon_atlas(MonstroEnv *env, MonstroIconId first_id, uint64_t texture_id, int icon_width,
                                 int icon_height, int atlas_width, int atlas_height, int cols, int rows)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   MONSTRO_REQUIRE(first_id != 0, "%s: icon ID 0 is reserved", __func__);
   MONSTRO_REQUIRE(cols > 0 && rows > 0 && icon_width > 0 && icon_height > 0 && atlas_width > 0 && atlas_height > 0,
                   "%s: a %d x %d grid of %d x %d icons in a %d x %d atlas", __func__, cols, rows, icon_width,
                   icon_height, atlas_width, atlas_height);

   float inv_w = 1.0f / (float)atlas_width;
   float inv_h = 1.0f / (float)atlas_height;

   MonstroIconId id = first_id;
   for (int row = 0; row < rows; row++)
   {
      for (int col = 0; col < cols; col++)
      {
         IconEntry entry;
         entry.id = id++;
         entry.texture_id = texture_id;
         entry.uv_min = {(float)(col * icon_width) * inv_w, (float)(row * icon_height) * inv_h};
         entry.uv_max = {(float)((col + 1) * icon_width) * inv_w, (float)((row + 1) * icon_height) * inv_h};
         env->icons.put(entry);
      }
   }
}

void monstro_register_icon(MonstroEnv *env, MonstroIconId id, uint64_t texture_id, MonstroVec2 uv_min,
                           MonstroVec2 uv_max)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   MONSTRO_REQUIRE(id != 0, "%s: icon ID 0 is reserved", __func__);

   IconEntry entry;
   entry.id = id;
   entry.texture_id = texture_id;
   entry.uv_min = uv_min;
   entry.uv_max = uv_max;
   env->icons.put(entry);
}

void monstro_register_icon_font(MonstroEnv *env, MonstroFont *font, uint32_t first_codepoint, int count)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   MONSTRO_REQUIRE(font, "%s: the font must not be null", __func__);
   MONSTRO_REQUIRE(first_codepoint != 0 && count > 0, "%s: %d code points from %u", __func__, count, first_codepoint);

   for (int i = 0; i < count; i++)
   {
      uint32_t cp = first_codepoint + (uint32_t)i;
      MonstroGlyph glyph;
      if (!monstro_font_get_glyph(font, cp, &glyph))
         continue;

      IconEntry entry;
      entry.id = cp;
      entry.in_atlas = true;
      entry.uv_min = {glyph.min_tex_coord[0], glyph.min_tex_coord[1]};
      entry.uv_max = {glyph.max_tex_coord[0], glyph.max_tex_coord[1]};
      entry.is_monochrome = true;
      env->icons.put(entry);
   }
}

bool monstro_get_icon(MonstroEnv *env, MonstroIconId id, uint64_t *out_texture_id, MonstroVec2 *out_uv_min,
                      MonstroVec2 *out_uv_max)
{
   requireEnv(env, __func__);
   const IconEntry *entry = id ? env->icons.find(id) : nullptr;
   if (!entry)
      return false;
   if (out_texture_id)
      *out_texture_id = entry->in_atlas ? env->atlas.texture_id : entry->texture_id;
   if (out_uv_min)
      *out_uv_min = entry->uv_min;
   if (out_uv_max)
      *out_uv_max = entry->uv_max;
   return true;
}

void monstro_icon(MonstroContext *ctx, MonstroIconId icon_id, float size)
{
   monstro::Window *win = monstro::requireWindow(ctx, __func__);
   if (icon_id == 0 || win->skip_items)
      return;

   Env *env = ctx->env;
   const IconEntry *entry = env->icons.find(icon_id);
   if (!entry)
      return;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   MonstroRect rect = {pos.x, pos.y, size, size};
   uint32_t tint = entry->is_monochrome ? monstro_color_to_u32(ctx->style().text.color) : 0xFFFFFFFF;
   ctx->canvas->addImage(entry->in_atlas ? env->atlas.texture_id : entry->texture_id, rect, entry->uv_min,
                         entry->uv_max, tint);
   monstro_draw_disabled_overlay(ctx, rect);

   monstro_add_item(ctx, size, size);
   uint64_t wid = win->getChildId("##icon");
   monstro_set_last_item(ctx, wid, rect, false, false, false, false);
}

const char *monstro_icon_str(MonstroIconId id, char *buf, size_t buf_size)
{
   MONSTRO_REQUIRE(buf && buf_size > 0, "%s: the buffer must not be null or empty", __func__);
   snprintf(buf, buf_size, "{icon:%u}", (unsigned)id);
   return buf;
}

} // extern "C"
