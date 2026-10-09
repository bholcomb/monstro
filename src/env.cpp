#include "env.h"
#include "context.h"
#include <cstdlib>
#include <cstring>

namespace monstro
{

/* ------------------------------------------------------------------------- */
/* Icon table                                                                */
/* ------------------------------------------------------------------------- */

IconTable::~IconTable() { delete[] slots; }

bool IconTable::init(uint32_t max_icons)
{
   // At most half full, so probes stay short.
   uint32_t slot_count = 1;
   while (slot_count < max_icons * 2)
      slot_count *= 2;
   slots = new (std::nothrow) IconEntry[slot_count];
   if (!slots)
      return false;
   slot_mask = slot_count - 1;
   capacity = max_icons;
   return true;
}

static uint32_t icon_slot(uint32_t id, uint32_t mask) { return (id * 2654435761u) & mask; }

void IconTable::put(const IconEntry &entry)
{
   uint32_t slot = icon_slot(entry.id, slot_mask);
   while (slots[slot].id != 0 && slots[slot].id != entry.id)
      slot = (slot + 1) & slot_mask;
   if (slots[slot].id == 0)
   {
      MONSTRO_REQUIRE(count < capacity, "icon table full: capacity %u, needed %u; raise MonstroEnvConfig.max_icons",
                      capacity, count + 1);
      count++;
   }
   slots[slot] = entry;
}

const IconEntry *IconTable::find(uint32_t id) const
{
   uint32_t slot = icon_slot(id, slot_mask);
   while (slots[slot].id != 0)
   {
      if (slots[slot].id == id)
         return &slots[slot];
      slot = (slot + 1) & slot_mask;
   }
   return nullptr;
}

/* ------------------------------------------------------------------------- */
/* Atlas                                                                     */
/* ------------------------------------------------------------------------- */

Atlas::~Atlas() { std::free(pixels); }

bool Atlas::init(uint32_t atlas_width, uint32_t atlas_height)
{
   pixels = (uint8_t *)std::malloc((size_t)atlas_width * atlas_height * 4);
   if (!pixels)
      return false;
   width = atlas_width;
   height = atlas_height;
   // Transparent white, so filtering at glyph edges blends toward white rather than black.
   for (size_t i = 0; i < (size_t)width * height; i++)
   {
      pixels[i * 4 + 0] = 255;
      pixels[i * 4 + 1] = 255;
      pixels[i * 4 + 2] = 255;
      pixels[i * 4 + 3] = 0;
   }

   // A 2 x 2 opaque white block; its centre samples pure white even with filtering.
   uint32_t x, y;
   allocate(2, 2, "the white texel", &x, &y);
   for (uint32_t dy = 0; dy < 2; dy++)
      for (uint32_t dx = 0; dx < 2; dx++)
         pixel(x + dx, y + dy)[3] = 255;
   white_uv = {(float)(x + 1) / (float)width, (float)(y + 1) / (float)height};
   version = 1;
   return true;
}

void Atlas::allocate(uint32_t w, uint32_t h, const char *what, uint32_t *out_x, uint32_t *out_y)
{
   // One pixel of padding to the right and below keeps filtering from bleeding between images.
   uint32_t padded_w = w + 1;
   uint32_t padded_h = h + 1;
   if (shelf_x + padded_w > width)
   {
      shelf_y += shelf_height;
      shelf_x = 0;
      shelf_height = 0;
   }
   MONSTRO_REQUIRE(padded_w <= width && shelf_y + padded_h <= height,
                   "font atlas full: capacity %u x %u pixels, needed a %u x %u image at row %u for %s; raise "
                   "MonstroEnvConfig.atlas_width and atlas_height or load fewer glyphs",
                   width, height, w, h, shelf_y, what);
   *out_x = shelf_x;
   *out_y = shelf_y;
   shelf_x += padded_w;
   if (padded_h > shelf_height)
      shelf_height = padded_h;
}

/* ------------------------------------------------------------------------- */
/* Environment                                                               */
/* ------------------------------------------------------------------------- */

Env *requireEnv(Env *env, const char *function)
{
   MONSTRO_REQUIRE(env, "%s: the environment must not be null", function);
   return env;
}

void requireVertexLayout(const MonstroVertexLayout &layout)
{
   MONSTRO_REQUIRE(layout.buffer_count >= 1 && layout.buffer_count <= MONSTRO_MAX_VERTEX_BUFFERS,
                   "vertex layout: buffer_count %u is outside 1..%d", layout.buffer_count, MONSTRO_MAX_VERTEX_BUFFERS);
   struct Attribute
   {
      const char *name;
      MonstroVertexAttribute place;
      uint32_t size;
   };
   const Attribute attributes[3] = {
       {"position", layout.position, 8}, {"texcoord", layout.texcoord, 8}, {"color", layout.color, 4}};
   for (const Attribute &a : attributes)
   {
      MONSTRO_REQUIRE(a.place.buffer < layout.buffer_count,
                      "vertex layout: %s is in buffer %u but the layout has %u buffers", a.name, a.place.buffer,
                      layout.buffer_count);
      uint32_t stride = layout.stride[a.place.buffer];
      MONSTRO_REQUIRE((uint64_t)a.place.offset + a.size <= stride,
                      "vertex layout: %s (%u bytes at offset %u) does not fit buffer %u's stride of %u bytes", a.name,
                      a.size, a.place.offset, a.place.buffer, stride);
   }
   for (int i = 0; i < 3; i++)
   {
      for (int j = i + 1; j < 3; j++)
      {
         const Attribute &a = attributes[i];
         const Attribute &b = attributes[j];
         bool overlap = a.place.buffer == b.place.buffer && a.place.offset < b.place.offset + b.size &&
                        b.place.offset < a.place.offset + a.size;
         MONSTRO_REQUIRE(!overlap, "vertex layout: %s (offset %u) and %s (offset %u) overlap in buffer %u", a.name,
                         a.place.offset, b.name, b.place.offset, a.place.buffer);
      }
   }
   for (uint32_t buffer = 0; buffer < layout.buffer_count; buffer++)
   {
      bool used = false;
      for (const Attribute &a : attributes)
         used = used || a.place.buffer == buffer;
      MONSTRO_REQUIRE(used, "vertex layout: buffer %u holds no attribute; lower buffer_count", buffer);
   }
}

Env::Env(const MonstroEnvConfig &env_config) : config(env_config)
{
   settings.double_click_time = 0.30f;
   settings.double_click_distance = 6.0f;
   settings.drag_threshold = 6.0f;
   settings.key_repeat_delay = 0.25f;
   settings.key_repeat_rate = 0.05f;
   settings.tooltip_delay = 0.5f;
}

Env::~Env()
{
   for (MonstroFont &font : fonts)
      std::free(font.glyphs);
   delete[] canvases;
}

bool Env::init()
{
   if (!atlas.init(config.atlas_width, config.atlas_height) || !fonts.init(config.max_fonts) ||
       !themes.init(config.max_themes) || !icons.init(config.max_icons))
      return false;

   canvases = new (std::nothrow) Canvas[config.canvas_count];
   if (!canvases)
      return false;
   for (uint32_t i = 0; i < config.canvas_count; i++)
   {
      if (!canvases[i].init(this, (int)i, config))
         return false;
   }
   return true;
}

void Env::requireIdle(const char *function) const
{
   MONSTRO_REQUIRE(open_frames == 0,
                   "%s: the environment is read-only while a frame is open (%d open); call it between frames", function,
                   open_frames);
}

Canvas *Env::lendCanvas(Context *ctx, int frame_number, const MonstroFrameBuffers *buffers)
{
   // Canvases are lent in turn, so a frame's draw data outlives the next canvas_count - 1 frames.
   for (uint32_t i = 0; i < config.canvas_count; i++)
   {
      uint32_t candidate = (next_canvas + i) % config.canvas_count;
      Canvas *canvas = &canvases[candidate];
      if (!canvas->borrower)
      {
         next_canvas = (candidate + 1) % config.canvas_count;
         canvas->lend(ctx, frame_number, buffers);
         canvas->frame_id = ++frames_lent;
         open_frames++;
         return canvas;
      }
   }
   MONSTRO_FATAL("all %u canvases are lent: capacity %u, needed %u open frames; end a frame first or raise "
                 "MonstroEnvConfig.canvas_count",
                 config.canvas_count, config.canvas_count, config.canvas_count + 1);
}

void Env::returnCanvas(Canvas *canvas)
{
   canvas->release();
   open_frames--;
}

MonstroFont *Env::addFont(const char *name, const char *function)
{
   MONSTRO_REQUIRE(name, "%s: the font name must not be null", function);
   MonstroFont *font = &fonts.add();
   monstro_utf8_copy(font->name, sizeof(font->name), name);
   if (!default_font)
      default_font = font;
   return font;
}

Theme *Env::findTheme(const char *name)
{
   for (Theme &theme : themes)
   {
      if (std::strcmp(theme.name, name) == 0)
         return &theme;
   }
   return nullptr;
}

size_t Env::residentBytes() const
{
   size_t bytes = sizeof(Env) + (size_t)atlas.width * atlas.height * 4 + fonts.bytes() + themes.bytes() +
                  sizeof(IconEntry) * (icons.slot_mask + 1);
   for (int i = 0; i < fonts.size(); i++)
      bytes += sizeof(MonstroGlyph) * fonts[i].glyph_count;
   for (uint32_t i = 0; i < config.canvas_count; i++)
      bytes += canvases[i].residentBytes();
   return bytes;
}

} // namespace monstro

