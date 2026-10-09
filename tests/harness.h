#ifndef MONSTRO_TESTS_HARNESS_H
#define MONSTRO_TESTS_HARNESS_H

#include <monstro.h>

/*
 * A small headless test harness. TEST cases run in the test process.
 * FATAL_TEST cases run in a child process and pass only when the child aborts
 * and its stderr contains every expected message fragment.
 */

typedef void (*TestFunction)();

struct TestRegistrar
{
   TestRegistrar(const char *name, TestFunction function, const char *const *expected_messages);
};

void test_failed(const char *file, int line, const char *expression);

#define TEST(name)                                                                                                     \
   static void name();                                                                                                 \
   static TestRegistrar name##_registrar(#name, name, nullptr);                                                        \
   static void name()

#define FATAL_TEST(name, ...)                                                                                          \
   static void name();                                                                                                 \
   static const char *const name##_expected[] = {__VA_ARGS__, nullptr};                                                \
   static TestRegistrar name##_registrar(#name, name, name##_expected);                                                \
   static void name()

#define CHECK(expression)                                                                                              \
   do                                                                                                                  \
   {                                                                                                                   \
      if (!(expression))                                                                                               \
         test_failed(__FILE__, __LINE__, #expression);                                                                 \
   } while (0)

// A context with an 800x600 display and, unless disabled, the example font.
struct Ui
{
   MonstroContext *ctx = nullptr;
   MonstroFont *font = nullptr;

   explicit Ui(bool with_font = true, const MonstroContextConfig *config = nullptr);
   ~Ui();
   Ui(const Ui &) = delete;
   Ui &operator=(const Ui &) = delete;

   void begin(float delta_time = 1.0f / 60.0f);
   void end();

   // One whole frame with the left button pressed (then released) at x, y.
   void moveMouse(float x, float y);
   void pressLeft(bool down);
   void pressKey(MonstroKey key, bool down);
   void type(const char *utf8);

   uint32_t vertexCount() const;
   uint32_t indexCount() const;
};

#endif
