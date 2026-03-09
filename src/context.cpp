#include "context.h"
#include <new>
#include <stdexcept>
#include <cmath>
#include <cstring>

namespace monstro {

Context::Context() {
    font_manager = monstro_font_manager_create();
    monstro_style_set_default(&style);
}

Context::~Context() {
    if (font_manager) {
        monstro_font_manager_destroy(font_manager);
    }
}

void Context::beginFrame(float dt) {
    delta_time = dt;
    time += dt;
    frame++;
    
    hovered_id_prev_frame = hovered_id;
    hovered_id = 0;
    
    if (!active_id_is_alive && active_id_prev_frame == active_id && active_id != 0) {
        setActiveId(0);
    }
    active_id_prev_frame = active_id;
    active_id_is_alive = false;
    
    mouse.newFrame(dt, time);
    keyboard.newFrame(dt);
    
    hovered_window = findHoveredWindow(mouse.pos_x, mouse.pos_y, false);
    
    for (int i = 0; i < window_count; i++) {
        windows[i].active = false;
    }
    
    draw_buffer.beginFrame();
    final_command_count = 0;
    
    next_window_pos_cond = MONSTRO_COND_NONE;
    next_window_size_cond = MONSTRO_COND_NONE;
    next_window_focus_cond = MONSTRO_COND_NONE;
    next_window_pos_x = -INFINITY;
    next_window_pos_y = -INFINITY;
    next_window_size_x = -INFINITY;
    next_window_size_y = -INFINITY;
    
    want_capture_mouse = false;
    want_capture_keyboard = false;
    want_text_input = false;

    last_item_id = 0;
    last_item_rect = {};
    last_item_hovered = false;
    last_item_active = false;
    last_item_clicked = false;
    last_item_edited = false;
    last_item_deactivated_after_edit = false;

    tooltip.beginFrame();

    if (window_stack_top != 0) {
        throw std::runtime_error("Unbalanced window stack: missing monstro_end_window() calls");
    }
    if (!id_stack.isBalanced()) {
        throw std::runtime_error("Unbalanced ID stack: missing monstro_pop_id() calls");
    }
    if (disabled_stack_depth != 0) {
        throw std::runtime_error("Unbalanced disabled stack: missing monstro_end_disabled() calls");
    }

    next_window_size_x = display_width;
    next_window_size_y = display_height;
    next_window_size_cond = MONSTRO_COND_ALWAYS;
    next_window_pos_x = 0.0f;
    next_window_pos_y = 0.0f;
    next_window_pos_cond = MONSTRO_COND_ALWAYS;
    MonstroWindowFlags root_flags = (MonstroWindowFlags)(MONSTRO_WINDOW_ROOT | MONSTRO_WINDOW_INPUTS);
    monstro_begin_window(this, "root", nullptr, root_flags);
    root_window = findWindow("root");
}

MonstroTreeNodeState* Context::findOrCreateTreeState(uint64_t id, bool default_open) {
    for (int i = 0; i < tree_state_count; i++) {
        if (tree_states[i].id == id) {
            tree_states[i].last_frame_active = frame;
            return &tree_states[i];
        }
    }
    if (tree_state_count >= MONSTRO_MAX_TREE_STATES) return nullptr;

    MonstroTreeNodeState* state = &tree_states[tree_state_count++];
    state->id = id;
    state->open = default_open;
    state->last_frame_active = frame;
    return state;
}

void Context::endFrame() {
    tooltip.endFrame(this);

    monstro_end_window(this);

    if (root_window) {
        root_window->collectDrawCommands(this);
    }

    if (popup_stack_top > 0 && mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT)) {
        bool opened_this_frame = popup_stack[popup_stack_top - 1].open_frame == frame;
        bool is_modal = popup_stack[popup_stack_top - 1].is_modal;
        if (!opened_this_frame && !is_modal) {
            bool clicked_inside_popup = hovered_window != nullptr &&
                (hovered_window->flags & MONSTRO_WINDOW_POPUP);
            if (!clicked_inside_popup) {
                popup_stack_top = 0;
            }
        }
    }