/* ------------------------------------------------------------------------- */
/* C API                                                                     */
/* ------------------------------------------------------------------------- */

using monstro::Env;
using monstro::requireEnv;

extern "C"
{

MonstroEnvConfig monstro_env_config_default(void)
{
   MonstroEnvConfig config = {};
   config.canvas_count = MONSTRO_DEFAULT_CANVAS_COUNT;
   config.max_vertices = MONSTRO_DEFAULT_MAX_VERTICES;
   config.max_indices = MONSTRO_DEFAULT_MAX_INDICES;
   config.max_draw_commands = MONSTRO_DEFAULT_MAX_DRAW_COMMANDS;
   config.max_path_points = MONSTRO_DEFAULT_MAX_PATH_POINTS;
   config.atlas_width = MONSTRO_DEFAULT_ATLAS_SIZE;
   config.atlas_height = MONSTRO_DEFAULT_ATLAS_SIZE;
   config.max_fonts = MONSTRO_DEFAULT_MAX_FONTS;
   config.max_themes = MONSTRO_DEFAULT_MAX_THEMES;
   config.max_icons = MONSTRO_DEFAULT_MAX_ICONS;
   config.vertex_layout.buffer_count = 1;
   config.vertex_layout.stride[0] = sizeof(MonstroVertex);
   config.vertex_layout.position = {0, (uint32_t)offsetof(MonstroVertex, position)};
   config.vertex_layout.texcoord = {0, (uint32_t)offsetof(MonstroVertex, texcoord)};
   config.vertex_layout.color = {0, (uint32_t)offsetof(MonstroVertex, color)};
   return config;
}

MonstroEnv *monstro_init(const MonstroEnvConfig *config)
{
   MonstroEnvConfig defaults = monstro_env_config_default();
   const MonstroEnvConfig &c = config ? *config : defaults;
   MONSTRO_REQUIRE(c.canvas_count >= 1 && c.max_vertices >= 4 && c.max_indices >= 6 && c.max_draw_commands >= 1 &&
                       c.max_path_points >= 4 && c.atlas_width >= 16 && c.atlas_height >= 16 && c.max_fonts >= 1 &&
                       c.max_themes >= 3 && c.max_icons >= 1,
                   "MonstroEnvConfig capacities too small to draw anything (canvas_count %u, max_vertices %u, "
                   "max_indices %u, max_draw_commands %u, max_path_points %u, atlas %u x %u, max_fonts %u, "
                   "max_themes %u (3 are built in), max_icons %u)",
                   c.canvas_count, c.max_vertices, c.max_indices, c.max_draw_commands, c.max_path_points, c.atlas_width,
                   c.atlas_height, c.max_fonts, c.max_themes, c.max_icons);
   monstro::requireVertexLayout(c.vertex_layout);

   Env *env = new (std::nothrow) Env(c);
   if (!env)
      return nullptr;
   if (!env->init())
   {
      delete env;
      return nullptr;
   }

   MonstroStyle style;
   monstro_style_set_default(&style);
   monstro_theme_register(env, "default", &style);
   monstro_style_set_dark(&style);
   monstro_theme_register(env, "dark", &style);
   monstro_style_set_light(&style);
   monstro_theme_register(env, "light", &style);
   return env;
}

void monstro_shutdown(MonstroEnv *env)
{
   requireEnv(env, __func__);
   MONSTRO_REQUIRE(env->context_count == 0,
                   "monstro_shutdown: %d contexts still use this environment; destroy them first", env->context_count);
   delete env;
}

MonstroEnvConfig monstro_env_get_config(MonstroEnv *env) { return requireEnv(env, __func__)->config; }

MonstroSettings monstro_env_get_settings(MonstroEnv *env) { return requireEnv(env, __func__)->settings; }

void monstro_env_set_settings(MonstroEnv *env, const MonstroSettings *settings)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   MONSTRO_REQUIRE(settings, "%s: settings must not be null", __func__);
   env->settings = *settings;
}

