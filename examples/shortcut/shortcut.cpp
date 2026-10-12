#include "shortcut.h"
#include <cstdio>
#include <cstring>

void shortcut_editor_init(ShortcutEditor *se)
{
   memset(se, 0, sizeof(*se));
   se->rebinding_index = -1;
}

int shortcut_editor_add(ShortcutEditor *se, const char *action, MonstroModFlags mods, MonstroKey key)
{
   if (se->binding_count >= SHORTCUT_MAX_BINDINGS)
      return -1;
   int i = se->binding_count++;
   strncpy(se->bindings[i].action, action, SHORTCUT_MAX_NAME_LEN - 1);
   se->bindings[i].mods = mods;
   se->bindings[i].key = key;
   se->bindings[i].bound = (key != MONSTRO_KEY_COUNT);
   return i;
}

static const char *key_name(MonstroKey k)
{
   switch (k)
   {
   case MONSTRO_KEY_TAB:
      return "Tab";
   case MONSTRO_KEY_LEFT_ARROW:
      return "Left";
   case MONSTRO_KEY_RIGHT_ARROW:
      return "Right";
   case MONSTRO_KEY_UP_ARROW:
      return "Up";
   case MONSTRO_KEY_DOWN_ARROW:
      return "Down";
   case MONSTRO_KEY_PAGE_UP:
      return "PageUp";
   case MONSTRO_KEY_PAGE_DOWN:
      return "PageDn";
   case MONSTRO_KEY_HOME:
      return "Home";
   case MONSTRO_KEY_END:
      return "End";
   case MONSTRO_KEY_INSERT:
      return "Ins";
   case MONSTRO_KEY_DELETE:
      return "Del";
   case MONSTRO_KEY_BACKSPACE:
      return "Backspace";
   case MONSTRO_KEY_SPACE:
      return "Space";
   case MONSTRO_KEY_ENTER:
      return "Enter";
   case MONSTRO_KEY_ESCAPE:
      return "Esc";
   case MONSTRO_KEY_A:
      return "A";
   case MONSTRO_KEY_C:
      return "C";
   case MONSTRO_KEY_V:
      return "V";
   case MONSTRO_KEY_X:
      return "X";
   case MONSTRO_KEY_Y:
      return "Y";
   case MONSTRO_KEY_Z:
      return "Z";
   default:
      return "?";
   }
}

static void format_binding(char *buf, size_t sz, MonstroModFlags mods, MonstroKey key)
{
   buf[0] = '\0';
   if (mods & MONSTRO_MOD_CTRL)
      strncat(buf, "Ctrl+", sz - strlen(buf) - 1);
   if (mods & MONSTRO_MOD_SHIFT)
      strncat(buf, "Shift+", sz - strlen(buf) - 1);
   if (mods & MONSTRO_MOD_ALT)
      strncat(buf, "Alt+", sz - strlen(buf) - 1);
   if (mods & MONSTRO_MOD_SUPER)
      strncat(buf, "Super+", sz - strlen(buf) - 1);
   strncat(buf, key_name(key), sz - strlen(buf) - 1);
}

void shortcut_editor_draw(ShortcutEditor *se, MonstroContext *ctx)
{
   monstro_set_next_window_pos(ctx, 50, 50, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 500, 400, MONSTRO_COND_FIRST_USE_EVER);

   if (monstro_begin_window(ctx, "Keyboard Shortcuts", nullptr, MONSTRO_WINDOW_DEFAULT))
   {

      MonstroTableFlags tflags = (MonstroTableFlags)(MONSTRO_TABLE_BORDERS | MONSTRO_TABLE_ROW_BACKGROUND);
      if (monstro_begin_table(ctx, "##shortcuts", 3, tflags, 0, 0))
      {
         monstro_table_setup_column(ctx, "Action", MONSTRO_TABLE_COLUMN_NONE, 200);
         monstro_table_setup_column(ctx, "Shortcut", MONSTRO_TABLE_COLUMN_NONE, 150);
         monstro_table_setup_column(ctx, "", MONSTRO_TABLE_COLUMN_WIDTH_FIXED, 80);
         monstro_table_headers_row(ctx);

         for (int i = 0; i < se->binding_count; i++)
         {
            ShortcutBinding *b = &se->bindings[i];
            monstro_table_next_row(ctx);

            monstro_table_next_column(ctx);
            monstro_label(ctx, b->action);

            monstro_table_next_column(ctx);
            if (se->capturing && se->rebinding_index == i)
            {
               monstro_label_colored(ctx, "Press a key...", monstro_color_rgb(1.0f, 0.78f, 0.31f));

               for (int k = 0; k < MONSTRO_KEY_COUNT; k++)
               {
                  MonstroKey mk = (MonstroKey)k;
                  if (mk == MONSTRO_KEY_LEFT_CTRL || mk == MONSTRO_KEY_RIGHT_CTRL || mk == MONSTRO_KEY_LEFT_SHIFT ||
                      mk == MONSTRO_KEY_RIGHT_SHIFT || mk == MONSTRO_KEY_LEFT_ALT || mk == MONSTRO_KEY_RIGHT_ALT ||
                      mk == MONSTRO_KEY_LEFT_SUPER || mk == MONSTRO_KEY_RIGHT_SUPER)
                     continue;

                  if (monstro_is_key_pressed(ctx, mk, false))
                  {
                     if (mk == MONSTRO_KEY_ESCAPE)
                     {
                        se->capturing = false;
                        se->rebinding_index = -1;
                     }
                     else
                     {
                        b->key = mk;
                        b->mods = monstro_get_key_mods(ctx);
                        b->bound = true;
                        se->capturing = false;
                        se->rebinding_index = -1;
                     }
                     break;
                  }
               }
            }
            else
            {
               if (b->bound)
               {
                  char buf[128];
                  format_binding(buf, sizeof(buf), b->mods, b->key);
                  monstro_label(ctx, buf);
               }
               else
               {
                  monstro_label_colored(ctx, "(unbound)", monstro_color_rgb(0.47f, 0.47f, 0.47f));
               }
            }

            monstro_table_next_column(ctx);
            char btn_id[32];
            snprintf(btn_id, sizeof(btn_id), "rebind%d##Rebind", i);
            if (monstro_button(ctx, btn_id, nullptr))
            {
               se->capturing = true;
               se->rebinding_index = i;
            }
         }
         monstro_end_table(ctx);
      }
      monstro_end_window(ctx);
   }
}
