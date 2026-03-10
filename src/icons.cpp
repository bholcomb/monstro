#include "context.h"
#include <cstdio>
#include <cstring>

extern "C"
{

bool monstro_register_icon_atlas(MonstroContext *ctx, MonstroIconId first_id, uint64_t texture_id, int icon_width,
                                 int icon_height, int atlas_width, int atlas_height, int cols, int rows)
{
   if (!ctx || first_id == 0 || cols <= 0 || rows <= 0 || atlas_width <= 0 || atlas_height <= 0)
      return false;

   float inv_w = 1.0f / (float)atlas_width;
   float inv_h = 1.0f / (float)atlas_height;

   MonstroIconId id = first_id;
   for (int row = 0; row < rows; row++)
   {
      for (int col = 0; col < cols; col++)
      {
         MonstroIconEntry entry;
         entry.texture_id = texture_id;
         entry.uv_min.x = (float)(col * icon_width) * inv_w;
         entry.uv_min.y = (float)(row * icon_height) * inv_h;
         entry.uv_max.x = (float)((col + 1) * icon_width) * inv_w;
         entry.uv_max.y = (float)((row + 1) * icon_height) * inv_h;
         entry.is_monochrome = false;
         ctx->icon_map[id++] = entry;
      }
   }

   return true;
}

bool monstro_register_icon(MonstroContext *ctx, MonstroIconId id, uint64_t texture_id, MonstroVec2 uv_min,
                           MonstroVec2 uv_max)
{
   if (!ctx || id == 0)
      return false;

   MonstroIconEntry entry;
   entry.texture_id = texture_id;
   entry.uv_min = uv_min;
   entry.uv_max = uv_max;
   entry.is_monochrome = false;
   ctx->icon_map[id] = entry;
   return true;
}

bool monstro_register_icon_font(MonstroContext *ctx, MonstroFont *font, uint32_t first_codepoint, int count)
{
   if (!ctx || !font || count <= 0)
      return false;

   uint64_t texture_id = monstro_font_get_atlas_texture(font);

   for (int i = 0; i < count; i++)
   {
      uint32_t cp = first_codepoint + (uint32_t)i;
      MonstroGlyph glyph;
      if (!monstro_font_get_glyph(font, cp, &glyph))
         continue;

      MonstroIconEntry entry;
      entry.texture_id = texture_id;
      entry.uv_min = {glyph.min_tex_coord[0], glyph.min_tex_coord[1]};
      entry.uv_max = {glyph.max_tex_coord[0], glyph.max_tex_coord[1]};
      entry.is_monochrome = true;
      ctx->icon_map[cp] = entry;
   }

   return true;
}

bool monstro_get_icon(MonstroContext *ctx, MonstroIconId id, uint64_t *out_texture_id, MonstroVec2 *out_uv_min,
                      MonstroVec2 *out_uv_max)
{
   if (!ctx || id == 0)
      return false;

   auto it = ctx->icon_map.find(id);
   if (it == ctx->icon_map.end())
      return false;

   const MonstroIconEntry &entry = it->second;
   if (out_texture_id)
      *out_texture_id = entry.texture_id;
   if (out_uv_min)
      *out_uv_min = entry.uv_min;
   if (out_uv_max)
      *out_uv_max = entry.uv_max;
   return true;
}

void monstro_icon(MonstroContext *ctx, MonstroIconId icon_id, float size)
{
   if (!ctx || !ctx->current_window || icon_id == 0)
      return;

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return;

   auto it = ctx->icon_map.find(icon_id);
   if (it == ctx->icon_map.end())
      return;

   const MonstroIconEntry &entry = it->second;
   monstro::Canvas *canvas = &win->canvas;
   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);

   MonstroRect rect = {pos.x, pos.y, size, size};
   canvas->addImage(entry.texture_id, rect, entry.uv_min, entry.uv_max, 0xFFFFFFFF);
   monstro_draw_disabled_overlay(ctx, rect);

   monstro_add_item(ctx, size, size);
   uint64_t wid = win->getChildId("##icon");
   monstro_set_last_item(ctx, wid, rect, false, false, false, false);
}

const char *monstro_icon_str(MonstroIconId id, char *buf, size_t buf_size)
{
   if (!buf || buf_size == 0)
      return "";
   snprintf(buf, buf_size, "{icon:%u}", (unsigned)id);
   return buf;
}

} // extern "C"
