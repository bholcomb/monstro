#ifndef EXAMPLE_THEMES_H
#define EXAMPLE_THEMES_H

/*
 * Two textured game themes for the widget tour, built from Kenney's CC0 UI packs
 * (see scifi/README.md and rpg/README.md): "scifi" and "rpg". Each is a table of
 * art, one PNG (or part of one) per style slot drawn as a nine-patch or a
 * sprite, over a few colours and sizes where no art fits.
 *
 * The host uploads the pixels: game_theme_register calls upload once per image
 * and puts the texture ID it returns in the theme's style items.
 */

#include <monstro.h>
#include <stddef.h>
#include <stdint.h>

// Uploads one RGBA8 image (rows top to bottom, width * 4 bytes each) and returns its texture ID.
typedef uint64_t (*GameThemeUpload)(void *user, const char *path, const unsigned char *rgba, int width, int height);

// One slot's art: the style item at `slot` (an offset into MonstroStyle) becomes this file's picture, or the
// part of it at x, y, w, h (pixels; w 0 is the whole image). With any border set it is a nine-patch with those
// borders in pixels; with none, a sprite.
typedef struct GameThemeArt
{
   size_t slot;
   const char *file;
   float left, right, top, bottom;
   int x, y, w, h;
} GameThemeArt;

typedef struct GameTheme
{
   const char *name; // the theme's name, and its folder under the art root
   const char *font; // a TrueType file in the folder, or NULL for the environment's default font
   float font_size;
   void (*base)(MonstroStyle *style); // the colours and sizes the art sits on
   const GameThemeArt *art;
   int art_count;
} GameTheme;

extern const GameTheme GAME_THEME_SCIFI;
extern const GameTheme GAME_THEME_RPG;

// Loads the theme's art from art_root/<name>/ and its font, and registers the theme. Stops the program with a
// message when a file cannot be read.
MonstroTheme *game_theme_register(MonstroEnv *env, const GameTheme *theme, const char *art_root, GameThemeUpload upload,
                                  void *user);

#endif
