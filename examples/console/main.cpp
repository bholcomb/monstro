#include "../common/example_app.h"
#include "console.h"
#include <cstdio>
#include <cstdlib>

static Console g_console;

static void cmd_help(const char *, void *ud)
{
   Console *c = (Console *)ud;
   console_log(c, CONSOLE_LOG_INFO, "Available commands: help, clear, echo, spam");
}

static void cmd_clear(const char *, void *ud) { console_clear((Console *)ud); }

static void cmd_echo(const char *args, void *ud)
{
   console_log((Console *)ud, CONSOLE_LOG_INFO, "%s", args ? args : "");
}

static void cmd_spam(const char *, void *ud)
{
   Console *c = (Console *)ud;
   console_log(c, CONSOLE_LOG_INFO, "This is an info message");
   console_log(c, CONSOLE_LOG_WARNING, "This is a warning message");
   console_log(c, CONSOLE_LOG_ERROR, "This is an error message");
   console_log(c, CONSOLE_LOG_DEBUG, "This is a debug message");
}

static void on_init(ExampleApp *app, void *)
{
   // The console's text is a bitmap font: DejaVu Sans Mono in a 16 x 6 grid from space, by tools/make_bitmap_font.c.
   MonstroFont *mono =
       monstro_font_load_bitmap_from_file(app->env, "mono", "data/fonts/DejaVuSansMono-bitmap.png", 16, 6, 32, 16.0f);
   if (!mono)
   {
      fprintf(stderr, "cannot load data/fonts/DejaVuSansMono-bitmap.png; run the example from bin/\n");
      abort();
   }
   MonstroStyle style = *monstro_theme_get_style(monstro_context_get_theme(app->ui));
   style.font = mono;
   monstro_context_set_theme(app->ui, monstro_theme_register(app->env, "console", &style));

   console_init(&g_console);
   console_add_command(&g_console, "help", cmd_help, &g_console);
   console_add_command(&g_console, "clear", cmd_clear, &g_console);
   console_add_command(&g_console, "echo", cmd_echo, &g_console);
   console_add_command(&g_console, "spam", cmd_spam, &g_console);
   console_log(&g_console, CONSOLE_LOG_INFO, "Console initialized. Type 'help' for commands.");
}

static void on_update(ExampleApp *app, void *, float dt) { console_draw(&g_console, app->ui, dt); }

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;

   ExampleAppDesc desc = {};
   desc.title = "Monstro Console Example";
   desc.width = 800;
   desc.height = 600;
   desc.init = on_init;
   desc.update = on_update;
   return example_app_run(desc);
}
