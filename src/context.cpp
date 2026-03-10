#include "context.h"
#include <cmath>
#include <cstring>
#include <new>
#include <stdexcept>

namespace monstro
{

Context::Context()
{
   font_manager = monstro_font_manager_create();
   monstro_style_set_default(&style);
}

Context::~Context()
{
   if (font_manager)
   {
      monstro_font_manager_destroy(font_manager);
   }
}

void Context::beginFrame(float dt)
{
   delta_time = dt;
   time += dt;
   frame++;

   hovered_id_prev_frame = hovered_id;
   hovered_id = 0;

   if (!active_id_is_alive && active_id_prev_frame == active_id && active_id != 0)
   {
      setActiveId(0);
   }
   active_id_prev_frame = active_id;
   active_id_is_alive = false;

   mouse.newFrame(dt, time);
   keyboard.newFrame(dt);

   hovered_window = findHoveredWindow(mouse.pos_x, mouse.pos_y, false);

   for (int i = 0; i < window_count; i++)
   {
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

   if (window_stack_top != 0)
   {
      throw std::runtime_error("Unbalanced window stack: missing monstro_end_window() calls");
   }
   if (!id_stack.isBalanced())
   {
      throw std::runtime_error("Unbalanced ID stack: missing monstro_pop_id() calls");
   }
   if (disabled_stack_depth != 0)
   {
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

MonstroTreeNodeState *Context::findOrCreateTreeState(uint64_t id, bool default_open)
{
   for (int i = 0; i < tree_state_count; i++)
   {
      if (tree_states[i].id == id)
      {
         tree_states[i].last_frame_active = frame;
         return &tree_states[i];
      }
   }
   if (tree_state_count >= MONSTRO_MAX_TREE_STATES)
      return nullptr;

   MonstroTreeNodeState *state = &tree_states[tree_state_count++];
   state->id = id;
   state->open = default_open;
   state->last_frame_active = frame;
   return state;
}

void Context::endFrame()
{
   tooltip.endFrame(this);

   monstro_end_window(this);

   if (root_window)
   {
      root_window->collectDrawCommands(this);
   }

   if (popup_stack_top > 0 && mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
   {
      bool opened_this_frame = popup_stack[popup_stack_top - 1].open_frame == frame;
      bool is_modal = popup_stack[popup_stack_top - 1].is_modal;
      if (!opened_this_frame && !is_modal)
      {
         bool clicked_inside_popup = hovered_window != nullptr && (hovered_window->flags & MONSTRO_WINDOW_POPUP);
         if (!clicked_inside_popup)
         {
            popup_stack_top = 0;
         }
      }
   }

   if (hovered_window != nullptr && active_id == 0 && mouse.anyButtonPressed())
   {
      focusWindow(hovered_window);
   }

   if (hovered_window != nullptr && active_id == 0 && hovered_id == 0 && mouse.isPressed(MONSTRO_MOUSE_BUTTON_MIDDLE))
   {
      MonstroRect title_rect = hovered_window->getTitleBarRect();
      if (monstro_rect_contains(title_rect, mouse.pos_x, mouse.pos_y))
      {
         setActiveId(hovered_window->move_id);
      }
   }

   mouse.endFrame();
   keyboard.endFrame();
}

void Context::setDefaultFont(MonstroFont *font)
{
   default_font = font;
   style.font = font;
}

void Context::setActiveId(uint64_t id) { active_id = id; }

void Context::keepAliveId(uint64_t id)
{
   if (active_id == id)
   {
      active_id_is_alive = true;
   }
}

Layout *Context::getCurrentLayout()
{
   if (!current_window)
      return nullptr;
   return current_window->getCurrentLayout();
}

} // namespace monstro

/* ------------------------------------------------------------------------- */
/* C API Implementation                                                      */
/* ------------------------------------------------------------------------- */

extern "C"
{

MonstroContext *monstro_context_create(void)
{
   MonstroContext *ctx = new (std::nothrow) MonstroContext();
   if (!ctx)
      return nullptr;

   if (!ctx->font_manager)
   {
      delete ctx;
      return nullptr;
   }

   return ctx;
}

void monstro_context_destroy(MonstroContext *ctx) { delete ctx; }

MonstroDrawBuffer *monstro_context_get_draw_buffer(MonstroContext *ctx) { return ctx ? &ctx->draw_buffer : nullptr; }

void monstro_context_set_default_font(MonstroContext *ctx, MonstroFont *font)
{
   if (ctx)
      ctx->setDefaultFont(font);
}

MonstroFont *monstro_context_get_default_font(MonstroContext *ctx) { return ctx ? ctx->getDefaultFont() : nullptr; }

void monstro_set_display_size(MonstroContext *ctx, float width, float height)
{
   if (!ctx)
      return;
   ctx->display_width = width;
   ctx->display_height = height;
}

void monstro_get_display_size(MonstroContext *ctx, float *out_width, float *out_height)
{
   if (!ctx)
      return;
   if (out_width)
      *out_width = ctx->display_width;
   if (out_height)
      *out_height = ctx->display_height;
}

void monstro_begin_frame(MonstroContext *ctx, float delta_time)
{
   if (ctx)
      ctx->beginFrame(delta_time);
}

void monstro_end_frame(MonstroContext *ctx)
{
   if (ctx)
      ctx->endFrame();
}

float monstro_get_delta_time(MonstroContext *ctx) { return ctx ? ctx->delta_time : 0.0f; }

float monstro_get_time(MonstroContext *ctx) { return ctx ? ctx->time : 0.0f; }

MonstroStyle *monstro_get_style(MonstroContext *ctx) { return ctx ? &ctx->style : nullptr; }

void monstro_set_style(MonstroContext *ctx, const MonstroStyle *style)
{
   if (ctx && style)
      ctx->style = *style;
}

/* ------------------------------------------------------------------------- */
/* Per-widget style get                                                      */
/* ------------------------------------------------------------------------- */

const MonstroStyleText *monstro_get_text_style(MonstroContext *ctx) { return ctx ? &ctx->style.text : nullptr; }
const MonstroStyleButton *monstro_get_button_style(MonstroContext *ctx) { return ctx ? &ctx->style.button : nullptr; }
const MonstroStyleButton *monstro_get_contextual_button_style(MonstroContext *ctx)
{
   return ctx ? &ctx->style.contextual_button : nullptr;
}
const MonstroStyleButton *monstro_get_menu_button_style(MonstroContext *ctx)
{
   return ctx ? &ctx->style.menu_button : nullptr;
}
const MonstroStyleToggle *monstro_get_option_style(MonstroContext *ctx) { return ctx ? &ctx->style.option : nullptr; }
const MonstroStyleToggle *monstro_get_checkbox_style(MonstroContext *ctx)
{
   return ctx ? &ctx->style.checkbox : nullptr;
}
const MonstroStyleSelectable *monstro_get_selectable_style(MonstroContext *ctx)
{
   return ctx ? &ctx->style.selectable : nullptr;
}
const MonstroStyleSlider *monstro_get_slider_style(MonstroContext *ctx) { return ctx ? &ctx->style.slider : nullptr; }
const MonstroStyleKnob *monstro_get_knob_style(MonstroContext *ctx) { return ctx ? &ctx->style.knob : nullptr; }
const MonstroStyleProgress *monstro_get_progress_style(MonstroContext *ctx)
{
   return ctx ? &ctx->style.progress : nullptr;
}
const MonstroStyleProperty *monstro_get_property_style(MonstroContext *ctx)
{
   return ctx ? &ctx->style.property : nullptr;
}
const MonstroStyleEdit *monstro_get_edit_style(MonstroContext *ctx) { return ctx ? &ctx->style.edit : nullptr; }
const MonstroStyleTree *monstro_get_tree_style(MonstroContext *ctx) { return ctx ? &ctx->style.tree : nullptr; }
const MonstroStyleTab *monstro_get_tab_style(MonstroContext *ctx) { return ctx ? &ctx->style.tab : nullptr; }
const MonstroStyleTable *monstro_get_table_style(MonstroContext *ctx) { return ctx ? &ctx->style.table : nullptr; }
const MonstroStyleCombo *monstro_get_combo_style(MonstroContext *ctx) { return ctx ? &ctx->style.combo : nullptr; }
const MonstroStyleMenu *monstro_get_menu_style(MonstroContext *ctx) { return ctx ? &ctx->style.menu : nullptr; }
const MonstroStyleSeparator *monstro_get_separator_style(MonstroContext *ctx)
{
   return ctx ? &ctx->style.separator : nullptr;
}
const MonstroStyleTooltip *monstro_get_tooltip_style(MonstroContext *ctx)
{
   return ctx ? &ctx->style.tooltip : nullptr;
}
const MonstroStyleChart *monstro_get_chart_style(MonstroContext *ctx) { return ctx ? &ctx->style.chart : nullptr; }
const MonstroStyleScrollbar *monstro_get_scrollh_style(MonstroContext *ctx)
{
   return ctx ? &ctx->style.scrollh : nullptr;
}
const MonstroStyleScrollbar *monstro_get_scrollv_style(MonstroContext *ctx)
{
   return ctx ? &ctx->style.scrollv : nullptr;
}
const MonstroStyleWindow *monstro_get_window_style(MonstroContext *ctx) { return ctx ? &ctx->style.window : nullptr; }

/* ------------------------------------------------------------------------- */
/* Per-widget style set                                                      */
/* ------------------------------------------------------------------------- */

void monstro_set_text_style(MonstroContext *ctx, const MonstroStyleText *s)
{
   if (ctx && s)
      ctx->style.text = *s;
}
void monstro_set_button_style(MonstroContext *ctx, const MonstroStyleButton *s)
{
   if (ctx && s)
      ctx->style.button = *s;
}
void monstro_set_contextual_button_style(MonstroContext *ctx, const MonstroStyleButton *s)
{
   if (ctx && s)
      ctx->style.contextual_button = *s;
}
void monstro_set_menu_button_style(MonstroContext *ctx, const MonstroStyleButton *s)
{
   if (ctx && s)
      ctx->style.menu_button = *s;
}
void monstro_set_option_style(MonstroContext *ctx, const MonstroStyleToggle *s)
{
   if (ctx && s)
      ctx->style.option = *s;
}
void monstro_set_checkbox_style(MonstroContext *ctx, const MonstroStyleToggle *s)
{
   if (ctx && s)
      ctx->style.checkbox = *s;
}
void monstro_set_selectable_style(MonstroContext *ctx, const MonstroStyleSelectable *s)
{
   if (ctx && s)
      ctx->style.selectable = *s;
}
void monstro_set_slider_style(MonstroContext *ctx, const MonstroStyleSlider *s)
{
   if (ctx && s)
      ctx->style.slider = *s;
}
void monstro_set_knob_style(MonstroContext *ctx, const MonstroStyleKnob *s)
{
   if (ctx && s)
      ctx->style.knob = *s;
}
void monstro_set_progress_style(MonstroContext *ctx, const MonstroStyleProgress *s)
{
   if (ctx && s)
      ctx->style.progress = *s;
}
void monstro_set_property_style(MonstroContext *ctx, const MonstroStyleProperty *s)
{
   if (ctx && s)
      ctx->style.property = *s;
}
void monstro_set_edit_style(MonstroContext *ctx, const MonstroStyleEdit *s)
{
   if (ctx && s)
      ctx->style.edit = *s;
}
void monstro_set_tree_style(MonstroContext *ctx, const MonstroStyleTree *s)
{
   if (ctx && s)
      ctx->style.tree = *s;
}
void monstro_set_tab_style(MonstroContext *ctx, const MonstroStyleTab *s)
{
   if (ctx && s)
      ctx->style.tab = *s;
}
void monstro_set_table_style(MonstroContext *ctx, const MonstroStyleTable *s)
{
   if (ctx && s)
      ctx->style.table = *s;
}
void monstro_set_combo_style(MonstroContext *ctx, const MonstroStyleCombo *s)
{
   if (ctx && s)
      ctx->style.combo = *s;
}
void monstro_set_menu_style(MonstroContext *ctx, const MonstroStyleMenu *s)
{
   if (ctx && s)
      ctx->style.menu = *s;
}
void monstro_set_separator_style(MonstroContext *ctx, const MonstroStyleSeparator *s)
{
   if (ctx && s)
      ctx->style.separator = *s;
}
void monstro_set_tooltip_style(MonstroContext *ctx, const MonstroStyleTooltip *s)
{
   if (ctx && s)
      ctx->style.tooltip = *s;
}
void monstro_set_chart_style(MonstroContext *ctx, const MonstroStyleChart *s)
{
   if (ctx && s)
      ctx->style.chart = *s;
}
void monstro_set_scrollh_style(MonstroContext *ctx, const MonstroStyleScrollbar *s)
{
   if (ctx && s)
      ctx->style.scrollh = *s;
}
void monstro_set_scrollv_style(MonstroContext *ctx, const MonstroStyleScrollbar *s)
{
   if (ctx && s)
      ctx->style.scrollv = *s;
}
void monstro_set_window_style(MonstroContext *ctx, const MonstroStyleWindow *s)
{
   if (ctx && s)
      ctx->style.window = *s;
}

/* ------------------------------------------------------------------------- */
/* Per-widget style push (backup + apply)                                    */
/* ------------------------------------------------------------------------- */

#define MONSTRO_PUSH_STYLE(CTX, ENUM_TAG, UNION_FIELD, STYLE_FIELD, SRC)                                               \
   do                                                                                                                  \
   {                                                                                                                   \
      if (!(CTX) || !(SRC) || (CTX)->style_stack_top >= MONSTRO_MAX_STYLE_VAR_STACK)                                   \
         return;                                                                                                       \
      MonstroStyleBackup *_b = &(CTX)->style_stack[(CTX)->style_stack_top++];                                          \
      _b->type = (ENUM_TAG);                                                                                           \
      _b->data.UNION_FIELD = (CTX)->style.STYLE_FIELD;                                                                 \
      (CTX)->style.STYLE_FIELD = *(SRC);                                                                               \
   } while (0)

void monstro_push_text_style(MonstroContext *ctx, const MonstroStyleText *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_TEXT, text, text, s);
}
void monstro_push_button_style(MonstroContext *ctx, const MonstroStyleButton *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_BUTTON, button, button, s);
}
void monstro_push_contextual_button_style(MonstroContext *ctx, const MonstroStyleButton *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_CONTEXTUAL_BUTTON, button, contextual_button, s);
}
void monstro_push_menu_button_style(MonstroContext *ctx, const MonstroStyleButton *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_MENU_BUTTON, button, menu_button, s);
}
void monstro_push_option_style(MonstroContext *ctx, const MonstroStyleToggle *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_OPTION, toggle, option, s);
}
void monstro_push_checkbox_style(MonstroContext *ctx, const MonstroStyleToggle *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_CHECKBOX, toggle, checkbox, s);
}
void monstro_push_selectable_style(MonstroContext *ctx, const MonstroStyleSelectable *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_SELECTABLE, selectable, selectable, s);
}
void monstro_push_slider_style(MonstroContext *ctx, const MonstroStyleSlider *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_SLIDER, slider, slider, s);
}
void monstro_push_knob_style(MonstroContext *ctx, const MonstroStyleKnob *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_KNOB, knob, knob, s);
}
void monstro_push_progress_style(MonstroContext *ctx, const MonstroStyleProgress *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_PROGRESS, progress, progress, s);
}
void monstro_push_property_style(MonstroContext *ctx, const MonstroStyleProperty *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_PROPERTY, property, property, s);
}
void monstro_push_edit_style(MonstroContext *ctx, const MonstroStyleEdit *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_EDIT, edit, edit, s);
}
void monstro_push_tree_style(MonstroContext *ctx, const MonstroStyleTree *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_TREE, tree, tree, s);
}
void monstro_push_tab_style(MonstroContext *ctx, const MonstroStyleTab *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_TAB, tab, tab, s);
}
void monstro_push_table_style(MonstroContext *ctx, const MonstroStyleTable *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_TABLE, table, table, s);
}
void monstro_push_combo_style(MonstroContext *ctx, const MonstroStyleCombo *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_COMBO, combo, combo, s);
}
void monstro_push_menu_style(MonstroContext *ctx, const MonstroStyleMenu *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_MENU, menu, menu, s);
}
void monstro_push_separator_style(MonstroContext *ctx, const MonstroStyleSeparator *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_SEPARATOR, separator, separator, s);
}
void monstro_push_tooltip_style(MonstroContext *ctx, const MonstroStyleTooltip *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_TOOLTIP, tooltip, tooltip, s);
}
void monstro_push_chart_style(MonstroContext *ctx, const MonstroStyleChart *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_CHART, chart, chart, s);
}
void monstro_push_scrollh_style(MonstroContext *ctx, const MonstroStyleScrollbar *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_SCROLLH, scrollbar, scrollh, s);
}
void monstro_push_scrollv_style(MonstroContext *ctx, const MonstroStyleScrollbar *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_SCROLLV, scrollbar, scrollv, s);
}
void monstro_push_window_style(MonstroContext *ctx, const MonstroStyleWindow *s)
{
   MONSTRO_PUSH_STYLE(ctx, MONSTRO_PUSHED_WINDOW, window, window, s);
}

