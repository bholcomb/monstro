#include "../context.h"
#include <cmath>
#include <cstring>

static bool button_behavior(MonstroContext *ctx, MonstroRect r, uint64_t id, bool *out_hovered, bool *out_held,
                            MonstroSelectableFlags flags)
{
   bool enabled = !(flags & MONSTRO_SELECTABLE_DISABLED);
   bool hovered = ctx->hoverItem(id, r, enabled);
   *out_hovered = hovered;
   *out_held = false;

   bool pressed = false;
   bool held = false;

   if (hovered)
   {
      if (flags & MONSTRO_SELECTABLE_MENU)
      {
         if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
         {
            pressed = true;
         }
      }
      else if (flags & MONSTRO_SELECTABLE_MENU_ITEM)
      {
         if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
         {
            pressed = true;
         }
         if (monstro_is_mouse_released(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
         {
            pressed = true;
         }
      }
      else
      {
         if (monstro_is_mouse_clicked(ctx, MONSTRO_MOUSE_BUTTON_LEFT, false))
         {
            ctx->setActiveId(id);
            pressed = true;
         }
      }
   }

   if (ctx->active_id == id)
   {
      if (enabled && !ctx->disabled && monstro_is_mouse_down(ctx, MONSTRO_MOUSE_BUTTON_LEFT))
      {
         held = true;
      }
      else
      {
         ctx->setActiveId(0);
      }
   }

   *out_held = held;
   return pressed;
}

static MonstroVec2 measure_selectable_label(MonstroFont *font, const char *display)
{
   MonstroVec2 label_size = {0.0f, 0.0f};
   if (font)
   {
      label_size = monstro_font_measure_text(font, display, 0.0f);
   }
   if (label_size.x == 0.0f)
      label_size.x = (float)strlen(display) * 8.0f;
   if (label_size.y == 0.0f)
      label_size.y = 16.0f;
   return label_size;
}

static float resolve_selectable_width(MonstroContext *ctx, monstro::Window *win, float base_outer_width,
                                      MonstroSelectableFlags flags)
{
   float resolved = base_outer_width;

   if ((flags & MONSTRO_SELECTABLE_SPAN_ALL_COLUMNS) && !ctx->canvas->columns.empty())
   {
      MonstroColumnsState *columns = &ctx->canvas->columns.top();
      if (columns->columns_w > 0.0f)
      {
         resolved = columns->columns_w;
      }
   }
   else if (flags & MONSTRO_SELECTABLE_DRAW_FILL_WIDTH)
   {
      monstro::Layout *layout = win->getCurrentLayout();
      if (layout && layout->myAvailableWidth > 0.0f)
      {
         resolved = layout->myAvailableWidth;
      }
   }

   return resolved;
}

static float resolve_selectable_x(MonstroContext *ctx, float default_x, MonstroSelectableFlags flags)
{
   if ((flags & MONSTRO_SELECTABLE_SPAN_ALL_COLUMNS) && !ctx->canvas->columns.empty())
   {
      MonstroColumnsState *columns = &ctx->canvas->columns.top();
      return columns->columns_x;
   }
   return default_x;
}

static void draw_selectable_toggle(monstro::Canvas *canvas, MonstroRect outer, MonstroStyleSelectable *style,
                                   bool active)
{
   float toggle_size = outer.h - style->padding[1] * 2.0f;
   if (toggle_size <= 0.0f)
      return;

   MonstroRect toggle_rect = {
       outer.x + outer.w - style->padding[0] - toggle_size,
       outer.y + style->padding[1],
       toggle_size,
       toggle_size,
   };
   canvas->addRect(toggle_rect, monstro_color_to_u32(style->text_normal), style->rounding, MONSTRO_CORNER_ALL, 1.0f);
   if (active)
   {
      MonstroRect fill_rect = {
          toggle_rect.x + 3.0f,
          toggle_rect.y + 3.0f,
          toggle_rect.w - 6.0f,
          toggle_rect.h - 6.0f,
      };
      if (fill_rect.w > 0.0f && fill_rect.h > 0.0f)
      {
         canvas->addRectFilled(fill_rect, monstro_color_to_u32(style->text_hover_active), style->rounding,
                               MONSTRO_CORNER_ALL);
      }
   }
}

static bool selectable_impl(MonstroContext *ctx, const char *label, bool *selected, float width, float height,
                            MonstroSelectableFlags flags, const char *tooltip)
{
   monstro::requireWindow(ctx, __func__, {MONSTRO_ARG(label)});

   monstro::Window *win = ctx->current_window;
   if (win->skip_items)
      return false;

   MonstroStyle *style = &ctx->style();
   MonstroStyleSelectable *ss = &style->selectable;
   monstro::Canvas *canvas = ctx->canvas;
   MonstroFont *font = style->font;

   uint64_t id = win->getItemId(label);
   const char *display = monstro_display_text(label);
   MonstroVec2 label_size = measure_selectable_label(font, display);

   float pad_x = ss->padding[0];
   float pad_y = ss->padding[1];
   bool has_toggle = (flags & MONSTRO_SELECTABLE_HAS_TOGGLE) != 0;
   float toggle_gap = has_toggle ? pad_x : 0.0f;
   float toggle_extra = has_toggle ? (label_size.y + toggle_gap) : 0.0f;

   float content_w = (width > 0.0f) ? width : (label_size.x + toggle_extra);
   float content_h = (height > 0.0f) ? height : label_size.y;

   MonstroVec2 pos = monstro_get_cursor_screen_pos(ctx);
   float outer_x = resolve_selectable_x(ctx, pos.x, flags);
   float outer_w = content_w + pad_x * 2.0f;
   float outer_h = content_h + pad_y * 2.0f;
   outer_w = resolve_selectable_width(ctx, win, outer_w, flags);
   if (outer_w < 0.0f)
      outer_w = 0.0f;

   MonstroRect outer = {outer_x, pos.y, outer_w, outer_h};
   MonstroRect inner = {outer.x + pad_x, outer.y + pad_y, content_w, content_h};

   bool hovered = false;
   bool held = false;
   bool pressed = button_behavior(ctx, outer, id, &hovered, &held, flags);

   bool is_selected = selected && *selected;
   MonstroStyleItem *bg = &ss->normal;
   MonstroColor text_color = ss->text_normal;
   if (is_selected)
   {
      bg = held ? &ss->pressed_active : (hovered ? &ss->hover_active : &ss->normal_active);
      text_color = held ? ss->text_pressed_active : (hovered ? ss->text_hover_active : ss->text_normal_active);
   }
   else
   {
      bg = held ? &ss->pressed : (hovered ? &ss->hover : &ss->normal);
      text_color = held ? ss->text_pressed : (hovered ? ss->text_hover : ss->text_normal);
   }

   if ((hovered || held || is_selected) && bg->type == MONSTRO_STYLE_ITEM_COLOR)
   {
      canvas->addRectFilled(outer, monstro_color_to_u32(bg->data.color), ss->rounding, MONSTRO_CORNER_ALL);
   }

   // The text is placed in the inner rect, left of a toggle, as the style aligns it.
   if (font)
   {
      MonstroRect text_rect = {inner.x, inner.y, inner.w - toggle_extra, inner.h};
      canvas->addTextEx(font, text_rect, monstro_color_to_u32(text_color), display, ss->text_alignment);
   }

   if (has_toggle)
   {
      draw_selectable_toggle(canvas, outer, ss, is_selected);
   }

   if (pressed && !(flags & MONSTRO_SELECTABLE_DONT_CLOSE_POPUP))
   {
      if (win->flags & MONSTRO_WINDOW_POPUP)
      {
         monstro_close_current_popup(ctx);
      }
   }

   if (pressed && selected)
   {
      *selected = !*selected;
   }

   monstro_add_item(ctx, outer.w, outer.h);
   monstro_draw_disabled_overlay(ctx, outer);
   monstro_set_last_item(ctx, id, outer, hovered, false, pressed, false);

   if (hovered && tooltip)
   {
      monstro_tooltip(ctx, tooltip);
   }

   return pressed;
}

bool monstro_selectable(MonstroContext *ctx, const char *label, bool *selected, MonstroSelectableFlags flags,
                        const char *tooltip)
{
   return selectable_impl(ctx, label, selected, 0.0f, 0.0f, flags, tooltip);
}

bool monstro_selectable_sized(MonstroContext *ctx, const char *label, bool *selected, float width, float height,
                              MonstroSelectableFlags flags, const char *tooltip)
{
   return selectable_impl(ctx, label, selected, width, height, flags, tooltip);
}
