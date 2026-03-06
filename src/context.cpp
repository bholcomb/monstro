#include "internal/types.h"
#include <stdlib.h>
#include <string.h>

MonstroContext* monstro_context_create(void) {
    MonstroContext* ctx = (MonstroContext*)calloc(1, sizeof(MonstroContext));
    if (!ctx) return nullptr;
    
    ctx->canvas = monstro_canvas_create();
    if (!ctx->canvas) {
        free(ctx);
        return nullptr;
    }
    
    ctx->font_manager = monstro_font_manager_create();
    if (!ctx->font_manager) {
        monstro_canvas_destroy(ctx->canvas);
        free(ctx);
        return nullptr;
    }
    
    monstro_style_set_default(&ctx->style);
    
    ctx->double_click_time = 0.30f;
    ctx->double_click_distance = 6.0f;
    ctx->mouse_drag_threshold = 6.0f;
    ctx->key_repeat_delay = 0.25f;
    ctx->key_repeat_rate = 0.05f;
    ctx->mouse_cursor = MONSTRO_MOUSE_CURSOR_ARROW;
    
    // Initialize mouse button down_time to -1 (not pressed) - C# pattern
    for (int i = 0; i < MONSTRO_MOUSE_BUTTON_COUNT; i++) {
        ctx->mouse.buttons[i].down_time = -1.0f;
        ctx->mouse.buttons[i].clicked_time = -1.0f;
    }
    
    // Initialize key down_time to -1 (not pressed) - C# pattern
    for (int i = 0; i < MONSTRO_KEY_COUNT; i++) {
        ctx->keyboard.keys[i].down_time = -1.0f;
    }
    
    ctx->id_stack[0] = 0;
    ctx->id_stack_top = 0;
    
    return ctx;
}

void monstro_context_destroy(MonstroContext* ctx) {
    if (!ctx) return;
    
    if (ctx->font_manager) {
        monstro_font_manager_destroy(ctx->font_manager);
    }
    
    if (ctx->canvas) {
        monstro_canvas_destroy(ctx->canvas);
    }
    
    free(ctx);
}

MonstroCanvas* monstro_context_get_canvas(MonstroContext* ctx) {
    if (!ctx) return nullptr;
    return ctx->canvas;
}

void monstro_context_set_default_font(MonstroContext* ctx, MonstroFont* font) {
    if (!ctx) return;
    // Set both the internal default and the style font
    // In the future, default_font will be the embedded fallback
    // For now, they're the same
    ctx->default_font = font;
    ctx->style.font = font;
}

MonstroFont* monstro_context_get_default_font(MonstroContext* ctx) {
    if (!ctx) return nullptr;
    return ctx->style.font;
}

void monstro_begin_frame(MonstroContext* ctx, float display_width, float display_height, float delta_time) {
    if (!ctx) return;
    
    ctx->display_width = display_width;
    ctx->display_height = display_height;
    ctx->delta_time = delta_time;
    ctx->time += delta_time;
    ctx->frame++;
    
    ctx->hovered_id_prev_frame = ctx->hovered_id;
    ctx->hovered_id = 0;
    
    // Clear active_id if it wasn't kept alive in the previous frame
    if (!ctx->active_id_is_alive && ctx->active_id_prev_frame == ctx->active_id && ctx->active_id != 0) {
        monstro_set_active_id(ctx, 0);
    }
    ctx->active_id_prev_frame = ctx->active_id;
    ctx->active_id_is_alive = false;
    
    for (int i = 0; i < MONSTRO_MOUSE_BUTTON_COUNT; i++) {
        MonstroMouseButtonState* btn = &ctx->mouse.buttons[i];
        
        // C# pattern: down_time < 0 means "not pressed", >= 0 means "pressed"
        // pressed = button is down AND was not down last frame (down_time was negative)
        btn->pressed = btn->down && (btn->down_time < 0.0f);
        // released = button is up AND was down last frame (down_time was >= 0)
        btn->released = !btn->down && (btn->down_time >= 0.0f);
        btn->double_clicked = false;
        
        if (btn->pressed) {
            float dx = ctx->mouse.pos_x - btn->clicked_pos_x;
            float dy = ctx->mouse.pos_y - btn->clicked_pos_y;
            float dist_sq = dx * dx + dy * dy;
            float time_since_click = ctx->time - btn->clicked_time;
            
            if (time_since_click < ctx->double_click_time &&
                dist_sq < ctx->double_click_distance * ctx->double_click_distance) {
                btn->double_clicked = true;
                btn->clicked_time = -1.0f;  // Prevent triple-click detection
            } else {
                btn->clicked_time = ctx->time;
            }
            
            btn->clicked_pos_x = ctx->mouse.pos_x;
            btn->clicked_pos_y = ctx->mouse.pos_y;
        }
        
        // C# pattern: down_time = down ? (was_negative ? 0 : down_time + dt) : -1
        if (btn->down) {
            btn->down_time = (btn->down_time < 0.0f) ? 0.0f : btn->down_time + delta_time;
        } else {
            btn->down_time = -1.0f;
        }
    }
    
    for (int i = 0; i < MONSTRO_KEY_COUNT; i++) {
        MonstroKeyState* key = &ctx->keyboard.keys[i];
        
        // C# pattern: down_time < 0 means "not pressed", >= 0 means "pressed"
        key->pressed = key->down && (key->down_time < 0.0f);
        key->released = !key->down && (key->down_time >= 0.0f);
        
        if (key->down) {
            key->down_time = (key->down_time < 0.0f) ? 0.0f : key->down_time + delta_time;
        } else {
            key->down_time = -1.0f;
        }
    }
    
    ctx->mouse.delta_x = ctx->mouse.pos_x - ctx->mouse.prev_pos_x;
    ctx->mouse.delta_y = ctx->mouse.pos_y - ctx->mouse.prev_pos_y;
    ctx->mouse.prev_pos_x = ctx->mouse.pos_x;
    ctx->mouse.prev_pos_y = ctx->mouse.pos_y;
    
    // Find hovered window using previous frame's window data
    ctx->hovered_window = monstro_find_hovered_window(ctx, ctx->mouse.pos_x, ctx->mouse.pos_y);
    
    // Reset all windows to inactive for this frame
    for (int i = 0; i < ctx->window_count; i++) {
        ctx->windows[i].active = false;
    }
    
    monstro_canvas_clear(ctx->canvas);
    
    ctx->next_window_pos_cond = MONSTRO_COND_NONE;
    ctx->next_window_size_cond = MONSTRO_COND_NONE;
    ctx->next_window_collapsed_cond = MONSTRO_COND_NONE;
    
    ctx->want_capture_mouse = false;
    ctx->want_capture_keyboard = false;
    ctx->want_text_input = false;
    
    ctx->current_window = nullptr;
    
    ctx->mouse_cursor = MONSTRO_MOUSE_CURSOR_ARROW;
    
    ctx->id_stack_top = 0;
    ctx->id_stack[0] = 0;
}

