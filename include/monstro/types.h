#ifndef MONSTRO_TYPES_H
#define MONSTRO_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Forward Declarations                                                      */
/* ------------------------------------------------------------------------- */

typedef struct MonstroContext MonstroContext;
typedef struct MonstroCanvas MonstroCanvas;
typedef struct MonstroFont MonstroFont;
typedef struct MonstroFontManager MonstroFontManager;

/* ------------------------------------------------------------------------- */
/* Core Types                                                                */
/* ------------------------------------------------------------------------- */

typedef struct MonstroVec2 {
    float x, y;
} MonstroVec2;

typedef struct MonstroRect {
    float x, y, w, h;
} MonstroRect;

static inline bool monstro_rect_contains(MonstroRect r, float px, float py) {
    return px >= r.x && px < r.x + r.w &&
           py >= r.y && py < r.y + r.h;
}

typedef struct MonstroColor {
    float r, g, b, a;
} MonstroColor;

/* ------------------------------------------------------------------------- */
/* Color Helpers                                                             */
/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */
/* Alignment                                                                 */
/* ------------------------------------------------------------------------- */

typedef enum MonstroAlign {
    MONSTRO_ALIGN_LEFT      = 1 << 0,
    MONSTRO_ALIGN_RIGHT     = 1 << 1,
    MONSTRO_ALIGN_TOP       = 1 << 2,
    MONSTRO_ALIGN_BOTTOM    = 1 << 3,
    MONSTRO_ALIGN_HCENTER   = MONSTRO_ALIGN_LEFT | MONSTRO_ALIGN_RIGHT,
    MONSTRO_ALIGN_VCENTER   = MONSTRO_ALIGN_TOP | MONSTRO_ALIGN_BOTTOM,
    MONSTRO_ALIGN_CENTER    = MONSTRO_ALIGN_HCENTER | MONSTRO_ALIGN_VCENTER,
    MONSTRO_ALIGN_DEFAULT   = MONSTRO_ALIGN_LEFT | MONSTRO_ALIGN_VCENTER
} MonstroAlign;

/* ------------------------------------------------------------------------- */
/* Corner Flags                                                              */
/* ------------------------------------------------------------------------- */

typedef enum MonstroCornerFlags {
    MONSTRO_CORNER_NONE   = 0,
    MONSTRO_CORNER_LL     = 1 << 0,
    MONSTRO_CORNER_LR     = 1 << 1,
    MONSTRO_CORNER_UR     = 1 << 2,
    MONSTRO_CORNER_UL     = 1 << 3,
    MONSTRO_CORNER_TOP    = MONSTRO_CORNER_UL | MONSTRO_CORNER_UR,
    MONSTRO_CORNER_BOTTOM = MONSTRO_CORNER_LL | MONSTRO_CORNER_LR,
    MONSTRO_CORNER_LEFT   = MONSTRO_CORNER_LL | MONSTRO_CORNER_UL,
    MONSTRO_CORNER_RIGHT  = MONSTRO_CORNER_LR | MONSTRO_CORNER_UR,
    MONSTRO_CORNER_ALL    = MONSTRO_CORNER_LL | MONSTRO_CORNER_LR | MONSTRO_CORNER_UL | MONSTRO_CORNER_UR
} MonstroCornerFlags;

/* ------------------------------------------------------------------------- */
/* Window Flags and Conditions                                               */
/* ------------------------------------------------------------------------- */