    if (hovered_window != nullptr && active_id == 0 && mouse.anyButtonPressed()) {
        focusWindow(hovered_window);
    }

    if (hovered_window != nullptr &&
        active_id == 0 &&
        hovered_id == 0 &&
        mouse.isPressed(MONSTRO_MOUSE_BUTTON_MIDDLE)) {
        MonstroRect title_rect = hovered_window->getTitleBarRect();
        if (monstro_rect_contains(title_rect, mouse.pos_x, mouse.pos_y)) {
            setActiveId(hovered_window->move_id);
        }
    }

    mouse.endFrame();
    keyboard.endFrame();
}

void Context::setDefaultFont(MonstroFont* font) {
    default_font = font;
    style.font = font;
}

void Context::setActiveId(uint64_t id) {
    active_id = id;
}

void Context::keepAliveId(uint64_t id) {
    if (active_id == id) {
        active_id_is_alive = true;
    }
}

Layout* Context::getCurrentLayout() {
    if (!current_window) return nullptr;
    return current_window->getCurrentLayout();
}

} // namespace monstro

/* ------------------------------------------------------------------------- */
/* C API Implementation                                                      */
/* ------------------------------------------------------------------------- */

extern "C" {

MonstroContext* monstro_context_create(void) {
    MonstroContext* ctx = new (std::nothrow) MonstroContext();
    if (!ctx) return nullptr;
    
    if (!ctx->font_manager) {
        delete ctx;
        return nullptr;
    }
    
    return ctx;
}

void monstro_context_destroy(MonstroContext* ctx) {
    delete ctx;
}

MonstroDrawBuffer* monstro_context_get_draw_buffer(MonstroContext* ctx) {
    return ctx ? &ctx->draw_buffer : nullptr;
}

void monstro_context_set_default_font(MonstroContext* ctx, MonstroFont* font) {
    if (ctx) ctx->setDefaultFont(font);
}

MonstroFont* monstro_context_get_default_font(MonstroContext* ctx) {
    return ctx ? ctx->getDefaultFont() : nullptr;
}

void monstro_set_display_size(MonstroContext* ctx, float width, float height) {
    if (!ctx) return;
    ctx->display_width = width;
    ctx->display_height = height;
}

void monstro_begin_frame(MonstroContext* ctx, float delta_time) {
    if (ctx) ctx->beginFrame(delta_time);
}

void monstro_end_frame(MonstroContext* ctx) {
    if (ctx) ctx->endFrame();
}

float monstro_get_delta_time(MonstroContext* ctx) {
    return ctx ? ctx->delta_time : 0.0f;
}

float monstro_get_time(MonstroContext* ctx) {
    return ctx ? ctx->time : 0.0f;
}

MonstroStyle* monstro_get_style(MonstroContext* ctx) {
    return ctx ? &ctx->style : nullptr;
}

void monstro_set_style(MonstroContext* ctx, const MonstroStyle* style) {
    if (ctx && style) ctx->style = *style;
}

void monstro_push_style_float(MonstroContext* ctx, float* var, float new_value) {
    if (!ctx || ctx->style_var_stack_top >= MONSTRO_MAX_STYLE_VAR_STACK || !var) return;
    MonstroStyleVarBackup* entry = &ctx->style_var_stack[ctx->style_var_stack_top++];
    entry->ptr = var;
    entry->type = MONSTRO_STYLE_VAR_FLOAT;
    entry->old_value.f = *var;
    *var = new_value;
}

void monstro_push_style_vec2(MonstroContext* ctx, float var[2], float x, float y) {
    if (!ctx || ctx->style_var_stack_top >= MONSTRO_MAX_STYLE_VAR_STACK || !var) return;
    MonstroStyleVarBackup* entry = &ctx->style_var_stack[ctx->style_var_stack_top++];
    entry->ptr = var;
    entry->type = MONSTRO_STYLE_VAR_VEC2;
    entry->old_value.v2[0] = var[0];
    entry->old_value.v2[1] = var[1];
    var[0] = x;
    var[1] = y;
}

void monstro_push_style_color(MonstroContext* ctx, MonstroColor* var, MonstroColor new_value) {
    if (!ctx || ctx->style_var_stack_top >= MONSTRO_MAX_STYLE_VAR_STACK || !var) return;
    MonstroStyleVarBackup* entry = &ctx->style_var_stack[ctx->style_var_stack_top++];
    entry->ptr = var;
    entry->type = MONSTRO_STYLE_VAR_COLOR;
    entry->old_value.color = *var;
    *var = new_value;
}

void monstro_push_style_item(MonstroContext* ctx, MonstroStyleItem* var, MonstroStyleItem new_value) {
    if (!ctx || ctx->style_var_stack_top >= MONSTRO_MAX_STYLE_VAR_STACK || !var) return;
    MonstroStyleVarBackup* entry = &ctx->style_var_stack[ctx->style_var_stack_top++];
    entry->ptr = var;
    entry->type = MONSTRO_STYLE_VAR_ITEM;
    entry->old_value.item = *var;
    *var = new_value;
}

void monstro_push_style_int(MonstroContext* ctx, int* var, int new_value) {
    if (!ctx || ctx->style_var_stack_top >= MONSTRO_MAX_STYLE_VAR_STACK || !var) return;
    MonstroStyleVarBackup* entry = &ctx->style_var_stack[ctx->style_var_stack_top++];
    entry->ptr = var;
    entry->type = MONSTRO_STYLE_VAR_INT;
    entry->old_value.i = *var;
    *var = new_value;
}

void monstro_pop_style_var(MonstroContext* ctx, int count) {
    if (!ctx) return;
    for (int i = 0; i < count && ctx->style_var_stack_top > 0; i++) {
        ctx->style_var_stack_top--;
        MonstroStyleVarBackup* entry = &ctx->style_var_stack[ctx->style_var_stack_top];
        switch (entry->type) {
            case MONSTRO_STYLE_VAR_FLOAT:
                *(float*)entry->ptr = entry->old_value.f;
                break;
            case MONSTRO_STYLE_VAR_VEC2: {
                float* v = (float*)entry->ptr;
                v[0] = entry->old_value.v2[0];
                v[1] = entry->old_value.v2[1];
                break;
            }
            case MONSTRO_STYLE_VAR_COLOR:
                *(MonstroColor*)entry->ptr = entry->old_value.color;
                break;
            case MONSTRO_STYLE_VAR_ITEM:
                *(MonstroStyleItem*)entry->ptr = entry->old_value.item;
                break;
            case MONSTRO_STYLE_VAR_INT:
                *(int*)entry->ptr = entry->old_value.i;
                break;
        }
    }
}

} // extern "C"