void monstro_end_frame(MonstroContext* ctx) {
    if (!ctx) return;
    
    // Note: active_id cleanup is now done in begin_frame, matching C# implementation
    
    ctx->mouse.wheel_x = 0.0f;
    ctx->mouse.wheel_y = 0.0f;
    ctx->keyboard.input_text_len = 0;
    ctx->keyboard.input_text[0] = '\0';
}

float monstro_get_delta_time(MonstroContext* ctx) {
    return ctx ? ctx->delta_time : 0.0f;
}

float monstro_get_time(MonstroContext* ctx) {
    return ctx ? ctx->time : 0.0f;
}

MonstroCanvas* monstro_get_canvas(MonstroContext* ctx) {
    return ctx ? ctx->canvas : nullptr;
}

MonstroStyle* monstro_get_style(MonstroContext* ctx) {
    return ctx ? &ctx->style : nullptr;
}

/* ------------------------------------------------------------------------- */
/* ID Stack                                                                  */
/* ------------------------------------------------------------------------- */

uint64_t monstro_hash_string(const char* str) {
    if (!str) return 0;
    uint64_t hash = 14695981039346656037ULL;
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= 1099511628211ULL;
    }
    return hash;
}

uint64_t monstro_hash_combine(uint64_t seed, uint64_t value) {
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

uint64_t monstro_hash_data(const void* data, size_t size, uint64_t seed) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t hash = seed;
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

void monstro_push_id(MonstroContext* ctx, uint64_t id) {
    if (!ctx || ctx->id_stack_top >= MONSTRO_MAX_ID_STACK_DEPTH - 1) return;
    
    // C# pattern: push the ID directly onto the stack (no combining here)
    // Combining happens in getId when the hash is created
    ctx->id_stack_top++;
    ctx->id_stack[ctx->id_stack_top] = id;
}

void monstro_pop_id(MonstroContext* ctx) {
    if (!ctx || ctx->id_stack_top <= 0) return;
    ctx->id_stack_top--;
}

uint64_t monstro_get_id(MonstroContext* ctx, uint64_t id) {
    if (!ctx) return 0;
    uint64_t combined_id = monstro_hash_combine(ctx->id_stack[ctx->id_stack_top], id);
    // Keep this ID alive (matches C# getChildId behavior)
    monstro_keep_alive_id(ctx, combined_id);
    return combined_id;
}

/* ------------------------------------------------------------------------- */
/* Active/Hovered ID Helpers                                                 */
/* ------------------------------------------------------------------------- */

void monstro_set_active_id(MonstroContext* ctx, uint64_t id) {
    if (!ctx) return;
    // C# pattern: just set the ID, don't auto-set alive flag
    // Alive flag is set by keepAliveId or by monstro_get_id
    ctx->active_id = id;
}

void monstro_keep_alive_id(MonstroContext* ctx, uint64_t id) {
    if (!ctx) return;
    if (ctx->active_id == id) {
        ctx->active_id_is_alive = true;
    }
}
