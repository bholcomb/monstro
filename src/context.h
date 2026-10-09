#ifndef MONSTRO_INTERNAL_CONTEXT_H
#define MONSTRO_INTERNAL_CONTEXT_H

#include "canvas.h"
#include "constants.h"
#include "editstate.h"
#include "fatal.h"
#include "fixed.h"
#include "font.h"
#include "helpers.h"
#include "idStack.h"
#include "input.h"
#include "monstro.h"
#include "popup.h"
#include "tooltip.h"
#include "uistate.h"
#include "window.h"
#include <initializer_list>

namespace monstro
{

struct Env;
struct Theme;

// A pointer argument an API function cannot work without; see requireWindow.
struct RequiredArg
{
   const char *name;
   const void *value;
};

#define MONSTRO_ARG(x)                                                                                                 \
   monstro::RequiredArg { #x, (const void *)(x) }

/*
 * One UI: its input, display size and the interaction state it keeps between
 * frames (windows, popups, tree, tab bar and table state, hot and active IDs,
 * text editing). Everything per-frame lives in the canvas it borrows from its
 * environment between begin_frame and end_frame.
 */
struct Context
{
   Context(Env *environment, const MonstroContextConfig &context_config);
   ~Context();

   Context(const Context &) = delete;
   Context &operator=(const Context &) = delete;

   bool init();
   size_t residentBytes() const;

   void beginFrame(float deltaTime, const MonstroFrameBuffers *buffers);
   const MonstroDrawData *endFrame();

   // The frame's working style and ID stack, in the borrowed canvas: only inside a frame.
   MonstroStyle &style() const;
   IdStack &ids() const;
   // The theme's style between frames, the working copy inside one.
   const MonstroStyle &currentStyle() const;

   void setActiveId(uint64_t id);
   void keepAliveId(uint64_t id);

   Window *findWindow(const char *name);
   Window *createWindow(const char *name, MonstroWindowFlags flags);
   Window *findHoveredWindow(float x, float y, bool excludeChildren = false);
   // Numbers every window in draw order, which hover testing follows too.
   void rankWindowsForDrawing();
   void focusWindow(Window *window);
   void pushWindow(Window *win);
   void popWindow();

   Layout *getCurrentLayout();

   // Every begin/end and push/pop pair is closed; checked at the end of the frame.
   void requireBalanced(const char *where) const;

   MonstroTreeNodeState *findOrCreateTreeState(uint64_t id, bool default_open);

   // Input changes wait in input_events until a frame takes them; see input.h.
   void queueMousePos(float x, float y);
   void queueMouseButton(MonstroMouseButton button, bool down);
   void queueMouseWheel(float x, float y);
   void queueKey(MonstroKey key, bool down);
   void takeInputEvents();

   Env *env = nullptr;
   MonstroContextConfig config = {};
   Theme *theme = nullptr;

   Canvas *canvas = nullptr; // lent for the current frame, null between frames
   bool in_frame = false;
   uint64_t last_content_hash = 0;

   ConfiguredArray<Window> windows{"window table", "raise MonstroContextConfig.max_windows"};
   Window *root_window = nullptr;
   Window *current_window = nullptr;
   Window *hovered_window = nullptr;
   Window *focused_window = nullptr;

   ConfiguredArray<MonstroPopupInternal> popup_stack{"popup stack", "raise MonstroContextConfig.max_popups"};
   ConfiguredArray<MonstroTabBarState> tab_bars{"tab bar table", "raise MonstroContextConfig.max_tab_bars"};
   ConfiguredArray<MonstroTreeNodeState> tree_states{"tree node table", "raise MonstroContextConfig.max_tree_nodes"};
   ConfiguredArray<MonstroTableState> tables{"table state table", "raise MonstroContextConfig.max_tables"};

   uint64_t hovered_id = 0;
   uint64_t hovered_id_prev_frame = 0;
   uint64_t active_id = 0;
   uint64_t active_id_prev_frame = 0;
   bool active_id_is_alive = false;

   uint64_t last_item_id = 0;
   MonstroRect last_item_rect = {};
   bool last_item_hovered = false;
   bool last_item_active = false;
   bool last_item_clicked = false;
   bool last_item_edited = false;
   bool last_item_deactivated_after_edit = false;

   bool disabled = false;

   ConfiguredArray<InputEvent> input_events{"input event queue",
                                            "raise MonstroContextConfig.max_input_events or begin frames more often"};
   MouseState mouse;
   KeyboardState keyboard;
   MonstroMouseCursor mouse_cursor = MONSTRO_MOUSE_CURSOR_ARROW;

   bool want_capture_mouse = false;
   bool want_capture_keyboard = false;
   bool want_text_input = false;

   float display_width = 0.0f;
   float display_height = 0.0f;
   float delta_time = 0.0f;
   float time = 0.0f;
   int frame = 0;

   float next_window_pos_x = 0.0f, next_window_pos_y = 0.0f;
   float next_window_size_x = 0.0f, next_window_size_y = 0.0f;
   bool next_window_focus = false;
   MonstroCondition next_window_pos_cond = MONSTRO_COND_NONE;
   MonstroCondition next_window_size_cond = MONSTRO_COND_NONE;

   TooltipState tooltip;
   EditState edit_state;
};

// The context an API function works on: it must not be null, nor any of args.
Context *requireContext(Context *ctx, const char *function, std::initializer_list<RequiredArg> args = {});
// Closes the popup with this ID, if it is open, and the popups opened from it.
void closePopup(Context *ctx, uint64_t popup_id);

// The context, for an API function given a mouse button or key: it must be one Monstro tracks.
Context *requireMouseButton(Context *ctx, const char *function, MonstroMouseButton button);
Context *requireKey(Context *ctx, const char *function, MonstroKey key);
// The window an API function draws into: the context must be valid and inside a frame, and args not null.
Window *requireWindow(Context *ctx, const char *function, std::initializer_list<RequiredArg> args = {});

} // namespace monstro

typedef monstro::Context MonstroContext;

#endif
