#include "../common/example_app.h"
#include "shortcut.h"

static ShortcutEditor g_editor;

static void on_init(ExampleApp *, void *)
{
   shortcut_editor_init(&g_editor);
   shortcut_editor_add(&g_editor, "New File", (MonstroModFlags)(MONSTRO_MOD_CTRL), MONSTRO_KEY_A);
   shortcut_editor_add(&g_editor, "Open File", (MonstroModFlags)(MONSTRO_MOD_CTRL), MONSTRO_KEY_C);
   shortcut_editor_add(&g_editor, "Save", (MonstroModFlags)(MONSTRO_MOD_CTRL), MONSTRO_KEY_V);
   shortcut_editor_add(&g_editor, "Undo", (MonstroModFlags)(MONSTRO_MOD_CTRL), MONSTRO_KEY_Z);
   shortcut_editor_add(&g_editor, "Redo", (MonstroModFlags)(MONSTRO_MOD_CTRL | MONSTRO_MOD_SHIFT), MONSTRO_KEY_Z);
   shortcut_editor_add(&g_editor, "Copy", (MonstroModFlags)(MONSTRO_MOD_CTRL), MONSTRO_KEY_C);
   shortcut_editor_add(&g_editor, "Paste", (MonstroModFlags)(MONSTRO_MOD_CTRL), MONSTRO_KEY_V);
   shortcut_editor_add(&g_editor, "Cut", (MonstroModFlags)(MONSTRO_MOD_CTRL), MONSTRO_KEY_X);
   shortcut_editor_add(&g_editor, "Select All", (MonstroModFlags)(MONSTRO_MOD_CTRL), MONSTRO_KEY_A);
   shortcut_editor_add(&g_editor, "Delete", MONSTRO_MOD_NONE, MONSTRO_KEY_DELETE);
   shortcut_editor_add(&g_editor, "Toggle Console", MONSTRO_MOD_NONE, MONSTRO_KEY_TAB);
}

static void on_update(ExampleApp *app, void *, float) { shortcut_editor_draw(&g_editor, app->ui); }

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   ExampleAppDesc desc = {};
   desc.title = "Monstro Shortcut Editor Example";
   desc.width = 600;
   desc.height = 500;
   desc.init = on_init;
   desc.update = on_update;
   return example_app_run(desc);
}