typedef enum MonstroWindowFlags {
    MONSTRO_WINDOW_NONE                 = 0,

    /* Decorations */
    MONSTRO_WINDOW_BACKGROUND           = 1 << 0,
    MONSTRO_WINDOW_TITLE_BAR            = 1 << 1,
    MONSTRO_WINDOW_MENU_BAR             = 1 << 2,
    MONSTRO_WINDOW_BORDERS              = 1 << 3,
    MONSTRO_WINDOW_HORIZONTAL_SCROLLBAR = 1 << 4,
    MONSTRO_WINDOW_VERTICAL_SCROLLBAR   = 1 << 5,
    MONSTRO_WINDOW_SCROLLBARS           = MONSTRO_WINDOW_HORIZONTAL_SCROLLBAR | MONSTRO_WINDOW_VERTICAL_SCROLLBAR,

    /* Behavior */
    MONSTRO_WINDOW_MOVABLE              = 1 << 10,
    MONSTRO_WINDOW_COLLAPSABLE          = 1 << 11,
    MONSTRO_WINDOW_RESIZABLE            = 1 << 12,
    MONSTRO_WINDOW_INPUTS               = 1 << 13,
    MONSTRO_WINDOW_SCROLL_WITH_MOUSE    = 1 << 14,
    MONSTRO_WINDOW_FOCUS_ON_APPEARING   = 1 << 15,
    MONSTRO_WINDOW_BRING_TO_FRONT       = 1 << 16,
    MONSTRO_WINDOW_AUTO_RESIZE          = 1 << 17,

    /* Window Types */
    MONSTRO_WINDOW_ROOT                 = 1 << 20,
    MONSTRO_WINDOW_CHILD                = 1 << 21,
    MONSTRO_WINDOW_COMBO                = 1 << 22,
    MONSTRO_WINDOW_TOOLTIP              = 1 << 23,
    MONSTRO_WINDOW_POPUP                = 1 << 24,
    MONSTRO_WINDOW_MODAL                = 1 << 25,
    MONSTRO_WINDOW_CHILD_MENU           = 1 << 26,

    /* Common Presets */
    MONSTRO_WINDOW_DEFAULT = MONSTRO_WINDOW_BACKGROUND | MONSTRO_WINDOW_TITLE_BAR | MONSTRO_WINDOW_MENU_BAR |
                             MONSTRO_WINDOW_BORDERS | MONSTRO_WINDOW_MOVABLE | MONSTRO_WINDOW_COLLAPSABLE |
                             MONSTRO_WINDOW_RESIZABLE | MONSTRO_WINDOW_INPUTS | MONSTRO_WINDOW_BRING_TO_FRONT |
                             MONSTRO_WINDOW_CHILD,

    MONSTRO_WINDOW_ROOT_DEFAULT = MONSTRO_WINDOW_MENU_BAR | MONSTRO_WINDOW_INPUTS | MONSTRO_WINDOW_ROOT
} MonstroWindowFlags;

typedef enum MonstroCondition {
    MONSTRO_COND_NONE           = 0,
    MONSTRO_COND_ALWAYS         = 1 << 0,
    MONSTRO_COND_ONCE           = 1 << 1,
    MONSTRO_COND_FIRST_USE_EVER = 1 << 2,
    MONSTRO_COND_APPEARING      = 1 << 3
} MonstroCondition;

/* ------------------------------------------------------------------------- */
/* Layout Direction                                                          */
/* ------------------------------------------------------------------------- */

typedef enum MonstroLayoutDirection {
    MONSTRO_LAYOUT_HORIZONTAL = 0,
    MONSTRO_LAYOUT_VERTICAL = 1
} MonstroLayoutDirection;

/* ------------------------------------------------------------------------- */
/* Input Enums                                                               */
/* ------------------------------------------------------------------------- */

