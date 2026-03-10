#ifndef EXAMPLE_CONSOLE_H
#define EXAMPLE_CONSOLE_H

#include <monstro.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum ConsoleLogLevel
{
   CONSOLE_LOG_INFO = 0,
   CONSOLE_LOG_WARNING = 1,
   CONSOLE_LOG_ERROR = 2,
   CONSOLE_LOG_DEBUG = 3
} ConsoleLogLevel;

#define CONSOLE_MAX_ENTRIES 1024
#define CONSOLE_MAX_MSG_LEN 256
#define CONSOLE_MAX_CMD_LEN 256
#define CONSOLE_MAX_HISTORY 64

typedef void (*ConsoleCommandFn)(const char *args, void *user_data);

typedef struct ConsoleCommand
{
   char name[64];
   ConsoleCommandFn callback;
   void *user_data;
} ConsoleCommand;

typedef struct ConsoleEntry
{
   char message[CONSOLE_MAX_MSG_LEN];
   ConsoleLogLevel level;
   float timestamp;
} ConsoleEntry;

typedef struct Console
{
   ConsoleEntry entries[CONSOLE_MAX_ENTRIES];
   int entry_count;
   int entry_start;

   char filter[128];
   bool show_info;
   bool show_warning;
   bool show_error;
   bool show_debug;
   bool auto_scroll;

   char cmd_input[CONSOLE_MAX_CMD_LEN];
   char history[CONSOLE_MAX_HISTORY][CONSOLE_MAX_CMD_LEN];
   int history_count;
   int history_pos;

   ConsoleCommand commands[64];
   int command_count;

   float time_elapsed;
} Console;

void console_init(Console *con);
void console_log(Console *con, ConsoleLogLevel level, const char *fmt, ...);
void console_add_command(Console *con, const char *name, ConsoleCommandFn fn, void *user_data);
void console_draw(Console *con, MonstroContext *ctx, float dt);
void console_clear(Console *con);

#ifdef __cplusplus
}
#endif

#endif
