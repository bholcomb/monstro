#ifndef MONSTRO_INTERNAL_ENV_H
#define MONSTRO_INTERNAL_ENV_H

#include "canvas.h"
#include "constants.h"
#include "fixed.h"
#include "font.h"
#include "monstro.h"

namespace monstro
{

struct Theme
{
   char name[MONSTRO_THEME_NAME_SIZE] = {};
   MonstroStyle style = {};
};

struct IconEntry
{
   uint32_t id = 0; // 0 marks a free slot
   uint64_t texture_id = 0;
   bool in_atlas = false; // drawn with the atlas texture, whatever ID the host gives it
   MonstroVec2 uv_min = {0.0f, 0.0f};
   MonstroVec2 uv_max = {1.0f, 1.0f};
   bool is_monochrome = false;
};

// Icons by ID: open addressing over a fixed slot array, so registering never allocates or throws.
struct IconTable
{
   IconEntry *slots = nullptr;
   uint32_t slot_mask = 0;
   uint32_t count = 0;
   uint32_t capacity = 0;

   ~IconTable();
   bool init(uint32_t max_icons);
   void put(const IconEntry &entry);
   const IconEntry *find(uint32_t id) const;
};

// The one RGBA8 texture every font is packed into, with a white texel for solid shapes.
struct Atlas
{
   uint8_t *pixels = nullptr;
   uint32_t width = 0;
   uint32_t height = 0;
   uint64_t version = 0;
   uint64_t texture_id = 0;
   MonstroVec2 white_uv = {0.0f, 0.0f};

   // Shelf packing: rows of rectangles, left to right, top to bottom.
   uint32_t shelf_x = 0;
   uint32_t shelf_y = 0;
   uint32_t shelf_height = 0;

   ~Atlas();
   bool init(uint32_t atlas_width, uint32_t atlas_height);
   // Space for a w x h image; stops the process when the atlas is full. what names the request.
   void allocate(uint32_t w, uint32_t h, const char *what, uint32_t *out_x, uint32_t *out_y);
   uint8_t *pixel(uint32_t x, uint32_t y) { return pixels + ((size_t)y * width + x) * 4; }
};

struct Env
{
   MonstroEnvConfig config = {};
   MonstroSettings settings = {};

   Atlas atlas;
   ConfiguredArray<MonstroFont> fonts{"font table", "raise MonstroEnvConfig.max_fonts"};
   MonstroFont *default_font = nullptr;
   ConfiguredArray<Theme> themes{"theme table", "raise MonstroEnvConfig.max_themes"};
   IconTable icons;

   Canvas *canvases = nullptr;
   uint32_t next_canvas = 0;

   int context_count = 0;
   int open_frames = 0;

   explicit Env(const MonstroEnvConfig &env_config);
   ~Env();
   Env(const Env &) = delete;
   Env &operator=(const Env &) = delete;

   bool init();

   // The environment is read-only while any frame is open; function names the caller for the message.
   void requireIdle(const char *function) const;

   Canvas *lendCanvas(Context *ctx, int frame_number, const MonstroFrameBuffers *buffers);
   void returnCanvas(Canvas *canvas);

   MonstroFont *addFont(const char *name, const char *function);
   Theme *findTheme(const char *name);
   size_t residentBytes() const;
};

// The environment an API function works on: it must not be null.
Env *requireEnv(Env *env, const char *function);
// Validates a vertex layout, stopping the process with what is wrong.
void requireVertexLayout(const MonstroVertexLayout &layout);

} // namespace monstro

#endif