/* ------------------------------------------------------------------------- */
/* DrawBuffer C API                                                          */
/* ------------------------------------------------------------------------- */

extern "C" {

void monstro_context_get_draw_commands(MonstroContext* ctx, const MonstroDrawCommand** out_commands, uint32_t* out_count) {
    if (!ctx) {
        if (out_commands) *out_commands = nullptr;
        if (out_count) *out_count = 0;
        return;
    }
    if (out_commands) *out_commands = ctx->final_commands;
    if (out_count) *out_count = ctx->final_command_count;
}

void monstro_draw_buffer_get_vertex_buffer(MonstroDrawBuffer* buf, const MonstroVertex** out_data, size_t* out_byte_size) {
    if (!buf) {
        if (out_data) *out_data = nullptr;
        if (out_byte_size) *out_byte_size = 0;
        return;
    }
    buf->getVertexBuffer(out_data, out_byte_size);
}

void monstro_draw_buffer_get_index_buffer(MonstroDrawBuffer* buf, const uint32_t** out_data, uint32_t* out_count) {
    if (!buf) {
        if (out_data) *out_data = nullptr;
        if (out_count) *out_count = 0;
        return;
    }
    buf->getIndexBuffer(out_data, out_count);
}

void monstro_draw_buffer_set_vertex_buffer(MonstroDrawBuffer* buf, MonstroVertex* buffer, size_t capacity_in_vertices) {
    if (buf) buf->setVertexBuffer(buffer, capacity_in_vertices);
}

void monstro_draw_buffer_set_index_buffer(MonstroDrawBuffer* buf, uint32_t* buffer, size_t capacity_in_indices) {
    if (buf) buf->setIndexBuffer(buffer, capacity_in_indices);
}

} // extern "C"

