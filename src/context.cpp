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
    
    if (window_stack_top != 0) {
        throw std::runtime_error("Unbalanced window stack: missing monstro_end_window() calls");
    }
    if (!id_stack.isBalanced()) {
        throw std::runtime_error("Unbalanced ID stack: missing monstro_pop_id() calls");
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

void Context::endFrame() {
    monstro_end_window(this);

    if (root_window) {
        root_window->collectDrawCommands(this);
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

MonstroLayoutInternal* Context::getCurrentLayout() {
    if (!current_window) return nullptr;
    Window* win = current_window;
    if (win->layout_stack_top < 0) return nullptr;
    return &win->layout_stack[win->layout_stack_top];
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
