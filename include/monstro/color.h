#ifndef MONSTRO_COLOR_H
#define MONSTRO_COLOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MonstroColor {
    float r, g, b, a;
} MonstroColor;

static inline uint32_t monstro_color_to_u32(MonstroColor c) {
    uint8_t ri = (uint8_t)(c.r * 255.0f + 0.5f);
    uint8_t gi = (uint8_t)(c.g * 255.0f + 0.5f);
    uint8_t bi = (uint8_t)(c.b * 255.0f + 0.5f);
    uint8_t ai = (uint8_t)(c.a * 255.0f + 0.5f);
    return ((uint32_t)ai << 24) | ((uint32_t)bi << 16) | ((uint32_t)gi << 8) | (uint32_t)ri;
}

static inline MonstroColor monstro_color_from_u32(uint32_t c) {
    MonstroColor color;
    color.r = (float)((c >> 0) & 0xFF) / 255.0f;
    color.g = (float)((c >> 8) & 0xFF) / 255.0f;
    color.b = (float)((c >> 16) & 0xFF) / 255.0f;
    color.a = (float)((c >> 24) & 0xFF) / 255.0f;
    return color;
}

static inline MonstroColor monstro_color_rgba(float r, float g, float b, float a) {
    MonstroColor c = { r, g, b, a };
    return c;
}

static inline MonstroColor monstro_color_rgb(float r, float g, float b) {
    MonstroColor c = { r, g, b, 1.0f };
    return c;
}

#define MONSTRO_COLOR_WHITE          ((MonstroColor){ 1.0f, 1.0f, 1.0f, 1.0f })
#define MONSTRO_COLOR_BLACK          ((MonstroColor){ 0.0f, 0.0f, 0.0f, 1.0f })
#define MONSTRO_COLOR_TRANSPARENT    ((MonstroColor){ 0.0f, 0.0f, 0.0f, 0.0f })

#define MONSTRO_COLOR_RED            ((MonstroColor){ 1.0f, 0.0f, 0.0f, 1.0f })
#define MONSTRO_COLOR_GREEN          ((MonstroColor){ 0.0f, 1.0f, 0.0f, 1.0f })
#define MONSTRO_COLOR_BLUE           ((MonstroColor){ 0.0f, 0.0f, 1.0f, 1.0f })

#define MONSTRO_COLOR_YELLOW         ((MonstroColor){ 1.0f, 1.0f, 0.0f, 1.0f })
#define MONSTRO_COLOR_CYAN           ((MonstroColor){ 0.0f, 1.0f, 1.0f, 1.0f })
#define MONSTRO_COLOR_MAGENTA        ((MonstroColor){ 1.0f, 0.0f, 1.0f, 1.0f })

#define MONSTRO_COLOR_GRAY           ((MonstroColor){ 0.5f, 0.5f, 0.5f, 1.0f })
#define MONSTRO_COLOR_LIGHT_GRAY     ((MonstroColor){ 0.75f, 0.75f, 0.75f, 1.0f })
#define MONSTRO_COLOR_DARK_GRAY      ((MonstroColor){ 0.25f, 0.25f, 0.25f, 1.0f })

#define MONSTRO_COLOR_ORANGE         ((MonstroColor){ 1.0f, 0.5f, 0.0f, 1.0f })
#define MONSTRO_COLOR_PINK           ((MonstroColor){ 1.0f, 0.75f, 0.8f, 1.0f })
#define MONSTRO_COLOR_PURPLE         ((MonstroColor){ 0.5f, 0.0f, 0.5f, 1.0f })
#define MONSTRO_COLOR_BROWN          ((MonstroColor){ 0.6f, 0.3f, 0.0f, 1.0f })

#define MONSTRO_COLOR_NAVY           ((MonstroColor){ 0.0f, 0.0f, 0.5f, 1.0f })
#define MONSTRO_COLOR_TEAL           ((MonstroColor){ 0.0f, 0.5f, 0.5f, 1.0f })
#define MONSTRO_COLOR_MAROON         ((MonstroColor){ 0.5f, 0.0f, 0.0f, 1.0f })
#define MONSTRO_COLOR_OLIVE          ((MonstroColor){ 0.5f, 0.5f, 0.0f, 1.0f })
#define MONSTRO_COLOR_LIME           ((MonstroColor){ 0.0f, 1.0f, 0.0f, 1.0f })
#define MONSTRO_COLOR_AQUA           ((MonstroColor){ 0.0f, 1.0f, 1.0f, 1.0f })
#define MONSTRO_COLOR_FUCHSIA        ((MonstroColor){ 1.0f, 0.0f, 1.0f, 1.0f })
#define MONSTRO_COLOR_SILVER         ((MonstroColor){ 0.75f, 0.75f, 0.75f, 1.0f })

#define MONSTRO_COLOR_CORAL          ((MonstroColor){ 1.0f, 0.5f, 0.31f, 1.0f })
#define MONSTRO_COLOR_SALMON         ((MonstroColor){ 0.98f, 0.5f, 0.45f, 1.0f })
#define MONSTRO_COLOR_GOLD           ((MonstroColor){ 1.0f, 0.84f, 0.0f, 1.0f })
#define MONSTRO_COLOR_INDIGO         ((MonstroColor){ 0.29f, 0.0f, 0.51f, 1.0f })
#define MONSTRO_COLOR_VIOLET         ((MonstroColor){ 0.93f, 0.51f, 0.93f, 1.0f })
#define MONSTRO_COLOR_TURQUOISE      ((MonstroColor){ 0.25f, 0.88f, 0.82f, 1.0f })
#define MONSTRO_COLOR_SLATE          ((MonstroColor){ 0.44f, 0.5f, 0.56f, 1.0f })
#define MONSTRO_COLOR_CHARCOAL       ((MonstroColor){ 0.21f, 0.27f, 0.31f, 1.0f })

#define MONSTRO_COLOR_WHITE_50       ((MonstroColor){ 1.0f, 1.0f, 1.0f, 0.5f })
#define MONSTRO_COLOR_BLACK_50       ((MonstroColor){ 0.0f, 0.0f, 0.0f, 0.5f })
#define MONSTRO_COLOR_RED_50         ((MonstroColor){ 1.0f, 0.0f, 0.0f, 0.5f })
#define MONSTRO_COLOR_GREEN_50       ((MonstroColor){ 0.0f, 1.0f, 0.0f, 0.5f })
#define MONSTRO_COLOR_BLUE_50        ((MonstroColor){ 0.0f, 0.0f, 1.0f, 0.5f })
#define MONSTRO_COLOR_YELLOW_50      ((MonstroColor){ 1.0f, 1.0f, 0.0f, 0.5f })
#define MONSTRO_COLOR_CYAN_50        ((MonstroColor){ 0.0f, 1.0f, 1.0f, 0.5f })
#define MONSTRO_COLOR_MAGENTA_50     ((MonstroColor){ 1.0f, 0.0f, 1.0f, 0.5f })
#define MONSTRO_COLOR_GRAY_50        ((MonstroColor){ 0.5f, 0.5f, 0.5f, 0.5f })

#ifdef __cplusplus
}
#endif

#endif
