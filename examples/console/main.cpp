#include "../common/example_app.h"
#include "console.h"
#include <cstdio>

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

static void on_init(ExampleApp *, void *)
{
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
