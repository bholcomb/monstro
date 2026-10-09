#include "context.h"
#include "env.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

namespace monstro
{

Context::Context(Env *environment, const MonstroContextConfig &context_config)
    : env(environment), config(context_config)
{
   theme = &env->themes[0];
}

Context::~Context() {}

bool Context::init()
{
   return windows.init(config.max_windows) && popup_stack.init(config.max_popups) &&
          tab_bars.init(config.max_tab_bars) && tree_states.init(config.max_tree_nodes) &&
          tables.init(config.max_tables) && input_events.init(config.max_input_events);
}

size_t Context::residentBytes() const
{
   return sizeof(Context) + windows.bytes() + popup_stack.bytes() + tab_bars.bytes() + tree_states.bytes() +
          tables.bytes() + input_events.bytes();
}

static void require_args(const char *function, std::initializer_list<RequiredArg> args)
{
   for (const RequiredArg &arg : args)
      MONSTRO_REQUIRE(arg.value, "%s: %s must not be null", function, arg.name);
}

Context *requireContext(Context *ctx, const char *function, std::initializer_list<RequiredArg> args)
{
   MONSTRO_REQUIRE(ctx, "%s: the context must not be null", function);
   require_args(function, args);
   return ctx;
}

Context *requireMouseButton(Context *ctx, const char *function, MonstroMouseButton button)
{
   requireContext(ctx, function);
   MONSTRO_REQUIRE(button >= 0 && button < MONSTRO_MOUSE_BUTTON_COUNT, "%s: mouse button %d is outside 0..%d", function,
                   (int)button, MONSTRO_MOUSE_BUTTON_COUNT - 1);
   return ctx;
}

Context *requireKey(Context *ctx, const char *function, MonstroKey key)
{
   requireContext(ctx, function);
   MONSTRO_REQUIRE(key >= 0 && key < MONSTRO_KEY_COUNT, "%s: key %d is outside 0..%d", function, (int)key,
                   MONSTRO_KEY_COUNT - 1);
   return ctx;
}

Window *requireWindow(Context *ctx, const char *function, std::initializer_list<RequiredArg> args)
{
   MONSTRO_REQUIRE(ctx, "%s: the context must not be null", function);
   MONSTRO_REQUIRE(ctx->current_window,
                   "%s called outside a frame: window and widget calls belong between "
                   "monstro_begin_frame and monstro_end_frame",
                   function);
   require_args(function, args);
   return ctx->current_window;
}

MonstroStyle &Context::style() const
{
   MONSTRO_REQUIRE(canvas, "the working style exists only inside a frame (between monstro_begin_frame and "
                           "monstro_end_frame)");
   return canvas->style;
}

IdStack &Context::ids() const
{
   MONSTRO_REQUIRE(canvas, "the ID stack exists only inside a frame (between monstro_begin_frame and "
                           "monstro_end_frame)");
   return canvas->ids;
}

const MonstroStyle &Context::currentStyle() const { return canvas ? canvas->style : theme->style; }

void Context::beginFrame(float dt, const MonstroFrameBuffers *buffers)
{
   MONSTRO_REQUIRE(!in_frame,
                   "monstro_begin_frame called during frame %d: the previous frame is missing "
                   "monstro_end_frame",
                   frame);
   frame++;
   canvas = env->lendCanvas(this, frame, buffers);
   in_frame = true;

   // The frame works on a copy of the shared theme, so pushes never reach the theme or other contexts.
   canvas->style = theme->style;
   if (!canvas->style.font)
      canvas->style.font = env->default_font;

   const MonstroSettings &settings = env->settings;
   mouse.double_click_time = settings.double_click_time;
   mouse.double_click_distance = settings.double_click_distance;
   mouse.drag_threshold = settings.drag_threshold;
   keyboard.repeat_delay = settings.key_repeat_delay;
   keyboard.repeat_rate = settings.key_repeat_rate;
   tooltip.myHoverDelay = settings.tooltip_delay;

   delta_time = dt;
   time += dt;

   hovered_id_prev_frame = hovered_id;
   hovered_id = 0;
   hover_top_prev_frame = hover_top;
   hover_top = {};

   if (!active_id_is_alive && active_id_prev_frame == active_id && active_id != 0)
   {
      setActiveId(0);
   }
   active_id_prev_frame = active_id;
   active_id_is_alive = false;

   takeInputEvents();
   mouse.newFrame(dt, time);
   keyboard.newFrame(dt);

   hovered_window = findHoveredWindow(mouse.pos_x, mouse.pos_y, false);

   for (Window &window : windows)
   {
      window.active = false;
   }

   want_capture_mouse = false;
   want_capture_keyboard = false;
   want_text_input = false;

   last_item_id = 0;
   last_item_rect = {};
   last_item_hovered = false;
   last_item_active = false;
   last_item_clicked = false;
   last_item_edited = false;
   last_item_activated = false;
   last_item_deactivated = false;
   last_item_deactivated_after_edit = false;
   active_items_prev_frame = active_items;
   active_items.clear();
   disabled = false;

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
   for (MonstroTreeNodeState &existing : tree_states)
   {
      if (existing.id == id)
      {
         existing.last_frame_active = frame;
         return &existing;
      }
   }

   MonstroTreeNodeState *state = &tree_states.add();
   state->id = id;
   state->open = default_open;
   state->last_frame_active = frame;
   return state;
}

const MonstroDrawData *Context::endFrame()
{
   MONSTRO_REQUIRE(in_frame, "monstro_end_frame called without monstro_begin_frame");
   MONSTRO_REQUIRE(current_window == root_window,
                   "window '%s' is still open at the end of frame %d: its monstro_end_window (or end_child, "
                   "end_popup, end_menu, end_combo) is missing",
                   current_window->name, frame);

   tooltip.endFrame(this);

   root_window->end();
   popWindow();
   requireBalanced("monstro_end_frame");

   rankWindowsForDrawing();
   const MonstroDrawData *data = canvas->finish(last_content_hash);
   last_content_hash = data->content_hash;
   canvas->draw_data.display_size[0] = display_width;
   canvas->draw_data.display_size[1] = display_height;

   if (!popup_stack.empty() && mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
   {
      bool opened_this_frame = popup_stack.top().open_frame == frame;
      bool is_modal = popup_stack.top().is_modal;
      if (!opened_this_frame && !is_modal)
      {
         bool clicked_inside_popup = hovered_window != nullptr && (hovered_window->flags & MONSTRO_WINDOW_POPUP);
         if (!clicked_inside_popup)
         {
            popup_stack.clear();
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
   env->returnCanvas(canvas);
   canvas = nullptr;
   in_frame = false;
   return data;
}

void Context::requireBalanced(const char *where) const
{
   const Canvas &c = *canvas;
   MONSTRO_REQUIRE(c.windows.empty() && c.ids.isBalanced() && c.style_stack.empty() && c.disabled_stack.empty() &&
                       c.columns.empty() && !c.table.state && c.tab_bars.empty(),
                   "unbalanced begin/end or push/pop pairs at %s (frame %d): %d windows, %d IDs, %d styles, %d "
                   "disabled scopes, %d column sets and %d tab bars still open; table open: %s",
                   where, frame, c.windows.size(), c.ids.depth(), c.style_stack.size(), c.disabled_stack.size(),
                   c.columns.size(), c.tab_bars.size(), c.table.state ? "yes" : "no");
}

void Context::setActiveId(uint64_t id) { active_id = id; }

static MonstroRect intersect(MonstroRect a, MonstroRect b)
{
   float x0 = a.x > b.x ? a.x : b.x;
   float y0 = a.y > b.y ? a.y : b.y;
   float x1 = a.x + a.w < b.x + b.w ? a.x + a.w : b.x + b.w;
   float y1 = a.y + a.h < b.y + b.h ? a.y + a.h : b.y + b.h;
   return {x0, y0, x1 > x0 ? x1 - x0 : 0.0f, y1 > y0 ? y1 - y0 : 0.0f};
}

bool Context::hoverItem(uint64_t id, MonstroRect rect, bool enabled)
{
   Window *win = current_window;
   if (win != hovered_window)
      return false;
   MonstroRect visible = intersect(rect, canvas->getClipRect());
   if (!monstro_rect_contains(visible, mouse.pos_x, mouse.pos_y))
      return false;

   // Under the mouse and drawn after the items before it, so on top of them.
   hover_top = {id, win->id, visible};

   if (!enabled || disabled)
      return false;
   if (active_id != 0 && active_id != id)
      return false;
   if (hovered_id != 0 && hovered_id != id)
      return false;
   const HoverTop &top = hover_top_prev_frame;
   bool covered = top.id != 0 && top.id != id && top.window == win->id &&
                  monstro_rect_contains(top.rect, mouse.pos_x, mouse.pos_y);
   if (covered)
      return false;
   hovered_id = id;
   return true;
}

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

using monstro::requireContext;
using monstro::requireWindow;

extern "C"
{

MonstroContextConfig monstro_context_config_default(void)
{
   MonstroContextConfig config;
   config.max_windows = MONSTRO_DEFAULT_MAX_WINDOWS;
   config.max_tree_nodes = MONSTRO_DEFAULT_MAX_TREE_NODES;
   config.max_tab_bars = MONSTRO_DEFAULT_MAX_TAB_BARS;
   config.max_tables = MONSTRO_DEFAULT_MAX_TABLES;
   config.max_popups = MONSTRO_DEFAULT_MAX_POPUPS;
   config.max_input_events = MONSTRO_DEFAULT_MAX_INPUT_EVENTS;
   return config;
}

MonstroContext *monstro_context_create(MonstroEnv *env, const MonstroContextConfig *config)
{
   monstro::requireEnv(env, __func__)->requireIdle(__func__);
   MonstroContextConfig defaults = monstro_context_config_default();
   const MonstroContextConfig &c = config ? *config : defaults;
   MONSTRO_REQUIRE(c.max_windows >= 2 && c.max_tree_nodes >= 1 && c.max_tab_bars >= 1 && c.max_tables >= 1 &&
                       c.max_popups >= 1 && c.max_input_events >= 1,
                   "MonstroContextConfig capacities too small (max_windows %u, at least 2 with the root; "
                   "max_tree_nodes %u, max_tab_bars %u, max_tables %u, max_popups %u, max_input_events %u, at "
                   "least 1 each)",
                   c.max_windows, c.max_tree_nodes, c.max_tab_bars, c.max_tables, c.max_popups, c.max_input_events);

   MonstroContext *ctx = new (std::nothrow) MonstroContext(env, c);
   if (!ctx)
      return nullptr;
   if (!ctx->init())
   {
      delete ctx;
      return nullptr;
   }
   env->context_count++;
   return ctx;
}

void monstro_context_destroy(MonstroContext *ctx)
{
   requireContext(ctx, __func__);
   MONSTRO_REQUIRE(!ctx->in_frame, "%s: frame %d is still open; end it first", __func__, ctx->frame);
   ctx->env->requireIdle(__func__);
   ctx->env->context_count--;
   delete ctx;
}

MonstroEnv *monstro_context_get_env(MonstroContext *ctx) { return requireContext(ctx, __func__)->env; }

void monstro_context_set_theme(MonstroContext *ctx, MonstroTheme *theme)
{
   requireContext(ctx, __func__, {MONSTRO_ARG(theme)});
   MONSTRO_REQUIRE(!ctx->in_frame, "%s: change the theme between frames", __func__);
   ctx->theme = theme;
}

MonstroTheme *monstro_context_get_theme(MonstroContext *ctx) { return requireContext(ctx, __func__)->theme; }

void monstro_set_display_size(MonstroContext *ctx, float width, float height)
{
   requireContext(ctx, __func__);
   ctx->display_width = width;
   ctx->display_height = height;
}

void monstro_get_display_size(MonstroContext *ctx, float *out_width, float *out_height)
{
   requireContext(ctx, __func__);
   if (out_width)
      *out_width = ctx->display_width;
   if (out_height)
      *out_height = ctx->display_height;
}

void monstro_begin_frame(MonstroContext *ctx, float delta_time)
{
   requireContext(ctx, __func__)->beginFrame(delta_time, nullptr);
}

void monstro_begin_frame_into(MonstroContext *ctx, float delta_time, const MonstroFrameBuffers *buffers)
{
   requireContext(ctx, __func__, {MONSTRO_ARG(buffers)})->beginFrame(delta_time, buffers);
}

const MonstroDrawData *monstro_end_frame(MonstroContext *ctx) { return requireContext(ctx, __func__)->endFrame(); }

float monstro_get_delta_time(MonstroContext *ctx) { return requireContext(ctx, __func__)->delta_time; }

float monstro_get_time(MonstroContext *ctx) { return requireContext(ctx, __func__)->time; }

/* ------------------------------------------------------------------------- */
/* Current style                                                             */
/* ------------------------------------------------------------------------- */

const MonstroStyle *monstro_get_style(MonstroContext *ctx) { return &requireContext(ctx, __func__)->currentStyle(); }

#define MONSTRO_STYLE_GETTER(TYPE, NAME, FIELD)                                                                        \
   const TYPE *monstro_get_##NAME##_style(MonstroContext *ctx)                                                         \
   {                                                                                                                   \
      return &requireContext(ctx, __func__)->currentStyle().FIELD;                                                     \
   }

MONSTRO_STYLE_GETTER(MonstroStyleText, text, text)
MONSTRO_STYLE_GETTER(MonstroStyleButton, button, button)
MONSTRO_STYLE_GETTER(MonstroStyleButton, contextual_button, contextual_button)
MONSTRO_STYLE_GETTER(MonstroStyleButton, menu_button, menu_button)
MONSTRO_STYLE_GETTER(MonstroStyleToggle, option, option)
MONSTRO_STYLE_GETTER(MonstroStyleToggle, checkbox, checkbox)
MONSTRO_STYLE_GETTER(MonstroStyleSelectable, selectable, selectable)
MONSTRO_STYLE_GETTER(MonstroStyleSlider, slider, slider)
MONSTRO_STYLE_GETTER(MonstroStyleKnob, knob, knob)
MONSTRO_STYLE_GETTER(MonstroStyleProgress, progress, progress)
MONSTRO_STYLE_GETTER(MonstroStyleProperty, property, property)
MONSTRO_STYLE_GETTER(MonstroStyleEdit, edit, edit)
MONSTRO_STYLE_GETTER(MonstroStyleTree, tree, tree)
MONSTRO_STYLE_GETTER(MonstroStyleTab, tab, tab)
MONSTRO_STYLE_GETTER(MonstroStyleTable, table, table)
MONSTRO_STYLE_GETTER(MonstroStyleCombo, combo, combo)
MONSTRO_STYLE_GETTER(MonstroStyleMenu, menu, menu)
MONSTRO_STYLE_GETTER(MonstroStyleSeparator, separator, separator)
MONSTRO_STYLE_GETTER(MonstroStyleTooltip, tooltip, tooltip)
MONSTRO_STYLE_GETTER(MonstroStyleChart, chart, chart)
MONSTRO_STYLE_GETTER(MonstroStyleScrollbar, scrollh, scrollh)
MONSTRO_STYLE_GETTER(MonstroStyleScrollbar, scrollv, scrollv)
MONSTRO_STYLE_GETTER(MonstroStyleWindow, window, window)

#undef MONSTRO_STYLE_GETTER

/* ------------------------------------------------------------------------- */
/* Per-widget style push (backup + apply)                                    */
/* ------------------------------------------------------------------------- */

#define MONSTRO_PUSH_STYLE(CTX, ENUM_TAG, UNION_FIELD, STYLE_FIELD, SRC)                                               \
   do                                                                                                                  \
   {                                                                                                                   \
      monstro::requireWindow((CTX), __func__, {MONSTRO_ARG(SRC)});                                                     \
      MonstroStyleBackup _b = {};                                                                                      \
      _b.type = (ENUM_TAG);                                                                                            \
      _b.data.UNION_FIELD = (CTX)->style().STYLE_FIELD;                                                                \
      (CTX)->canvas->style_stack.push(_b);                                                                             \
      (CTX)->style().STYLE_FIELD = *(SRC);                                                                             \
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
   monstro::requireWindow(ctx, __func__);
   MonstroStyle &style = ctx->style();
   for (int i = 0; i < count; i++)
   {
      MonstroStyleBackup backup = ctx->canvas->style_stack.pop();
      MonstroStyleBackup *b = &backup;
      switch (b->type)
      {
      case MONSTRO_PUSHED_TEXT:
         style.text = b->data.text;
         break;
      case MONSTRO_PUSHED_BUTTON:
         style.button = b->data.button;
         break;
      case MONSTRO_PUSHED_CONTEXTUAL_BUTTON:
         style.contextual_button = b->data.button;
         break;
      case MONSTRO_PUSHED_MENU_BUTTON:
         style.menu_button = b->data.button;
         break;
      case MONSTRO_PUSHED_OPTION:
         style.option = b->data.toggle;
         break;
      case MONSTRO_PUSHED_CHECKBOX:
         style.checkbox = b->data.toggle;
         break;
      case MONSTRO_PUSHED_SELECTABLE:
         style.selectable = b->data.selectable;
         break;
      case MONSTRO_PUSHED_SLIDER:
         style.slider = b->data.slider;
         break;
      case MONSTRO_PUSHED_KNOB:
         style.knob = b->data.knob;
         break;
      case MONSTRO_PUSHED_PROGRESS:
         style.progress = b->data.progress;
         break;
      case MONSTRO_PUSHED_PROPERTY:
         style.property = b->data.property;
         break;
      case MONSTRO_PUSHED_EDIT:
         style.edit = b->data.edit;
         break;
      case MONSTRO_PUSHED_TREE:
         style.tree = b->data.tree;
         break;
      case MONSTRO_PUSHED_TAB:
         style.tab = b->data.tab;
         break;
      case MONSTRO_PUSHED_TABLE:
         style.table = b->data.table;
         break;
      case MONSTRO_PUSHED_COMBO:
         style.combo = b->data.combo;
         break;
      case MONSTRO_PUSHED_MENU:
         style.menu = b->data.menu;
         break;
      case MONSTRO_PUSHED_SEPARATOR:
         style.separator = b->data.separator;
         break;
      case MONSTRO_PUSHED_TOOLTIP:
         style.tooltip = b->data.tooltip;
         break;
      case MONSTRO_PUSHED_CHART:
         style.chart = b->data.chart;
         break;
      case MONSTRO_PUSHED_SCROLLH:
         style.scrollh = b->data.scrollbar;
         break;
      case MONSTRO_PUSHED_SCROLLV:
         style.scrollv = b->data.scrollbar;
         break;
      case MONSTRO_PUSHED_WINDOW:
         style.window = b->data.window;
         break;
      }
   }
}

} // extern "C"

/* ------------------------------------------------------------------------- */
/* ID Stack C API                                                            */
/* ------------------------------------------------------------------------- */

extern "C"
{

void monstro_push_id(MonstroContext *ctx, const char *name)
{
   requireWindow(ctx, __func__, {MONSTRO_ARG(name)});
   ctx->ids().push(name);
}

void monstro_pop_id(MonstroContext *ctx)
{
   requireWindow(ctx, __func__);
   ctx->ids().pop();
}

uint64_t monstro_get_id(MonstroContext *ctx, const char *name)
{
   requireWindow(ctx, __func__, {MONSTRO_ARG(name)});
   uint64_t combined = ctx->ids().getId(name);
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
   requireContext(ctx, __func__);
   ctx->last_item_id = id;
   ctx->last_item_rect = rect;
   ctx->last_item_hovered = hovered;
   ctx->last_item_active = active;
   ctx->last_item_clicked = clicked;
   ctx->last_item_edited = edited;
   ctx->last_item_activated = false;
   ctx->last_item_deactivated = false;
   ctx->last_item_deactivated_after_edit = false;
   if (id == 0)
      return;

   using ActiveItem = monstro::Context::ActiveItem;
   auto find = [id](monstro::FixedStack<ActiveItem, MONSTRO_MAX_ACTIVE_ITEMS> &items) -> ActiveItem *
   {
      for (int i = 0; i < items.size(); i++)
      {
         if (items[i].id == id)
            return &items[i];
      }
      return nullptr;
   };
   const ActiveItem *before = find(ctx->active_items_prev_frame);
   ActiveItem *now = find(ctx->active_items);
   if (active)
   {
      if (!now)
      {
         MONSTRO_REQUIRE(ctx->active_items.size() < MONSTRO_MAX_ACTIVE_ITEMS,
                         "more than %d items are active at once in frame %d (window '%s')", MONSTRO_MAX_ACTIVE_ITEMS,
                         ctx->frame, ctx->current_window ? ctx->current_window->name : "");
         now = &ctx->active_items.push({id, before && before->edited});
      }
      now->edited = now->edited || edited;
   }
   ctx->last_item_activated = active && !before;
   ctx->last_item_deactivated = !active && before && !now;
   ctx->last_item_deactivated_after_edit = ctx->last_item_deactivated && (before->edited || edited);
}

bool monstro_is_item_hovered(MonstroContext *ctx) { return requireContext(ctx, __func__)->last_item_hovered; }

bool monstro_is_item_active(MonstroContext *ctx) { return requireContext(ctx, __func__)->last_item_active; }

bool monstro_is_item_clicked(MonstroContext *ctx, MonstroMouseButton button)
{
   monstro::requireMouseButton(ctx, __func__, button);
   return ctx->last_item_hovered && monstro_is_mouse_clicked(ctx, button, false);
}

bool monstro_is_item_edited(MonstroContext *ctx) { return requireContext(ctx, __func__)->last_item_edited; }

bool monstro_is_item_activated(MonstroContext *ctx) { return requireContext(ctx, __func__)->last_item_activated; }

bool monstro_is_item_deactivated(MonstroContext *ctx) { return requireContext(ctx, __func__)->last_item_deactivated; }

bool monstro_is_item_deactivated_after_edit(MonstroContext *ctx)
{
   return requireContext(ctx, __func__)->last_item_deactivated_after_edit;
}

MonstroRect monstro_get_item_rect(MonstroContext *ctx) { return requireContext(ctx, __func__)->last_item_rect; }

MonstroVec2 monstro_get_item_rect_min(MonstroContext *ctx)
{
   MonstroRect r = requireContext(ctx, __func__)->last_item_rect;
   return {r.x, r.y};
}

MonstroVec2 monstro_get_item_rect_max(MonstroContext *ctx)
{
   MonstroRect r = requireContext(ctx, __func__)->last_item_rect;
   return {r.x + r.w, r.y + r.h};
}

MonstroVec2 monstro_get_item_rect_size(MonstroContext *ctx)
{
   MonstroRect r = requireContext(ctx, __func__)->last_item_rect;
   return {r.w, r.h};
}

/* ------------------------------------------------------------------------- */
/* Disabled State                                                            */
/* ------------------------------------------------------------------------- */

void monstro_begin_disabled(MonstroContext *ctx, bool disabled)
{
   requireWindow(ctx, __func__);
   // A scope is disabled when it asks to be or when any enclosing scope is.
   ctx->canvas->disabled_stack.push(ctx->disabled);
   ctx->disabled = ctx->disabled || disabled;
}

void monstro_end_disabled(MonstroContext *ctx)
{
   requireWindow(ctx, __func__);
   ctx->disabled = ctx->canvas->disabled_stack.pop();
}

bool monstro_is_disabled(MonstroContext *ctx) { return requireContext(ctx, __func__)->disabled; }