typedef enum MonstroKey {
    MONSTRO_KEY_NONE = 0,
    MONSTRO_KEY_TAB,
    MONSTRO_KEY_LEFT_ARROW,
    MONSTRO_KEY_RIGHT_ARROW,
    MONSTRO_KEY_UP_ARROW,
    MONSTRO_KEY_DOWN_ARROW,
    MONSTRO_KEY_PAGE_UP,
    MONSTRO_KEY_PAGE_DOWN,
    MONSTRO_KEY_HOME,
    MONSTRO_KEY_END,
    MONSTRO_KEY_INSERT,
    MONSTRO_KEY_DELETE,
    MONSTRO_KEY_BACKSPACE,
    MONSTRO_KEY_SPACE,
    MONSTRO_KEY_ENTER,
    MONSTRO_KEY_ESCAPE,
    MONSTRO_KEY_LEFT_CTRL,
    MONSTRO_KEY_LEFT_SHIFT,
    MONSTRO_KEY_LEFT_ALT,
    MONSTRO_KEY_LEFT_SUPER,
    MONSTRO_KEY_RIGHT_CTRL,
    MONSTRO_KEY_RIGHT_SHIFT,
    MONSTRO_KEY_RIGHT_ALT,
    MONSTRO_KEY_RIGHT_SUPER,
    MONSTRO_KEY_MENU,
    MONSTRO_KEY_0, MONSTRO_KEY_1, MONSTRO_KEY_2, MONSTRO_KEY_3, MONSTRO_KEY_4,
    MONSTRO_KEY_5, MONSTRO_KEY_6, MONSTRO_KEY_7, MONSTRO_KEY_8, MONSTRO_KEY_9,
    MONSTRO_KEY_A, MONSTRO_KEY_B, MONSTRO_KEY_C, MONSTRO_KEY_D, MONSTRO_KEY_E,
    MONSTRO_KEY_F, MONSTRO_KEY_G, MONSTRO_KEY_H, MONSTRO_KEY_I, MONSTRO_KEY_J,
    MONSTRO_KEY_K, MONSTRO_KEY_L, MONSTRO_KEY_M, MONSTRO_KEY_N, MONSTRO_KEY_O,
    MONSTRO_KEY_P, MONSTRO_KEY_Q, MONSTRO_KEY_R, MONSTRO_KEY_S, MONSTRO_KEY_T,
    MONSTRO_KEY_U, MONSTRO_KEY_V, MONSTRO_KEY_W, MONSTRO_KEY_X, MONSTRO_KEY_Y,
    MONSTRO_KEY_Z,
    MONSTRO_KEY_F1, MONSTRO_KEY_F2, MONSTRO_KEY_F3, MONSTRO_KEY_F4, MONSTRO_KEY_F5,
    MONSTRO_KEY_F6, MONSTRO_KEY_F7, MONSTRO_KEY_F8, MONSTRO_KEY_F9, MONSTRO_KEY_F10,
    MONSTRO_KEY_F11, MONSTRO_KEY_F12,
    MONSTRO_KEY_APOSTROPHE,
    MONSTRO_KEY_COMMA,
    MONSTRO_KEY_MINUS,
    MONSTRO_KEY_PERIOD,
    MONSTRO_KEY_SLASH,
    MONSTRO_KEY_SEMICOLON,
    MONSTRO_KEY_EQUAL,
    MONSTRO_KEY_LEFT_BRACKET,
    MONSTRO_KEY_BACKSLASH,
    MONSTRO_KEY_RIGHT_BRACKET,
    MONSTRO_KEY_GRAVE_ACCENT,
    MONSTRO_KEY_CAPS_LOCK,
    MONSTRO_KEY_SCROLL_LOCK,
    MONSTRO_KEY_NUM_LOCK,
    MONSTRO_KEY_PRINT_SCREEN,
    MONSTRO_KEY_PAUSE,
    MONSTRO_KEY_KEYPAD_0, MONSTRO_KEY_KEYPAD_1, MONSTRO_KEY_KEYPAD_2,
    MONSTRO_KEY_KEYPAD_3, MONSTRO_KEY_KEYPAD_4, MONSTRO_KEY_KEYPAD_5,
    MONSTRO_KEY_KEYPAD_6, MONSTRO_KEY_KEYPAD_7, MONSTRO_KEY_KEYPAD_8,
    MONSTRO_KEY_KEYPAD_9,
    MONSTRO_KEY_KEYPAD_DECIMAL,
    MONSTRO_KEY_KEYPAD_DIVIDE,
    MONSTRO_KEY_KEYPAD_MULTIPLY,
    MONSTRO_KEY_KEYPAD_SUBTRACT,
    MONSTRO_KEY_KEYPAD_ADD,
    MONSTRO_KEY_KEYPAD_ENTER,
    MONSTRO_KEY_KEYPAD_EQUAL,
    MONSTRO_KEY_COUNT
} MonstroKey;

