#include "harness.h"
#include "context.h"
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace
{

struct TestCase
{
   const char *name;
   TestFunction function;
   const char *const *expected_messages;
};

TestCase g_tests[512];
int g_test_count = 0;
int g_failures = 0;
const char *g_font_path = "examples/basic/data/freeSans.ttf";

bool run_fatal_case(const TestCase &test)
{
   int pipe_fds[2];
   if (pipe(pipe_fds) != 0)
   {
      std::perror("pipe");
      return false;
   }
   std::fflush(stdout);
   std::fflush(stderr);

   pid_t child = fork();
   if (child == 0)
   {
      close(pipe_fds[0]);
      dup2(pipe_fds[1], STDERR_FILENO);
      test.function();
      _exit(0);
   }
   close(pipe_fds[1]);

   std::string output;
   char chunk[4096];
   ssize_t got;
   while ((got = read(pipe_fds[0], chunk, sizeof(chunk))) > 0)
      output.append(chunk, (size_t)got);
   close(pipe_fds[0]);

   int status = 0;
   waitpid(child, &status, 0);

   bool aborted = WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT;
   bool messages_found = true;
   for (const char *const *expected = test.expected_messages; *expected; expected++)
   {
      if (output.find(*expected) == std::string::npos)
      {
         std::printf("  missing message fragment: \"%s\"\n", *expected);
         messages_found = false;
      }
   }
   if (!aborted)
      std::printf("  expected an abort; the child %s %d\n", WIFSIGNALED(status) ? "died with signal" : "exited with",
                  WIFSIGNALED(status) ? WTERMSIG(status) : WEXITSTATUS(status));
   if (!aborted || !messages_found)
      std::printf("  child stderr:\n%s\n", output.c_str());
   return aborted && messages_found;
}

} // namespace

TestRegistrar::TestRegistrar(const char *name, TestFunction function, const char *const *expected_messages)
{
   if (g_test_count >= (int)(sizeof(g_tests) / sizeof(g_tests[0])))
   {
      std::fprintf(stderr, "too many tests\n");
      std::abort();
   }
   g_tests[g_test_count++] = {name, function, expected_messages};
}

void test_failed(const char *file, int line, const char *expression)
{
   std::printf("  CHECK failed at %s:%d: %s\n", file, line, expression);
   g_failures++;
}

MonstroFont *load_test_font(MonstroEnv *env)
{
   MonstroFont *font = monstro_font_load_ttf_from_file(env, "test", g_font_path, 16.0f, 32, 480);
   if (!font)
   {
      std::fprintf(stderr, "cannot load %s (run the tests from the repository root or pass --font)\n", g_font_path);
      std::abort();
   }
   return font;
}

Ui::Ui(bool with_font, const MonstroContextConfig *config, const MonstroEnvConfig *env_config)
{
   env = monstro_init(env_config);
   owns_env = true;
   if (!env)
   {
      std::fprintf(stderr, "monstro_init failed\n");
      std::abort();
   }
   if (with_font)
      font = load_test_font(env);
   ctx = monstro_context_create(env, config);
   if (!ctx)
   {
      std::fprintf(stderr, "monstro_context_create failed\n");
      std::abort();
   }
   monstro_set_display_size(ctx, 800.0f, 600.0f);
}

Ui::Ui(MonstroEnv *shared_env, const MonstroContextConfig *config) : env(shared_env)
{
   font = monstro_env_get_default_font(env);
   ctx = monstro_context_create(env, config);
   if (!ctx)
   {
      std::fprintf(stderr, "monstro_context_create failed\n");
      std::abort();
   }
   monstro_set_display_size(ctx, 800.0f, 600.0f);
}

Ui::~Ui()
{
   monstro_context_destroy(ctx);
   if (owns_env)
      monstro_shutdown(env);
}

void Ui::begin(float delta_time) { monstro_begin_frame(ctx, delta_time); }

const MonstroDrawData *Ui::end() { return data = monstro_end_frame(ctx); }

void Ui::moveMouse(float x, float y) { monstro_set_mouse_pos(ctx, x, y); }

void Ui::pressLeft(bool down) { monstro_set_mouse_button(ctx, MONSTRO_MOUSE_BUTTON_LEFT, down); }

void Ui::pressKey(MonstroKey key, bool down) { monstro_set_key(ctx, key, down); }

void Ui::type(const char *utf8) { monstro_add_input_text(ctx, utf8); }

monstro::Window *find_window_titled(Ui &ui, const char *prefix, const char *title)
{
   monstro::Window *found = nullptr;
   int count = 0;
   for (monstro::Window &w : ui.ctx->windows)
   {
      if (std::strncmp(w.name, prefix, std::strlen(prefix)) == 0 && std::strcmp(w.title, title) == 0)
      {
         found = &w;
         count++;
      }
   }
   if (count > 1)
   {
      std::fprintf(stderr, "%d windows named %s... titled '%s'\n", count, prefix, title);
      std::abort();
   }
   return found;
}

uint32_t Ui::vertexCount() const
{
   if (ctx->canvas)
      return ctx->canvas->vertex_count;
   return data ? data->vertex_count : 0;
}

uint32_t Ui::indexCount() const
{
   if (ctx->canvas)
      return ctx->canvas->index_count;
   return data ? data->index_count : 0;
}

int main(int argc, char **argv)
{
   const char *filter = nullptr;
   for (int i = 1; i < argc; i++)
   {
      if (std::strcmp(argv[i], "--font") == 0 && i + 1 < argc)
         g_font_path = argv[++i];
      else
         filter = argv[i];
   }

   int run = 0;
   int failed_tests = 0;
   for (int i = 0; i < g_test_count; i++)
   {
      const TestCase &test = g_tests[i];
      if (filter && !std::strstr(test.name, filter))
         continue;
      run++;
      std::printf("%s\n", test.name);
      std::fflush(stdout);
      int failures_before = g_failures;
      bool ok = true;
      if (test.expected_messages)
         ok = run_fatal_case(test);
      else
         test.function();
      if (!ok || g_failures != failures_before)
      {
         failed_tests++;
         std::printf("  FAILED\n");
      }
   }

   std::printf("%d tests, %d failed\n", run, failed_tests);
   return failed_tests == 0 && run > 0 ? 0 : 1;
}
