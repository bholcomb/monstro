#include "../context.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

static const int MAX_PATH_LEN = 4096;
static const int MAX_ENTRIES = 2048;
static const int MAX_FILTER_LEN = 128;
static const int MAX_TITLE_LEN = 128;

struct FileEntry
{
   char name[256];
   bool is_dir;
};

struct MonstroFileDialogState
{
   bool open;
   MonstroFileDialogMode mode;
   MonstroFileDialogResult result;

   char title[MAX_TITLE_LEN];
   char current_path[MAX_PATH_LEN];
   char selected_file[MAX_PATH_LEN];
   char filename_input[256];
   char filter[MAX_FILTER_LEN];

   FileEntry entries[MAX_ENTRIES];
   int entry_count;
   int selected_index;

   bool needs_refresh;
};

static bool match_filter(const char *name, const char *filter)
{
   if (!filter || filter[0] == '\0' || (filter[0] == '*' && filter[1] == '\0'))
      return true;

   const char *ext = strrchr(name, '.');
   if (!ext)
      return false;
   ext++;

   const char *p = filter;
   char token[64];
   while (*p)
   {
      while (*p == ' ' || *p == ';' || *p == ',')
         p++;
      if (!*p)
         break;

      const char *start = p;
      if (*p == '*' && *(p + 1) == '.')
         p += 2;
      else if (*p == '.')
         p++;

      start = p;
      int len = 0;
      while (*p && *p != ' ' && *p != ';' && *p != ',' && len < 63)
      {
         token[len++] = *p++;
      }
      token[len] = '\0';

      if (len > 0)
      {
#ifdef _WIN32
         if (_stricmp(ext, token) == 0)
            return true;
#else
         if (strcasecmp(ext, token) == 0)
            return true;
#endif
      }
   }
   return false;
}

static void refresh_directory(MonstroFileDialogState *state)
{
   state->entry_count = 0;
   state->selected_index = -1;

#ifdef _WIN32
   char search_path[MAX_PATH_LEN];
   snprintf(search_path, sizeof(search_path), "%s\\*", state->current_path);

   WIN32_FIND_DATAA fd;
   HANDLE hFind = FindFirstFileA(search_path, &fd);
   if (hFind == INVALID_HANDLE_VALUE)
      return;

   do
   {
      if (strcmp(fd.cFileName, ".") == 0)
         continue;
      if (state->entry_count >= MAX_ENTRIES)
         break;

      FileEntry *e = &state->entries[state->entry_count];
      strncpy(e->name, fd.cFileName, sizeof(e->name) - 1);
      e->name[sizeof(e->name) - 1] = '\0';
      e->is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

      if (!e->is_dir && !match_filter(e->name, state->filter))
         continue;

      state->entry_count++;
   } while (FindNextFileA(hFind, &fd));
   FindClose(hFind);
#else
   DIR *dir = opendir(state->current_path);
   if (!dir)
      return;

   struct dirent *ent;
   while ((ent = readdir(dir)) != nullptr)
   {
      if (strcmp(ent->d_name, ".") == 0)
         continue;
      if (state->entry_count >= MAX_ENTRIES)
         break;

      FileEntry *e = &state->entries[state->entry_count];
      strncpy(e->name, ent->d_name, sizeof(e->name) - 1);
      e->name[sizeof(e->name) - 1] = '\0';

      char full[MAX_PATH_LEN];
      snprintf(full, sizeof(full), "%s/%s", state->current_path, ent->d_name);
      struct stat st;
      if (stat(full, &st) == 0)
      {
         e->is_dir = S_ISDIR(st.st_mode);
      }
      else
      {
         e->is_dir = (ent->d_type == DT_DIR);
      }

      if (!e->is_dir && !match_filter(e->name, state->filter))
         continue;

      state->entry_count++;
   }
   closedir(dir);
#endif

   std::sort(state->entries, state->entries + state->entry_count,
             [](const FileEntry &a, const FileEntry &b)
             {
                if (a.is_dir != b.is_dir)
                   return a.is_dir > b.is_dir;
                return strcmp(a.name, b.name) < 0;
             });

   state->needs_refresh = false;
}

static void navigate_to(MonstroFileDialogState *state, const char *path)
{
   strncpy(state->current_path, path, MAX_PATH_LEN - 1);
   state->current_path[MAX_PATH_LEN - 1] = '\0';

   size_t len = strlen(state->current_path);
   while (len > 1 && (state->current_path[len - 1] == '/' || state->current_path[len - 1] == '\\'))
   {
      state->current_path[--len] = '\0';
   }

   state->needs_refresh = true;
   state->filename_input[0] = '\0';
}

