#include "context.h"
#include "env.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace monstro
{

static void set_window_name_and_title(Window *window, const char *window_name)
{
   if (!window)
      return;

   monstro_copy_id_text(window_name, window->name, sizeof(window->name));

   const char *title = monstro_display_text(window_name);
   if (!title)
      title = "";
   monstro_utf8_copy(window->title, sizeof(window->title), title);
}

/* ------------------------------------------------------------------------- */
/* Window Member Functions                                                   */
/* ------------------------------------------------------------------------- */

void Window::init(Context *context, const char *windowName, MonstroWindowFlags windowFlags)
{
   ctx = context;

   set_window_name_and_title(this, windowName);

   id = ctx->ids().getId(name);
   move_id = getChildId("MOVE");
   flags = windowFlags;

   set_pos_allow_flags = (MonstroCondition)(MONSTRO_COND_ALWAYS | MONSTRO_COND_ONCE | MONSTRO_COND_FIRST_USE_EVER |
                                            MONSTRO_COND_APPEARING);
   set_size_allow_flags = (MonstroCondition)(MONSTRO_COND_ALWAYS | MONSTRO_COND_ONCE | MONSTRO_COND_FIRST_USE_EVER |
                                             MONSTRO_COND_APPEARING);
}

uint64_t Window::getChildId(const char *childName)
{
   uint64_t childId = ctx->ids().getId(childName);
   ctx->keepAliveId(childId);
   return childId;
}

/* ------------------------------------------------------------------------- */
/* Begin / End                                                               */
/* ------------------------------------------------------------------------- */

bool Window::begin(bool *p_open)
{
   if (p_open && *p_open == false)
      return false;

   bool appearing = (last_frame_active != ctx->frame - 1);
   p_open_ref = p_open;
   ctx->ids().push(id);

   if (flags & MONSTRO_WINDOW_MODAL)
   {
      makeLastSibling();
   }

   if (ctx->next_window_pos_cond != MONSTRO_COND_NONE)
   {
      bool shouldSet = (set_pos_allow_flags & ctx->next_window_pos_cond) != 0;
      bool isValid = ctx->next_window_pos_x != -INFINITY && ctx->next_window_pos_y != -INFINITY;
      if (shouldSet && isValid)
      {
         setPosition(ctx->next_window_pos_x, ctx->next_window_pos_y, ctx->next_window_pos_cond);
         ctx->next_window_pos_cond = MONSTRO_COND_NONE;
         ctx->next_window_pos_x = -INFINITY;
         ctx->next_window_pos_y = -INFINITY;
      }
   }

   if (ctx->next_window_size_cond != MONSTRO_COND_NONE)
   {
      bool shouldSet = (set_size_allow_flags & ctx->next_window_size_cond) != 0;
      bool isValid = ctx->next_window_size_x != -INFINITY && ctx->next_window_size_y != -INFINITY;
      if (shouldSet && isValid)
      {
         setSize(ctx->next_window_size_x, ctx->next_window_size_y, ctx->next_window_size_cond);
         ctx->next_window_size_cond = MONSTRO_COND_NONE;
         ctx->next_window_size_x = -INFINITY;
         ctx->next_window_size_y = -INFINITY;
      }
   }

   // This window's per-frame stacks start above those of the windows it is nested in.
   Canvas &c = canvas();
   layout_base = c.layouts.size();
   group_base = c.groups.size();
   clip_base = c.clip_rects.size();
   layer_base = c.layers.size();

   MonstroStyle *pad_style = &ctx->style();
   beginLayout(pad_style->window.padding[0], pad_style->window.padding[1], MONSTRO_LAYOUT_VERTICAL);

   last_frame_active = ctx->frame;
   active = true;

   // A child region is clipped by its parent's current clip; any other window starts unclipped.
   bool clipped_by_parent = (flags & MONSTRO_WINDOW_CHILD) && parent;
   c.pushBaseClipRect(clipped_by_parent ? c.getClipRect() : MonstroRect{0, 0, 100000, 100000});
   c.pushLayer(0);
   ctx->pushWindow(this);

   if (ctx->next_window_focus || (appearing && (flags & MONSTRO_WINDOW_FOCUS_ON_APPEARING)))
   {
      ctx->focusWindow(this);
   }
   ctx->next_window_focus = false;

   if ((flags & MONSTRO_WINDOW_TITLE_BAR) && (flags & MONSTRO_WINDOW_COLLAPSABLE))
   {
      MonstroRect title_rect = getTitleBarRect();
      if (ctx->hovered_window == this && monstro_rect_contains(title_rect, ctx->mouse.pos_x, ctx->mouse.pos_y) &&
          ctx->mouse.isDoubleClicked(MONSTRO_MOUSE_BUTTON_LEFT))
      {
         collapsed = !collapsed;
      }
   }
   else
   {
      collapsed = false;
   }

   ctx->keepAliveId(move_id);
   if (ctx->active_id == move_id)
   {
      if (ctx->mouse.isDown(MONSTRO_MOUSE_BUTTON_MIDDLE))
      {
         if (flags & MONSTRO_WINDOW_MOVABLE)
         {
            move(ctx->mouse.delta_x, ctx->mouse.delta_y);
         }
         ctx->focusWindow(this);
      }
      else
      {
         ctx->setActiveId(0);
      }
   }

   drawBackground();

   skip_items = collapsed;

   drawForeground();

   float title_h = getTitleBarHeight();
   float menu_h = getMenuBarHeight();
   float header_h = title_h + menu_h;

   if (title_h > 0.0f)
      addItem(0, title_h);
   if (menu_h > 0.0f)
      addItem(0, menu_h);

   content_clip_pushed = false;

   if (!collapsed)
   {
      updateScrollState(header_h);
      clampScrollOffsets();

      pushContentClipRect();

      beginLayout(MONSTRO_LAYOUT_VERTICAL);
      Layout *content = getCurrentLayout();
      content->myScrollOffsetX = -scroll_x;
      content->myScrollOffsetY = -scroll_y;
      content->myAvailableWidth = content_view_width;
      content->myAvailableHeight = content_view_height;
      MonstroVec2 cursor = getCursorScreenPos();
      content->myVisibleMaxScreenX = cursor.x + content_view_width;
      content->myVisibleMaxScreenY = cursor.y + content_view_height;
   }

   if (!collapsed && ctx->hovered_window == this)
   {
      bool scroll_with_mouse = (flags & MONSTRO_WINDOW_SCROLL_WITH_MOUSE) != 0;
      if (scroll_with_mouse && show_scrollbar_y && ctx->mouse.wheel_y != 0.0f)
      {
         float scroll_speed = 40.0f;
         scroll_y -= ctx->mouse.wheel_y * scroll_speed;
      }
      if (scroll_with_mouse && show_scrollbar_x && ctx->mouse.wheel_x != 0.0f)
      {
         float scroll_speed = 40.0f;
         scroll_x -= ctx->mouse.wheel_x * scroll_speed;
      }
      clampScrollOffsets();
   }

   if (ctx->hovered_window == this && (flags & MONSTRO_WINDOW_INPUTS))
   {
      ctx->want_capture_mouse = true;
   }

   layout_depth_at_begin = layoutDepth();
   return true;
}

bool Window::end()
{
   MonstroStyle *style = &ctx->style();
   float header_h = getTitleBarHeight() + getMenuBarHeight();

   MONSTRO_REQUIRE(layoutDepth() == layout_depth_at_begin && groupDepth() == 0,
                   "window '%s' ends with %d layouts open (expected %d) and %d groups open: a monstro_end_layout or "
                   "monstro_end_group is missing or doubled",
                   name, layoutDepth(), layout_depth_at_begin, groupDepth());

   if (!collapsed)
   {
      Layout *content_layout = getCurrentLayout();
      if (content_layout)
      {
         content_width = content_layout->mySizeX + style->window.padding[0] * 2.0f;
         content_height = content_layout->mySizeY + style->window.padding[1] * 2.0f;
         if (content_height < 0.0f)
            content_height = 0.0f;
      }
      endLayout();
   }

   if (content_clip_pushed)
   {
      canvas().popClipRect();
   }

   if (flags & MONSTRO_WINDOW_AUTO_RESIZE)
   {
      if (content_width > 0.0f)
         size.x = content_width;
      if (content_height > 0.0f)
         size.y = content_height + header_h;
   }

   updateScrollState(header_h);
   clampScrollOffsets();

   if (!(flags & MONSTRO_WINDOW_AUTO_RESIZE))
   {
      if (show_scrollbar_y)
      {
         drawVerticalScrollbar(header_h);
      }
      if (show_scrollbar_x)
      {
         drawHorizontalScrollbar(header_h);
      }
   }

   if ((flags & MONSTRO_WINDOW_RESIZABLE) && !collapsed)
   {
      float grip_sz = 14.0f;
      float grip_x = pos.x + size.x - grip_sz;
      float grip_y = pos.y + size.y - grip_sz;
      MonstroRect grip_rect = {grip_x, grip_y, grip_sz, grip_sz};

      uint64_t resize_id = getChildId("##resize");
      ctx->keepAliveId(resize_id);

      bool grip_hovered =
          ctx->hovered_window == this && monstro_rect_contains(grip_rect, ctx->mouse.pos_x, ctx->mouse.pos_y);

      if (grip_hovered && ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
      {
         ctx->setActiveId(resize_id);
      }

      bool grip_held = (ctx->active_id == resize_id);
      if (grip_held)
      {
         if (ctx->mouse.isDown(MONSTRO_MOUSE_BUTTON_LEFT))
         {
            size.x += ctx->mouse.delta_x;
            size.y += ctx->mouse.delta_y;
            if (size.x < style->window.min_size[0])
               size.x = style->window.min_size[0];
            if (size.y < style->window.min_size[1])
               size.y = style->window.min_size[1];
         }
         else
         {
            ctx->setActiveId(0);
         }
      }

      uint32_t grip_col;
      if (grip_held)
      {
         grip_col = monstro_color_to_u32(style->window.header.active.data.color);
      }
      else if (grip_hovered)
      {
         grip_col = monstro_color_to_u32(style->window.header.hover.data.color);
      }
      else
      {
         grip_col = monstro_color_to_u32(style->window.scaler.data.color);
      }

      MonstroVec2 t1 = {pos.x + size.x, pos.y + size.y - grip_sz};
      MonstroVec2 t2 = {pos.x + size.x - grip_sz, pos.y + size.y};
      MonstroVec2 t3 = {pos.x + size.x, pos.y + size.y};
      canvas().addTriangleFilled(t1, t2, t3, grip_col);
   }

   Canvas &c = canvas();
   c.popClipRect();
   int clips_open = c.clip_rects.size() - clip_base;
   int layers_open = c.layers.size() - layer_base;
   MONSTRO_REQUIRE(clips_open == 1 && layers_open == 1,
                   "window '%s' ends with unbalanced canvas stacks: %d clip rects and %d layers open (expected 1 "
                   "each); a monstro_push_clip_rect or monstro_pop_clip_rect is unpaired",
                   name, clips_open, layers_open);
   c.clip_rects.truncate(clip_base);
   c.layers.truncate(layer_base);
   c.layouts.truncate(layout_base);
   p_open_ref = nullptr;
   ctx->ids().pop();
   return true;
}

/* ------------------------------------------------------------------------- */
/* Drawing                                                                   */
/* ------------------------------------------------------------------------- */

void Window::updateScrollState(float header_h)
{
   MonstroStyle *style = &ctx->style();
   const float scroll_epsilon = 1.0f;
   float scroll_content_w = getScrollableContentWidth();
   float scroll_content_h = getScrollableContentHeight();
   float bar_w = style->window.scrollbar_size[0];
   float bar_h = style->window.scrollbar_size[1];
   if (bar_w <= 0.0f)
      bar_w = 10.0f;
   if (bar_h <= 0.0f)
      bar_h = 10.0f;

   float base_w = size.x - style->window.padding[0] * 2.0f;
   float base_h = size.y - style->window.padding[1] * 2.0f - header_h;
   if (base_w < 0.0f)
      base_w = 0.0f;
   if (base_h < 0.0f)
      base_h = 0.0f;

   show_scrollbar_x = false;
   show_scrollbar_y = false;
   content_view_width = base_w;
   content_view_height = base_h;

   if (flags & MONSTRO_WINDOW_AUTO_RESIZE)
      return;

   bool allow_x = (flags & MONSTRO_WINDOW_HORIZONTAL_SCROLLBAR) != 0;
   bool allow_y = (flags & MONSTRO_WINDOW_VERTICAL_SCROLLBAR) != 0;

   if (allow_y && scroll_content_h > base_h + scroll_epsilon)
   {
      show_scrollbar_y = true;
   }
   if (allow_x && scroll_content_w > base_w - (show_scrollbar_y ? bar_w : 0.0f) + scroll_epsilon)
   {
      show_scrollbar_x = true;
   }
   if (allow_y && !show_scrollbar_y && scroll_content_h > base_h - (show_scrollbar_x ? bar_h : 0.0f) + scroll_epsilon)
   {
      show_scrollbar_y = true;
   }
   if (allow_x && !show_scrollbar_x && scroll_content_w > base_w - (show_scrollbar_y ? bar_w : 0.0f) + scroll_epsilon)
   {
      show_scrollbar_x = true;
   }

   content_view_width = base_w - (show_scrollbar_y ? bar_w : 0.0f);
   content_view_height = base_h - (show_scrollbar_x ? bar_h : 0.0f);
   if (content_view_width < 0.0f)
      content_view_width = 0.0f;
   if (content_view_height < 0.0f)
      content_view_height = 0.0f;
}

float Window::getMaxScrollX() const
{
   float max_scroll = getScrollableContentWidth() - content_view_width;
   return max_scroll > 0.0f ? max_scroll : 0.0f;
}

float Window::getMaxScrollY() const
{
   float max_scroll = getScrollableContentHeight() - content_view_height;
   return max_scroll > 0.0f ? max_scroll : 0.0f;
}

float Window::getScrollableContentWidth() const
{
   float padding = ctx->style().window.padding[0] * 2.0f;
   float scroll_content_w = content_width - padding;
   return scroll_content_w > 0.0f ? scroll_content_w : 0.0f;
}

float Window::getScrollableContentHeight() const
{
   float padding = ctx->style().window.padding[1] * 2.0f;
   float scroll_content_h = content_height - padding;
   return scroll_content_h > 0.0f ? scroll_content_h : 0.0f;
}

void Window::clampScrollOffsets()
{
   float max_scroll_x = getMaxScrollX();
   float max_scroll_y = getMaxScrollY();
   if (!show_scrollbar_x)
      scroll_x = 0.0f;
   if (!show_scrollbar_y)
      scroll_y = 0.0f;
   if (scroll_x < 0.0f)
      scroll_x = 0.0f;
   if (scroll_x > max_scroll_x)
      scroll_x = max_scroll_x;
   if (scroll_y < 0.0f)
      scroll_y = 0.0f;
   if (scroll_y > max_scroll_y)
      scroll_y = max_scroll_y;
}

void Window::drawVerticalScrollbar(float header_h)
{
   if (!show_scrollbar_y || collapsed)
      return;

   MonstroStyle *style = &ctx->style();
   MonstroStyleScrollbar *ss = &style->scrollv;
   float bar_w = style->window.scrollbar_size[0];
   if (bar_w <= 0.0f)
      bar_w = 10.0f;

   float max_scroll = getMaxScrollY();
   MonstroRect content_rect = getContentRect();
   MonstroRect track_rect = {content_rect.x + content_rect.w, pos.y + header_h, bar_w, content_rect.h};

   if (ss->normal.type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas().addRectFilled(track_rect, monstro_color_to_u32(ss->normal.data.color), ss->rounding, MONSTRO_CORNER_ALL);
   }

   float scroll_content_h = getScrollableContentHeight();
   float thumb_ratio = scroll_content_h > 0.0f ? (content_rect.h / scroll_content_h) : 1.0f;
   float thumb_h = track_rect.h * thumb_ratio;
   if (thumb_h < 20.0f)
      thumb_h = 20.0f;
   if (thumb_h > track_rect.h)
      thumb_h = track_rect.h;

   float scroll_ratio = max_scroll > 0.0f ? (scroll_y / max_scroll) : 0.0f;
   float thumb_y = track_rect.y + scroll_ratio * (track_rect.h - thumb_h);
   MonstroRect thumb_rect = {track_rect.x + ss->padding[0], thumb_y, track_rect.w - ss->padding[0] * 2.0f, thumb_h};

   bool thumb_hovered = false;
   bool thumb_held = false;
   uint64_t sb_id = ctx->ids().getId("##vscroll");
   ctx->keepAliveId(sb_id);

   if (ctx->hovered_window == this)
   {
      float mx = ctx->mouse.pos_x;
      float my = ctx->mouse.pos_y;
      thumb_hovered = monstro_rect_contains(track_rect, mx, my);
      if (thumb_hovered && ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
      {
         ctx->setActiveId(sb_id);
      }
   }

   if (ctx->active_id == sb_id)
   {
      if (ctx->mouse.isDown(MONSTRO_MOUSE_BUTTON_LEFT))
      {
         thumb_held = true;
         float scrollable_track = track_rect.h - thumb_h;
         if (scrollable_track > 0.0f)
         {
            scroll_y += ctx->mouse.delta_y * (max_scroll / scrollable_track);
            clampScrollOffsets();
         }
      }
      else
      {
         ctx->setActiveId(0);
      }
   }

   MonstroStyleItem *cursor_style = &ss->cursor_normal;
   if (thumb_held)
      cursor_style = &ss->cursor_active;
   else if (thumb_hovered)
      cursor_style = &ss->cursor_hover;

   if (cursor_style->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas().addRectFilled(thumb_rect, monstro_color_to_u32(cursor_style->data.color), ss->rounding_cursor,
                             MONSTRO_CORNER_ALL);
   }
   if (ss->border > 0.0f)
   {
      canvas().addRect(track_rect, monstro_color_to_u32(ss->border_color), ss->rounding, MONSTRO_CORNER_ALL,
                       ss->border);
   }
   if (ss->border_cursor > 0.0f)
   {
      canvas().addRect(thumb_rect, monstro_color_to_u32(ss->cursor_border_color), ss->rounding_cursor,
                       MONSTRO_CORNER_ALL, ss->border_cursor);
   }
}

void Window::drawHorizontalScrollbar(float header_h)
{
   if (!show_scrollbar_x || collapsed)
      return;

   MonstroStyle *style = &ctx->style();
   MonstroStyleScrollbar *ss = &style->scrollh;
   float bar_h = style->window.scrollbar_size[1];
   if (bar_h <= 0.0f)
      bar_h = 10.0f;

   float max_scroll = getMaxScrollX();
   MonstroRect content_rect = getContentRect();
   MonstroRect track_rect = {content_rect.x, pos.y + header_h + content_rect.h, content_rect.w, bar_h};

   if (ss->normal.type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas().addRectFilled(track_rect, monstro_color_to_u32(ss->normal.data.color), ss->rounding, MONSTRO_CORNER_ALL);
   }

   float scroll_content_w = getScrollableContentWidth();
   float thumb_ratio = scroll_content_w > 0.0f ? (content_rect.w / scroll_content_w) : 1.0f;
   float thumb_w = track_rect.w * thumb_ratio;
   if (thumb_w < 20.0f)
      thumb_w = 20.0f;
   if (thumb_w > track_rect.w)
      thumb_w = track_rect.w;

   float scroll_ratio = max_scroll > 0.0f ? (scroll_x / max_scroll) : 0.0f;
   float thumb_x = track_rect.x + scroll_ratio * (track_rect.w - thumb_w);
   MonstroRect thumb_rect = {thumb_x, track_rect.y + ss->padding[1], thumb_w, track_rect.h - ss->padding[1] * 2.0f};

   bool thumb_hovered = false;
   bool thumb_held = false;
   uint64_t sb_id = ctx->ids().getId("##hscroll");
   ctx->keepAliveId(sb_id);

   if (ctx->hovered_window == this)
   {
      float mx = ctx->mouse.pos_x;
      float my = ctx->mouse.pos_y;
      thumb_hovered = monstro_rect_contains(track_rect, mx, my);
      if (thumb_hovered && ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
      {
         ctx->setActiveId(sb_id);
      }
   }

   if (ctx->active_id == sb_id)
   {
      if (ctx->mouse.isDown(MONSTRO_MOUSE_BUTTON_LEFT))
      {
         thumb_held = true;
         float scrollable_track = track_rect.w - thumb_w;
         if (scrollable_track > 0.0f)
         {
            scroll_x += ctx->mouse.delta_x * (max_scroll / scrollable_track);
            clampScrollOffsets();
         }
      }
      else
      {
         ctx->setActiveId(0);
      }
   }

   MonstroStyleItem *cursor_style = &ss->cursor_normal;
   if (thumb_held)
      cursor_style = &ss->cursor_active;
   else if (thumb_hovered)
      cursor_style = &ss->cursor_hover;

   if (cursor_style->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas().addRectFilled(thumb_rect, monstro_color_to_u32(cursor_style->data.color), ss->rounding_cursor,
                             MONSTRO_CORNER_ALL);
   }
   if (ss->border > 0.0f)
   {
      canvas().addRect(track_rect, monstro_color_to_u32(ss->border_color), ss->rounding, MONSTRO_CORNER_ALL,
                       ss->border);
   }
   if (ss->border_cursor > 0.0f)
   {
      canvas().addRect(thumb_rect, monstro_color_to_u32(ss->cursor_border_color), ss->rounding_cursor,
                       MONSTRO_CORNER_ALL, ss->border_cursor);
   }
}

void Window::pushContentClipRect()
{
   if (content_clip_pushed)
      return;
   MonstroRect content_rect = getContentRect();
   if (content_rect.w <= 0.0f || content_rect.h <= 0.0f)
   {
      canvas().pushClipRectFullscreen();
   }
   else
   {
      // The content starts a padding in from the window's edge, so the clip reaches a padding past the view,
      // up to a scrollbar where there is one.
      const MonstroStyle &style = ctx->style();
      if (!show_scrollbar_y)
         content_rect.w += style.window.padding[0];
      if (!show_scrollbar_x)
         content_rect.h += style.window.padding[1];
      canvas().pushClipRect(content_rect);
   }
   content_clip_pushed = true;
}

void Window::drawBackground()
{
   MonstroStyle *style = &ctx->style();

   // A modal dims everything drawn below it.
   if (flags & MONSTRO_WINDOW_MODAL)
   {
      MonstroRect screen = {0.0f, 0.0f, ctx->display_width, ctx->display_height};
      canvas().addRectFilled(screen, monstro_color_to_u32(style->window.modal_dim_color), 0.0f, MONSTRO_CORNER_NONE);
   }

   float saved_rounding = style->window.rounding;

   if (flags & MONSTRO_WINDOW_ROOT)
   {
      canvas().pushClipRectFullscreen();
      style->window.rounding = 0.0f;
   }
   else if (size.y <= 0.0f)
   {
      canvas().pushClipRectFullscreen();
   }
   else
   {
      MonstroRect clip_rect = getRect();
      clip_rect.w += 1;
      clip_rect.h += 1;
      canvas().pushClipRect(clip_rect);
   }

   if (!collapsed && (flags & MONSTRO_WINDOW_BACKGROUND))
   {
      MonstroStyleItem *bg = &style->window.fixed_background;
      MonstroRect r = getRect();
      switch (bg->type)
      {
      case MONSTRO_STYLE_ITEM_COLOR:
         canvas().addRectFilled(r, monstro_color_to_u32(bg->data.color), style->window.rounding, MONSTRO_CORNER_ALL);
         break;
      case MONSTRO_STYLE_ITEM_IMAGE:
         canvas().addImage(bg->data.texture_id, r, {0, 0}, {1, 1}, 0xFFFFFFFF);
         break;
      case MONSTRO_STYLE_ITEM_SPRITE:
         canvas().addImage(bg->data.sprite.texture_id, r, bg->data.sprite.uv_min, bg->data.sprite.uv_max, 0xFFFFFFFF);
         break;
      case MONSTRO_STYLE_ITEM_NINEPATCH:
         canvas().addNinePatch(&bg->data.ninepatch, r, 0xFFFFFFFF);
         break;
      }
   }

   if (flags & MONSTRO_WINDOW_ROOT)
   {
      style->window.rounding = saved_rounding;
   }
}

static void draw_collapse_arrow(Canvas &canvas, float cx, float cy, float sz, bool pointing_right, uint32_t color)
{
   float half = sz * 0.5f;
   if (pointing_right)
   {
      MonstroVec2 p1 = {cx - half * 0.4f, cy - half};
      MonstroVec2 p2 = {cx + half * 0.6f, cy};
      MonstroVec2 p3 = {cx - half * 0.4f, cy + half};
      canvas.addTriangleFilled(p1, p2, p3, color);
   }
   else
   {
      MonstroVec2 p1 = {cx - half, cy - half * 0.4f};
      MonstroVec2 p2 = {cx + half, cy - half * 0.4f};
      MonstroVec2 p3 = {cx, cy + half * 0.6f};
      canvas.addTriangleFilled(p1, p2, p3, color);
   }
}

void Window::drawForeground()
{
   MonstroStyle *style = &ctx->style();

   float saved_rounding = style->window.rounding;
   if (flags & MONSTRO_WINDOW_ROOT)
   {
      style->window.rounding = 0.0f;
   }

   if (collapsed)
   {
      MonstroRect title_rect = getTitleBarRect();
      canvas().addRectFilled(title_rect, monstro_color_to_u32(style->window.header.active.data.color),
                             style->window.rounding, MONSTRO_CORNER_ALL);

      float btn_sz = title_rect.h * 0.6f;
      float arrow_x = title_rect.x + style->window.header.padding[0] + btn_sz * 0.5f;
      float arrow_y = title_rect.y + title_rect.h * 0.5f;

      uint64_t collapse_id = getChildId("##collapse");
      MonstroRect collapse_hit = {title_rect.x + style->window.header.padding[0], title_rect.y, btn_sz + 4.0f,
                                  title_rect.h};
      bool col_hovered =
          ctx->hovered_window == this && monstro_rect_contains(collapse_hit, ctx->mouse.pos_x, ctx->mouse.pos_y);

      if (col_hovered)
      {
         ctx->hovered_id = collapse_id;
         if (ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
         {
            collapsed = false;
         }
      }

      uint32_t arrow_col = monstro_color_to_u32(style->window.header.label_normal);
      draw_collapse_arrow(canvas(), arrow_x, arrow_y, btn_sz, true, arrow_col);

      float text_left = arrow_x + btn_sz * 0.5f + 4.0f;
      MonstroRect text_rect = {text_left, title_rect.y, title_rect.w - (text_left - title_rect.x), title_rect.h};
      if (style->font)
      {
         canvas().addTextEx(style->font, text_rect, monstro_color_to_u32(style->window.header.label_normal), title,
                            MONSTRO_ALIGN_LEFT);
      }
   }
   else
   {
      if (flags & MONSTRO_WINDOW_TITLE_BAR)
      {
         MonstroRect title_rect = getTitleBarRect();
         uint32_t header_color = monstro_color_to_u32(style->window.header.active.data.color);
         canvas().addRectFilled(title_rect, header_color, style->window.rounding, MONSTRO_CORNER_TOP);

         float btn_sz = title_rect.h * 0.6f;
         float content_left = title_rect.x + style->window.header.padding[0];
         float content_right = title_rect.x + title_rect.w - style->window.header.padding[0];

         if (flags & MONSTRO_WINDOW_COLLAPSABLE)
         {
            float arrow_x = content_left + btn_sz * 0.5f;
            float arrow_y = title_rect.y + title_rect.h * 0.5f;

            uint64_t collapse_id = getChildId("##collapse");
            MonstroRect collapse_hit = {content_left, title_rect.y, btn_sz + 4.0f, title_rect.h};
            bool col_hovered =
                ctx->hovered_window == this && monstro_rect_contains(collapse_hit, ctx->mouse.pos_x, ctx->mouse.pos_y);

            if (col_hovered)
            {
               ctx->hovered_id = collapse_id;
               if (ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
               {
                  collapsed = true;
               }
            }

            uint32_t arrow_col = monstro_color_to_u32(style->window.header.label_normal);
            draw_collapse_arrow(canvas(), arrow_x, arrow_y, btn_sz, false, arrow_col);

            content_left += btn_sz + 4.0f;
         }

         if (p_open_ref != nullptr)
         {
            float close_x = content_right - btn_sz;
            float close_y = title_rect.y + (title_rect.h - btn_sz) * 0.5f;
            MonstroRect close_rect = {close_x, close_y, btn_sz, btn_sz};

            uint64_t close_id = getChildId("##close");
            bool close_hovered =
                ctx->hovered_window == this && monstro_rect_contains(close_rect, ctx->mouse.pos_x, ctx->mouse.pos_y);

            uint32_t close_col = monstro_color_to_u32(style->window.header.label_normal);
            if (close_hovered)
            {
               ctx->hovered_id = close_id;
               canvas().addRectFilled(close_rect, monstro_color_to_u32(style->window.header.hover.data.color),
                                      style->window.rounding, MONSTRO_CORNER_ALL);
               if (ctx->mouse.isPressed(MONSTRO_MOUSE_BUTTON_LEFT))
               {
                  *p_open_ref = false;
               }
            }

            float pad = btn_sz * 0.2f;
            MonstroVec2 x1a = {close_rect.x + pad, close_rect.y + pad};
            MonstroVec2 x1b = {close_rect.x + btn_sz - pad, close_rect.y + btn_sz - pad};
            MonstroVec2 x2a = {close_rect.x + btn_sz - pad, close_rect.y + pad};
            MonstroVec2 x2b = {close_rect.x + pad, close_rect.y + btn_sz - pad};
            canvas().addLine(x1a, x1b, close_col, 2.0f);
            canvas().addLine(x2a, x2b, close_col, 2.0f);

            content_right -= btn_sz + 4.0f;
         }

         MonstroRect text_rect = {content_left, title_rect.y, content_right - content_left, title_rect.h};
         uint32_t label_color = monstro_color_to_u32(style->window.header.label_normal);
         if (style->font)
         {
            canvas().addTextEx(style->font, text_rect, label_color, title, MONSTRO_ALIGN_CENTER);
         }
      }

      if (flags & MONSTRO_WINDOW_MENU_BAR)
      {
         MonstroRect menu_rect = getMenuBarRect();
         canvas().addRectFilled(menu_rect, monstro_color_to_u32(style->window.menu_border_color), 0,
                                MONSTRO_CORNER_NONE);
      }

      if (flags & MONSTRO_WINDOW_BORDERS)
      {
         MonstroStyleItem *border = &style->window.border_style;
         MonstroRect r = getRect();
         switch (border->type)
         {
         case MONSTRO_STYLE_ITEM_COLOR:
            canvas().addRect(r, monstro_color_to_u32(border->data.color), style->window.rounding, MONSTRO_CORNER_ALL,
                             style->window.border);
            break;
         case MONSTRO_STYLE_ITEM_IMAGE:
            canvas().addImage(border->data.texture_id, r, {0, 0}, {1, 1}, 0xFFFFFFFF);
            break;
         case MONSTRO_STYLE_ITEM_SPRITE:
            canvas().addImage(border->data.sprite.texture_id, r, border->data.sprite.uv_min, border->data.sprite.uv_max,
                              0xFFFFFFFF);
            break;
         case MONSTRO_STYLE_ITEM_NINEPATCH:
            canvas().addNinePatch(&border->data.ninepatch, r, 0xFFFFFFFF);
            break;
         }

         if (!(flags & MONSTRO_WINDOW_TITLE_BAR))
         {
            MonstroRect title_rect = getTitleBarRect();
            MonstroVec2 p1 = {title_rect.x + 1.0f, title_rect.y};
            MonstroVec2 p2 = {title_rect.x + title_rect.w - 1.0f, title_rect.y};
            canvas().addLine(p1, p2, monstro_color_to_u32(style->window.border_color), style->window.border);
         }
      }
   }

   if (flags & MONSTRO_WINDOW_ROOT)
   {
      style->window.rounding = saved_rounding;
   }
}

/* ------------------------------------------------------------------------- */
/* Position / Cursor                                                         */
/* ------------------------------------------------------------------------- */

MonstroVec2 Window::getCursorPos() const
{
   const Layout *l = const_cast<Window *>(this)->getCurrentLayout();
   if (!l)
      return {0.0f, 0.0f};
   return {l->myPosX + l->myCursorX, l->myPosY + l->myCursorY};
}

void Window::setCursorPos(float x, float y)
{
   Layout *l = getCurrentLayout();
   if (!l)
      return;
   l->myCursorX = x - l->myPosX;
   l->myCursorY = y - l->myPosY;
}

MonstroVec2 Window::getCursorScreenPos() const
{
   const Layout *l = const_cast<Window *>(this)->getCurrentLayout();
   if (!l)
      return {pos.x, pos.y};
   return {pos.x + l->myPosX + l->myCursorX + l->myScrollOffsetX,
           pos.y + l->myPosY + l->myCursorY + l->myScrollOffsetY};
}

void Window::setCursorScreenPos(float x, float y)
{
   const Layout *l = getCurrentLayout();
   if (!l)
      return;
   setCursorPos(x - pos.x - l->myScrollOffsetX, y - pos.y - l->myScrollOffsetY);
}

/* ------------------------------------------------------------------------- */
/* Geometry                                                                  */
/* ------------------------------------------------------------------------- */

MonstroRect Window::getRect() const { return {pos.x, pos.y, size.x, size.y}; }

MonstroRect Window::getContentRect() const
{
   float header_h = getTitleBarHeight() + getMenuBarHeight();
   return {pos.x, pos.y + header_h, content_view_width, content_view_height};
}

bool Window::mouseOverTitle() const
{
   MonstroRect r = getTitleBarRect();
   return monstro_rect_contains(r, ctx->mouse.pos_x, ctx->mouse.pos_y);
}

float Window::getTitleBarHeight() const
{
   if (flags & MONSTRO_WINDOW_TITLE_BAR)
   {
      const MonstroStyle *style = &ctx->style();
      float font_height = style->font ? monstro_font_get_line_height(style->font) : 16.0f;
      return font_height + style->window.group_padding[1];
   }
   return 0.0f;
}

float Window::getMenuBarHeight() const
{
   if (flags & MONSTRO_WINDOW_MENU_BAR)
   {
      const MonstroStyle *style = &ctx->style();
      float font_height = style->font ? monstro_font_get_line_height(style->font) : 16.0f;
      return font_height + style->window.menu_padding[1];
   }
   return 0.0f;
}

MonstroRect Window::getTitleBarRect() const { return {pos.x, pos.y, size.x, getTitleBarHeight()}; }

MonstroRect Window::getMenuBarRect() const
{
   float title_h = getTitleBarHeight();
   float menu_h = getMenuBarHeight();
   return {pos.x, pos.y + title_h, size.x, menu_h};
}

/* ------------------------------------------------------------------------- */
/* Conditional Position / Size                                               */
/* ------------------------------------------------------------------------- */

void Window::setPosition(float x, float y, MonstroCondition cond)
{
   if ((cond & set_pos_allow_flags) == 0)
      return;

   set_pos_allow_flags = (MonstroCondition)(set_pos_allow_flags & ~(MONSTRO_COND_ONCE | MONSTRO_COND_FIRST_USE_EVER |
                                                                    MONSTRO_COND_APPEARING));

   pos.x = x;
   pos.y = y;
}

void Window::setSize(float w, float h, MonstroCondition cond)
{
   if ((cond & set_size_allow_flags) == 0)
      return;

   set_size_allow_flags = (MonstroCondition)(set_size_allow_flags & ~(MONSTRO_COND_ONCE | MONSTRO_COND_FIRST_USE_EVER |
                                                                      MONSTRO_COND_APPEARING));

   size.x = w;
   if (h != 0.0f || !(flags & MONSTRO_WINDOW_AUTO_RESIZE))
   {
      size.y = h;
   }
}

/* ------------------------------------------------------------------------- */
/* Layout                                                                    */
/* ------------------------------------------------------------------------- */

Canvas &Window::canvas() const { return *ctx->canvas; }

int Window::layoutDepth() const { return canvas().layouts.size() - layout_base; }

int Window::groupDepth() const { return canvas().groups.size() - group_base; }

Layout *Window::getCurrentLayout()
{
   // The window's layouts end where the next window up the window stack begins its own.
   Canvas &c = canvas();
   int end = c.layouts.size();
   int i = c.windows.size() - 1;
   for (; i >= 0 && c.windows[i] != this; i--)
      end = c.windows[i]->layout_base;
   if (i < 0 && ctx->current_window != this)
      return nullptr;
   return end > layout_base ? &c.layouts[end - 1] : nullptr;
}

void Window::setLayoutDirection(MonstroLayoutDirection direction)
{
   Layout *l = getCurrentLayout();
   if (l)
      l->myDirection = direction;
}

void Window::addItem(float width, float height)
{
   Layout *layout = getCurrentLayout();
   if (!layout)
      return;
   layout->addItem(width, height);
}

void Window::beginLayout(MonstroLayoutDirection direction)
{
   MonstroVec2 cursor = getCursorPos();
   beginLayout(cursor.x, cursor.y, direction);
}

void Window::beginLayout(float x, float y, MonstroLayoutDirection direction)
{
   Layout layout = {};
   layout.init(direction, x, y);

   // A nested layout inherits its parent's scroll offset and visible area.
   if (layoutDepth() > 0)
   {
      const Layout &outer = canvas().layouts.top();
      layout.myScrollOffsetX = outer.myScrollOffsetX;
      layout.myScrollOffsetY = outer.myScrollOffsetY;
      layout.myAvailableWidth = outer.myAvailableWidth;
      layout.myAvailableHeight = outer.myAvailableHeight;
      layout.myVisibleMaxScreenX = outer.myVisibleMaxScreenX;
      layout.myVisibleMaxScreenY = outer.myVisibleMaxScreenY;
   }
   canvas().layouts.push(layout);
}

void Window::endLayout()
{
   MONSTRO_REQUIRE(layoutDepth() > 0,
                   "layout stack underflow: monstro_end_layout without monstro_begin_layout in window '%s'", name);
   Layout finished = canvas().layouts.pop();
   addItem(finished.mySizeX, finished.mySizeY);
}

/* ------------------------------------------------------------------------- */
/* Window Tree Management                                                    */
/* ------------------------------------------------------------------------- */

void Window::addChild(Window *win)
{
   win->parent = this;
   win->sibling = nullptr;

   if (child == nullptr)
   {
      child = win;
      return;
   }

   Window *temp = child;
   while (temp->sibling != nullptr)
   {
      temp = temp->sibling;
   }
   temp->sibling = win;
}

void Window::remove()
{
   if (parent == nullptr)
      return;

   if (parent->child == this)
   {
      parent->child = sibling;
   }
   else
   {
      Window *temp = parent->child;
      while (temp->sibling != nullptr)
      {
         if (temp->sibling == this)
         {
            temp->sibling = sibling;
            break;
         }
         temp = temp->sibling;
      }
   }
}

void Window::makeLastSibling()
{
   if (parent != nullptr && sibling != nullptr)
   {
      Window *p = parent;
      remove();
      p->addChild(this);
   }
}

/*
 * Draw order. A window draws, then the windows begun after it at the same
 * level (its later siblings), then its popups, menus and combo lists, so those
 * stay above the windows that follow it. Child panels (begin_child) are part
 * of their parent: they draw right after it, at its depth, so a window in
 * front covers them. Tooltips draw last, over everything.
 */
static bool is_panel(const Window *w) { return (w->flags & MONSTRO_WINDOW_CHILD) != 0; }

static bool is_tooltip(const Window *w) { return (w->flags & MONSTRO_WINDOW_TOOLTIP) != 0; }

// A window's child panels, and theirs, right after it.
static int rank_panels(Window *w, int rank)
{
   for (Window *c = w->child; c; c = c->sibling)
   {
      if (is_panel(c))
      {
         c->draw_rank = rank++;
         rank = rank_panels(c, rank);
      }
   }
   return rank;
}

static int rank_chain(Window *first, int rank);

// The popups of a window and of its child panels.
static int rank_popups(Window *w, int rank)
{
   rank = rank_chain(w->child, rank);
   for (Window *c = w->child; c; c = c->sibling)
   {
      if (is_panel(c))
         rank = rank_popups(c, rank);
   }
   return rank;
}

// A sibling chain, panels and tooltips aside: a window and its panels, its later siblings, then its popups.
static int rank_chain(Window *first, int rank)
{
   Window *w = first;
   while (w && (is_panel(w) || is_tooltip(w)))
      w = w->sibling;
   if (!w)
      return rank;
   w->draw_rank = rank++;
   rank = rank_panels(w, rank);
   rank = rank_chain(w->sibling, rank);
   return rank_popups(w, rank);
}

void Context::rankWindowsForDrawing()
{
   int rank = rank_chain(root_window, 0);
   for (Window &w : windows)
   {
      if (is_tooltip(&w))
         w.draw_rank = rank++;
   }
}

/* ------------------------------------------------------------------------- */
/* Context Window Management Member Functions                                */
/* ------------------------------------------------------------------------- */

Window *Context::findWindow(const char *name)
{
   if (!name)
      return nullptr;

   char window_id[256];
   monstro_copy_id_text(name, window_id, sizeof(window_id));

   for (Window &window : windows)
   {
      if (std::strcmp(window.name, window_id) == 0)
      {
         return &window;
      }
   }
   return nullptr;
}

Window *Context::createWindow(const char *name, MonstroWindowFlags flags)
{
   Window *win = &windows.add();

   win->init(this, name, flags);

   if (current_window)
   {
      current_window->addChild(win);
   }

   return win;
}

static bool window_belongs_to_modal_popup(const Window *w)
{
   const Window *current = w;
   while (current != nullptr)
   {
      if ((current->flags & MONSTRO_WINDOW_MODAL) || (current->flags & MONSTRO_WINDOW_POPUP))
         return true;
      current = current->parent;
   }
   return false;
}

Window *Context::findHoveredWindow(float x, float y, bool excludeChildren)
{
   if (root_window != nullptr)
   {
      // Of the windows under the point, the one drawn on top is hovered.
      rankWindowsForDrawing();
      Window *result = nullptr;
      for (Window &w : windows)
      {
         bool takes_input =
             w.active && (w.flags & MONSTRO_WINDOW_INPUTS) && !(excludeChildren && (w.flags & MONSTRO_WINDOW_CHILD));
         if (takes_input && monstro_rect_contains(w.getRect(), x, y) && (!result || w.draw_rank > result->draw_rank))
            result = &w;
      }

      if (result && !popup_stack.empty() && popup_stack.top().is_modal)
      {
         if (!window_belongs_to_modal_popup(result))
         {
            return nullptr;
         }
      }

      return result;
   }
   return nullptr;
}

void Context::focusWindow(Window *win)
{
   focused_window = win;

   if (win == nullptr)
      return;

   if (win->flags & MONSTRO_WINDOW_BRING_TO_FRONT)
   {
      win->makeLastSibling();
   }
}

void Context::pushWindow(Window *win)
{
   FixedStack<Window *, MONSTRO_MAX_WINDOW_DEPTH> &stack = canvas->windows;
   for (int i = 0; i < stack.size(); i++)
   {
      MONSTRO_REQUIRE(stack[i] != win, "window '%s' is begun again while it is still open (frame %d)", win->name,
                      frame);
   }
   current_window = stack.push(win);
   canvas->setOwner(win, win->name);
}

void Context::popWindow()
{
   FixedStack<Window *, MONSTRO_MAX_WINDOW_DEPTH> &stack = canvas->windows;
   stack.pop();
   current_window = stack.empty() ? nullptr : stack.top();
   if (current_window)
      canvas->setOwner(current_window, current_window->name);
   else
      canvas->setOwner(nullptr, nullptr);
}

} // namespace monstro