#undef MONSTRO_PUSH_STYLE

/* ------------------------------------------------------------------------- */
/* Unified pop                                                               */
/* ------------------------------------------------------------------------- */

void monstro_pop_style(MonstroContext *ctx, int count)
{
   if (!ctx)
      return;
   for (int i = 0; i < count && ctx->style_stack_top > 0; i++)
   {
      ctx->style_stack_top--;
      MonstroStyleBackup *b = &ctx->style_stack[ctx->style_stack_top];
      switch (b->type)
      {
      case MONSTRO_PUSHED_TEXT:
         ctx->style.text = b->data.text;
         break;
      case MONSTRO_PUSHED_BUTTON:
         ctx->style.button = b->data.button;
         break;
      case MONSTRO_PUSHED_CONTEXTUAL_BUTTON:
         ctx->style.contextual_button = b->data.button;
         break;
      case MONSTRO_PUSHED_MENU_BUTTON:
         ctx->style.menu_button = b->data.button;
         break;
      case MONSTRO_PUSHED_OPTION:
         ctx->style.option = b->data.toggle;
         break;
      case MONSTRO_PUSHED_CHECKBOX:
         ctx->style.checkbox = b->data.toggle;
         break;
      case MONSTRO_PUSHED_SELECTABLE:
         ctx->style.selectable = b->data.selectable;
         break;
      case MONSTRO_PUSHED_SLIDER:
         ctx->style.slider = b->data.slider;
         break;
      case MONSTRO_PUSHED_KNOB:
         ctx->style.knob = b->data.knob;
         break;
      case MONSTRO_PUSHED_PROGRESS:
         ctx->style.progress = b->data.progress;
         break;
      case MONSTRO_PUSHED_PROPERTY:
         ctx->style.property = b->data.property;
         break;
      case MONSTRO_PUSHED_EDIT:
         ctx->style.edit = b->data.edit;
         break;
      case MONSTRO_PUSHED_TREE:
         ctx->style.tree = b->data.tree;
         break;
      case MONSTRO_PUSHED_TAB:
         ctx->style.tab = b->data.tab;
         break;
      case MONSTRO_PUSHED_TABLE:
         ctx->style.table = b->data.table;
         break;
      case MONSTRO_PUSHED_COMBO:
         ctx->style.combo = b->data.combo;
         break;
      case MONSTRO_PUSHED_MENU:
         ctx->style.menu = b->data.menu;
         break;
      case MONSTRO_PUSHED_SEPARATOR:
         ctx->style.separator = b->data.separator;
         break;
      case MONSTRO_PUSHED_TOOLTIP:
         ctx->style.tooltip = b->data.tooltip;
         break;
      case MONSTRO_PUSHED_CHART:
         ctx->style.chart = b->data.chart;
         break;
      case MONSTRO_PUSHED_SCROLLH:
         ctx->style.scrollh = b->data.scrollbar;
         break;
      case MONSTRO_PUSHED_SCROLLV:
         ctx->style.scrollv = b->data.scrollbar;
         break;
      case MONSTRO_PUSHED_WINDOW:
         ctx->style.window = b->data.window;
         break;
      }
   }
}

} // extern "C"