extern "C"
{

MonstroFileDialogState *monstro_file_dialog_create(void)
{
   auto *s = new MonstroFileDialogState();
   memset(s, 0, sizeof(*s));
   s->selected_index = -1;
   return s;
}

void monstro_file_dialog_destroy(MonstroFileDialogState *state) { delete state; }

void monstro_file_dialog_open(MonstroFileDialogState *state, const char *title, const char *start_path,
                              const char *filter, MonstroFileDialogMode mode)
{
   if (!state)
      return;

   state->open = true;
   state->mode = mode;
   state->result = MONSTRO_FILE_DIALOG_NONE;
   state->selected_index = -1;
   state->filename_input[0] = '\0';
   state->selected_file[0] = '\0';

   strncpy(state->title, title ? title : "File Dialog", MAX_TITLE_LEN - 1);
   state->title[MAX_TITLE_LEN - 1] = '\0';

   strncpy(state->filter, filter ? filter : "*", MAX_FILTER_LEN - 1);
   state->filter[MAX_FILTER_LEN - 1] = '\0';

   if (start_path && start_path[0])
   {
      navigate_to(state, start_path);
   }
   else
   {
#ifdef _WIN32
      GetCurrentDirectoryA(MAX_PATH_LEN, state->current_path);
#else
      if (!getcwd(state->current_path, MAX_PATH_LEN))
         strncpy(state->current_path, "/", MAX_PATH_LEN);
#endif
      state->needs_refresh = true;
   }
}

MonstroFileDialogResult monstro_file_dialog_draw(MonstroContext *ctx, MonstroFileDialogState *state)
{
   if (!ctx || !state || !state->open)
      return MONSTRO_FILE_DIALOG_NONE;

   if (state->needs_refresh)
      refresh_directory(state);

   monstro_open_popup_ex(ctx, state->title, true);

   MonstroWindowFlags flags =
       (MonstroWindowFlags)(MONSTRO_WINDOW_MODAL | MONSTRO_WINDOW_POPUP | MONSTRO_WINDOW_TITLE_BAR |
                            MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_INPUTS | MONSTRO_WINDOW_BACKGROUND);

   bool open = true;
   if (monstro_begin_popup_modal(ctx, state->title, &open, flags))
   {

      monstro_label(ctx, state->current_path);
      monstro_separator(ctx);

      float child_h = 300.0f;
      MonstroWindowFlags child_flags =
          (MonstroWindowFlags)(MONSTRO_WINDOW_CHILD | MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_INPUTS |
                               MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_VERTICAL_SCROLLBAR);

      if (monstro_begin_child(ctx, "##fd_list", 400.0f, child_h, child_flags))
      {
         for (int i = 0; i < state->entry_count; i++)
         {
            FileEntry *e = &state->entries[i];
            char display[300];
            if (e->is_dir)
            {
               snprintf(display, sizeof(display), "[DIR] %s", e->name);
            }
            else
            {
               snprintf(display, sizeof(display), "      %s", e->name);
            }

            bool selected = (i == state->selected_index);
            if (monstro_selectable(ctx, display, &selected, MONSTRO_SELECTABLE_DRAW_FILL_WIDTH, nullptr))
            {
               if (e->is_dir)
               {
                  if (strcmp(e->name, "..") == 0)
                  {
                     char parent[MAX_PATH_LEN];
                     strncpy(parent, state->current_path, MAX_PATH_LEN);
                     char *last_sep = strrchr(parent, '/');
#ifdef _WIN32
                     if (!last_sep)
                        last_sep = strrchr(parent, '\\');
#endif
                     if (last_sep && last_sep != parent)
                     {
                        *last_sep = '\0';
                     }
                     else if (last_sep == parent)
                     {
                        parent[1] = '\0';
                     }
                     navigate_to(state, parent);
                  }
                  else
                  {
                     char sub[MAX_PATH_LEN];
#ifdef _WIN32
                     snprintf(sub, sizeof(sub), "%s\\%s", state->current_path, e->name);
#else
                     snprintf(sub, sizeof(sub), "%s/%s", state->current_path, e->name);
#endif
                     navigate_to(state, sub);
                  }
               }
               else
               {
                  state->selected_index = i;
                  strncpy(state->filename_input, e->name, sizeof(state->filename_input) - 1);
                  state->filename_input[sizeof(state->filename_input) - 1] = '\0';
               }
            }
         }
      }
      monstro_end_child(ctx);

      MonstroInputTextFlags text_flags = MONSTRO_INPUT_TEXT_NONE;
      if (state->mode == MONSTRO_FILE_DIALOG_OPEN)
         text_flags = MONSTRO_INPUT_TEXT_NONE;

      monstro_input_text(ctx, "File##fd_filename", state->filename_input, sizeof(state->filename_input), text_flags,
                         nullptr);

      const char *ok_label = (state->mode == MONSTRO_FILE_DIALOG_SAVE) ? "Save" : "Open";
      monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
      {
         if (monstro_button(ctx, ok_label, nullptr))
         {
            if (state->filename_input[0] != '\0')
            {
#ifdef _WIN32
               snprintf(state->selected_file, MAX_PATH_LEN, "%s\\%s", state->current_path, state->filename_input);
#else
               snprintf(state->selected_file, MAX_PATH_LEN, "%s/%s", state->current_path, state->filename_input);
#endif
               state->result = MONSTRO_FILE_DIALOG_OK;
               state->open = false;
               monstro_close_current_popup(ctx);
            }
         }

         if (monstro_button(ctx, "Cancel", nullptr))
         {
            state->result = MONSTRO_FILE_DIALOG_CANCEL;
            state->open = false;
            monstro_close_current_popup(ctx);
         }
      }
      monstro_end_layout(ctx);

      monstro_end_popup(ctx);
   }

   if (!open)
   {
      state->result = MONSTRO_FILE_DIALOG_CANCEL;
      state->open = false;
   }

   return state->result;
}

const char *monstro_file_dialog_get_path(const MonstroFileDialogState *state)
{
   if (!state)
      return "";
   return state->selected_file;
}

bool monstro_file_dialog_is_open(const MonstroFileDialogState *state)
{
   if (!state)
      return false;
   return state->open;
}

} // extern "C"
