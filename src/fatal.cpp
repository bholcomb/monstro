#include "fatal.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

void monstro_fatal(const char *file, int line, const char *condition, const char *format, ...)
{
   std::fprintf(stderr, "monstro: fatal error at %s:%d: ", file, line);
   if (condition)
      std::fprintf(stderr, "requirement '%s' failed: ", condition);
   va_list args;
   va_start(args, format);
   std::vfprintf(stderr, format, args);
   va_end(args);
   std::fputc('\n', stderr);
   std::fflush(stderr);
   std::abort();
}