/* ------------------------------------------------------------------------- */
/* DrawBuffer C API                                                          */
/* ------------------------------------------------------------------------- */

extern "C"
{

void monstro_context_get_draw_commands(MonstroContext *ctx, const MonstroDrawCommand **out_commands,
                                       uint32_t *out_count)
{
   if (!ctx)
   {
      if (out_commands)
         *out_commands = nullptr;
      if (out_count)
         *out_count = 0;
      return;
   }
   if (out_commands)
      *out_commands = ctx->final_commands;
   if (out_count)
      *out_count = ctx->final_command_count;
}

void monstro_draw_buffer_get_vertex_buffer(MonstroDrawBuffer *buf, const MonstroVertex **out_data,
                                           size_t *out_byte_size)
{
   if (!buf)
   {
      if (out_data)
         *out_data = nullptr;
      if (out_byte_size)
         *out_byte_size = 0;
      return;
   }
   buf->getVertexBuffer(out_data, out_byte_size);
}

void monstro_draw_buffer_get_index_buffer(MonstroDrawBuffer *buf, const uint32_t **out_data, uint32_t *out_count)
{
   if (!buf)
   {
      if (out_data)
         *out_data = nullptr;
      if (out_count)
         *out_count = 0;
      return;
   }
   buf->getIndexBuffer(out_data, out_count);
}

void monstro_draw_buffer_set_vertex_buffer(MonstroDrawBuffer *buf, MonstroVertex *buffer, size_t capacity_in_vertices)
{
   if (buf)
      buf->setVertexBuffer(buffer, capacity_in_vertices);
}

void monstro_draw_buffer_set_index_buffer(MonstroDrawBuffer *buf, uint32_t *buffer, size_t capacity_in_indices)
{
   if (buf)
      buf->setIndexBuffer(buffer, capacity_in_indices);
}

} // extern "C"

