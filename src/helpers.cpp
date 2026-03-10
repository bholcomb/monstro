#include "helpers.h"

uint32_t decode_utf8(const char** str) {
    const uint8_t* s = (const uint8_t*)*str;
    uint32_t c = *s++;

    if (c < 0x80) {
        *str = (const char*)s;
        return c;
    }

    if ((c & 0xE0) == 0xC0) {
        c = ((c & 0x1F) << 6) | (*s++ & 0x3F);
    } else if ((c & 0xF0) == 0xE0) {
        c = ((c & 0x0F) << 12) | ((*s++ & 0x3F) << 6);
        c |= (*s++ & 0x3F);
    } else if ((c & 0xF8) == 0xF0) {
        c = ((c & 0x07) << 18) | ((*s++ & 0x3F) << 12);
        c |= ((*s++ & 0x3F) << 6);
        c |= (*s++ & 0x3F);
    }

    *str = (const char*)s;
    return c;
}

bool parse_icon_tag(const char** str, uint32_t* out_icon_id) {
    const char* s = *str;
    if (s[0] != '{') return false;
    if (s[1] != 'i' || s[2] != 'c' || s[3] != 'o' || s[4] != 'n' || s[5] != ':')
        return false;

    s += 6;
    uint32_t id = 0;
    if (*s < '0' || *s > '9') return false;
    while (*s >= '0' && *s <= '9') {
        id = id * 10 + (*s - '0');
        s++;
    }
    if (*s != '}') return false;
    s++;

    *out_icon_id = id;
    *str = s;
    return true;
}