typedef enum MonstroMouseButton {
    MONSTRO_MOUSE_BUTTON_LEFT = 0,
    MONSTRO_MOUSE_BUTTON_RIGHT = 1,
    MONSTRO_MOUSE_BUTTON_MIDDLE = 2,
    MONSTRO_MOUSE_BUTTON_COUNT = 5
} MonstroMouseButton;

typedef enum MonstroModFlags {
    MONSTRO_MOD_NONE = 0,
    MONSTRO_MOD_CTRL = 1 << 0,
    MONSTRO_MOD_SHIFT = 1 << 1,
    MONSTRO_MOD_ALT = 1 << 2,
    MONSTRO_MOD_SUPER = 1 << 3
} MonstroModFlags;

typedef enum MonstroMouseCursor {
    MONSTRO_MOUSE_CURSOR_NONE = -1,
    MONSTRO_MOUSE_CURSOR_ARROW = 0,
    MONSTRO_MOUSE_CURSOR_TEXT_INPUT,
    MONSTRO_MOUSE_CURSOR_RESIZE_ALL,
    MONSTRO_MOUSE_CURSOR_RESIZE_NS,
    MONSTRO_MOUSE_CURSOR_RESIZE_EW,
    MONSTRO_MOUSE_CURSOR_RESIZE_NESW,
    MONSTRO_MOUSE_CURSOR_RESIZE_NWSE,
    MONSTRO_MOUSE_CURSOR_HAND,
    MONSTRO_MOUSE_CURSOR_NOT_ALLOWED,
    MONSTRO_MOUSE_CURSOR_CROSSHAIR,
    MONSTRO_MOUSE_CURSOR_WAIT,
    MONSTRO_MOUSE_CURSOR_COUNT
} MonstroMouseCursor;

/* ------------------------------------------------------------------------- */
/* Canvas Types                                                              */
/* ------------------------------------------------------------------------- */

typedef void (*MonstroCustomDrawCallback)(void* user_data, const float clip_rect[4], int layer);

typedef struct MonstroVertex {
    float position[2];
    float texcoord[2];
    uint32_t color;
} MonstroVertex;

typedef struct MonstroDrawCommand {
    float clip_rect[4];
    int layer;
    uint32_t order;
    uint64_t texture_id;
    uint32_t index_offset;
    uint32_t index_count;
    MonstroCustomDrawCallback custom_draw;
    void* user_data;
} MonstroDrawCommand;

typedef struct MonstroNinePatch {
    uint64_t texture_id;
    MonstroVec2 uv_min;
    MonstroVec2 uv_max;
    float border_left;
    float border_right;
    float border_top;
    float border_bottom;
    uint32_t texture_width;
    uint32_t texture_height;
} MonstroNinePatch;

/* ------------------------------------------------------------------------- */
/* Style Types                                                               */
/* ------------------------------------------------------------------------- */

typedef enum MonstroStyleItemType {
    MONSTRO_STYLE_ITEM_COLOR = 0,
    MONSTRO_STYLE_ITEM_IMAGE = 1
} MonstroStyleItemType;

typedef struct MonstroStyleItem {
    MonstroStyleItemType type;
    union {
        MonstroColor color;
        uint64_t texture_id;
    } data;
} MonstroStyleItem;

/* ------------------------------------------------------------------------- */
/* Font Types                                                                */
/* ------------------------------------------------------------------------- */

typedef enum MonstroTextureFormat {
    MONSTRO_TEXTURE_FORMAT_R8 = 0,
    MONSTRO_TEXTURE_FORMAT_RGBA8 = 1
} MonstroTextureFormat;

typedef struct MonstroGlyph {
    float min_tex_coord[2];
    float max_tex_coord[2];
    float size[2];
    float offset[2];
    float advance[2];
} MonstroGlyph;

typedef struct MonstroTextureData {
    uint32_t width;
    uint32_t height;
    MonstroTextureFormat format;
    size_t data_size;
    const void* data;
} MonstroTextureData;

/* ------------------------------------------------------------------------- */
/* Color Constants                                                           */
/* ------------------------------------------------------------------------- */

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