/* ------------------------------------------------------------------------- */
/* ID Stack C API                                                            */
/* ------------------------------------------------------------------------- */

extern "C"
{

void monstro_push_id(MonstroContext *ctx, const char *name)
{
   if (ctx)
      ctx->id_stack.push(name);
}

void monstro_pop_id(MonstroContext *ctx)
{
   if (ctx)
      ctx->id_stack.pop();
}

uint64_t monstro_get_id(MonstroContext *ctx, const char *name)
{
   if (!ctx || !name)
      return 0;
   uint64_t combined = ctx->id_stack.getId(name);
   ctx->keepAliveId(combined);
   return combined;
}

} // extern "C"

/* ------------------------------------------------------------------------- */
/* Item State Queries                                                        */
/* ------------------------------------------------------------------------- */

void monstro_set_last_item(MonstroContext *ctx, uint64_t id, MonstroRect rect, bool hovered, bool active, bool clicked,
                           bool edited)
{
   if (!ctx)
      return;
   ctx->last_item_id = id;
   ctx->last_item_rect = rect;
   ctx->last_item_hovered = hovered;
   ctx->last_item_active = active;
   ctx->last_item_clicked = clicked;
   ctx->last_item_edited = edited;

   bool was_active = (ctx->active_id_prev_frame == id && ctx->active_id != id);
   ctx->last_item_deactivated_after_edit = was_active && edited;
}