/* ------------------------------------------------------------------------- */
/* Themes                                                                    */
/* ------------------------------------------------------------------------- */

MonstroTheme *monstro_theme_register(MonstroEnv *env, const char *name, const MonstroStyle *style)
{
   requireEnv(env, __func__)->requireIdle(__func__);
   MONSTRO_REQUIRE(name && style, "%s: the name and style must not be null", __func__);
   MONSTRO_REQUIRE(!env->findTheme(name), "%s: a theme named '%s' is already registered; themes are immutable",
                   __func__, name);
   monstro::Theme *theme = &env->themes.add();
   monstro_utf8_copy(theme->name, sizeof(theme->name), name);
   theme->style = *style;
   return theme;
}

MonstroTheme *monstro_theme_find(MonstroEnv *env, const char *name)
{
   requireEnv(env, __func__);
   MONSTRO_REQUIRE(name, "%s: the name must not be null", __func__);
   return env->findTheme(name);
}

const MonstroStyle *monstro_theme_get_style(MonstroTheme *theme)
{
   MONSTRO_REQUIRE(theme, "%s: the theme must not be null", __func__);
   return &theme->style;
}

const char *monstro_theme_get_name(MonstroTheme *theme)
{
   MONSTRO_REQUIRE(theme, "%s: the theme must not be null", __func__);
   return theme->name;
}

} // extern "C"
