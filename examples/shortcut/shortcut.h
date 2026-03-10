#ifndef EXAMPLE_SHORTCUT_H
#define EXAMPLE_SHORTCUT_H

#include <monstro.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SHORTCUT_MAX_BINDINGS 64
#define SHORTCUT_MAX_NAME_LEN 64

typedef struct ShortcutBinding
{
   char action[SHORTCUT_MAX_NAME_LEN];
   MonstroModFlags mods;
   MonstroKey key;
   bool bound;
} ShortcutBinding;

typedef struct ShortcutEditor
{
   ShortcutBinding bindings[SHORTCUT_MAX_BINDINGS];
   int binding_count;
   int rebinding_index;
   bool capturing;
} ShortcutEditor;

void shortcut_editor_init(ShortcutEditor *se);
int shortcut_editor_add(ShortcutEditor *se, const char *action, MonstroModFlags mods, MonstroKey key);
void shortcut_editor_draw(ShortcutEditor *se, MonstroContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
