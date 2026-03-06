#include "../include/monstro/style.h"
#include <string.h>

// Helper to create color from 0-255 values
static inline MonstroColor rgba(int r, int g, int b, int a) {
    return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
}

void monstro_style_set_default(MonstroStyle* style) {
    if (!style) return;
    
    memset(style, 0, sizeof(MonstroStyle));
    
    // Text
    style->text.color = rgba(200, 200, 200, 255);
    style->text.padding[0] = 4.0f;
    style->text.padding[1] = 4.0f;
    
    // Button
    style->button.normal.type = MONSTRO_STYLE_ITEM_COLOR;
    style->button.normal.data.color = rgba(50, 50, 50, 255);
    style->button.hover.type = MONSTRO_STYLE_ITEM_COLOR;
    style->button.hover.data.color = rgba(70, 70, 70, 255);
    style->button.active.type = MONSTRO_STYLE_ITEM_COLOR;
    style->button.active.data.color = rgba(35, 35, 35, 255);
    style->button.border_color = rgba(80, 80, 80, 255);
    
    style->button.text_normal = rgba(200, 200, 200, 255);
    style->button.text_hover = rgba(220, 220, 220, 255);
    style->button.text_active = rgba(180, 180, 180, 255);
    style->button.text_alignment = MONSTRO_ALIGN_CENTER;
    
    style->button.border = 1.0f;
    style->button.rounding = 4.0f;
    style->button.padding[0] = 8.0f;
    style->button.padding[1] = 4.0f;
    style->button.image_padding[0] = 2.0f;
    style->button.image_padding[1] = 2.0f;
    
    // Checkbox
    style->checkbox.normal.type = MONSTRO_STYLE_ITEM_COLOR;
    style->checkbox.normal.data.color = rgba(50, 50, 50, 255);
    style->checkbox.hover.type = MONSTRO_STYLE_ITEM_COLOR;
    style->checkbox.hover.data.color = rgba(70, 70, 70, 255);
    style->checkbox.active.type = MONSTRO_STYLE_ITEM_COLOR;
    style->checkbox.active.data.color = rgba(35, 35, 35, 255);
    style->checkbox.border_color = rgba(100, 100, 100, 255);
    
    style->checkbox.cursor_normal.type = MONSTRO_STYLE_ITEM_COLOR;
    style->checkbox.cursor_normal.data.color = rgba(100, 150, 200, 255);
    style->checkbox.cursor_hover.type = MONSTRO_STYLE_ITEM_COLOR;
    style->checkbox.cursor_hover.data.color = rgba(120, 170, 220, 255);
    
    style->checkbox.text_normal = rgba(200, 200, 200, 255);
    style->checkbox.text_hover = rgba(220, 220, 220, 255);
    style->checkbox.text_active = rgba(180, 180, 180, 255);
    style->checkbox.text_alignment = MONSTRO_ALIGN_LEFT;
    
    style->checkbox.padding[0] = 4.0f;
    style->checkbox.padding[1] = 4.0f;
    style->checkbox.spacing = 8.0f;
    style->checkbox.border = 1.0f;
    style->checkbox.rounding = 2.0f;
    
    // Separator
    style->separator.color = rgba(80, 80, 80, 255);
    style->separator.thickness = 1.0f;
    style->separator.padding[0] = 0.0f;
    style->separator.padding[1] = 4.0f;
    style->separator.text_padding[0] = 8.0f;
    style->separator.text_padding[1] = 0.0f;
    
    // Window
    style->window.background = rgba(30, 30, 30, 240);
    style->window.border_color = rgba(60, 60, 60, 255);
    
    style->window.header.normal.type = MONSTRO_STYLE_ITEM_COLOR;
    style->window.header.normal.data.color = rgba(40, 40, 40, 255);
    style->window.header.hover.type = MONSTRO_STYLE_ITEM_COLOR;
    style->window.header.hover.data.color = rgba(50, 50, 50, 255);
    style->window.header.active.type = MONSTRO_STYLE_ITEM_COLOR;
    style->window.header.active.data.color = rgba(35, 35, 35, 255);
    
    style->window.header.label_normal = rgba(200, 200, 200, 255);
    style->window.header.label_hover = rgba(220, 220, 220, 255);
    style->window.header.label_active = rgba(180, 180, 180, 255);
    
    style->window.header.padding[0] = 8.0f;
    style->window.header.padding[1] = 6.0f;
    style->window.header.label_padding[0] = 4.0f;
    style->window.header.label_padding[1] = 2.0f;
    
    style->window.border = 1.0f;
    style->window.rounding = 4.0f;
    style->window.spacing[0] = 6.0f;
    style->window.spacing[1] = 6.0f;
    style->window.padding[0] = 8.0f;
    style->window.padding[1] = 8.0f;
    
    // Tree
    style->tree.indent = 16.0f;
}

void monstro_style_set_dark(MonstroStyle* style) {
    monstro_style_set_default(style);
}

void monstro_style_set_light(MonstroStyle* style) {
    if (!style) return;
    
    monstro_style_set_default(style);
    
    // Override with light theme colors
    style->text.color = rgba(40, 40, 40, 255);
    
    style->button.normal.data.color = rgba(220, 220, 220, 255);
    style->button.hover.data.color = rgba(200, 200, 200, 255);
    style->button.active.data.color = rgba(180, 180, 180, 255);
    style->button.border_color = rgba(150, 150, 150, 255);
    style->button.text_normal = rgba(40, 40, 40, 255);
    style->button.text_hover = rgba(20, 20, 20, 255);
    style->button.text_active = rgba(60, 60, 60, 255);
    
    style->window.background = rgba(240, 240, 240, 255);
    style->window.border_color = rgba(180, 180, 180, 255);
    style->window.header.normal.data.color = rgba(220, 220, 220, 255);
    style->window.header.hover.data.color = rgba(210, 210, 210, 255);
    style->window.header.active.data.color = rgba(200, 200, 200, 255);
    style->window.header.label_normal = rgba(40, 40, 40, 255);
}