/* ------------------------------------------------------------------------- */
/* ID Stack C API                                                            */
/* ------------------------------------------------------------------------- */

extern "C" {

void monstro_push_id(MonstroContext* ctx, const char* name) {
    if (ctx) ctx->id_stack.push(name);
}

void monstro_pop_id(MonstroContext* ctx) {
    if (ctx) ctx->id_stack.pop();
}

uint64_t monstro_get_id(MonstroContext* ctx, const char* name) {
    if (!ctx || !name) return 0;
    uint64_t combined = ctx->id_stack.getId(name);
    ctx->keepAliveId(combined);
    return combined;
}

} // extern "C"

/* ------------------------------------------------------------------------- */
/* Item State Queries                                                        */
/* ------------------------------------------------------------------------- */

void monstro_set_last_item(MonstroContext* ctx, uint64_t id, MonstroRect rect,
                           bool hovered, bool active, bool clicked, bool edited) {
    if (!ctx) return;
    ctx->last_item_id = id;
    ctx->last_item_rect = rect;
    ctx->last_item_hovered = hovered;
    ctx->last_item_active = active;
    ctx->last_item_clicked = clicked;
    ctx->last_item_edited = edited;

    bool was_active = (ctx->active_id_prev_frame == id && ctx->active_id != id);
    ctx->last_item_deactivated_after_edit = was_active && edited;
}

bool monstro_is_item_hovered(MonstroContext* ctx) {
    return ctx ? ctx->last_item_hovered : false;
}

bool monstro_is_item_active(MonstroContext* ctx) {
    return ctx ? ctx->last_item_active : false;
}

bool monstro_is_item_clicked(MonstroContext* ctx, MonstroMouseButton button) {
    if (!ctx) return false;
    return ctx->last_item_hovered && monstro_is_mouse_clicked(ctx, button, false);
}

bool monstro_is_item_edited(MonstroContext* ctx) {
    return ctx ? ctx->last_item_edited : false;
}

bool monstro_is_item_deactivated_after_edit(MonstroContext* ctx) {
    return ctx ? ctx->last_item_deactivated_after_edit : false;
}

MonstroRect monstro_get_item_rect(MonstroContext* ctx) {
    if (!ctx) return {0, 0, 0, 0};
    return ctx->last_item_rect;
}

MonstroVec2 monstro_get_item_rect_min(MonstroContext* ctx) {
    if (!ctx) return {0, 0};
    return {ctx->last_item_rect.x, ctx->last_item_rect.y};
}

MonstroVec2 monstro_get_item_rect_max(MonstroContext* ctx) {
    if (!ctx) return {0, 0};
    return {ctx->last_item_rect.x + ctx->last_item_rect.w,
            ctx->last_item_rect.y + ctx->last_item_rect.h};
}

MonstroVec2 monstro_get_item_rect_size(MonstroContext* ctx) {
    if (!ctx) return {0, 0};
    return {ctx->last_item_rect.w, ctx->last_item_rect.h};
}

/* ------------------------------------------------------------------------- */
/* Disabled State                                                            */
/* ------------------------------------------------------------------------- */

void monstro_begin_disabled(MonstroContext* ctx, bool disabled) {
    if (!ctx) return;
    ctx->disabled_stack_depth++;
    if (disabled) {
        ctx->disabled = true;
    }
}

void monstro_end_disabled(MonstroContext* ctx) {
    if (!ctx || ctx->disabled_stack_depth <= 0) return;
    ctx->disabled_stack_depth--;
    if (ctx->disabled_stack_depth == 0) {
        ctx->disabled = false;
    }
}

bool monstro_is_disabled(MonstroContext* ctx) {
    return ctx ? ctx->disabled : false;
}
