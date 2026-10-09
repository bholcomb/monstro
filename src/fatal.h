#ifndef MONSTRO_INTERNAL_FATAL_H
#define MONSTRO_INTERNAL_FATAL_H

/*
 * The library's one invariant check, always on.
 *
 * Monstro never throws across its C API and never recovers silently from a
 * broken precondition: API misuse, unbalanced begin/end pairs, null handles,
 * out-of-range arguments and exhausted capacities all stop the process with
 * the place, the failed condition and the values involved, so the cause gets
 * fixed instead of hidden.
 */

#if defined(__GNUC__) || defined(__clang__)
#define MONSTRO_PRINTF_FORMAT(fmt_index, args_index) __attribute__((format(printf, fmt_index, args_index)))
#else
#define MONSTRO_PRINTF_FORMAT(fmt_index, args_index)
#endif

// Prints "monstro: fatal error at <file>:<line>: [requirement '<condition>' failed: ]<message>" to stderr and aborts.
[[noreturn]] void monstro_fatal(const char *file, int line, const char *condition, const char *format, ...)
    MONSTRO_PRINTF_FORMAT(4, 5);

#define MONSTRO_FATAL(...) monstro_fatal(__FILE__, __LINE__, nullptr, __VA_ARGS__)

#define MONSTRO_REQUIRE(condition, ...)                                                                                \
   do                                                                                                                  \
   {                                                                                                                   \
      if (!(condition))                                                                                                \
         monstro_fatal(__FILE__, __LINE__, #condition, __VA_ARGS__);                                                   \
   } while (0)

#endif
