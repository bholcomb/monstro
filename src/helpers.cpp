#include "helpers.h"
#include <cstring>

static bool is_continuation(uint8_t byte) { return (byte & 0xC0) == 0x80; }

uint32_t decode_utf8(const char **str)
{
   const uint8_t *s = (const uint8_t *)*str;
   uint32_t lead = s[0];
   if (lead < 0x80)
   {
      if (lead != 0)
         *str += 1;
      return lead;
   }

   int extra;
   uint32_t code_point;
   uint32_t minimum;
   if (lead >= 0xC2 && lead <= 0xDF)
   {
      extra = 1;
      code_point = lead & 0x1F;
      minimum = 0x80;
   }
   else if (lead >= 0xE0 && lead <= 0xEF)
   {
      extra = 2;
      code_point = lead & 0x0F;
      minimum = 0x800;
   }
   else if (lead >= 0xF0 && lead <= 0xF4)
   {
      extra = 3;
      code_point = lead & 0x07;
      minimum = 0x10000;
   }
   else
   {
      *str += 1;
      return MONSTRO_REPLACEMENT_CHARACTER;
   }

   // A truncated sequence stops at the first byte that is not a continuation,
   // which includes the terminator, so decoding never reads past it.
   for (int i = 1; i <= extra; i++)
   {
      if (!is_continuation(s[i]))
      {
         *str += i;
         return MONSTRO_REPLACEMENT_CHARACTER;
      }
      code_point = (code_point << 6) | (s[i] & 0x3F);
   }
   *str += 1 + extra;

   if (code_point < minimum || code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF))
      return MONSTRO_REPLACEMENT_CHARACTER;
   return code_point;
}

size_t monstro_utf8_fit(const char *text, size_t max_bytes)
{
   size_t length = 0;
   while (length < max_bytes && text[length] != '\0')
      length++;
   if (text[length] == '\0')
      return length;
   // Cut before the code point that straddles the limit.
   for (int i = 0; i < 3 && length > 0 && is_continuation((uint8_t)text[length]); i++)
      length--;
   return length;
}

void monstro_utf8_copy(char *out, size_t out_size, const char *text)
{
   size_t length = monstro_utf8_fit(text, out_size - 1);
   memcpy(out, text, length);
   out[length] = '\0';
}

void monstro_utf8_trim_partial(char *text)
{
   size_t length = strlen(text);
   size_t start = length;
   for (int i = 0; i < 3 && start > 0 && is_continuation((uint8_t)text[start - 1]); i++)
      start--;
   if (start == 0)
      return;
   uint8_t lead = (uint8_t)text[start - 1];
   size_t needed = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
   if (length - (start - 1) < needed)
      text[start - 1] = '\0';
}

int monstro_utf8_next(const char *text, int pos)
{
   const char *s = text + pos;
   decode_utf8(&s);
   return (int)(s - text);
}

int monstro_utf8_prev(const char *text, int pos)
{
   if (pos <= 0)
      return 0;
   int start = pos - 1;
   for (int i = 0; i < 3 && start > 0 && is_continuation((uint8_t)text[start]); i++)
      start--;
   // Stray continuation bytes decode one at a time; step over one of them.
   return monstro_utf8_next(text, start) == pos ? start : pos - 1;
}

int monstro_utf8_snap(const char *text, int pos)
{
   for (int i = 0; i < 3 && pos > 0 && is_continuation((uint8_t)text[pos]); i++)
      pos--;
   return pos;
}

int monstro_utf8_count(const char *text, int bytes)
{
   int count = 0;
   const char *s = text;
   while (s < text + bytes && *s)
   {
      decode_utf8(&s);
      count++;
   }
   return count;
}

bool parse_icon_tag(const char **str, uint32_t *out_icon_id)
{
   const char *s = *str;
   if (s[0] != '{')
      return false;
   if (s[1] != 'i' || s[2] != 'c' || s[3] != 'o' || s[4] != 'n' || s[5] != ':')
      return false;

   s += 6;
   uint32_t id = 0;
   if (*s < '0' || *s > '9')
      return false;
   while (*s >= '0' && *s <= '9')
   {
      id = id * 10 + (*s - '0');
      s++;
   }
   if (*s != '}')
      return false;
   s++;

   *out_icon_id = id;
   *str = s;
   return true;
}
