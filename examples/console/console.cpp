#include "console.h"
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

void console_init(Console *con)
{
   memset(con, 0, sizeof(*con));
   con->show_info = true;
   con->show_warning = true;
   con->show_error = true;
   con->show_debug = true;
   con->auto_scroll = true;
   con->history_pos = -1;
}

void console_clear(Console *con)
{
   con->entry_count = 0;
   con->entry_start = 0;
}

void console_log(Console *con, ConsoleLogLevel level, const char *fmt, ...)
{
   int idx;
   if (con->entry_count < CONSOLE_MAX_ENTRIES)
   {
      idx = con->entry_count++;
   }
   else
   {
      idx = con->entry_start;
      con->entry_start = (con->entry_start + 1) % CONSOLE_MAX_ENTRIES;
   }

   ConsoleEntry *e = &con->entries[idx];
   e->level = level;
   e->timestamp = con->time_elapsed;

   va_list args;
   va_start(args, fmt);
   vsnprintf(e->message, CONSOLE_MAX_MSG_LEN, fmt, args);
   va_end(args);
}

void console_add_command(Console *con, const char *name, ConsoleCommandFn fn, void *user_data)
{
   if (con->command_count >= 64)
      return;
   ConsoleCommand *c = &con->commands[con->command_count++];
   strncpy(c->name, name, sizeof(c->name) - 1);
   c->callback = fn;
   c->user_data = user_data;
}

static void execute_command(Console *con, const char *line)
{
   if (!line || !line[0])
      return;

   if (con->history_count < CONSOLE_MAX_HISTORY)
   {
      strncpy(con->history[con->history_count], line, CONSOLE_MAX_CMD_LEN - 1);
      con->history_count++;
   }
   con->history_pos = -1;

   console_log(con, CONSOLE_LOG_INFO, "> %s", line);

   char cmd_name[64] = {};
   const char *args = nullptr;
   const char *sp = strchr(line, ' ');
   if (sp)
   {
      size_t len = (size_t)(sp - line);
      if (len >= sizeof(cmd_name))
         len = sizeof(cmd_name) - 1;
      memcpy(cmd_name, line, len);
      args = sp + 1;
   }
   else
   {
      strncpy(cmd_name, line, sizeof(cmd_name) - 1);
   }

   for (int i = 0; i < con->command_count; i++)
   {
      if (strcmp(con->commands[i].name, cmd_name) == 0)
      {
         con->commands[i].callback(args, con->commands[i].user_data);
         return;
      }
   }

   console_log(con, CONSOLE_LOG_ERROR, "Unknown command: %s", cmd_name);
}

static bool passes_filter(Console *con, const ConsoleEntry *e)
{
   switch (e->level)
   {
   case CONSOLE_LOG_INFO:
      if (!con->show_info)
         return false;
      break;
   case CONSOLE_LOG_WARNING:
      if (!con->show_warning)
         return false;
      break;
   case CONSOLE_LOG_ERROR:
      if (!con->show_error)
         return false;
      break;
   case CONSOLE_LOG_DEBUG:
      if (!con->show_debug)
         return false;
      break;
   }

   if (con->filter[0])
   {
      if (!strstr(e->message, con->filter))
         return false;
   }
   return true;
}

static MonstroColor level_color(ConsoleLogLevel level)
{
   switch (level)
   {
   case CONSOLE_LOG_INFO:
      return {200, 200, 200, 255};
   case CONSOLE_LOG_WARNING:
      return {255, 200, 80, 255};
   case CONSOLE_LOG_ERROR:
      return {255, 80, 80, 255};
   case CONSOLE_LOG_DEBUG:
      return {120, 180, 255, 255};
   default:
      return {200, 200, 200, 255};
   }
}

static const char *level_prefix(ConsoleLogLevel level)
{
   switch (level)
   {
   case CONSOLE_LOG_INFO:
      return "[INF]";
   case CONSOLE_LOG_WARNING:
      return "[WRN]";
   case CONSOLE_LOG_ERROR:
      return "[ERR]";
   case CONSOLE_LOG_DEBUG:
      return "[DBG]";
   default:
      return "[???]";
   }
}

void console_draw(Console *con, MonstroContext *ctx, float dt)
{
   con->time_elapsed += dt;

   monstro_set_next_window_pos(ctx, 50, 50, MONSTRO_COND_FIRST_USE_EVER);
   monstro_set_next_window_size(ctx, 600, 500, MONSTRO_COND_FIRST_USE_EVER);

   if (monstro_begin_window(ctx, "Console", nullptr, MONSTRO_WINDOW_DEFAULT))
   {

      monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
      {
         monstro_checkbox(ctx, "Info", &con->show_info, nullptr);
         monstro_checkbox(ctx, "Warn", &con->show_warning, nullptr);
         monstro_checkbox(ctx, "Error", &con->show_error, nullptr);
         monstro_checkbox(ctx, "Debug", &con->show_debug, nullptr);
         if (monstro_button(ctx, "Clear", nullptr))
         {
            console_clear(con);
         }
      }
      monstro_end_layout(ctx);

      monstro_input_text(ctx, "Filter##con_filter", con->filter, sizeof(con->filter), MONSTRO_INPUT_TEXT_NONE, nullptr);
      monstro_separator(ctx);

      MonstroWindowFlags child_flags =
          (MonstroWindowFlags)(MONSTRO_WINDOW_CHILD | MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_INPUTS |
                               MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_VERTICAL_SCROLLBAR);

      if (monstro_begin_child(ctx, "##log_area", 0, -30, child_flags))
      {
         int total = con->entry_count < CONSOLE_MAX_ENTRIES ? con->entry_count : CONSOLE_MAX_ENTRIES;
         for (int i = 0; i < total; i++)
         {
            int idx = (con->entry_start + i) % CONSOLE_MAX_ENTRIES;
            const ConsoleEntry *e = &con->entries[idx];
            if (!passes_filter(con, e))
               continue;

            char line[CONSOLE_MAX_MSG_LEN + 32];
            snprintf(line, sizeof(line), "%s %s", level_prefix(e->level), e->message);
            monstro_label_colored(ctx, line, level_color(e->level));
         }
         monstro_end_child(ctx);
      }

      MonstroInputTextFlags flags = (MonstroInputTextFlags)(MONSTRO_INPUT_TEXT_ENTER_RETURNS);
      if (monstro_input_text(ctx, "##cmd", con->cmd_input, sizeof(con->cmd_input), flags, nullptr))
      {
         if (con->cmd_input[0])
         {
            execute_command(con, con->cmd_input);
            con->cmd_input[0] = '\0';
         }
      }
      monstro_end_window(ctx);
   }
}
