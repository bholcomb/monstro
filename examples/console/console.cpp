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
      snprintf(con->history[con->history_count], CONSOLE_MAX_CMD_LEN, "%s", line);
      con->history_count++;
   }
   con->history_pos = -1;

   console_log(con, CONSOLE_LOG_INFO, "> %s", line);

   char cmd_name[64] = {};
   const char *args = nullptr;
   const char *sp = strchr(line, ' ');
   size_t len = sp ? (size_t)(sp - line) : strlen(line);
   if (len >= sizeof(cmd_name))
      len = sizeof(cmd_name) - 1;
   memcpy(cmd_name, line, len);
   if (sp)
      args = sp + 1;

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
      return MONSTRO_COLOR_LIGHT_GRAY;
   case CONSOLE_LOG_WARNING:
      return MONSTRO_COLOR_GOLD;
   case CONSOLE_LOG_ERROR:
      return MONSTRO_COLOR_RED;
   case CONSOLE_LOG_DEBUG:
      return MONSTRO_COLOR_CYAN;
   default:
      return MONSTRO_COLOR_LIGHT_GRAY;
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

      monstro_input_text(ctx, "con_filter##Filter", con->filter, sizeof(con->filter), MONSTRO_INPUT_TEXT_NONE, nullptr);
      monstro_separator(ctx);

      const MonstroStyle *style = monstro_get_style(ctx);
      MonstroFont *font = style->font;
      float text_h = font ? monstro_font_get_line_height(font) : 16.0f;
      float command_row_h = text_h + style->edit.padding[1] * 3.0f;

      MonstroWindowFlags child_flags =
          (MonstroWindowFlags)(MONSTRO_WINDOW_CHILD | MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_INPUTS |
                               MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_VERTICAL_SCROLLBAR);

      if (monstro_begin_child(ctx, "log_area##", 0, -command_row_h, child_flags))
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
      if (monstro_input_text(ctx, "cmd##", con->cmd_input, sizeof(con->cmd_input), flags, nullptr))
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