bool monstro_is_item_hovered(MonstroContext *ctx) { return ctx ? ctx->last_item_hovered : false; }

bool monstro_is_item_active(MonstroContext *ctx) { return ctx ? ctx->last_item_active : false; }

bool monstro_is_item_clicked(MonstroContext *ctx, MonstroMouseButton button)
{
   if (!ctx)
      return false;
   return ctx->last_item_hovered && monstro_is_mouse_clicked(ctx, button, false);
}

bool monstro_is_item_edited(MonstroContext *ctx) { return ctx ? ctx->last_item_edited : false; }

bool monstro_is_item_deactivated_after_edit(MonstroContext *ctx)
{
   return ctx ? ctx->last_item_deactivated_after_edit : false;
}

MonstroRect monstro_get_item_rect(MonstroContext *ctx)
{
   if (!ctx)
      return {0, 0, 0, 0};
   return ctx->last_item_rect;
}

MonstroVec2 monstro_get_item_rect_min(MonstroContext *ctx)
{
   if (!ctx)
      return {0, 0};
   return {ctx->last_item_rect.x, ctx->last_item_rect.y};
}

MonstroVec2 monstro_get_item_rect_max(MonstroContext *ctx)
{
   if (!ctx)
      return {0, 0};
   return {ctx->last_item_rect.x + ctx->last_item_rect.w, ctx->last_item_rect.y + ctx->last_item_rect.h};
}

MonstroVec2 monstro_get_item_rect_size(MonstroContext *ctx)
{
   if (!ctx)
      return {0, 0};
   return {ctx->last_item_rect.w, ctx->last_item_rect.h};
}

/* ------------------------------------------------------------------------- */
/* Disabled State                                                            */
/* ------------------------------------------------------------------------- */

void monstro_begin_disabled(MonstroContext *ctx, bool disabled)
{
   if (!ctx)
      return;
   ctx->disabled_stack_depth++;
   if (disabled)
   {
      ctx->disabled = true;
   }
}

void monstro_end_disabled(MonstroContext *ctx)
{
   if (!ctx || ctx->disabled_stack_depth <= 0)
      return;
   ctx->disabled_stack_depth--;
   if (ctx->disabled_stack_depth == 0)
   {
      ctx->disabled = false;
   }
}

bool monstro_is_disabled(MonstroContext *ctx) { return ctx ? ctx->disabled : false; }
