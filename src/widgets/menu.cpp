#include "../context.h"
#include <cstring>
#include <cmath>
#include <cstdio>

bool monstro_begin_menu_bar(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return false;
    
    MonstroWindowInternal* win = ctx->current_window;
    if (win->skip_items) return false;
    
    if (!(win->flags & MONSTRO_WINDOW_MENU_BAR)) {
        return false;
    }
    
    MonstroStyle* style = &ctx->style;
    
    // Get menu bar rect position relative to window (like C#: win.menuBarRect.position - win.position)
    MonstroRect menu_bar_rect = win->getMenuBarRect(style);
    float rel_x = menu_bar_rect.x - win->pos_x;
    float rel_y = menu_bar_rect.y - win->pos_y;
    
    // Begin layout at the menu bar position (like C#: win.beginLayout(position, Direction.Horizontal))
    win->beginLayoutAt(rel_x, rel_y, MONSTRO_LAYOUT_HORIZONTAL);
    
    ctx->pushId(monstro_hash_string("menuBar"));
    
    return true;
}

void monstro_end_menu_bar(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;
    
    MonstroWindowInternal* win = ctx->current_window;
    if (win->skip_items) return;
    
    ctx->popId();
    
    // End layout like C#: win.endLayout() - this adds the menu bar size to parent layout
    win->endLayout();
}

bool monstro_begin_menu(MonstroContext* ctx, const char* label, bool enabled) {
    if (!ctx || !ctx->current_window || !label) return false;
    
    MonstroWindowInternal* win = ctx->current_window;
    if (win->skip_items) return false;
    
    MonstroStyle* style = &ctx->style;
    MonstroFont* font = style->font;
    
    (void)enabled; // TODO: use for disabled styling
    
    MonstroVec2 text_size = monstro_font_measure_text(font, label, 0.0f);
    if (text_size.x == 0.0f) text_size.x = (float)strlen(label) * 8.0f;
    if (text_size.y == 0.0f) text_size.y = 16.0f;
    
    float label_width = text_size.x + style->menu.padding[0];
    float label_height = win->getMenuBarHeight(style);
    if (label_height == 0.0f) {
        label_height = text_size.y + style->menu.padding[1] * 2.0f;
    }
    
    bool pressed = false;
    bool opened = monstro_is_popup_open(ctx, label);
    
    MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
    MonstroRect menu_bar_rect = win->getMenuBarRect(style);
    float popup_x = pos.x;
    float popup_y = pos.y + menu_bar_rect.h;
    
    bool should_open = false;
    MonstroSelectableFlags sel_flags = (MonstroSelectableFlags)(
        MONSTRO_SELECTABLE_MENU | MONSTRO_SELECTABLE_DONT_CLOSE_POPUP
    );
    if (!enabled) {
        sel_flags = (MonstroSelectableFlags)(sel_flags | MONSTRO_SELECTABLE_DISABLED);
    }
    pressed = monstro_selectable_sized(ctx, label, &should_open, label_width, label_height, sel_flags, nullptr);
    
    if (!opened && should_open) {
        monstro_open_popup(ctx, label);
        return false;
    }
    
    if (opened) {
        if (pressed) {
            monstro_close_current_popup(ctx);
            return false;
        } else {
            // Set position for the popup window
            monstro_set_next_window_pos(ctx, popup_x, popup_y, MONSTRO_COND_ALWAYS);
            monstro_set_next_window_size(ctx, 10.0f, 10.0f, MONSTRO_COND_FIRST_USE_EVER);
            
            // Use beginPopup which checks the popup stack and creates the window
            // Pass MONSTRO_WINDOW_CHILD_MENU flag to indicate this is a menu popup
            opened = monstro_begin_popup(ctx, label, MONSTRO_WINDOW_CHILD_MENU);
        }
    }
    
    return opened;
}

void monstro_end_menu(MonstroContext* ctx) {
    monstro_end_popup(ctx);
}

bool monstro_menu_item(MonstroContext* ctx, const char* label, bool* selected, bool enabled) {
    if (!ctx || !ctx->current_window || !label) return false;
    
    MonstroWindowInternal* win = ctx->current_window;
    if (win->skip_items) return false;
    
    MonstroStyle* style = &ctx->style;
    MonstroFont* font = style->font;
    
    MonstroVec2 label_size = monstro_font_measure_text(font, label, 0.0f);
    if (label_size.x == 0.0f) label_size.x = (float)strlen(label) * 8.0f;
    if (label_size.y == 0.0f) label_size.y = 16.0f;
    
    MonstroSelectableFlags sel_flags = MONSTRO_SELECTABLE_MENU_ITEM;
    if (!enabled) {
        sel_flags = (MonstroSelectableFlags)(sel_flags | MONSTRO_SELECTABLE_DISABLED);
    }
    
    bool temp_selected = selected ? *selected : false;
    bool pressed = monstro_selectable(ctx, label, &temp_selected, sel_flags, nullptr);
    
    if (pressed && selected) {
        *selected = temp_selected;
    }
    
    return pressed;
}
