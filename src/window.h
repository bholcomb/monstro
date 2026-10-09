#ifndef MONSTRO_INTERNAL_WINDOW_H
#define MONSTRO_INTERNAL_WINDOW_H

#include "canvas.h"
#include "constants.h"
#include "layout.h"
#include "monstro.h"

namespace monstro
{

struct Context;

// A window's state kept between frames. Its per-frame stacks (layouts, groups,
// clip rects, layers) live in the canvas lent for the frame, above the bases
// recorded when the window begins.
struct Window
{
   Context *ctx = nullptr;

   char name[256] = {};
   char title[256] = {};
   bool title_overridden = false;
   uint64_t id = 0;
   MonstroWindowFlags flags = MONSTRO_WINDOW_NONE;
   int last_frame_active = 0;
   bool active = false;
   bool skip_items = false;
   uint64_t move_id = 0;
   uint64_t popup_id = 0;     // the popup this window shows, when it is one
   bool child_region = false; // begun by monstro_begin_child, so ended by monstro_end_child

   int layout_base = 0;           // layouts below this belong to enclosing windows
   int layout_depth_at_begin = 0; // the window's own layouts once begin() has set up its content
   int group_base = 0;
   int clip_base = 0;
   int layer_base = 0;
   int draw_rank = 0; // position in the draw order; see Context::rankWindowsForDrawing

   MonstroVec2 pos = {0.0f, 0.0f};
   MonstroVec2 size = {0.0f, 0.0f};
   MonstroRect hit_rect = {}; // where it takes the mouse: its rect, clipped as drawn (a child panel by its parent)
   float scroll_x = 0.0f, scroll_y = 0.0f;
   float content_height = 0.0f;
   float content_width = 0.0f;
   float content_view_width = 0.0f;
   float content_view_height = 0.0f;
   bool show_scrollbar_x = false;
   bool show_scrollbar_y = false;
   bool collapsed = false;
   bool content_clip_pushed = false;
   bool *p_open_ref = nullptr;

   Window *parent = nullptr;
   Window *child = nullptr;
   Window *sibling = nullptr;

   MonstroCondition set_pos_allow_flags = MONSTRO_COND_NONE;
   MonstroCondition set_size_allow_flags = MONSTRO_COND_NONE;

   void init(Context *context, const char *windowName, MonstroWindowFlags windowFlags);

   uint64_t getChildId(const char *childName);

   void move(float dx, float dy)
   {
      pos.x += dx;
      pos.y += dy;
   }

   MonstroVec2 getCursorPos() const;
   void setCursorPos(float x, float y);
   MonstroVec2 getCursorScreenPos() const;
   void setCursorScreenPos(float x, float y);

   MonstroRect getRect() const;
   MonstroRect getContentRect() const;
   float getTitleBarHeight() const;
   MonstroRect getTitleBarRect() const;
   float getMenuBarHeight() const;
   MonstroRect getMenuBarRect() const;

   bool begin(bool *p_open);
   bool end();
   bool mouseOverTitle() const;

   void addChild(Window *win);
   void remove();
   void makeLastSibling();

   int layoutDepth() const;
   int groupDepth() const;
   Canvas &canvas() const;

   void setPosition(float x, float y, MonstroCondition cond);
   void setSize(float w, float h, MonstroCondition cond);

   Layout *getCurrentLayout();
   void setLayoutDirection(MonstroLayoutDirection direction);
   void addItem(float width, float height);
   void beginLayout(MonstroLayoutDirection direction);
   void beginLayout(float x, float y, MonstroLayoutDirection direction);
   void endLayout();

   void pushContentClipRect();

 private:
   float getScrollableContentWidth() const;
   float getScrollableContentHeight() const;
   void drawBackground();
   void drawForeground();
   void updateScrollState(float header_h);
   void clampScrollOffsets();
   float getMaxScrollX() const;
   float getMaxScrollY() const;
   void drawVerticalScrollbar(float header_h);
   void drawHorizontalScrollbar(float header_h);
};

} // namespace monstro

typedef monstro::Window MonstroWindowInternal;

#endif
