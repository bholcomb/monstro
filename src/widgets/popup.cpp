#include "../context.h"
#include <cstring>
#include <cstdio>

static bool is_popup_open_internal(MonstroContext* ctx, uint64_t popup_id) {
    for (int i = 0; i < ctx->popup_stack_top; i++) {
        if (ctx->popup_stack[i].popup_id == popup_id) {
            return true;
        }
    }
    return false;
}

void monstro_open_popup(MonstroContext* ctx, const char* label) {
    if (!ctx || !ctx->current_window || !label) return;
    
    MonstroWindowInternal* win = ctx->current_window;
    uint64_t popup_id = ctx->getId(monstro_hash_string(label));
    
    if (is_popup_open_internal(ctx, popup_id)) return;
    if (ctx->popup_stack_top >= MONSTRO_MAX_POPUP_STACK) return;
    
    MonstroPopupInternal* popup = &ctx->popup_stack[ctx->popup_stack_top++];
    popup->popup_id = popup_id;
    popup->parent_window = win;
    popup->parent_id = ctx->getId(monstro_hash_string("popups"));
    popup->open_pos_x = ctx->mouse.pos_x;
    popup->open_pos_y = ctx->mouse.pos_y;
}

void monstro_close_current_popup(MonstroContext* ctx) {
    if (!ctx || ctx->popup_stack_top <= 0) return;
    ctx->popup_stack_top--;
}

bool monstro_begin_popup(MonstroContext* ctx, const char* label, MonstroWindowFlags extra_flags) {
    if (!ctx || !ctx->current_window || !label) return false;
    
    MonstroWindowInternal* parent_win = ctx->current_window;
    uint64_t popup_id = ctx->getId(monstro_hash_string(label));
    
    if (!is_popup_open_internal(ctx, popup_id)) {
        return false;
    }
    
    MonstroWindowFlags flags = (MonstroWindowFlags)(
        extra_flags | MONSTRO_WINDOW_POPUP | MONSTRO_WINDOW_AUTO_RESIZE
    );
    
    char popup_name[320];
    if (flags & MONSTRO_WINDOW_CHILD_MENU) {
        snprintf(popup_name, sizeof(popup_name), "menu_%s", label);
    } else if (flags & MONSTRO_WINDOW_COMBO) {
        snprintf(popup_name, sizeof(popup_name), "combo_%s", label);
    } else {
        snprintf(popup_name, sizeof(popup_name), "popup_%s", label);
    }
    
    bool closed = false;
    bool opened = monstro_begin_window(ctx, popup_name, &closed, flags);
    
    if (ctx->current_window) {
        ctx->current_window->makeLastSibling();
    }

    if (!(parent_win->flags & MONSTRO_WINDOW_BORDERS)) {
        if (ctx->current_window) {
            ctx->current_window->flags = (MonstroWindowFlags)(
                ctx->current_window->flags & ~MONSTRO_WINDOW_BORDERS);
        }
    }
    
    if (!opened) {
        monstro_end_popup(ctx);
    }
    
    return opened;
}

bool monstro_begin_popup_modal(MonstroContext* ctx, const char* label, bool* p_open, MonstroWindowFlags flags) {
    (void)ctx;
    (void)label;
    (void)p_open;
    (void)flags;
    return true;
}

bool monstro_begin_popup_context_item(MonstroContext* ctx, const char* label, MonstroMouseButton mouse_button) {
    if (!ctx || !label) return false;

    if (ctx->hovered_id != 0 && monstro_is_mouse_clicked(ctx, mouse_button, false)) {
        monstro_open_popup(ctx, label);
    }

    return monstro_begin_popup(ctx, label, (MonstroWindowFlags)0);
}

bool monstro_begin_popup_context_window(MonstroContext* ctx, const char* label, MonstroMouseButton mouse_button, bool also_over_items) {
    if (!ctx || !label) return false;

    if (ctx->hovered_window != nullptr && monstro_is_mouse_clicked(ctx, mouse_button, false)) {
        if (also_over_items || ctx->hovered_id == 0) {
            monstro_open_popup(ctx, label);
        }
    }

    return monstro_begin_popup(ctx, label, (MonstroWindowFlags)0);
}

bool monstro_begin_popup_context_void(MonstroContext* ctx, const char* label, MonstroMouseButton mouse_button) {
    if (!ctx || !label) return false;

    if (ctx->hovered_window == nullptr && monstro_is_mouse_clicked(ctx, mouse_button, false)) {
        monstro_open_popup(ctx, label);
    }

    return monstro_begin_popup(ctx, label, (MonstroWindowFlags)0);
}

void monstro_end_popup(MonstroContext* ctx) {
    if (!ctx || !ctx->current_window) return;
    monstro_end_window(ctx);
}

bool monstro_is_popup_open(MonstroContext* ctx, const char* str_id) {
    if (!ctx || !str_id) return false;
    
    uint64_t popup_id = ctx->getId(monstro_hash_string(str_id));
    return is_popup_open_internal(ctx, popup_id);
}
